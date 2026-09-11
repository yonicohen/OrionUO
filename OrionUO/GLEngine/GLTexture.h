/***********************************************************************************
**
** GLTexture.h
**
** Copyright (C) August 2016 Hotride
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef GLTEXTURE_H
#define GLTEXTURE_H
//----------------------------------------------------------------------------------
typedef vector<uchar> HIT_MAP_TYPE;
//----------------------------------------------------------------------------------
class CGLTexture
{
public:
    //!Габариты текстуры
    short Width = 0;
    short Height = 0;

    short ImageOffsetX = 0;
    short ImageOffsetY = 0;

    short ImageWidth = 0;
    short ImageHeight = 0;

    //!Буфер вершин
    GLuint VertexBuffer = 0;

    //!Буфер вершин для зеркального отображения анимации
    GLuint MirroredVertexBuffer = 0;

    CGLTexture();
    virtual ~CGLTexture();

    // Text is excluded from upscaling. EPX assumes flat-coloured pixel art and
    // rounds a corner wherever neighbours agree diagonally; glyphs are
    // anti-aliased and carry a drop shadow, so that rule fires along every soft
    // edge and smears them into blobs.
    bool AllowUpscale{ true };

    // Size actually uploaded, which is larger than Width/Height when the art was
    // upscaled. Width/Height stay logical so layout and hit testing are unchanged;
    // only the shader, which filters in texel space, needs the real size.
    int TexelWidth{ 0 };
    int TexelHeight{ 0 };

    GLuint Texture{ 0 };

    HIT_MAP_TYPE m_HitMap;

    virtual void Draw(int x, int y, bool checktrans = false);
    virtual void Draw(int x, int y, int width, int height, bool checktrans = false);

    virtual void DrawRotated(int x, int y, float angle);

    virtual void DrawTransparent(int x, int y, bool stencil = true);

    virtual bool Select(int x, int y, bool pixelCheck = true);

    virtual void Clear();
};
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
