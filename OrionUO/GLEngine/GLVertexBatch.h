/***********************************************************************************
**
** GLVertexBatch.h
**
** Immediate mode (glBegin/glVertex/glEnd) does not exist in any version of
** OpenGL ES, and is deprecated on desktop. This accumulates the same calls into
** client-side arrays and submits them with glDrawArrays, which both desktop GL
** and GLES understand.
**
** The call shape deliberately mirrors immediate mode so the drawing code reads
** the same way:
**
**     g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
**     g_GLBatch.TexCoord(0, 1); g_GLBatch.Vertex(0, height);
**     ...
**     g_GLBatch.End();
**
** Texture coordinate, colour and normal are sticky between vertices, exactly as
** the glTexCoord/glColor/glNormal state is.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef GLVERTEXBATCH_H
#define GLVERTEXBATCH_H
//----------------------------------------------------------------------------------
#include <vector>

class CGLVertexBatch
{
private:
    std::vector<float> m_Positions;
    std::vector<float> m_TexCoords;
    std::vector<float> m_Colors;
    std::vector<float> m_Normals;

    // Interleaved position/texcoord/colour, built only for the shader path.
    std::vector<float> m_Interleaved;

    GLenum m_Mode = GL_TRIANGLE_STRIP;
    bool m_Textured = false;
    bool m_Colored = false;
    bool m_Normaled = false;

    float m_CurrentColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    // The colour that was current when this batch first called Color(), so End()
    // can hand it back rather than leaving the batch's own colour behind.
    float m_RestoreColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float m_CurrentTexCoord[2] = { 0.0f, 0.0f };
    float m_CurrentNormal[3] = { 0.0f, 0.0f, 1.0f };

    // Backfills the vertices emitted before the first Color()/Normal() call with
    // the value the fixed-function pipeline would have applied to them.
    void DrawWithShader(int count);
    void SeedColorsFromGL();
    void SeedNormalsFromGL();

public:
    CGLVertexBatch() { Reserve(); }
    ~CGLVertexBatch() {}

    // Draw through the shader and vertex buffer rather than the fixed
    // function client arrays. Off until the program has been built.
    bool UseShaders = false;

    int m_SourceWidth = 0;
    int m_SourceHeight = 0;

    // Dimensions of the texture about to be drawn, so the shader can filter in
    // texel space. Zero means unknown, and the shader falls back to plain
    // bilinear.
    void SetSourceSize(int width, int height)
    {
        m_SourceWidth = width;
        m_SourceHeight = height;
    }

    void Reserve();

    void Begin(GLenum mode, bool textured);
    void End();

    void TexCoord(float u, float v);
    void Color(float r, float g, float b, float a);
    void Normal(float x, float y, float z);
    // Deliberately float-only: glVertex2i/glTexCoord2i sites pass ints, which
    // convert implicitly. Adding int overloads would make the many mixed calls
    // (glVertex2f(width, 0) with a float width) ambiguous.
    void Vertex(float x, float y);
};
//----------------------------------------------------------------------------------
extern CGLVertexBatch g_GLBatch;
//----------------------------------------------------------------------------------
// The current fixed function colour, tracked in software.
//
// A batch that submits a colour array leaves the current colour undefined, so
// End() has to put back the colour the caller had. Asking GL for it does not
// work everywhere: the Android emulator's GLES 1.1 encoder rejects
// glGetFloatv(GL_CURRENT_COLOR) with GL_INVALID_ENUM and leaves the buffer
// untouched, so the batch restored uninitialised stack garbage and every later
// texture was modulated to black.
//
// The client sets the colour through here instead, with glColor4f/glColor4ub
// redirected to it; GLVertexBatch.cpp undefines the macros to reach the real
// entry points.
extern float g_GLCurrentColor[4];

void GLSetColor4f(float r, float g, float b, float a);
void GLSetColor4ub(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

#define glColor4f GLSetColor4f
#define glColor4ub GLSetColor4ub
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
