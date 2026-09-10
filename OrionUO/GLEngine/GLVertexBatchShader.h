/***********************************************************************************
**
** GLVertexBatchShader.h
**
** The shader-and-buffer half of CGLVertexBatch.
**
** Everything the renderer draws goes through CGLVertexBatch, which makes it the
** one place a modern pipeline has to be built. This is that pipeline: a single
** program, one streaming vertex buffer, and generic vertex attributes, replacing
** glEnableClientState and the client-side arrays.
**
** It is deliberately usable from the existing OpenGL 2.1 compatibility context.
** The transform still comes from the fixed function matrix stack, read back with
** glGetFloatv, so this can be proven pixel-for-pixel against immediate mode
** before the 61 glTranslatef call sites are touched. Replacing the matrix stack
** is the next step, and moving to a Core profile the one after; neither is
** possible until the drawing itself no longer needs the fixed function pipeline.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef GLVERTEXBATCHSHADER_H
#define GLVERTEXBATCHSHADER_H
//----------------------------------------------------------------------------------
class CGLVertexBatchShader
{
private:
    GLuint m_Program = 0;
    GLuint m_VertexBuffer = 0;

    GLint m_AttribPosition = -1;
    GLint m_AttribTexCoord = -1;
    GLint m_AttribColor = -1;

    GLint m_UniformTransform = -1;
    GLint m_UniformTexture = -1;
    GLint m_UniformTextured = -1;

    bool m_Available = false;

    static GLuint CompileStage(GLenum type, const char *source);

public:
    CGLVertexBatchShader() {}
    ~CGLVertexBatchShader() {}

    // Builds the program and buffer. Returns false if the driver will not have
    // it, in which case the caller keeps using the fixed function path.
    bool Init();
    void Free();

    bool Available() const { return m_Available; }

    // Draws interleaved vertices: position.xy, texcoord.uv, color.rgba.
    void Draw(
        GLenum mode,
        const float *vertices,
        int vertexCount,
        int floatsPerVertex,
        bool textured);
};
//----------------------------------------------------------------------------------
extern CGLVertexBatchShader g_GLBatchShader;
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
