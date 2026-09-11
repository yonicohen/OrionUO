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
CGLVertexBatchShader g_GLBatchShader;
//----------------------------------------------------------------------------------
// GLSL 1.20 is what an OpenGL 2.1 context guarantees, and it is also what GLES 2.0
// accepts with only the precision qualifiers added, so the same source serves both
// once the renderer stops needing the fixed function pipeline.
static const char *s_VertexShader =
    "#version 120\n"
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
    "        float ndotl = max(dot(normalize(a_normal), u_lightDirection), 0.0);\n"
    "        v_color = vec4(u_lightConstant + u_lightDiffuse * ndotl, 1.0);\n"
    "    }\n"
    "    gl_Position = u_transform * vec4(a_position, 0.0, 1.0);\n"
    "}\n";

static const char *s_FragmentShader =
    "#version 120\n"
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
    "        if (u_sourceSize.x > 0.0)\n"
    "        {\n"
    "            vec2 texels = uv * u_sourceSize;\n"
    "            vec2 center = floor(texels) + 0.5;\n"
    "            vec2 width = max(fwidth(texels), vec2(0.0001));\n"
    "            texels = center + clamp((texels - center) / width, -0.5, 0.5);\n"
    "            uv = texels / u_sourceSize;\n"
    "        }\n"
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

    glGetLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    glGetLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glGetLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glGetFloatv(GL_LIGHT_MODEL_AMBIENT, modelAmbient);
    glGetMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbient);
    glGetMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);

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
    float transform[16] = {};
    g_GLMatrix.Transform(transform);

    glUseProgram(m_Program);
    glUniformMatrix4fv(m_UniformTransform, 1, GL_FALSE, transform);

    if (m_UniformTexture >= 0)
        glUniform1i(m_UniformTexture, 0);

    if (m_UniformTextured >= 0)
        glUniform1i(m_UniformTextured, textured ? 1 : 0);

    if (m_UniformSourceSize >= 0)
    {
        // Zero disables texel-space filtering in the shader, leaving plain bilinear.
        const bool sharp = g_SharpFilter;
        glUniform2f(
            m_UniformSourceSize,
            sharp ? (float)m_SourceWidth : 0.0f,
            sharp ? (float)m_SourceHeight : 0.0f);
    }

    if (m_UniformLighting >= 0)
    {
        if (lit && !m_LightingCached)
            CacheLightingState();

        glUniform1i(m_UniformLighting, lit ? 1 : 0);

        if (lit)
        {
            glUniform3fv(m_UniformLightDirection, 1, m_LightDirection);
            glUniform3fv(m_UniformLightConstant, 1, m_LightConstant);
            glUniform3fv(m_UniformLightDiffuse, 1, m_LightDiffuse);
        }
    }

    const GLsizei stride = (GLsizei)(floatsPerVertex * sizeof(float));

    glBindBuffer(GL_ARRAY_BUFFER, m_VertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER, (GLsizeiptr)(vertexCount * stride), vertices, GL_STREAM_DRAW);

    glEnableVertexAttribArray(m_AttribPosition);
    glVertexAttribPointer(m_AttribPosition, 2, GL_FLOAT, GL_FALSE, stride, (const void *)0);

    if (m_AttribTexCoord >= 0)
    {
        glEnableVertexAttribArray(m_AttribTexCoord);
        glVertexAttribPointer(
            m_AttribTexCoord, 2, GL_FLOAT, GL_FALSE, stride, (const void *)(2 * sizeof(float)));
    }

    if (m_AttribColor >= 0)
    {
        glEnableVertexAttribArray(m_AttribColor);
        glVertexAttribPointer(
            m_AttribColor, 4, GL_FLOAT, GL_FALSE, stride, (const void *)(4 * sizeof(float)));
    }

    if (m_AttribNormal >= 0)
    {
        glEnableVertexAttribArray(m_AttribNormal);
        glVertexAttribPointer(
            m_AttribNormal, 3, GL_FLOAT, GL_FALSE, stride, (const void *)(8 * sizeof(float)));
    }

    glDrawArrays(mode, 0, vertexCount);

    if (m_AttribNormal >= 0)
        glDisableVertexAttribArray(m_AttribNormal);

    if (m_AttribColor >= 0)
        glDisableVertexAttribArray(m_AttribColor);

    if (m_AttribTexCoord >= 0)
        glDisableVertexAttribArray(m_AttribTexCoord);

    glDisableVertexAttribArray(m_AttribPosition);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
//----------------------------------------------------------------------------------
