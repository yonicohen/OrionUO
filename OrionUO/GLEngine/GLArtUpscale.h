/***********************************************************************************
**
** GLArtUpscale.h
**
** Doubles the resolution of UO's artwork when it is uploaded.
**
** The art is a fixed low resolution set - the login screen is 640x480, world
** tiles are 44x44 - and there is no higher resolution version to switch to. On a
** Retina display the renderer magnifies it about four times, which no filter can
** fully rescue because the detail was never there.
**
** EPX (also known as Scale2x) is the classic answer for pixel art: it turns each
** pixel into a 2x2 block, and where a pixel's neighbours agree diagonally it
** rounds the corner instead of stepping it. Unlike a blur it invents no colours -
** every output pixel is one of the input pixels - so line art and outlines stay
** clean rather than going soft.
**
** Only the upload is affected. CGLTexture::Width and Height stay in logical
** units, so gump layout, hit testing and world geometry are untouched; the
** texture simply carries four times the detail underneath. TexelWidth and
** TexelHeight record what was actually uploaded, which the shader needs in order
** to filter in texel space.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef GLARTUPSCALE_H
#define GLARTUPSCALE_H
//----------------------------------------------------------------------------------
#include <vector>

namespace GLArtUpscale
{
// Above this many source pixels the memory cost stops being worth it; those
// textures are already large enough that magnification is not the problem.
const int MaxSourcePixels = 1024 * 1024;

bool Wanted(int width, int height);

// Each returns the doubled image, or leaves the output empty if it declined.
void Double16(const ushort *pixels, int width, int height, std::vector<ushort> &out);
void Double32(const uint *pixels, int width, int height, std::vector<uint> &out);
}; // namespace GLArtUpscale
//----------------------------------------------------------------------------------
// Off turns every texture back to its original resolution on the next load.
extern bool g_UpscaleArt;
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
