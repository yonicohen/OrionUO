// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/***********************************************************************************
**
** GLShader.cpp
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
//----------------------------------------------------------------------------------
CDeathShader g_DeathShader;
CColorizerShader g_ColorizerShader;
CColorizerShader g_FontColorizerShader;
CColorizerShader g_LightColorizerShader;
//----------------------------------------------------------------------------------
#ifdef ORION_GLES
//----------------------------------------------------------------------------------
// The client's own shaders, rewritten for GLES.
//
// The desktop versions are written against the fixed function pipeline: they
// read gl_TexCoord and gl_Color and are bound over whatever the renderer is
// already doing. GLES 2.0 has neither, so these share the batch's vertex stage
// and its attributes, and are swapped in as the program it draws with.
//
// What they do is unchanged, because what they do is the game: mode 1 looks a
// texel's red channel up in the hue table, which is how every coloured item,
// robe and weapon in UO gets its colour; modes above 5 shade land by its normal;
// and the death shader drains it all to grey.
//----------------------------------------------------------------------------------
namespace
{
const char *g_Precision = "precision mediump float;\n";

const char *g_ColorizerFragment =
    "uniform sampler2D u_texture;\n"
    "uniform int u_textured;\n"
    "uniform int u_drawMode;\n"
    "uniform float u_colors[96];\n"
    "varying vec2 v_texcoord;\n"
    "varying vec4 v_color;\n"
    "void main()\n"
    "{\n"
    "    vec4 texel = texture2D(u_texture, v_texcoord);\n"
    "    if (texel.a == 0.0)\n"
    "        discard;\n"
    "    if (u_drawMode == 1 ||\n"
    "        (u_drawMode == 2 && texel.r == texel.g && texel.r == texel.b))\n"
    "    {\n"
    "        int index = int(texel.r * 31.875) * 3;\n"
    "        gl_FragColor = vec4(u_colors[index], u_colors[index + 1], u_colors[index + 2],\n"
    "                            texel.a) * v_color;\n"
    "    }\n"
    "    else if (u_drawMode > 9)\n"
    "    {\n"
    "        float red = texel.r;\n"
    "        if (u_drawMode > 11)\n"
    "            red = 0.6;\n"
    "        else if (u_drawMode > 10)\n"
    "            red *= 0.5;\n"
    "        else\n"
    "            red *= 1.5;\n"
    "        gl_FragColor = vec4(red, red, red, texel.a) * v_color;\n"
    "    }\n"
    "    else if (u_drawMode > 6)\n"
    "    {\n"
    "        int index = int(texel.r * 31.875) * 3;\n"
    "        gl_FragColor = vec4(u_colors[index], u_colors[index + 1], u_colors[index + 2],\n"
    "                            texel.a) * v_color;\n"
    "    }\n"
    "    else\n"
    "        gl_FragColor = texel * v_color;\n"
    "}\n";

const char *g_DeathFragment =
    "uniform sampler2D u_texture;\n"
    "uniform int u_textured;\n"
    "uniform int u_drawMode;\n"
    "varying vec2 v_texcoord;\n"
    "varying vec4 v_color;\n"
    "void main()\n"
    "{\n"
    "    vec4 texel = texture2D(u_texture, v_texcoord);\n"
    "    if (texel.a == 0.0)\n"
    "        discard;\n"
    "    float grey = texel.r * 0.6 + texel.g * 0.05;\n"
    "    gl_FragColor = vec4(grey, grey, grey, texel.a);\n"
    "}\n";

const char *g_FontFragment =
    "uniform sampler2D u_texture;\n"
    "uniform int u_textured;\n"
    "uniform int u_drawMode;\n"
    "uniform float u_colors[96];\n"
    "varying vec2 v_texcoord;\n"
    "varying vec4 v_color;\n"
    "void main()\n"
    "{\n"
    "    vec4 texel = texture2D(u_texture, v_texcoord);\n"
    "    if (texel.a == 0.0)\n"
    "        discard;\n"
    "    if (u_drawMode == 1 ||\n"
    "        (u_drawMode == 2 && texel.r == texel.g && texel.r == texel.b))\n"
    "    {\n"
    "        int index = int(texel.r * 31.875) * 3;\n"
    "        gl_FragColor = vec4(u_colors[index], u_colors[index + 1], u_colors[index + 2],\n"
    "                            texel.a) * v_color;\n"
    "    }\n"
    "    else if (u_drawMode == 4 || (u_drawMode == 3 && texel.r > 0.04))\n"
    "    {\n"
    "        gl_FragColor = vec4(u_colors[90], u_colors[91], u_colors[92], texel.a) * v_color;\n"
    "    }\n"
    "    else\n"
    "        gl_FragColor = texel * v_color;\n"
    "}\n";

const char *g_LightFragment =
    "uniform sampler2D u_texture;\n"
    "uniform int u_textured;\n"
    "uniform int u_drawMode;\n"
    "uniform float u_colors[96];\n"
    "varying vec2 v_texcoord;\n"
    "varying vec4 v_color;\n"
    "void main()\n"
    "{\n"
    "    vec4 texel = texture2D(u_texture, v_texcoord);\n"
    "    if (texel.a != 0.0 && u_drawMode == 1)\n"
    "    {\n"
    "        int index = int(texel.r * 7.96875) * 3;\n"
    "        gl_FragColor = (texel * vec4(u_colors[index], u_colors[index + 1],\n"
    "                                     u_colors[index + 2], 1.0)) * 3.0;\n"
    "    }\n"
    "    else\n"
    "        gl_FragColor = texel;\n"
    "}\n";

// Which fragment stage a shader gets is decided by the order they are created
// in, which is the order COrion::Install builds them: colouriser, font, light.
int g_ColorizerCount = 0;
} // namespace
//----------------------------------------------------------------------------------
void UnuseShader()
{
    ShaderColorTable = 0;
    g_ShaderDrawMode = 0;
    g_ClientShaderActive = false;
    g_GLBatchShader.SetProgram(nullptr);
}
//----------------------------------------------------------------------------------
CGLShader::CGLShader()
{
}
//----------------------------------------------------------------------------------
CGLShader::~CGLShader()
{
}
//----------------------------------------------------------------------------------
bool CGLShader::Init(const char * /*vertexShaderData*/, const char * /*fragmentShaderData*/)
{
    return false;
}
//----------------------------------------------------------------------------------
bool CGLShader::Use()
{
    if (m_GLESProgram.Program == 0)
    {
        UnuseShader();
        return false;
    }

    glUseProgram(m_GLESProgram.Program);

    // The GUI sets these through the globals, on whatever program is current.
    ShaderColorTable = m_GLESProgram.UniformColors;
    g_ShaderDrawMode = m_GLESProgram.UniformDrawMode;
    g_ClientShaderActive = true;

    g_GLBatchShader.SetProgram(&m_GLESProgram);
    return true;
}
//----------------------------------------------------------------------------------
void CGLShader::Pause()
{
    g_GLBatchShader.SetProgram(nullptr);
}
//----------------------------------------------------------------------------------
void CGLShader::Resume()
{
    if (m_GLESProgram.Program != 0)
        g_GLBatchShader.SetProgram(&m_GLESProgram);
}
//----------------------------------------------------------------------------------
CDeathShader::CDeathShader()
    : CGLShader()
{
}
//----------------------------------------------------------------------------------
bool CDeathShader::Init(const char * /*vertexShaderData*/, const char * /*fragmentShaderData*/)
{
    const string fragment = string(g_Precision) + g_DeathFragment;
    return m_GLESProgram.Build(CGLVertexBatchShader::VertexSource(), fragment.c_str());
}
//----------------------------------------------------------------------------------
bool CDeathShader::Use()
{
    return CGLShader::Use();
}
//----------------------------------------------------------------------------------
CColorizerShader::CColorizerShader()
    : CGLShader()
{
}
//----------------------------------------------------------------------------------
bool CColorizerShader::Init(const char * /*vertexShaderData*/, const char * /*fragmentShaderData*/)
{
    const char *source = g_ColorizerFragment;

    if (g_ColorizerCount == 1)
        source = g_FontFragment;
    else if (g_ColorizerCount >= 2)
        source = g_LightFragment;

    g_ColorizerCount++;

    const string fragment = string(g_Precision) + source;
    return m_GLESProgram.Build(CGLVertexBatchShader::VertexSource(), fragment.c_str());
}
//----------------------------------------------------------------------------------
bool CColorizerShader::Use()
{
    return CGLShader::Use();
}
//----------------------------------------------------------------------------------
#else
//----------------------------------------------------------------------------------
void UnuseShader()
{
    WISPFUN_DEBUG("c_uns_sdr");
    glUseProgramObjectARB(0);
    ShaderColorTable = 0;
    g_ShaderDrawMode = 0;
    g_ClientShaderActive = false;
}
//----------------------------------------------------------------------------------
//-----------------------------------CGLShader--------------------------------------
//----------------------------------------------------------------------------------
CGLShader::CGLShader()
{
    WISPFUN_DEBUG("c32_f1");
}
//----------------------------------------------------------------------------------
bool CGLShader::Init(const char *vertexShaderData, const char *fragmentShaderData)
{
    GLint val = GL_FALSE;

    if (vertexShaderData != NULL && fragmentShaderData != NULL)
    {
        m_Shader = glCreateProgramObjectARB();

        m_VertexShader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
        glShaderSourceARB(m_VertexShader, 1, (const GLcharARB **)&vertexShaderData, NULL);
        glCompileShaderARB(m_VertexShader);

        glGetShaderiv(m_VertexShader, GL_COMPILE_STATUS, &val);
        if (val != GL_TRUE)
        {
            LOG("CGLShader::Init vertex shader compilation error:\n");
            auto error = glGetError();
            if (error != 0)
            {
                auto errorStr = gluErrorString(error);
                LOG("CGLShader::Init vertex shader error Code: %i\n", error);
                LOG("CGLShader::Init vertex shader error string: %s\n", errorStr);
            }
            return false;
        }
        LOG("vertex shader compiled successfully\n");

        glAttachObjectARB(m_Shader, m_VertexShader);

        m_FragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
        glShaderSourceARB(m_FragmentShader, 1, (const GLcharARB **)&fragmentShaderData, NULL);
        glCompileShaderARB(m_FragmentShader);

        glGetShaderiv(m_FragmentShader, GL_COMPILE_STATUS, &val);
        if (val != GL_TRUE)
        {
            LOG("CGLShader::Init fragment shader compilation error:\n");
            auto error = glGetError();
            if (error != 0)
            {
                auto errorStr = gluErrorString(error);
                LOG("CGLShader::Init fragment shader error Code: %i\n", error);
                LOG("CGLShader::Init fragment shader error string: %s\n", errorStr);
            }
            return false;
        }
        LOG("fragment shader compiled successfully\n");

        glAttachObjectARB(m_Shader, m_FragmentShader);

        glLinkProgramARB(m_Shader);
        glValidateProgramARB(m_Shader);
    }
    else
        return false;

    glGetShaderiv(m_Shader, GL_COMPILE_STATUS, &val);
    if (val != GL_TRUE)
    {
        LOG("CGLShader::Init shader program compilation error:\n");
        auto error = glGetError();
        if (error != 0)
        {
            auto errorStr = gluErrorString(error);
            LOG("CGLShader::Init shader program error Code: %i\n", error);
            LOG("CGLShader::Init shader program error string: %s\n", errorStr);
        }
        return false;
    }
    LOG("shader program compiled successfully\n");

    GLint isLinked = 0;
    glGetProgramiv(m_Shader, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE)
    {
        GLint maxLength = 0;
        glGetProgramiv(m_Shader, GL_INFO_LOG_LENGTH, &maxLength);

        // The maxLength includes the NULL character
        std::vector<GLchar> infoLog(maxLength);
        glGetProgramInfoLog(m_Shader, maxLength, &maxLength, &infoLog[0]);

        // The program is useless now. So delete it.
        glDeleteProgram(m_Shader);

        LOG("shader program failed to link\n");
        LOG("%s\n", infoLog);
        std::wstring str(infoLog.begin(), infoLog.end());
        LOG("%s\n", str);
        return false;
    }

    return val == GL_TRUE;
}
//----------------------------------------------------------------------------------
CGLShader::~CGLShader()
{
    WISPFUN_DEBUG("c32_f2");
    if (m_Shader != 0)
    {
        glDeleteObjectARB(m_Shader);
        m_Shader = 0;
    }

    if (m_VertexShader != 0)
    {
        glDeleteObjectARB(m_VertexShader);
        m_VertexShader = 0;
    }

    if (m_FragmentShader != 0)
    {
        glDeleteObjectARB(m_FragmentShader);
        m_FragmentShader = 0;
    }

    m_TexturePointer = 0;
}
//----------------------------------------------------------------------------------
bool CGLShader::Use()
{
    WISPFUN_DEBUG("c32_f3");
    UnuseShader();

    bool result = false;

    if (m_Shader != 0)
    {
        glUseProgram(m_Shader);
        g_ClientShaderActive = true;
        result = true;
    }

    return result;
}
//----------------------------------------------------------------------------------
void CGLShader::Pause()
{
    WISPFUN_DEBUG("c32_f4");
    glUseProgramObjectARB(0);
    g_ClientShaderActive = false;
}
//----------------------------------------------------------------------------------
void CGLShader::Resume()
{
    WISPFUN_DEBUG("c32_f5");
    glUseProgramObjectARB(m_Shader);
    g_ClientShaderActive = (m_Shader != 0);
}
//----------------------------------------------------------------------------------
//-----------------------------------CDeathShader-----------------------------------
//----------------------------------------------------------------------------------
CDeathShader::CDeathShader()
    : CGLShader()
{
    WISPFUN_DEBUG("c33_f1");
}
//----------------------------------------------------------------------------------
bool CDeathShader::Init(const char *vertexShaderData, const char *fragmentShaderData)
{
    if (CGLShader::Init(vertexShaderData, fragmentShaderData))
        m_TexturePointer = glGetUniformLocationARB(m_Shader, "usedTexture");
    else
        LOG("Failed to create DeathShader\n");

    return (m_Shader != 0);
}
//----------------------------------------------------------------------------------
bool CDeathShader::Use()
{
    return CGLShader::Use();
}
//----------------------------------------------------------------------------------
//----------------------------------CColorizerShader--------------------------------
//----------------------------------------------------------------------------------
CColorizerShader::CColorizerShader()
    : CGLShader()
{
    WISPFUN_DEBUG("c34_f1");
}
//----------------------------------------------------------------------------------
bool CColorizerShader::Init(const char *vertexShaderData, const char *fragmentShaderData)
{
    if (CGLShader::Init(vertexShaderData, fragmentShaderData))
    {
        m_TexturePointer = glGetUniformLocationARB(m_Shader, "usedTexture");
        m_ColorTablePointer = glGetUniformLocationARB(m_Shader, "colors");
        m_DrawModePointer = glGetUniformLocationARB(m_Shader, "drawMode");
    }
    else
        LOG("Failed to create ColorizerShader\n");

    return (m_Shader != 0);
}
//----------------------------------------------------------------------------------
bool CColorizerShader::Use()
{
    WISPFUN_DEBUG("c34_f2");
    bool result = CGLShader::Use();

    if (result)
    {
        ShaderColorTable = m_ColorTablePointer;
        g_ShaderDrawMode = m_DrawModePointer;
        glUniform1iARB(g_ShaderDrawMode, SDM_NO_COLOR);
    }

    return result;
}
//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
#endif // ORION_GLES
//----------------------------------------------------------------------------------
