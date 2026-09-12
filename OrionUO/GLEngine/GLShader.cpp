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
// GLES 1.x has no programmable pipeline, and this path is written against the
// ARB extension entry points, which GLES does not provide under any name or
// version. The shaders are stubbed out rather than faked: every caller already
// copes with Init() failing - Use() returns false and drawing falls back to the
// fixed function pipeline - so the client renders without hue colorisation
// instead of not rendering at all.
//
// Hues are not cosmetic in UO, so this is a stopgap. Restoring them means a
// GLES 2.0 renderer, which also has to replace the fixed function matrix stack
// and the client-side vertex arrays this code still relies on. See docs/ANDROID.md.
//----------------------------------------------------------------------------------
// GLES 1.1 has no shaders, but it does have the texture combiners, and the one
// effect the client cannot do without is the greyscale it draws the world in
// while the player is dead. It is built here out of two texture stages.
//
// DOT3_RGB computes 4*((A-0.5).(B-0.5)), which is a dot product biased around
// a half. Feeding it the texture directly gives luminance minus a half, and the
// bottom half of that clamps to black. So the first stage rescales the texel
// into [0.5, 1] (0.5*T + 0.5, which is INTERPOLATE against white with a factor
// of a half) and the second dots that against a constant carrying the same bias
// - the two halves cancel and what comes out is the luminance itself.
//
// Alpha is left alone throughout: it still comes from the texture modulated by
// the vertex colour, so the alpha test that masks UO art keeps working.
static const float GrayLuminance[3] = { 0.299f, 0.587f, 0.114f };
static GLuint g_GrayDummyTexture = 0;
static bool g_GrayscaleActive = false;

static void EndGrayscale()
{
    if (!g_GrayscaleActive)
        return;

    g_GrayscaleActive = false;

    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // The bound texture is tracked by GLEngine, and unit 0 was never rebound,
    // so nothing has to be restored here beyond the environment itself.
}

static bool BeginGrayscale()
{
    if (g_GrayscaleActive)
        return true;

    GLint units = 0;
    glGetIntegerv(GL_MAX_TEXTURE_UNITS, &units);

    if (units < 2)
        return false;

    if (g_GrayDummyTexture == 0)
    {
        // Unit 1 reads neither this texture nor any texture coordinate - its
        // combiner sources are PREVIOUS and CONSTANT - but the unit still has to
        // have a complete texture bound to be enabled at all.
        const uchar white[4] = { 0xFF, 0xFF, 0xFF, 0xFF };

        glGenTextures(1, &g_GrayDummyTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, g_GrayDummyTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        glActiveTexture(GL_TEXTURE0);
    }

    // Stage 0: colour becomes 0.5*texel + 0.5, alpha stays texel * vertex.
    const float halfWhite[4] = { 1.0f, 1.0f, 1.0f, 0.5f };

    glActiveTexture(GL_TEXTURE0);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, halfWhite);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_INTERPOLATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC2_RGB, GL_CONSTANT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND2_RGB, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_TEXTURE);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_ALPHA, GL_PRIMARY_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
    glTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1.0f);

    // Stage 1: dot the rescaled colour against the weights, carry alpha through.
    // DOT3 is undefined with a scale other than one, so it is left at one.
    const float weights[4] = { 0.5f + GrayLuminance[0] * 0.5f,
                               0.5f + GrayLuminance[1] * 0.5f,
                               0.5f + GrayLuminance[2] * 0.5f,
                               1.0f };

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, g_GrayDummyTexture);
    glEnable(GL_TEXTURE_2D);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
    glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, weights);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_DOT3_RGB);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_RGB, GL_PREVIOUS);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC1_RGB, GL_CONSTANT);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
    glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
    glTexEnvi(GL_TEXTURE_ENV, GL_SRC0_ALPHA, GL_PREVIOUS);
    glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
    glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE, 1.0f);
    glTexEnvf(GL_TEXTURE_ENV, GL_ALPHA_SCALE, 1.0f);

    glActiveTexture(GL_TEXTURE0);

    g_GrayscaleActive = true;
    return true;
}

void UnuseShader()
{
    EndGrayscale();
    ShaderColorTable = 0;
    g_ShaderDrawMode = 0;
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
    UnuseShader();
    return false;
}
//----------------------------------------------------------------------------------
void CGLShader::Pause()
{
}
//----------------------------------------------------------------------------------
void CGLShader::Resume()
{
}
//----------------------------------------------------------------------------------
CDeathShader::CDeathShader()
    : CGLShader()
{
}
//----------------------------------------------------------------------------------
bool CDeathShader::Init(const char * /*vertexShaderData*/, const char * /*fragmentShaderData*/)
{
    return false;
}
//----------------------------------------------------------------------------------
bool CDeathShader::Use()
{
    ShaderColorTable = 0;
    g_ShaderDrawMode = 0;
    return BeginGrayscale();
}
//----------------------------------------------------------------------------------
CColorizerShader::CColorizerShader()
    : CGLShader()
{
}
//----------------------------------------------------------------------------------
bool CColorizerShader::Init(const char * /*vertexShaderData*/, const char * /*fragmentShaderData*/)
{
    return false;
}
//----------------------------------------------------------------------------------
bool CColorizerShader::Use()
{
    return false;
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
