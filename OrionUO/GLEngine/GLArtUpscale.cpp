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

            // Edges clamp to the centre, so borders are simply doubled.
            const T up = (y > 0) ? pixels[(y - 1) * width + x] : center;
            const T down = (y < height - 1) ? pixels[(y + 1) * width + x] : center;
            const T left = (x > 0) ? pixels[y * width + (x - 1)] : center;
            const T right = (x < width - 1) ? pixels[y * width + (x + 1)] : center;

            T topLeft = center;
            T topRight = center;
            T bottomLeft = center;
            T bottomRight = center;

            if (left == up && left != down && up != right)
                topLeft = up;

            if (up == right && up != left && right != down)
                topRight = right;

            if (down == left && down != right && left != up)
                bottomLeft = left;

            if (right == down && right != up && down != left)
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
