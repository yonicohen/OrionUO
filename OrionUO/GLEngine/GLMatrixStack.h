/***********************************************************************************
**
** GLMatrixStack.h
**
** The transform the renderer used to get from the fixed function matrix stack.
**
** glTranslatef, glPushMatrix, glMatrixMode and glOrtho do not exist in a Core
** profile or in GLES 2.0, and they are the last thing tying this renderer to the
** compatibility profile - the drawing itself already goes through a shader and a
** vertex buffer.
**
** While the context is still a compatibility one every operation is also applied
** to GL, so the fixed function fallback keeps working and the two can be compared
** pixel for pixel. Once the context is Core, the forwarding is what goes away;
** the call sites do not change again.
**
** Matrices are column-major, the layout OpenGL itself uses, so they can be handed
** to glUniformMatrix4fv and to glLoadMatrixf without transposing.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef GLMATRIXSTACK_H
#define GLMATRIXSTACK_H
//----------------------------------------------------------------------------------
#include <vector>

class CGLMatrixStack
{
private:
    float m_ModelView[16] = {};
    float m_Projection[16] = {};
    std::vector<std::vector<float>> m_Stack;

    // Off once the context no longer has a fixed function pipeline.
    bool m_ForwardToGL = true;

    static void SetIdentity(float *matrix);
    static void Multiply(const float *left, const float *right, float *out);

public:
    CGLMatrixStack();
    ~CGLMatrixStack() {}

    void SetForwardToGL(bool value) { m_ForwardToGL = value; }

    void LoadIdentity();
    void Push();
    void Pop();

    void Translate(float x, float y, float z);
    void Scale(float x, float y, float z);
    void Rotate(float degrees, float x, float y, float z);

    void Ortho(float left, float right, float bottom, float top, float nearZ, float farZ);

    const float *ModelView() const { return m_ModelView; }
    const float *Projection() const { return m_Projection; }

    // projection * modelview, which is what a vertex shader wants.
    void Transform(float *out) const;
};
//----------------------------------------------------------------------------------
extern CGLMatrixStack g_GLMatrix;
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
