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
    "uniform mat4 u_transform;\n"
    "varying vec2 v_texcoord;\n"
    "varying vec4 v_color;\n"
    "void main()\n"
    "{\n"
    "    v_texcoord = a_texcoord;\n"
    "    v_color = a_color;\n"
    "    gl_Position = u_transform * vec4(a_position, 0.0, 1.0);\n"
    "}\n";

static const char *s_FragmentShader =
    "#version 120\n"
    "uniform sampler2D u_texture;\n"
    "uniform int u_textured;\n"
    "varying vec2 v_texcoord;\n"
    "varying vec4 v_color;\n"
    "void main()\n"
    "{\n"
    "    vec4 color = v_color;\n"
    "    if (u_textured != 0)\n"
    "        color *= texture2D(u_texture, v_texcoord);\n"
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
    m_UniformTransform = glGetUniformLocation(m_Program, "u_transform");
    m_UniformTexture = glGetUniformLocation(m_Program, "u_texture");
    m_UniformTextured = glGetUniformLocation(m_Program, "u_textured");

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
void CGLVertexBatchShader::Draw(
    GLenum mode, const float *vertices, int vertexCount, int floatsPerVertex, bool textured)
{
    if (!m_Available || vertexCount == 0)
        return;

    // Until the matrix stack is replaced, the transform is still the fixed
    // function one; read it back and hand it to the shader. This is what lets the
    // shader path be compared against immediate mode in the same context.
    GLfloat projection[16] = {};
    GLfloat modelview[16] = {};
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);

    // Column-major, as OpenGL stores them: transform = projection * modelview.
    GLfloat transform[16] = {};
    for (int column = 0; column < 4; column++)
    {
        for (int row = 0; row < 4; row++)
        {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++)
                sum += projection[k * 4 + row] * modelview[column * 4 + k];

            transform[column * 4 + row] = sum;
        }
    }

    glUseProgram(m_Program);
    glUniformMatrix4fv(m_UniformTransform, 1, GL_FALSE, transform);

    if (m_UniformTexture >= 0)
        glUniform1i(m_UniformTexture, 0);

    if (m_UniformTextured >= 0)
        glUniform1i(m_UniformTextured, textured ? 1 : 0);

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

    glDrawArrays(mode, 0, vertexCount);

    if (m_AttribColor >= 0)
        glDisableVertexAttribArray(m_AttribColor);

    if (m_AttribTexCoord >= 0)
        glDisableVertexAttribArray(m_AttribTexCoord);

    glDisableVertexAttribArray(m_AttribPosition);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
//----------------------------------------------------------------------------------
