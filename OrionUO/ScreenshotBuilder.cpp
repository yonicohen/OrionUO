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

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_WRITE_NO_STDIO_DEPRECATE
#include "../third_party/stb_image_write.h"

namespace
{
// stb_image_write covers PNG, BMP and JPEG but not TIFF, which the client has
// always offered. A baseline uncompressed RGB TIFF is a header, the pixels, and
// one directory of eight tags - small enough to write here rather than carry a
// library for it.
void PutLE16(std::vector<unsigned char> &out, unsigned short value)
{
    out.push_back((unsigned char)(value & 0xFF));
    out.push_back((unsigned char)(value >> 8));
}

void PutLE32(std::vector<unsigned char> &out, unsigned int value)
{
    for (int shift = 0; shift < 32; shift += 8)
        out.push_back((unsigned char)((value >> shift) & 0xFF));
}

void PutTag(
    std::vector<unsigned char> &out, unsigned short tag, unsigned short type, unsigned int count,
    unsigned int value)
{
    PutLE16(out, tag);
    PutLE16(out, type);
    PutLE32(out, count);
    PutLE32(out, value);
}

bool WriteTIFF(
    const string &path, int width, int height, const std::vector<unsigned char> &rgb)
{
    const unsigned int headerSize = 8;
    const unsigned int pixelBytes = (unsigned int)rgb.size();

    std::vector<unsigned char> out;
    out.reserve(headerSize + pixelBytes + 128);

    out.push_back('I'); // little endian
    out.push_back('I');
    PutLE16(out, 42);
    PutLE32(out, headerSize + pixelBytes); // the directory follows the pixels

    out.insert(out.end(), rgb.begin(), rgb.end());

    const unsigned int directory = headerSize + pixelBytes;
    const unsigned short entries = 8;

    // BitsPerSample needs three shorts, which do not fit in a tag's four bytes,
    // so they live after the directory and the tag points at them.
    const unsigned int bitsAt = directory + 2 + entries * 12 + 4;

    PutLE16(out, entries);
    PutTag(out, 256, 3, 1, (unsigned int)width);   // ImageWidth
    PutTag(out, 257, 3, 1, (unsigned int)height);  // ImageLength
    PutTag(out, 258, 3, 3, bitsAt);                // BitsPerSample
    PutTag(out, 259, 3, 1, 1);                     // Compression: none
    PutTag(out, 262, 3, 1, 2);                     // Photometric: RGB
    PutTag(out, 273, 4, 1, headerSize);            // StripOffsets
    PutTag(out, 277, 3, 1, 3);                     // SamplesPerPixel
    PutTag(out, 279, 4, 1, pixelBytes);            // StripByteCounts
    PutLE32(out, 0);                               // no next directory

    PutLE16(out, 8);
    PutLE16(out, 8);
    PutLE16(out, 8);

    FILE *file = fopen(path.c_str(), "wb");
    if (file == nullptr)
        return false;

    const bool ok = fwrite(&out[0], 1, out.size(), file) == out.size();
    fclose(file);
    return ok;
}
} // namespace
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

    // glReadPixels gave us BGRA, bottom row first. Every writer below wants RGB
    // top row first, and the alpha was forced opaque anyway.
    std::vector<unsigned char> rgb((size_t)width * height * 3);

    for (int row = 0; row < height; row++)
    {
        const uint *source = &pixels[(size_t)(height - 1 - row) * width];
        unsigned char *target = &rgb[(size_t)row * width * 3];

        for (int column = 0; column < width; column++)
        {
            const uint pixel = source[column];
            target[column * 3 + 0] = (unsigned char)((pixel >> 16) & 0xFF); // red
            target[column * 3 + 1] = (unsigned char)((pixel >> 8) & 0xFF);  // green
            target[column * 3 + 2] = (unsigned char)(pixel & 0xFF);         // blue
        }
    }

    bool written = false;

    switch (g_ConfigManager.ScreenshotFormat)
    {
        case SF_PNG:
        {
            path += ToPath(".png");
            written = stbi_write_png(
                          CStringFromPath(path), width, height, 3, &rgb[0], width * 3) != 0;
            break;
        }
        case SF_TIFF:
        {
            path += ToPath(".tiff");
            written = WriteTIFF(CStringFromPath(path), width, height, rgb);
            break;
        }
        case SF_JPEG:
        {
            path += ToPath(".jpeg");
            written =
                stbi_write_jpg(CStringFromPath(path), width, height, 3, &rgb[0], 90) != 0;
            break;
        }
        default:
        {
            path += ToPath(".bmp");
            written = stbi_write_bmp(CStringFromPath(path), width, height, 3, &rgb[0]) != 0;
            break;
        }
    }

    if (!written)
    {
        LOG("Failed to write screenshot to %s\n", CStringFromPath(path));
        return;
    }

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