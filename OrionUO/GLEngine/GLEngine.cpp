// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/***********************************************************************************
**
** GLEngine.cpp
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
//----------------------------------------------------------------------------------
CGLEngine g_GL;

BIND_TEXTURE_16_FUNCTION g_GL_BindTexture16_Ptr = &CGLEngine::GL1_BindTexture16;
BIND_TEXTURE_32_FUNCTION g_GL_BindTexture32_Ptr = &CGLEngine::GL1_BindTexture32;

DRAW_LAND_TEXTURE_FUNCTION g_GL_DrawLandTexture_Ptr = &CGLEngine::GL1_DrawLandTexture;
DRAW_TEXTURE_FUNCTION g_GL_Draw_Ptr = &CGLEngine::GL1_Draw;
DRAW_TEXTURE_ROTATED_FUNCTION g_GL_DrawRotated_Ptr = &CGLEngine::GL1_DrawRotated;
DRAW_TEXTURE_MIRRORED_FUNCTION g_GL_DrawMirrored_Ptr = &CGLEngine::GL1_DrawMirrored;
DRAW_TEXTURE_SITTING_FUNCTION g_GL_DrawSitting_Ptr = &CGLEngine::GL1_DrawSitting;
DRAW_TEXTURE_SHADOW_FUNCTION g_GL_DrawShadow_Ptr = &CGLEngine::GL1_DrawShadow;
DRAW_TEXTURE_STRETCHED_FUNCTION g_GL_DrawStretched_Ptr = &CGLEngine::GL1_DrawStretched;
DRAW_TEXTURE_RESIZEPIC_FUNCTION g_GL_DrawResizepic_Ptr = &CGLEngine::GL1_DrawResizepic;
//----------------------------------------------------------------------------------
CGLEngine::CGLEngine()
{
}
//----------------------------------------------------------------------------------
CGLEngine::~CGLEngine()
{
    WISPFUN_DEBUG("c29_f1");
    if (PositionBuffer != 0)
    {
        glDeleteBuffers(1, &PositionBuffer);
        PositionBuffer = 0;
    }

    Uninstall();
}
//----------------------------------------------------------------------------------
bool CGLEngine::GLSetupPixelFormat()
{
#if USE_WISP
    WISPFUN_DEBUG("c29_f2");
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),                                  //nSize
        1,                                                              //nVersion
        /*PFD_DRAW_TO_WINDOW |*/ PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, //dwFlags
        PFD_TYPE_RGBA,                                                  //iPixelType
        16,                                                             //cColorBits
        0,
        0, //cRedBits, cRedShift
        0,
        0, //cGreenBits, cGreenShift
        0,
        0, //cBlueBits, cBlueShift
        0,
        0, //cAlphaBits, cAlphaShift
        0,
        0,
        0,
        0,
        0,  //cAccumBits, cAccumRedBits, cAccumGreenBits, cAccumBlueBits, cAccumAlphaBits
        16, //cDepthBits
        1,  //cStencilBits
        0,  //cAuxBuffers
        PFD_MAIN_PLANE, //iLayerType
        0,              //bReserved
        0,              //dwLayerMask
        0,              //dwVisibleMask
        0               //dwDamageMask
    };

    int pixelformat = ChoosePixelFormat(DC, &pfd);

    if (!pixelformat)
    {
        MessageBox(NULL, L"ChoosePixelFormat failed", L"Error", MB_OK);
        return false;
    }

    if (!SetPixelFormat(DC, pixelformat, &pfd))
    {
        MessageBox(NULL, L"SetPixelFormat failed", L"Error", MB_OK);
        return false;
    }
