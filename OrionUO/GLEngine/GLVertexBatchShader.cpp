/***********************************************************************************
**
** GLVertexBatchShader.cpp
**
** See GLVertexBatchShader.h.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
//----------------------------------------------------------------------------------
// The instance exists on both paths; only what its methods do differs.
CGLVertexBatchShader g_GLBatchShader;

// GLES 1.x has no programmable pipeline, so there is nothing here to build.
// CGLVertexBatch checks Available() and keeps using the fixed function arrays,
// which is what the Android build wants: the whole point of targeting GLES 1.x
// is that it still has them. A GLES 2.0 renderer would use this file instead,
// and drop the fixed function path rather than falling back to it.
//----------------------------------------------------------------------------------
// GLSL 1.20 is what an OpenGL 2.1 context guarantees, and GLES 2.0 takes the same
// source with a different version line and precision qualifiers - so one shader
// serves both, which is the whole reason the renderer was written onto it.
#if defined(ORION_GLES)
#define SHADER_VERSION "#version 100\n"
#define SHADER_PRECISION "precision mediump float;\n"
// fwidth is an extension in GLES 2.0 rather than core. Where a driver lacks it
// the sharp bilinear filter degrades to ordinary bilinear, which is exactly what
// the branch below falls back to.
#define SHADER_DERIVATIVES "#extension GL_OES_standard_derivatives : enable\n"
#else
#define SHADER_VERSION "#version 120\n"
#define SHADER_PRECISION ""
#define SHADER_DERIVATIVES ""
#endif

static const char *s_VertexShader = SHADER_VERSION SHADER_PRECISION
    "attribute vec2 a_position;\n"
    "attribute vec2 a_texcoord;\n"
    "attribute vec4 a_color;\n"
    "attribute vec3 a_normal;\n"
    "uniform mat4 u_transform;\n"
    "uniform int u_lighting;\n"
    "uniform vec3 u_lightDirection;\n"
    "uniform vec3 u_lightConstant;\n"
    "uniform vec3 u_lightDiffuse;\n"
    "varying vec2 v_texcoord;\n"
    "varying vec4 v_color;\n"
    "void main()\n"
    "{\n"
    "    v_texcoord = a_texcoord;\n"
    "    v_color = a_color;\n"
    "\n"
    "    if (u_lighting != 0)\n"
    "    {\n"
    "        // The fixed function model this reproduces, with no specular and no\n"
    "        // colour material: emission + model_ambient*mat_ambient +\n"
    "        // light_ambient*mat_ambient + max(dot(N,L),0)*light_diffuse*mat_diffuse.\n"
    "        // The two products that do not vary per vertex arrive precomputed.\n"
    "        // Not normalized: the fixed function pipeline only normalizes when\n"
    "        // GL_NORMALIZE is enabled, and this client never enables it. Doing it\n"
    "        // here diverges wherever the source normals are not unit length -\n"
    "        // invisibly on one driver and badly on another.\n"
    "        float ndotl = max(dot(a_normal, u_lightDirection), 0.0);\n"
    "        v_color = vec4(u_lightConstant + u_lightDiffuse * ndotl, 1.0);\n"
    "    }\n"
    "    gl_Position = u_transform * vec4(a_position, 0.0, 1.0);\n"
    "}\n";

static const char *s_FragmentShader = SHADER_VERSION SHADER_DERIVATIVES SHADER_PRECISION
    "uniform sampler2D u_texture;\n"
    "uniform int u_textured;\n"
    "uniform vec2 u_sourceSize;\n"
    "varying vec2 v_texcoord;\n"
    "varying vec4 v_color;\n"
    "void main()\n"
    "{\n"
    "    vec4 color = v_color;\n"
    "    if (u_textured != 0)\n"
    "    {\n"
    "        vec2 uv = v_texcoord;\n"
    "\n"
    "        // Sharp bilinear. UO art is fixed low resolution - the login screen is\n"
    "        // 640x480 - and magnifying it several times with plain bilinear turns\n"
    "        // pixel art to mush, while nearest leaves harsh stair steps. This keeps\n"
    "        // each texel flat and blends only across the boundary between them, over\n"
    "        // as narrow a band as the magnification allows: crisp edges, smooth\n"
    "        // diagonals. At 1:1 the blend band is a whole texel and it degenerates\n"
    "        // to ordinary bilinear, so nothing changes when nothing is magnified.\n"
    "#ifdef GL_OES_standard_derivatives\n"
    "        if (u_sourceSize.x > 0.0)\n"
    "        {\n"
    "            vec2 texels = uv * u_sourceSize;\n"
    "            vec2 center = floor(texels) + 0.5;\n"
    "            vec2 width = max(fwidth(texels), vec2(0.0001));\n"
    "            texels = center + clamp((texels - center) / width, -0.5, 0.5);\n"
    "            uv = texels / u_sourceSize;\n"
    "        }\n"
    "#endif\n"
    "\n"
    "        color *= texture2D(u_texture, uv);\n"
    "    }\n"
    "\n"
    "    // The fixed function path runs with glAlphaFunc(GL_GREATER, 0.0), which\n"
    "    // Core profiles and GLES both drop. discard reproduces it exactly.\n"
    "    if (color.a <= 0.0)\n"
    "        discard;\n"
    "\n"
    "    gl_FragColor = color;\n"
    "}\n";
//----------------------------------------------------------------------------------
GLuint CGLVertexBatchShader::CompileStage(GLenum type, const char *source)
{
    GLuint stage = glCreateShader(type);
    if (stage == 0)
        return 0;

    glShaderSource(stage, 1, &source, nullptr);
    glCompileShader(stage);

    GLint compiled = GL_FALSE;
    glGetShaderiv(stage, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE)
    {
        char log[1024] = { 0 };
        glGetShaderInfoLog(stage, sizeof(log) - 1, nullptr, log);
        LOG("CGLVertexBatchShader: %s shader failed to compile: %s\n",
            (type == GL_VERTEX_SHADER) ? "vertex" : "fragment",
            log);
        glDeleteShader(stage);
        return 0;
    }

    return stage;
}
//----------------------------------------------------------------------------------
bool CGLVertexBatchShader::Init()
{
    Free();

    GLuint vertex = CompileStage(GL_VERTEX_SHADER, s_VertexShader);
    if (vertex == 0)
        return false;

    GLuint fragment = CompileStage(GL_FRAGMENT_SHADER, s_FragmentShader);
    if (fragment == 0)
    {
        glDeleteShader(vertex);
        return false;
    }

    m_Program = glCreateProgram();
    glAttachShader(m_Program, vertex);
    glAttachShader(m_Program, fragment);
    glLinkProgram(m_Program);

    // The program holds its own references once linked.
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(m_Program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE)
    {
        char log[1024] = { 0 };
        glGetProgramInfoLog(m_Program, sizeof(log) - 1, nullptr, log);
        LOG("CGLVertexBatchShader: link failed: %s\n", log);
        Free();
        return false;
    }

    m_AttribPosition = glGetAttribLocation(m_Program, "a_position");
    m_AttribTexCoord = glGetAttribLocation(m_Program, "a_texcoord");
    m_AttribColor = glGetAttribLocation(m_Program, "a_color");
    m_AttribNormal = glGetAttribLocation(m_Program, "a_normal");
    m_UniformTransform = glGetUniformLocation(m_Program, "u_transform");
    m_UniformTexture = glGetUniformLocation(m_Program, "u_texture");
    m_UniformTextured = glGetUniformLocation(m_Program, "u_textured");
    m_UniformSourceSize = glGetUniformLocation(m_Program, "u_sourceSize");
    m_UniformLighting = glGetUniformLocation(m_Program, "u_lighting");
    m_UniformLightDirection = glGetUniformLocation(m_Program, "u_lightDirection");
    m_UniformLightConstant = glGetUniformLocation(m_Program, "u_lightConstant");
    m_UniformLightDiffuse = glGetUniformLocation(m_Program, "u_lightDiffuse");

    if (m_AttribPosition < 0 || m_UniformTransform < 0)
    {
        LOG("CGLVertexBatchShader: program is missing expected inputs\n");
        Free();
        return false;
    }

    glGenBuffers(1, &m_VertexBuffer);
    if (m_VertexBuffer == 0)
    {
        LOG("CGLVertexBatchShader: could not create a vertex buffer\n");
        Free();
        return false;
    }

    m_Available = true;
    LOG("CGLVertexBatchShader: ready\n");
    return true;
}
//----------------------------------------------------------------------------------
void CGLVertexBatchShader::Free()
{
    if (m_VertexBuffer != 0)
    {
        glDeleteBuffers(1, &m_VertexBuffer);
        m_VertexBuffer = 0;
    }

    if (m_Program != 0)
    {
        glDeleteProgram(m_Program);
        m_Program = 0;
    }

    m_Available = false;
}
//----------------------------------------------------------------------------------
void CGLVertexBatchShader::CacheLightingState()
{
    // Light and material are set once at start-up and never change, so read them
    // back once rather than per draw. Reading them at all - instead of hardcoding
    // the numbers - keeps this honest against the fixed function path it has to
    // match, and makes the values obvious when the matrix stack goes away.
    GLfloat lightPosition[4] = {};
    GLfloat lightAmbient[4] = {};
    GLfloat lightDiffuse[4] = {};
    GLfloat modelAmbient[4] = {};
    GLfloat materialAmbient[4] = {};
    GLfloat materialDiffuse[4] = {};

#if defined(ORION_GLES)
    // There is no fixed function state to read back here, so the same values
    // CGLEngine::Install sets on the desktop are written straight in. If those
    // change, these have to change with them - which is why they are spelled out
    // rather than hidden behind constants.
    lightPosition[0] = -1.0f;
    lightPosition[1] = -1.0f;
    lightPosition[2] = 0.5f;
    lightPosition[3] = 0.0f;

    lightAmbient[0] = lightAmbient[1] = lightAmbient[2] = 2.0f;
    lightAmbient[3] = 1.0f;

    // GL's defaults for what Install never sets.
    lightDiffuse[0] = lightDiffuse[1] = lightDiffuse[2] = lightDiffuse[3] = 1.0f;
    modelAmbient[0] = modelAmbient[1] = modelAmbient[2] = modelAmbient[3] = 0.8f;
    materialAmbient[0] = materialAmbient[1] = materialAmbient[2] = 0.2f;
    materialAmbient[3] = 1.0f;
    materialDiffuse[0] = materialDiffuse[1] = materialDiffuse[2] = 0.8f;
    materialDiffuse[3] = 1.0f;
#else
    glGetLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glGetLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glGetLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glGetFloatv(GL_LIGHT_MODEL_AMBIENT, modelAmbient);
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
#endif

    // w == 0 means directional, and the direction is the position itself.
    float length = sqrtf(
        lightPosition[0] * lightPosition[0] + lightPosition[1] * lightPosition[1] +
        lightPosition[2] * lightPosition[2]);
    if (length <= 0.0f)
        length = 1.0f;

    for (int i = 0; i < 3; i++)
    {
        m_LightDirection[i] = lightPosition[i] / length;
        m_LightConstant[i] = modelAmbient[i] * materialAmbient[i] +
                             lightAmbient[i] * materialAmbient[i];
        m_LightDiffuse[i] = lightDiffuse[i] * materialDiffuse[i];
    }

    m_LightingCached = true;
}
//----------------------------------------------------------------------------------
void CGLVertexBatchShader::Draw(
    GLenum mode,
    const float *vertices,
    int vertexCount,
    int floatsPerVertex,
    bool textured,
    bool lit)
{
    if (!m_Available || vertexCount == 0)
        return;

    // The transform comes from our own stack rather than being read back from
    // GL. That is what a Core profile will require, and it also removes two
    // glGetFloatv calls per draw - a pipeline stall each, on every gump and tile.
    if (!m_StateBound)
    {
        glUseProgram(m_Program);

        if (m_UniformTexture >= 0)
            glUniform1i(m_UniformTexture, 0);
    }

    float transform[16] = {};
    g_GLMatrix.Transform(transform);

    if (!m_HaveLastTransform || memcmp(transform, m_LastTransform, sizeof(transform)) != 0)
    {
        memcpy(m_LastTransform, transform, sizeof(transform));
        m_HaveLastTransform = true;
        glUniformMatrix4fv(m_UniformTransform, 1, GL_FALSE, transform);
    }

    const int texturedValue = textured ? 1 : 0;

    if (m_UniformTextured >= 0 && texturedValue != m_LastTextured)
    {
        m_LastTextured = texturedValue;
        glUniform1i(m_UniformTextured, texturedValue);
    }

    if (m_UniformSourceSize >= 0)
    {
        // Zero disables texel-space filtering in the shader, leaving plain bilinear.
        const bool sharp = g_SharpFilter;
        const float width = sharp ? (float)m_SourceWidth : 0.0f;
        const float height = sharp ? (float)m_SourceHeight : 0.0f;

        if (width != m_LastSourceSize[0] || height != m_LastSourceSize[1])
        {
            m_LastSourceSize[0] = width;
            m_LastSourceSize[1] = height;
            glUniform2f(m_UniformSourceSize, width, height);
        }
    }

    const int lightingValue = lit ? 1 : 0;

    if (m_UniformLighting >= 0 && lightingValue != m_LastLighting)
    {
        if (lit && !m_LightingCached)
            CacheLightingState();

        m_LastLighting = lightingValue;
        glUniform1i(m_UniformLighting, lightingValue);

        if (lit)
        {
            glUniform3fv(m_UniformLightDirection, 1, m_LightDirection);
            glUniform3fv(m_UniformLightConstant, 1, m_LightConstant);
            glUniform3fv(m_UniformLightDiffuse, 1, m_LightDiffuse);
        }
    }

    const GLsizei stride = (GLsizei)(floatsPerVertex * sizeof(float));

    BindState(vertices, stride, vertexCount);

    glDrawArrays(mode, 0, vertexCount);
}
//----------------------------------------------------------------------------------
// Points the attributes at this batch's vertices, and leaves everything bound.
//
// On GLES the vertices are handed to GL directly rather than through a buffer
// object. GLES 2.0 allows that, and it is what the fixed function path did: a
// sprite is four vertices, and uploading four vertices into a buffer object -
// reallocating its store every time - costs far more than reading them.
void CGLVertexBatchShader::BindState(const float *vertices, GLsizei stride, int vertexCount)
{
#if defined(ORION_GLES)
    const bool repoint = (!m_StateBound || stride != m_LastStride || vertices != m_LastVertices);
    const char *base = (const char *)vertices;
#else
    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(vertexCount * stride), vertices, GL_STREAM_DRAW);

    const bool repoint = (!m_StateBound || stride != m_LastStride);
    const char *base = nullptr;
#endif

    if (!repoint)
        return;

    m_LastStride = stride;
    m_LastVertices = vertices;

    glEnableVertexAttribArray(m_AttribPosition);
    glVertexAttribPointer(m_AttribPosition, 2, GL_FLOAT, GL_FALSE, stride, base);

    if (m_AttribTexCoord >= 0)
    {
        glEnableVertexAttribArray(m_AttribTexCoord);
        glVertexAttribPointer(
            m_AttribTexCoord, 2, GL_FLOAT, GL_FALSE, stride, base + 2 * sizeof(float));
    }

    if (m_AttribColor >= 0)
    {
        glEnableVertexAttribArray(m_AttribColor);
        glVertexAttribPointer(
            m_AttribColor, 4, GL_FLOAT, GL_FALSE, stride, base + 4 * sizeof(float));
    }

    if (m_AttribNormal >= 0)
    {
        glEnableVertexAttribArray(m_AttribNormal);
        glVertexAttribPointer(
            m_AttribNormal, 3, GL_FLOAT, GL_FALSE, stride, base + 8 * sizeof(float));
    }

    m_StateBound = true;
}
//----------------------------------------------------------------------------------
void CGLVertexBatchShader::InvalidateState()
{
    m_StateBound = false;
    m_HaveLastTransform = false;
    m_LastTextured = -1;
    m_LastLighting = -1;
    m_LastSourceSize[0] = -1.0f;
    m_LastSourceSize[1] = -1.0f;
    m_LastStride = 0;
    m_LastVertices = nullptr;
}
//----------------------------------------------------------------------------------
