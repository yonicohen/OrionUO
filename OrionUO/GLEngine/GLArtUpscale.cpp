/***********************************************************************************
**
** GLArtUpscale.cpp
**
** See GLArtUpscale.h.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
//----------------------------------------------------------------------------------
bool g_UpscaleArt = true;
bool g_SharpFilter = true;
bool g_UseVSync = true;
//----------------------------------------------------------------------------------
namespace GLArtUpscale
{
bool Wanted(int width, int height)
{
    return g_UpscaleArt && width > 0 && height > 0 && (width * height) <= MaxSourcePixels;
}
//----------------------------------------------------------------------------------
// EPX. Each source pixel becomes a 2x2 block that starts as four copies of
// itself; a corner is replaced only when the two neighbours meeting at it agree
// with each other and disagree across the pixel. That condition is what rounds a
// diagonal step without touching a straight edge or a lone pixel.
// UO's artwork is dithered: two similar colours alternate pixel by pixel to fake
// shades the 16 bit palette does not have. Compared exactly those pixels are
// never equal, so EPX preserves every checkerboard and doubling the image makes
// the dither twice as obvious - which is what left the ornate frames looking
// mottled. Treating near-identical colours as equal lets those regions round and
// blend, while real edges, where the colours are genuinely far apart, still hold.
static bool SimilarColors(ushort a, ushort b)
{
    if (a == b)
        return true;

    // Transparent is its own thing; never merge it with a visible colour.
    if ((a == 0) != (b == 0))
        return false;

    const int redDelta = (int)((a >> 10) & 0x1F) - (int)((b >> 10) & 0x1F);
    const int greenDelta = (int)((a >> 5) & 0x1F) - (int)((b >> 5) & 0x1F);
    const int blueDelta = (int)(a & 0x1F) - (int)(b & 0x1F);

    // Three steps out of the 32 each channel has. Dither partners sit within
    // one or two; anything an artist drew as an edge is far wider apart.
    return (abs(redDelta) <= 3) && (abs(greenDelta) <= 3) && (abs(blueDelta) <= 3);
}
//----------------------------------------------------------------------------------
static bool SimilarColors(uint a, uint b)
{
    if (a == b)
        return true;

    if (((a >> 24) == 0) != ((b >> 24) == 0))
        return false;

    const int redDelta = (int)((a >> 16) & 0xFF) - (int)((b >> 16) & 0xFF);
    const int greenDelta = (int)((a >> 8) & 0xFF) - (int)((b >> 8) & 0xFF);
    const int blueDelta = (int)(a & 0xFF) - (int)(b & 0xFF);

    // The same three-in-thirty-two tolerance, scaled to eight bit channels.
    return (abs(redDelta) <= 24) && (abs(greenDelta) <= 24) && (abs(blueDelta) <= 24);
}
//----------------------------------------------------------------------------------
template <typename T>
static void Double(const T *pixels, int width, int height, std::vector<T> &out)
{
    out.clear();
    if (!Wanted(width, height) || pixels == nullptr)
        return;

    const int outWidth = width * 2;
    out.resize((size_t)outWidth * height * 2);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            const T center = pixels[y * width + x];

            // Wrap at the edges rather than clamping. Several gumps - the login
            // screen's background among them - are drawn tiled, with texture
            // coordinates past 1.0 and GL_REPEAT doing the repeating, so the
            // pixel neighbouring the last column really is the first column.
            // Clamping instead invents an edge that is not there and breaks the
            // seam between tiles. For art that is not tiled this only affects the
            // outermost pixel, which is transparent on virtually every sprite.
            const T up = pixels[((y + height - 1) % height) * width + x];
            const T down = pixels[((y + 1) % height) * width + x];
            const T left = pixels[y * width + ((x + width - 1) % width)];
            const T right = pixels[y * width + ((x + 1) % width)];

            T topLeft = center;
            T topRight = center;
            T bottomLeft = center;
            T bottomRight = center;

            if (SimilarColors(left, up) && !SimilarColors(left, down) &&
                !SimilarColors(up, right))
                topLeft = up;

            if (SimilarColors(up, right) && !SimilarColors(up, left) &&
                !SimilarColors(right, down))
                topRight = right;

            if (SimilarColors(down, left) && !SimilarColors(down, right) &&
                !SimilarColors(left, up))
                bottomLeft = left;

            if (SimilarColors(right, down) && !SimilarColors(right, up) &&
                !SimilarColors(down, left))
                bottomRight = down;

            const size_t base = (size_t)(y * 2) * outWidth + (size_t)(x * 2);
            out[base] = topLeft;
            out[base + 1] = topRight;
            out[base + outWidth] = bottomLeft;
            out[base + outWidth + 1] = bottomRight;
        }
    }
}
//----------------------------------------------------------------------------------
void Double16(const ushort *pixels, int width, int height, std::vector<ushort> &out)
{
    Double<ushort>(pixels, width, height, out);
}
//----------------------------------------------------------------------------------
void Double32(const uint *pixels, int width, int height, std::vector<uint> &out)
{
    Double<uint>(pixels, width, height, out);
}
}; // namespace GLArtUpscale
//----------------------------------------------------------------------------------
