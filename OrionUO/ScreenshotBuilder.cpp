// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/***********************************************************************************
**
** ScreenshotBuilder.h
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
#include "FileSystem.h"

CScreenshotBuilder g_ScreenshotBuilder;
//---------------------------------------------------------------------------
CScreenshotBuilder::CScreenshotBuilder()
{
}
//---------------------------------------------------------------------------
CScreenshotBuilder::~CScreenshotBuilder()
{
}
//---------------------------------------------------------------------------
void CScreenshotBuilder::SaveScreen()
{
    WISPFUN_DEBUG("c204_f1");
    SaveScreen(0, 0, g_OrionWindow.GetSize().Width, g_OrionWindow.GetSize().Height);
}
//---------------------------------------------------------------------------
#if defined(ORION_GLES)
// Screenshots go through FreeImage, which is the only thing in the client that
// uses it and is not cross-built for Android. Saving is skipped rather than
// reimplemented; nothing else depends on it.
void CScreenshotBuilder::SaveScreen(int x, int y, int width, int height)
{
    UNUSED(x);
    UNUSED(y);
    UNUSED(width);
    UNUSED(height);
    LOG("Screenshots are not available on this build.\n");
}
#else
void CScreenshotBuilder::SaveScreen(int x, int y, int width, int height)
{
    WISPFUN_DEBUG("c204_f2");
    auto path = g_App.ExeFilePath("snapshots");
    fs_path_create(path);

    SYSTEMTIME st;
    GetLocalTime(&st);

    char buf[100] = { 0 };

    sprintf_s(
        buf,
        "/snapshot_d(%i.%i.%i)_t(%i.%i.%i_%i)",
        st.wYear,
        st.wMonth,
        st.wDay,
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMilliseconds);

    path += ToPath(buf);

    UINT_LIST pixels = GetScenePixels(x, y, width, height);

    FIBITMAP *fBmp =
        FreeImage_ConvertFromRawBits((puchar)&pixels[0], width, height, width * 4, 32, 0, 0, 0);

    FREE_IMAGE_FORMAT format = FIF_BMP;

    switch (g_ConfigManager.ScreenshotFormat)
    {
        case SF_PNG:
        {
            path += ToPath(".png");
            format = FIF_PNG;
            break;
        }
        case SF_TIFF:
        {
            path += ToPath(".tiff");
            format = FIF_TIFF;
            break;
        }
        case SF_JPEG:
        {
            path += ToPath(".jpeg");
            format = FIF_JPEG;
            break;
        }
        default:
        {
            path += ToPath(".bmp");
            format = FIF_BMP;
            break;
        }
    }

    FreeImage_Save(format, fBmp, CStringFromPath(path));

    FreeImage_Unload(fBmp);

    if (g_GameState >= GS_GAME)
        g_Orion.CreateTextMessageF(3, 0, "Screenshot saved to: %s", CStringFromPath(path));
}
#endif
//---------------------------------------------------------------------------
UINT_LIST CScreenshotBuilder::GetScenePixels(int x, int y, int width, int height)
{
    WISPFUN_DEBUG("c204_f3");
    UINT_LIST pixels(width * height);

#if defined(ORION_GLES)
    // GLES guarantees only GL_RGBA/GL_UNSIGNED_BYTE for glReadPixels, so read
    // bytes and pack them into the ARGB words the caller expects, rather than
    // relying on BGRA with a _REV type.
    std::vector<uchar> raw((size_t)width * height * 4);
    glReadPixels(
        x,
        g_OrionWindow.GetSize().Height - y - height,
        width,
        height,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        &raw[0]);

    for (size_t i = 0; i < pixels.size(); i++)
    {
        pixels[i] = ((uint)raw[i * 4 + 0] << 16) | ((uint)raw[i * 4 + 1] << 8) |
                    (uint)raw[i * 4 + 2];
    }
#else
    glReadPixels(
        x,
        // glReadPixels works in framebuffer pixels, which on a high-DPI display
        // is larger than the window's logical size.
        (int)((g_OrionWindow.GetSize().Height - y - height) * g_OrionWindow.GetPixelRatio()),
        width,
        height,
        GL_BGRA,
        GL_UNSIGNED_INT_8_8_8_8_REV,
        &pixels[0]);
#endif

    for (uint &i : pixels)
        i |= 0xFF000000;

    return pixels;
}
//---------------------------------------------------------------------------