#endif
    return true;
}
//----------------------------------------------------------------------------------
bool CGLEngine::Install()
{
    WISPFUN_DEBUG("c29_f3");
    OldTexture = -1;

#if USE_WISP
    DC = ::GetDC(g_OrionWindow.Handle);
    if (!GLSetupPixelFormat())
        return false;

    RC = wglCreateContext(DC);
    if (!RC)
        return false;

    if (!wglMakeCurrent(DC, RC))
        return false;
#else
    m_context = SDL_GL_CreateContext(g_OrionWindow.m_window);
    if (m_context == nullptr)
    {
        LOG("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }

    if (SDL_GL_MakeCurrent(g_OrionWindow.m_window, m_context) < 0)
    {
        LOG("SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        return false;
    }
#endif

    int glewInitResult = glewInit();
    LOG("glewInit() = %i fb=%i v(%s) (shader: %i)\n",
        glewInitResult,
        GL_ARB_framebuffer_object,
        glGetString(GL_VERSION),
        GL_ARB_shader_objects);
    if (glewInitResult)
        return false;

    LOG("Graphics Successfully Initialized\n");
    LOG("OpenGL Info:\n");
    LOG("    Version: %s\n", glGetString(GL_VERSION));
    LOG("     Vendor: %s\n", glGetString(GL_VENDOR));
    LOG("   Renderer: %s\n", glGetString(GL_RENDERER));
    LOG("    Shading: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

    CanUseFrameBuffer =
        (GL_ARB_framebuffer_object && glBindFramebuffer && glDeleteFramebuffers &&
         glFramebufferTexture2D && glGenFramebuffers);

    CanUseBuffer =
        (GL_VERSION_1_5 && glBindBuffer && glBufferData && glDeleteBuffers && glGenBuffers);

    CanUseBuffer = false;

    if (CanUseBuffer)
    {
        glGenBuffers(3, &PositionBuffer);

        int positionArray[] = { 0, 1, 1, 1, 0, 0, 1, 0 };

        glBindBuffer(GL_ARRAY_BUFFER, PositionBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(positionArray), &positionArray[0], GL_STATIC_DRAW);

        g_GL_BindTexture16_Ptr = &CGLEngine::GL2_BindTexture16;
        g_GL_BindTexture32_Ptr = &CGLEngine::GL2_BindTexture32;

        g_GL_DrawLandTexture_Ptr = &CGLEngine::GL2_DrawLandTexture;
        g_GL_Draw_Ptr = &CGLEngine::GL2_Draw;
        g_GL_DrawRotated_Ptr = &CGLEngine::GL2_DrawRotated;
        g_GL_DrawMirrored_Ptr = &CGLEngine::GL2_DrawMirrored;
        g_GL_DrawSitting_Ptr = &CGLEngine::GL2_DrawSitting;
        g_GL_DrawShadow_Ptr = &CGLEngine::GL2_DrawShadow;
        g_GL_DrawStretched_Ptr = &CGLEngine::GL2_DrawStretched;
        g_GL_DrawResizepic_Ptr = &CGLEngine::GL2_DrawResizepic;
    }

    LOG("g_UseFrameBuffer = %i; CanUseBuffer = %i\n", CanUseFrameBuffer, CanUseBuffer);

    if (!CanUseFrameBuffer && g_ShowWarnings)
        g_OrionWindow.ShowMessage("Your graphics card does not support Frame Buffers!", "Warning!");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black Background
    glShadeModel(GL_SMOOTH);              // Enables Smooth Color Shading
    glClearDepth(1.0);                    // Depth Buffer Setup
    glDisable(GL_DITHER);

    //glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);   //Realy Nice perspective calculations
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);

    glEnable(GL_TEXTURE_2D);

#if USE_WISP
    typedef BOOL(WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int interval);
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
        (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT != NULL)
        wglSwapIntervalEXT(0);
#else
    // Frames are paced by the client's own timer, so vsync is a second limiter on
    // top of it rather than the only one. What it buys is the absence of tearing:
    // without it a swap lands mid-scanout whenever the frame timer and the display
    // refresh disagree, which they usually do.
    //
    // Late swap tearing first - it syncs when it can and skips the wait when a
    // frame runs long, rather than dropping to half rate - and plain vsync if the
    // driver has no such thing.
    if (g_UseVSync)
    {
        if (SDL_GL_SetSwapInterval(-1) < 0)
            SDL_GL_SetSwapInterval(1);
    }
    else
        SDL_GL_SetSwapInterval(0);

    LOG("VSync: %s (swap interval %d)\n",
        g_UseVSync ? "on" : "off",
        SDL_GL_GetSwapInterval());
#endif

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glClearStencil(0);
    glStencilMask(1);

    glEnable(GL_LIGHT0);

    GLfloat lightPosition[] = { -1.0f, -1.0f, 0.5f, 0.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, &lightPosition[0]);

    GLfloat lightAmbient[] = { 2.0f, 2.0f, 2.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, &lightAmbient[0]);

    GLfloat lav = 0.8f;
    GLfloat lightAmbientValues[] = { lav, lav, lav, lav };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, &lightAmbientValues[0]);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);

    // Build the shader pipeline that CGLVertexBatch can draw through. It is the
    // path a Core profile or GLES 2.0 will require; drawing through it now, in a
    // context that still has the fixed function pipeline, is what makes the two
    // comparable. Falling back costs nothing if the driver refuses it.
    g_GLBatch.UseShaders = g_GLBatchShader.Init();
    LOG("Vertex batch path: %s\n", g_GLBatch.UseShaders ? "shader + vertex buffer" : "fixed function arrays");

    ViewPort(0, 0, g_OrionWindow.GetSize().Width, g_OrionWindow.GetSize().Height);

    return true;
}
//----------------------------------------------------------------------------------
void CGLEngine::Uninstall()
{
    WISPFUN_DEBUG("c29_f4");
#if USE_WISP
    DC = 0;
    wglMakeCurrent(NULL, NULL);

    if (RC != 0)
    {
        wglDeleteContext(RC);
        RC = 0;
    }
#else
    if (m_context)
        SDL_GL_DeleteContext(m_context);
#endif
}
//----------------------------------------------------------------------------------
void CGLEngine::UpdateRect()
{
#if USE_WISP
    WISPFUN_DEBUG("c29_f5");
    RECT cr;
    GetClientRect(g_OrionWindow.Handle, &cr);
    int width = cr.right - cr.left;
    int height = cr.bottom - cr.top;
#else
    int width, height;
    // Logical points, not framebuffer pixels: the scene scale and the mouse
    // both work in points, and ApplySceneProjection converts to pixels.
    SDL_GetWindowSize(g_OrionWindow.m_window, &width, &height);
#endif

    // In the world the UI is drawn at window resolution, one scene unit per
    // pixel. Before that, every screen is 640x480 artwork with hardcoded
    // coordinates and no ability to reflow, so scale it up to fill the window
    // instead - preserving aspect ratio and centring what is left over. The
    // alternative, and what this used to do, was to shrink the window itself to
    // 640x480 on the way to the login screen and grow it again on the way out.
    if (g_GameState < GS_GAME)
    {
        const float scaleX = (float)width / (float)SceneWidth;
        const float scaleY = (float)height / (float)SceneHeight;
        SceneScale = (scaleX < scaleY) ? scaleX : scaleY;

        const int scaledWidth = (int)(SceneWidth * SceneScale);
        const int scaledHeight = (int)(SceneHeight * SceneScale);
        SceneOffsetX = (width - scaledWidth) / 2;
        SceneOffsetY = (height - scaledHeight) / 2;
        LOG("UpdateRect: window %dx%d (x%.1f dpi) state=%d scale=%.2f offset=%d,%d\n",
            width,
            height,
            g_OrionWindow.GetPixelRatio(),
            (int)g_GameState,
            SceneScale,
            SceneOffsetX,
            SceneOffsetY);

        // Letterbox bars are never drawn into, so clear the whole window once
        // here; otherwise they keep whatever was last rendered at that size.
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ApplySceneProjection();
    }
    else
    {
        SceneScale = 1.0f;
        SceneOffsetX = 0;
        SceneOffsetY = 0;
        LOG("UpdateRect: window %dx%d (x%.1f dpi) state=%d (in world, 1:1)\n",
            width,
            height,
            g_OrionWindow.GetPixelRatio(),
            (int)g_GameState);
        ViewPort(0, 0, width, height);
    }

    g_GumpManager.RedrawAll();
}
//----------------------------------------------------------------------------------
WISP_GEOMETRY::CPoint2Di CGLEngine::WindowToScene(int x, int y) const
{
    if (SceneScale <= 0.0f)
        return WISP_GEOMETRY::CPoint2Di(x, y);

    return WISP_GEOMETRY::CPoint2Di(
        (int)((x - SceneOffsetX) / SceneScale), (int)((y - SceneOffsetY) / SceneScale));
}
//----------------------------------------------------------------------------------
void CGLEngine::GL1_BindTexture16(CGLTexture &texture, int width, int height, pushort pixels)
{
    WISPFUN_DEBUG("c29_f6");
    GLuint tex = 0;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // Linear magnification: the pre-game screens are 640x480 artwork scaled up
    // to fill the window, and GL_NEAREST makes that visibly blocky. At 1:1, which
    // is what the world is drawn at, the two are indistinguishable.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // Upload at doubled resolution where it is worth it. Width and Height below
    // stay logical, so nothing above the renderer sees a difference.
    std::vector<ushort> upscaled;
    if (texture.AllowUpscale)
        GLArtUpscale::Double16(pixels, width, height, upscaled);
    const bool doubled = !upscaled.empty();

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB5_A1,
        doubled ? width * 2 : width,
        doubled ? height * 2 : height,
        0,
        GL_BGRA,
        GL_UNSIGNED_SHORT_1_5_5_5_REV,
        doubled ? &upscaled[0] : pixels);

    texture.Width = width;
    texture.Height = height;
    texture.TexelWidth = doubled ? width * 2 : width;
    texture.TexelHeight = doubled ? height * 2 : height;
    texture.Texture = tex;

    if (IgnoreHitMap)
        return;

    HIT_MAP_TYPE &hitMap = texture.m_HitMap;
    hitMap.resize(width * height);
    int pos = 0;

    IFOR (y, 0, height)
    {
        IFOR (x, 0, width)
        {
            hitMap[pos] = (pixels[pos] != 0);
            pos++;
        }
    }
}
//----------------------------------------------------------------------------------
void CGLEngine::GL1_BindTexture32(CGLTexture &texture, int width, int height, puint pixels)
{
    WISPFUN_DEBUG("c29_f7");
    GLuint tex = 0;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    std::vector<uint> upscaled;
    if (texture.AllowUpscale)
        GLArtUpscale::Double32(pixels, width, height, upscaled);
    const bool doubled = !upscaled.empty();

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA4,
        doubled ? width * 2 : width,
        doubled ? height * 2 : height,
        0,
        GL_BGRA,
        GL_UNSIGNED_INT_8_8_8_8,
        doubled ? &upscaled[0] : pixels);

    texture.Width = width;
    texture.Height = height;
    texture.TexelWidth = doubled ? width * 2 : width;
    texture.TexelHeight = doubled ? height * 2 : height;
    texture.Texture = tex;

    if (IgnoreHitMap)
        return;

    HIT_MAP_TYPE &hitMap = texture.m_HitMap;
    hitMap.resize(width * height);
    int pos = 0;

    IFOR (y, 0, height)
    {
        IFOR (x, 0, width)
        {
            hitMap[pos] = (pixels[pos] != 0);
            pos++;
        }
    }
}
//----------------------------------------------------------------------------------
void CGLEngine::GL2_CreateArrays(CGLTexture &texture, int width, int height)
{
    WISPFUN_DEBUG("c29_f8");
    GLuint vbo[2] = { 0 };
    glGenBuffers(2, &vbo[0]);

    int vertexArray[] = { 0, height, width, height, 0, 0, width, 0 };

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexArray), &vertexArray[0], GL_STATIC_DRAW);

    int mirroredVertexArray[] = { width, height, 0, height, width, 0, 0, 0 };

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(mirroredVertexArray), &mirroredVertexArray[0], GL_STATIC_DRAW);

    texture.VertexBuffer = vbo[0];
    texture.MirroredVertexBuffer = vbo[1];
}
//----------------------------------------------------------------------------------
void CGLEngine::GL2_BindTexture16(CGLTexture &texture, int width, int height, pushort pixels)
{
    WISPFUN_DEBUG("c29_f9");
    GL1_BindTexture16(texture, width, height, pixels);
    GL2_CreateArrays(texture, width, height);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL2_BindTexture32(CGLTexture &texture, int width, int height, puint pixels)
{
    WISPFUN_DEBUG("c29_f10");
    GL1_BindTexture32(texture, width, height, pixels);
    GL2_CreateArrays(texture, width, height);
}
//----------------------------------------------------------------------------------
void CGLEngine::BeginDraw()
{
    WISPFUN_DEBUG("c29_f11");
    Drawing = true;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // Resets the stack's modelview as well as GL's. Without this ours would
    // accumulate every translation ever applied, since nothing else clears it.
    g_GLMatrix.LoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);

    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);

    if (CanUseBuffer)
    {
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }
}
//----------------------------------------------------------------------------------
void CGLEngine::EndDraw()
{
    WISPFUN_DEBUG("c29_f12");
    Drawing = false;

    if (CanUseBuffer)
    {
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    glDisable(GL_ALPHA_TEST);

#if USE_WISP
    SwapBuffers(DC);
#else
    SDL_GL_SwapWindow(WISP_WINDOW::g_WispWindow->m_window);
#endif
}
//----------------------------------------------------------------------------------
void CGLEngine::BeginStencil()
{
    WISPFUN_DEBUG("c29_f13");
    glEnable(GL_STENCIL_TEST);

    glColorMask(false, false, false, false);

    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
}
//----------------------------------------------------------------------------------
void CGLEngine::EndStencil()
{
    WISPFUN_DEBUG("c29_f14");
    glColorMask(true, true, true, true);

    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_NOTEQUAL, 1, 1);

    glDisable(GL_STENCIL_TEST);
}
//----------------------------------------------------------------------------------
void CGLEngine::ViewPortScaled(int x, int y, int width, int height)
{
    WISPFUN_DEBUG("c29_f15");
    // The viewport is in framebuffer pixels; everything drawn through the
    // projection below stays in logical points, so on a high-DPI display the
    // same drawing lands on four times as many pixels and nothing that lays out
    // the UI has to know.
    const float pixelRatio = g_OrionWindow.GetPixelRatio();
    glViewport(
        (int)(x * pixelRatio),
        (int)((g_OrionWindow.GetSize().Height - y - height) * pixelRatio),
        (int)(width * pixelRatio),
        (int)(height * pixelRatio));


    GLdouble left = (GLdouble)x;
    GLdouble right = (GLdouble)(width + x);
    GLdouble top = (GLdouble)y;
    GLdouble bottom = (GLdouble)(height + y);

    GLdouble newRight = right * g_GlobalScale;
    GLdouble newBottom = bottom * g_GlobalScale;

    left = (left * g_GlobalScale) - (newRight - right);
    top = (top * g_GlobalScale) - (newBottom - bottom);

    g_GLMatrix.Ortho((float)left, (float)newRight, (float)newBottom, (float)top, -150.0f, 150.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::ViewPort(int x, int y, int width, int height)
{
    WISPFUN_DEBUG("c29_f16");
    // The viewport is in framebuffer pixels; everything drawn through the
    // projection below stays in logical points, so on a high-DPI display the
    // same drawing lands on four times as many pixels and nothing that lays out
    // the UI has to know.
    const float pixelRatio = g_OrionWindow.GetPixelRatio();
    glViewport(
        (int)(x * pixelRatio),
        (int)((g_OrionWindow.GetSize().Height - y - height) * pixelRatio),
        (int)(width * pixelRatio),
        (int)(height * pixelRatio));
    g_GLMatrix.Ortho((float)x, (float)(width + x), (float)(height + y), (float)y, -150.0f, 150.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::ApplySceneProjection()
{
    // Before the world, the UI is a scaled and centred 640x480 scene; in the
    // world it is drawn one scene unit per window pixel. Both cases go through
    // here so that anything restoring the projection gets the right one - gumps
    // release a framebuffer every frame and each release lands in RestorePort.
    if (g_GameState < GS_GAME && SceneScale > 0.0f)
    {
        const float pixelRatio = g_OrionWindow.GetPixelRatio();
        glViewport(
            (int)(SceneOffsetX * pixelRatio),
            (int)(SceneOffsetY * pixelRatio),
            (int)(SceneWidth * SceneScale * pixelRatio),
            (int)(SceneHeight * SceneScale * pixelRatio));
        g_GLMatrix.Ortho(0.0f, (float)SceneWidth, (float)SceneHeight, 0.0f, -150.0f, 150.0f);
        return;
    }

    const float windowPixelRatio = g_OrionWindow.GetPixelRatio();
    glViewport(
        0,
        0,
        (int)(g_OrionWindow.GetSize().Width * windowPixelRatio),
        (int)(g_OrionWindow.GetSize().Height * windowPixelRatio));
    g_GLMatrix.Ortho(
        0.0f,
        (float)g_OrionWindow.GetSize().Width,
        (float)g_OrionWindow.GetSize().Height,
        0.0f,
        -150.0f,
        150.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::RestorePort()
{
    WISPFUN_DEBUG("c29_f17");
    ApplySceneProjection();
}
//----------------------------------------------------------------------------------
void CGLEngine::PushScissor(int x, int y, int width, int height)
{
    WISPFUN_DEBUG("c29_f18");
    PushScissor(WISP_GEOMETRY::CRect(x, y, width, height));
}
//----------------------------------------------------------------------------------
void CGLEngine::PushScissor(const WISP_GEOMETRY::CPoint2Di &position, int width, int height)
{
    WISPFUN_DEBUG("c29_f19");
    PushScissor(WISP_GEOMETRY::CRect(position, width, height));
}
//----------------------------------------------------------------------------------
void CGLEngine::PushScissor(int x, int y, const WISP_GEOMETRY::CSize &size)
{
    WISPFUN_DEBUG("c29_f20");
    PushScissor(WISP_GEOMETRY::CRect(x, y, size));
}
//----------------------------------------------------------------------------------
void CGLEngine::PushScissor(
    const WISP_GEOMETRY::CPoint2Di &position, const WISP_GEOMETRY::CSize &size)
{
    WISPFUN_DEBUG("c29_f21");
    PushScissor(WISP_GEOMETRY::CRect(position, size));
}
//----------------------------------------------------------------------------------
void CGLEngine::PushScissor(const WISP_GEOMETRY::CRect &rect)
{
    WISPFUN_DEBUG("c29_f22");
    m_ScissorList.push_back(rect);

    glEnable(GL_SCISSOR_TEST);

    glScissor(rect.Position.X, rect.Position.Y, rect.Size.Width, rect.Size.Height);
}
//----------------------------------------------------------------------------------
void CGLEngine::PopScissor()
{
    WISPFUN_DEBUG("c29_f23");
    if (!m_ScissorList.empty())
        m_ScissorList.pop_back();

    if (m_ScissorList.empty())
        glDisable(GL_SCISSOR_TEST);
    else
    {
        WISP_GEOMETRY::CRect &rect = m_ScissorList.back();
        glScissor(rect.Position.X, rect.Position.Y, rect.Size.Width, rect.Size.Height);
    }
}
//----------------------------------------------------------------------------------
void CGLEngine::ClearScissorList()
{
    WISPFUN_DEBUG("c29_f24");
    m_ScissorList.clear();

    glDisable(GL_SCISSOR_TEST);
}
//----------------------------------------------------------------------------------
inline void CGLEngine::BindTexture(GLuint texture)
{
    WISPFUN_DEBUG("c29_f25");
    if (OldTexture != texture)
    {
        OldTexture = texture;
        glBindTexture(GL_TEXTURE_2D, texture);
    }
}
//----------------------------------------------------------------------------------
inline void CGLEngine::BindTexture(const CGLTexture &texture)
{
    BindTexture(texture.Texture);
    g_GLBatch.SetSourceSize(
        (texture.TexelWidth > 0) ? texture.TexelWidth : texture.Width,
        (texture.TexelHeight > 0) ? texture.TexelHeight : texture.Height);
}
//----------------------------------------------------------------------------------
void CGLEngine::DrawLine(int x, int y, int targetX, int targetY)
{
    WISPFUN_DEBUG("c29_f26");
    glDisable(GL_TEXTURE_2D);

    g_GLBatch.Begin(GL_LINES, false);
    g_GLBatch.Vertex(x, y);
    g_GLBatch.Vertex(targetX, targetY);
    g_GLBatch.End();

    glEnable(GL_TEXTURE_2D);
}
//----------------------------------------------------------------------------------
void CGLEngine::DrawPolygone(int x, int y, int width, int height)
{
    WISPFUN_DEBUG("c29_f27");
    glDisable(GL_TEXTURE_2D);

    g_GLMatrix.Translate((GLfloat)x, (GLfloat)y, 0.0f);

    g_GLBatch.Begin(GL_TRIANGLE_STRIP, false);
    g_GLBatch.Vertex(0, height);
    g_GLBatch.Vertex(width, height);
    g_GLBatch.Vertex(0, 0);
    g_GLBatch.Vertex(width, 0);
    g_GLBatch.End();

    g_GLMatrix.Translate((GLfloat)-x, (GLfloat)-y, 0.0f);

    glEnable(GL_TEXTURE_2D);
}
//----------------------------------------------------------------------------------
void CGLEngine::DrawCircle(float x, float y, float radius, int gradientMode)
{
    WISPFUN_DEBUG("c29_f28");
    glDisable(GL_TEXTURE_2D);

    g_GLMatrix.Translate(x, y, 0.0f);

    g_GLBatch.Begin(GL_TRIANGLE_FAN, false);

    g_GLBatch.Vertex(0, 0);

    if (gradientMode)
        g_GLBatch.Color(0.0f, 0.0f, 0.0f, 0.0f);

    float pi = (float)M_PI * 2.0f;

    for (int i = 0; i <= 360; i++)
    {
        float a = (i / 180.0f) * pi;
        g_GLBatch.Vertex(cos(a) * radius, sin(a) * radius);
    }

    g_GLBatch.End();

    g_GLMatrix.Translate(-x, -y, 0.0f);

    glEnable(GL_TEXTURE_2D);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL1_DrawLandTexture(const CGLTexture &texture, int x, int y, CLandObject *land)
{
    WISPFUN_DEBUG("c29_f29");
    BindTexture(texture);

    float translateX = x - 22.0f;
    float translateY = y - 22.0f;

    const RECT &rc = land->m_Rect;
    CVector *normals = land->m_Normals;

    g_GLMatrix.Translate(translateX, translateY, 0.0f);

    g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
    g_GLBatch.Normal((GLfloat)normals[0].X, (GLfloat)normals[0].Y, (GLfloat)normals[0].Z);
    g_GLBatch.TexCoord(0, 0);
    g_GLBatch.Vertex(22, -rc.left); //^

    g_GLBatch.Normal((GLfloat)normals[3].X, (GLfloat)normals[3].Y, (GLfloat)normals[3].Z);
    g_GLBatch.TexCoord(0, 1);
    g_GLBatch.Vertex(0, 22 - rc.top); //<

    g_GLBatch.Normal((GLfloat)normals[1].X, (GLfloat)normals[1].Y, (GLfloat)normals[1].Z);
    g_GLBatch.TexCoord(1, 0);
    g_GLBatch.Vertex(44, 22 - rc.bottom); //>

    g_GLBatch.Normal((GLfloat)normals[2].X, (GLfloat)normals[2].Y, (GLfloat)normals[2].Z);
    g_GLBatch.TexCoord(1, 1);
    g_GLBatch.Vertex(22, 44 - rc.right); //v
    g_GLBatch.End();

    g_GLMatrix.Translate(-translateX, -translateY, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL1_Draw(const CGLTexture &texture, int x, int y)
{
    WISPFUN_DEBUG("c29_f30");
    BindTexture(texture);

    int width = texture.Width;
    int height = texture.Height;

    g_GLMatrix.Translate((GLfloat)x, (GLfloat)y, 0.0f);

    g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
    g_GLBatch.TexCoord(0, 1);
    g_GLBatch.Vertex(0, height);
    g_GLBatch.TexCoord(1, 1);
    g_GLBatch.Vertex(width, height);
    g_GLBatch.TexCoord(0, 0);
    g_GLBatch.Vertex(0, 0);
    g_GLBatch.TexCoord(1, 0);
    g_GLBatch.Vertex(width, 0);
    g_GLBatch.End();

    g_GLMatrix.Translate((GLfloat)-x, (GLfloat)-y, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL1_DrawRotated(const CGLTexture &texture, int x, int y, float angle)
{
    WISPFUN_DEBUG("c29_f31");
    BindTexture(texture);

    int width = texture.Width;
    int height = texture.Height;

    GLfloat translateY = (GLfloat)(y - height);

    g_GLMatrix.Translate((GLfloat)x, translateY, 0.0f);

    g_GLMatrix.Rotate(angle, 0.0f, 0.0f, 1.0f);

    g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
    g_GLBatch.TexCoord(0, 1);
    g_GLBatch.Vertex(0, height);
    g_GLBatch.TexCoord(1, 1);
    g_GLBatch.Vertex(width, height);
    g_GLBatch.TexCoord(0, 0);
    g_GLBatch.Vertex(0, 0);
    g_GLBatch.TexCoord(1, 0);
    g_GLBatch.Vertex(width, 0);
    g_GLBatch.End();

    g_GLMatrix.Rotate(angle, 0.0f, 0.0f, -1.0f);
    g_GLMatrix.Translate((GLfloat)-x, -translateY, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL1_DrawMirrored(const CGLTexture &texture, int x, int y, bool mirror)
{
    WISPFUN_DEBUG("c29_f32");
    BindTexture(texture);

    int width = texture.Width;
    int height = texture.Height;

    g_GLMatrix.Translate((GLfloat)x, (GLfloat)y, 0.0f);

    g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);

    if (mirror)
    {
        g_GLBatch.TexCoord(0, 1);
        g_GLBatch.Vertex(width, height);
        g_GLBatch.TexCoord(1, 1);
        g_GLBatch.Vertex(0, height);
        g_GLBatch.TexCoord(0, 0);
        g_GLBatch.Vertex(width, 0);
        g_GLBatch.TexCoord(1, 0);
        g_GLBatch.Vertex(0, 0);
    }
    else
    {
        g_GLBatch.TexCoord(0, 1);
        g_GLBatch.Vertex(0, height);
        g_GLBatch.TexCoord(1, 1);
        g_GLBatch.Vertex(width, height);
        g_GLBatch.TexCoord(0, 0);
        g_GLBatch.Vertex(0, 0);
        g_GLBatch.TexCoord(1, 0);
        g_GLBatch.Vertex(width, 0);
    }

    g_GLBatch.End();

    g_GLMatrix.Translate((GLfloat)-x, (GLfloat)-y, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL1_DrawSitting(
    const CGLTexture &texture, int x, int y, bool mirror, float h3mod, float h6mod, float h9mod)
{
    WISPFUN_DEBUG("c29_f33");
    BindTexture(texture);

    g_GLMatrix.Translate((GLfloat)x, (GLfloat)y, 0.0f);

    float width = (float)texture.Width;
    float height = (float)texture.Height;

    float h03 = height * h3mod;
    float h06 = height * h6mod;
    float h09 = height * h9mod;

    float widthOffset = (float)(width + SittingCharacterOffset);
    g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);

    if (mirror)
    {
        if (h3mod)
        {
            g_GLBatch.TexCoord(0.0f, 0.0f);
            g_GLBatch.Vertex(width, 0);
            g_GLBatch.TexCoord(1.0f, 0.0f);
            g_GLBatch.Vertex(0, 0);
            g_GLBatch.TexCoord(0.0f, h3mod);
            g_GLBatch.Vertex(width, h03);
            g_GLBatch.TexCoord(1.0f, h3mod);
            g_GLBatch.Vertex(0, h03);
        }

        if (h6mod)
        {
            if (!h3mod)
            {
                g_GLBatch.TexCoord(0.0f, 0.0f);
                g_GLBatch.Vertex(width, 0);
                g_GLBatch.TexCoord(1.0f, 0.0f);
                g_GLBatch.Vertex(0, 0);
            }

            g_GLBatch.TexCoord(0.0f, h6mod);
            g_GLBatch.Vertex(widthOffset, h06);
            g_GLBatch.TexCoord(1.0f, h6mod);
            g_GLBatch.Vertex(SittingCharacterOffset, h06);
        }

        if (h9mod)
        {
            if (!h6mod)
            {
                g_GLBatch.TexCoord(0.0f, 0.0f);
                g_GLBatch.Vertex(widthOffset, 0);
                g_GLBatch.TexCoord(1.0f, 0.0f);
                g_GLBatch.Vertex(SittingCharacterOffset, 0);
            }

            g_GLBatch.TexCoord(0.0f, 1.0f);
            g_GLBatch.Vertex(widthOffset, h09);
            g_GLBatch.TexCoord(1.0f, 1.0f);
            g_GLBatch.Vertex(SittingCharacterOffset, h09);
        }
    }
    else
    {
        if (h3mod)
        {
            g_GLBatch.TexCoord(0.0f, 0.0f);
            g_GLBatch.Vertex(SittingCharacterOffset, 0);
            g_GLBatch.TexCoord(1.0f, 0.0f);
            g_GLBatch.Vertex(widthOffset, 0);
            g_GLBatch.TexCoord(0.0f, h3mod);
            g_GLBatch.Vertex(SittingCharacterOffset, h03);
            g_GLBatch.TexCoord(1.0f, h3mod);
            g_GLBatch.Vertex(widthOffset, h03);
        }

        if (h6mod)
        {
            if (!h3mod)
            {
                g_GLBatch.TexCoord(0.0f, 0.0f);
                g_GLBatch.Vertex(SittingCharacterOffset, 0);
                g_GLBatch.TexCoord(1.0f, 0.0f);
                g_GLBatch.Vertex(width + SittingCharacterOffset, 0);
            }

            g_GLBatch.TexCoord(0.0f, h6mod);
            g_GLBatch.Vertex(0, h06);
            g_GLBatch.TexCoord(1.0f, h6mod);
            g_GLBatch.Vertex(width, h06);
        }

        if (h9mod)
        {
            if (!h6mod)
            {
                g_GLBatch.TexCoord(0.0f, 0.0f);
                g_GLBatch.Vertex(0, 0);
                g_GLBatch.TexCoord(1.0f, 0.0f);
                g_GLBatch.Vertex(width, 0);
            }

            g_GLBatch.TexCoord(0.0f, 1.0f);
            g_GLBatch.Vertex(0, h09);
            g_GLBatch.TexCoord(1.0f, 1.0f);
            g_GLBatch.Vertex(width, h09);
        }
    }

    g_GLBatch.End();

    g_GLMatrix.Translate((GLfloat)-x, (GLfloat)-y, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL1_DrawShadow(const CGLTexture &texture, int x, int y, bool mirror)
{
    WISPFUN_DEBUG("c29_f34");
    BindTexture(texture);

    float width = (float)texture.Width;
    float height = texture.Height / 2.0f;

    GLfloat translateY = (GLfloat)(y + height * 0.75);

    g_GLMatrix.Translate((GLfloat)x, translateY, 0.0f);

    g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);

    float ratio = height / width;

    if (mirror)
    {
        g_GLBatch.TexCoord(0, 1);
        g_GLBatch.Vertex(width, height);
        g_GLBatch.TexCoord(1, 1);
        g_GLBatch.Vertex(0, height);
        g_GLBatch.TexCoord(0, 0);
        g_GLBatch.Vertex(width * (ratio + 1.0f), 0);
        g_GLBatch.TexCoord(1, 0);
        g_GLBatch.Vertex(width * ratio, 0);
    }
    else
    {
        g_GLBatch.TexCoord(0, 1);
        g_GLBatch.Vertex(0, height);
        g_GLBatch.TexCoord(1, 1);
        g_GLBatch.Vertex(width, height);
        g_GLBatch.TexCoord(0, 0);
        g_GLBatch.Vertex(width * ratio, 0);
        g_GLBatch.TexCoord(1, 0);
        g_GLBatch.Vertex(width * (ratio + 1.0f), 0);
    }

    g_GLBatch.End();

    g_GLMatrix.Translate((GLfloat)-x, -translateY, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL1_DrawStretched(
    const CGLTexture &texture, int x, int y, int drawWidth, int drawHeight)
{
    WISPFUN_DEBUG("c29_f35");
    BindTexture(texture);

    int width = texture.Width;
    int height = texture.Height;

    g_GLMatrix.Translate((GLfloat)x, (GLfloat)y, 0.0f);

    float drawCountX = drawWidth / (float)width;
    float drawCountY = drawHeight / (float)height;

    g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
    g_GLBatch.TexCoord(0.0f, drawCountY);
    g_GLBatch.Vertex(0, drawHeight);
    g_GLBatch.TexCoord(drawCountX, drawCountY);
    g_GLBatch.Vertex(drawWidth, drawHeight);
    g_GLBatch.TexCoord(0.0f, 0.0f);
    g_GLBatch.Vertex(0, 0);
    g_GLBatch.TexCoord(drawCountX, 0.0f);
    g_GLBatch.Vertex(drawWidth, 0);
    g_GLBatch.End();

    g_GLMatrix.Translate((GLfloat)-x, (GLfloat)-y, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL1_DrawResizepic(CGLTexture **th, int x, int y, int width, int height)
{
    WISPFUN_DEBUG("c29_f36");

    int offsetTop = max(th[0]->Height, th[2]->Height) - th[1]->Height;
    int offsetBottom = max(th[5]->Height, th[7]->Height) - th[6]->Height;
    int offsetLeft = max(th[0]->Width, th[5]->Width) - th[3]->Width;
    int offsetRight = max(th[2]->Width, th[7]->Width) - th[4]->Width;

    IFOR (i, 0, 9)
    {
        BindTexture(th[i]->Texture);

        int drawWidth = th[i]->Width;
        int drawHeight = th[i]->Height;
        float drawCountX = 1.0f;
        float drawCountY = 1.0f;
        int drawX = x;
        int drawY = y;

        switch (i)
        {
            case 1:
            {
                drawX += th[0]->Width;

                drawWidth = width - th[0]->Width - th[2]->Width;

                drawCountX = drawWidth / (float)th[i]->Width;

                break;
            }
            case 2:
            {
                drawX += width - drawWidth;
                drawY += offsetTop;

                break;
            }
            case 3:
            {
                drawY += th[0]->Height;
                drawX += offsetLeft;

                drawHeight = height - th[0]->Height - th[5]->Height;

                drawCountY = drawHeight / (float)th[i]->Height;

                break;
            }
            case 4:
            {
                drawX += width - drawWidth - offsetRight;
                drawY += th[2]->Height;

                drawHeight = height - th[2]->Height - th[7]->Height;

                drawCountY = drawHeight / (float)th[i]->Height;

                break;
            }
            case 5:
            {
                drawY += height - drawHeight;

                break;
            }
            case 6:
            {
                drawX += th[5]->Width;
                drawY += height - drawHeight - offsetBottom;

                drawWidth = width - th[5]->Width - th[7]->Width;

                drawCountX = drawWidth / (float)th[i]->Width;

                break;
            }
            case 7:
            {
                drawX += width - drawWidth;
                drawY += height - drawHeight;

                break;
            }
            case 8:
            {
                drawX += th[0]->Width;
                drawY += th[0]->Height;

                drawWidth = width - th[0]->Width - th[2]->Width;

                drawHeight = height - th[2]->Height - th[7]->Height;

                drawCountX = drawWidth / (float)th[i]->Width;
                drawCountY = drawHeight / (float)th[i]->Height;

                break;
            }
            default:
                break;
        }

        if (drawWidth < 1 || drawHeight < 1)
            continue;

        g_GLMatrix.Translate((GLfloat)drawX, (GLfloat)drawY, 0.0f);

        g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
        g_GLBatch.TexCoord(0.0f, drawCountY);
        g_GLBatch.Vertex(0, drawHeight);
        g_GLBatch.TexCoord(drawCountX, drawCountY);
        g_GLBatch.Vertex(drawWidth, drawHeight);
        g_GLBatch.TexCoord(0.0f, 0.0f);
        g_GLBatch.Vertex(0, 0);
        g_GLBatch.TexCoord(drawCountX, 0.0f);
        g_GLBatch.Vertex(drawWidth, 0);
        g_GLBatch.End();

        g_GLMatrix.Translate((GLfloat)-drawX, (GLfloat)-drawY, 0.0f);
    }
}

//----------------------------------------------------------------------------------
void CGLEngine::GL2_DrawLandTexture(const CGLTexture &texture, int x, int y, CLandObject *land)
{
    WISPFUN_DEBUG("c29_f37");
    BindTexture(texture);

    float translateX = x - 22.0f;
    float translateY = y - 22.0f;

    g_GLMatrix.Translate(translateX, translateY, 0.0f);

    glBindBuffer(GL_ARRAY_BUFFER, land->VertexBuffer);
    glVertexPointer(2, GL_INT, 0, (PVOID)0);

    glBindBuffer(GL_ARRAY_BUFFER, land->PositionBuffer);
    glTexCoordPointer(2, GL_INT, 0, (PVOID)0);

    glBindBuffer(GL_ARRAY_BUFFER, land->NormalBuffer);
    glNormalPointer(GL_FLOAT, 0, (PVOID)0);

    glEnableClientState(GL_NORMAL_ARRAY);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableClientState(GL_NORMAL_ARRAY);

    g_GLMatrix.Translate(-translateX, -translateY, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL2_Draw(const CGLTexture &texture, int x, int y)
{
    WISPFUN_DEBUG("c29_f38");
    BindTexture(texture);

    int width = texture.Width;
    int height = texture.Height;

    g_GLMatrix.Translate((GLfloat)x, (GLfloat)y, 0.0f);

    glBindBuffer(GL_ARRAY_BUFFER, texture.VertexBuffer);
    glVertexPointer(2, GL_INT, 0, (PVOID)0);

    glBindBuffer(GL_ARRAY_BUFFER, PositionBuffer);
    glTexCoordPointer(2, GL_INT, 0, (PVOID)0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    g_GLMatrix.Translate((GLfloat)-x, (GLfloat)-y, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL2_DrawRotated(const CGLTexture &texture, int x, int y, float angle)
{
    WISPFUN_DEBUG("c29_f39");
    BindTexture(texture);

    int width = texture.Width;
    int height = texture.Height;

    GLfloat translateY = (GLfloat)(y - height);

    g_GLMatrix.Translate((GLfloat)x, translateY, 0.0f);

    g_GLMatrix.Rotate(angle, 0.0f, 0.0f, 1.0f);

    glBindBuffer(GL_ARRAY_BUFFER, texture.VertexBuffer);
    glVertexPointer(2, GL_INT, 0, (PVOID)0);

    glBindBuffer(GL_ARRAY_BUFFER, PositionBuffer);
    glTexCoordPointer(2, GL_INT, 0, (PVOID)0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    g_GLMatrix.Rotate(angle, 0.0f, 0.0f, -1.0f);
    g_GLMatrix.Translate((GLfloat)-x, -translateY, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL2_DrawMirrored(const CGLTexture &texture, int x, int y, bool mirror)
{
    WISPFUN_DEBUG("c29_f40");
    BindTexture(texture);

    int width = texture.Width;
    int height = texture.Height;

    g_GLMatrix.Translate((GLfloat)x, (GLfloat)y, 0.0f);

    if (mirror)
        glBindBuffer(GL_ARRAY_BUFFER, texture.MirroredVertexBuffer);
    else
        glBindBuffer(GL_ARRAY_BUFFER, texture.VertexBuffer);

    glVertexPointer(2, GL_INT, 0, (PVOID)0);

    glBindBuffer(GL_ARRAY_BUFFER, PositionBuffer);
    glTexCoordPointer(2, GL_INT, 0, (PVOID)0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    g_GLMatrix.Translate((GLfloat)-x, (GLfloat)-y, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL2_DrawSitting(
    const CGLTexture &texture, int x, int y, bool mirror, float h3mod, float h6mod, float h9mod)
{
    WISPFUN_DEBUG("c29_f41");
    BindTexture(texture);

    g_GLMatrix.Translate((GLfloat)x, (GLfloat)y, 0.0f);

    float width = (float)texture.Width;
    float height = (float)texture.Height;

    float h03 = height * h3mod;
    float h06 = height * h6mod;
    float h09 = height * h9mod;

    float widthOffset = (float)(width + SittingCharacterOffset);
    g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);

    if (mirror)
    {
        if (h3mod)
        {
            g_GLBatch.TexCoord(0.0f, 0.0f);
            g_GLBatch.Vertex(width, 0);
            g_GLBatch.TexCoord(1.0f, 0.0f);
            g_GLBatch.Vertex(0, 0);
            g_GLBatch.TexCoord(0.0f, h3mod);
            g_GLBatch.Vertex(width, h03);
            g_GLBatch.TexCoord(1.0f, h3mod);
            g_GLBatch.Vertex(0, h03);
        }

        if (h6mod)
        {
            if (!h3mod)
            {
                g_GLBatch.TexCoord(0.0f, 0.0f);
                g_GLBatch.Vertex(width, 0);
                g_GLBatch.TexCoord(1.0f, 0.0f);
                g_GLBatch.Vertex(0, 0);
            }

            g_GLBatch.TexCoord(0.0f, h6mod);
            g_GLBatch.Vertex(widthOffset, h06);
            g_GLBatch.TexCoord(1.0f, h6mod);
            g_GLBatch.Vertex(SittingCharacterOffset, h06);
        }

        if (h9mod)
        {
            if (!h6mod)
            {
                g_GLBatch.TexCoord(0.0f, 0.0f);
                g_GLBatch.Vertex(widthOffset, 0);
                g_GLBatch.TexCoord(1.0f, 0.0f);
                g_GLBatch.Vertex(SittingCharacterOffset, 0);
            }

            g_GLBatch.TexCoord(0.0f, 1.0f);
            g_GLBatch.Vertex(widthOffset, h09);
            g_GLBatch.TexCoord(1.0f, 1.0f);
            g_GLBatch.Vertex(SittingCharacterOffset, h09);
        }
    }
    else
    {
        if (h3mod)
        {
            g_GLBatch.TexCoord(0.0f, 0.0f);
            g_GLBatch.Vertex(SittingCharacterOffset, 0);
            g_GLBatch.TexCoord(1.0f, 0.0f);
            g_GLBatch.Vertex(widthOffset, 0);
            g_GLBatch.TexCoord(0.0f, h3mod);
            g_GLBatch.Vertex(SittingCharacterOffset, h03);
            g_GLBatch.TexCoord(1.0f, h3mod);
            g_GLBatch.Vertex(widthOffset, h03);
        }

        if (h6mod)
        {
            if (!h3mod)
            {
                g_GLBatch.TexCoord(0.0f, 0.0f);
                g_GLBatch.Vertex(SittingCharacterOffset, 0);
                g_GLBatch.TexCoord(1.0f, 0.0f);
                g_GLBatch.Vertex(width + SittingCharacterOffset, 0);
            }

            g_GLBatch.TexCoord(0.0f, h6mod);
            g_GLBatch.Vertex(0, h06);
            g_GLBatch.TexCoord(1.0f, h6mod);
            g_GLBatch.Vertex(width, h06);
        }

        if (h9mod)
        {
            if (!h6mod)
            {
                g_GLBatch.TexCoord(0.0f, 0.0f);
                g_GLBatch.Vertex(0, 0);
                g_GLBatch.TexCoord(1.0f, 0.0f);
                g_GLBatch.Vertex(width, 0);
            }

            g_GLBatch.TexCoord(0.0f, 1.0f);
            g_GLBatch.Vertex(0, h09);
            g_GLBatch.TexCoord(1.0f, 1.0f);
            g_GLBatch.Vertex(width, h09);
        }
    }

    g_GLBatch.End();

    g_GLMatrix.Translate((GLfloat)-x, (GLfloat)-y, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL2_DrawShadow(const CGLTexture &texture, int x, int y, bool mirror)
{
    WISPFUN_DEBUG("c29_f42");
    BindTexture(texture);

    float width = (float)texture.Width;
    float height = texture.Height / 2.0f;

    GLfloat translateY = (GLfloat)(y + height * 0.75);

    g_GLMatrix.Translate((GLfloat)x, translateY, 0.0f);

    float ratio = height / width;
    float verticles[8];

    if (mirror)
    {
        verticles[0] = width;
        verticles[1] = height;
        verticles[2] = 0.0f;
        verticles[3] = height;
        verticles[4] = width * (ratio + 1.0f);
        verticles[5] = 0.0f;
        verticles[6] = width * ratio;
        verticles[7] = 0.0f;
    }
    else
    {
        verticles[0] = 0.0f;
        verticles[1] = height;
        verticles[2] = width;
        verticles[3] = height;
        verticles[4] = width * ratio;
        verticles[5] = 0.0f;
        verticles[6] = width * (ratio + 1.0f);
        verticles[7] = 0.0f;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glVertexPointer(2, GL_FLOAT, 0, &verticles[0]);

    glBindBuffer(GL_ARRAY_BUFFER, PositionBuffer);
    glTexCoordPointer(2, GL_INT, 0, (PVOID)0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    g_GLMatrix.Translate((GLfloat)-x, -translateY, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL2_DrawStretched(
    const CGLTexture &texture, int x, int y, int drawWidth, int drawHeight)
{
    WISPFUN_DEBUG("c29_f43");
    BindTexture(texture);

    int width = texture.Width;
    int height = texture.Height;

    g_GLMatrix.Translate((GLfloat)x, (GLfloat)y, 0.0f);

    float drawCountX = drawWidth / (float)width;
    float drawCountY = drawHeight / (float)height;

    g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
    g_GLBatch.TexCoord(0.0f, drawCountY);
    g_GLBatch.Vertex(0, drawHeight);
    g_GLBatch.TexCoord(drawCountX, drawCountY);
    g_GLBatch.Vertex(drawWidth, drawHeight);
    g_GLBatch.TexCoord(0.0f, 0.0f);
    g_GLBatch.Vertex(0, 0);
    g_GLBatch.TexCoord(drawCountX, 0.0f);
    g_GLBatch.Vertex(drawWidth, 0);
    g_GLBatch.End();

    g_GLMatrix.Translate((GLfloat)-x, (GLfloat)-y, 0.0f);
}
//----------------------------------------------------------------------------------
void CGLEngine::GL2_DrawResizepic(CGLTexture **th, int x, int y, int width, int height)
{
    WISPFUN_DEBUG("c29_f44");
    IFOR (i, 0, 9)
    {
        BindTexture(th[i]->Texture);

        int drawWidth = th[i]->Width;
        int drawHeight = th[i]->Height;
        float drawCountX = 1.0f;
        float drawCountY = 1.0f;
        int drawX = x;
        int drawY = y;

        switch (i)
        {
            case 1:
            {
                drawX += th[0]->Width;

                drawWidth = width - th[0]->Width - th[2]->Width;

                drawCountX = drawWidth / (float)th[i]->Width;

                break;
            }
            case 2:
            {
                drawX += width - drawWidth;

                break;
            }
            case 3:
            {
                drawY += th[0]->Height;

                drawHeight = height - th[0]->Height - th[5]->Height;

                drawCountY = drawHeight / (float)th[i]->Height;

                break;
            }
            case 4:
            {
                drawX += width - drawWidth;
                drawY += th[2]->Height;

                drawHeight = height - th[2]->Height - th[7]->Height;

                drawCountY = drawHeight / (float)th[i]->Height;

                break;
            }
            case 5:
            {
                drawY += height - drawHeight;

                break;
            }
            case 6:
            {
                drawX += th[5]->Width;
                drawY += height - drawHeight;

                drawWidth = width - th[5]->Width - th[7]->Width;

                drawCountX = drawWidth / (float)th[i]->Width;

                break;
            }
            case 7:
            {
                drawX += width - drawWidth;
                drawY += height - drawHeight;

                break;
            }
            case 8:
            {
                drawX += th[0]->Width;
                drawY += th[0]->Height;

                drawWidth = width - th[0]->Width - th[2]->Width;

                drawHeight = height - th[2]->Height - th[7]->Height;

                drawCountX = drawWidth / (float)th[i]->Width;
                drawCountY = drawHeight / (float)th[i]->Height;

                break;
            }
            default:
                break;
        }

        if (drawWidth < 1 || drawHeight < 1)
            continue;

        g_GLMatrix.Translate((GLfloat)drawX, (GLfloat)drawY, 0.0f);

        g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
        g_GLBatch.TexCoord(0.0f, drawCountY);
        g_GLBatch.Vertex(0, drawHeight);
        g_GLBatch.TexCoord(drawCountX, drawCountY);
        g_GLBatch.Vertex(drawWidth, drawHeight);
        g_GLBatch.TexCoord(0.0f, 0.0f);
        g_GLBatch.Vertex(0, 0);
        g_GLBatch.TexCoord(drawCountX, 0.0f);
        g_GLBatch.Vertex(drawWidth, 0);
        g_GLBatch.End();

        g_GLMatrix.Translate((GLfloat)-drawX, (GLfloat)-drawY, 0.0f);
    }
}
//----------------------------------------------------------------------------------
