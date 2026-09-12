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
** The transform comes from CGLMatrixStack rather than from GL, so nothing here
** needs the fixed function pipeline any more. What still does is the context
** itself: it is created as 2.1 compatibility, and CGLMatrixStack mirrors every
** operation into GL so the fixed function fallback keeps working and the two
** paths stay comparable. Asking for a Core profile is now a change of two lines
** and a lot of testing, rather than a rewrite.
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
    GLint m_AttribNormal = -1;

    GLint m_UniformTransform = -1;
    GLint m_UniformTexture = -1;
    GLint m_UniformTextured = -1;
    GLint m_UniformSourceSize = -1;

    int m_SourceWidth = 0;
    int m_SourceHeight = 0;
    GLint m_UniformLighting = -1;
    GLint m_UniformLightDirection = -1;
    GLint m_UniformLightConstant = -1;
    GLint m_UniformLightDiffuse = -1;

    // Light and material never change after start-up, so they are read back once.
    bool m_LightingCached = false;
    float m_LightDirection[3] = {};
    float m_LightConstant[3] = {};
    float m_LightDiffuse[3] = {};

    void CacheLightingState();

    bool m_Available = false;

    // What is already set on the GL side. The client draws one sprite per batch,
    // so anything re-sent per draw is re-sent thousands of times a frame: on a
    // tile based mobile GPU that alone was the difference between smooth and
    // unusable. Everything here is skipped when it has not changed.
    bool m_StateBound = false;
    float m_LastTransform[16] = {};
    bool m_HaveLastTransform = false;
    int m_LastTextured = -1;
    int m_LastLighting = -1;
    float m_LastSourceSize[2] = { -1.0f, -1.0f };
    GLsizei m_LastStride = 0;
    const float *m_LastVertices = nullptr;

    void BindState(const float *vertices, GLsizei stride, int vertexCount);

    static GLuint CompileStage(GLenum type, const char *source);

public:
    CGLVertexBatchShader() {}
    ~CGLVertexBatchShader() {}

    // Builds the program and buffer. Returns false if the driver will not have
    // it, in which case the caller keeps using the fixed function path.
    bool Init();
    void Free();

    bool Available() const { return m_Available; }

    // Forgets what it believes GL is set to. Called when anything outside this
    // class may have changed the program, the buffer or the attribute arrays.
    void InvalidateState();

    // Source texture dimensions, for filtering magnified art in texel space.
    void SetSourceSize(int width, int height)
    {
        m_SourceWidth = width;
        m_SourceHeight = height;
    }

    // Draws interleaved vertices: position.xy, texcoord.uv, color.rgba.
    void Draw(
        GLenum mode,
        const float *vertices,
        int vertexCount,
        int floatsPerVertex,
        bool textured,
        bool lit);
};
//----------------------------------------------------------------------------------
extern CGLVertexBatchShader g_GLBatchShader;
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
