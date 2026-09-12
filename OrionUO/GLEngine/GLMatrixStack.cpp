/***********************************************************************************
**
** GLMatrixStack.cpp
**
** See GLMatrixStack.h.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#include "stdafx.h"
//----------------------------------------------------------------------------------
CGLMatrixStack g_GLMatrix;
//----------------------------------------------------------------------------------
CGLMatrixStack::CGLMatrixStack()
{
    SetIdentity(m_ModelView);
    SetIdentity(m_Projection);
    m_Stack.reserve(8);
}
//----------------------------------------------------------------------------------
void CGLMatrixStack::SetIdentity(float *matrix)
{
    for (int i = 0; i < 16; i++)
        matrix[i] = 0.0f;

    matrix[0] = 1.0f;
    matrix[5] = 1.0f;
    matrix[10] = 1.0f;
    matrix[15] = 1.0f;
}
//----------------------------------------------------------------------------------
void CGLMatrixStack::Multiply(const float *left, const float *right, float *out)
{
    // Column-major: element (row, column) lives at [column * 4 + row].
    float result[16] = {};

    for (int column = 0; column < 4; column++)
    {
        for (int row = 0; row < 4; row++)
        {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++)
                sum += left[k * 4 + row] * right[column * 4 + k];

            result[column * 4 + row] = sum;
        }
    }

    for (int i = 0; i < 16; i++)
        out[i] = result[i];
}
//----------------------------------------------------------------------------------
void CGLMatrixStack::LoadIdentity()
{
    SetIdentity(m_ModelView);

    if (m_ForwardToGL)
    {
        #if !defined(ORION_GLES)
        glMatrixMode(GL_MODELVIEW);
        #endif
        #if !defined(ORION_GLES)
        glLoadIdentity();
        #endif
    }
}
//----------------------------------------------------------------------------------
void CGLMatrixStack::Scale(float x, float y, float z)
{
    // Scaling multiplies the three basis columns, leaving the translation alone -
    // the same shortcut Translate takes, for the same reason.
    for (int row = 0; row < 4; row++)
    {
        m_ModelView[row] *= x;
        m_ModelView[4 + row] *= y;
        m_ModelView[8 + row] *= z;
    }

    if (m_ForwardToGL)
    {
        #if !defined(ORION_GLES)
        glMatrixMode(GL_MODELVIEW);
        #endif
        #if !defined(ORION_GLES)
        glScalef(x, y, z);
        #endif
    }
}
//----------------------------------------------------------------------------------
void CGLMatrixStack::Push()
{
    m_Stack.push_back(std::vector<float>(m_ModelView, m_ModelView + 16));

    if (m_ForwardToGL)
    {
        #if !defined(ORION_GLES)
        glMatrixMode(GL_MODELVIEW);
        #endif
        #if !defined(ORION_GLES)
        glPushMatrix();
        #endif
    }
}
//----------------------------------------------------------------------------------
void CGLMatrixStack::Pop()
{
    if (!m_Stack.empty())
    {
        const std::vector<float> &top = m_Stack.back();
        for (int i = 0; i < 16; i++)
            m_ModelView[i] = top[i];

        m_Stack.pop_back();
    }

    if (m_ForwardToGL)
    {
        #if !defined(ORION_GLES)
        glMatrixMode(GL_MODELVIEW);
        #endif
        #if !defined(ORION_GLES)
        glPopMatrix();
        #endif
    }
}
//----------------------------------------------------------------------------------
void CGLMatrixStack::Translate(float x, float y, float z)
{
    // A translation only touches the last column, so apply it directly rather
    // than building a matrix and multiplying: this runs for every gump and every
    // tile on screen, every frame.
    for (int row = 0; row < 4; row++)
    {
        m_ModelView[12 + row] += m_ModelView[row] * x + m_ModelView[4 + row] * y +
                                 m_ModelView[8 + row] * z;
    }

    if (m_ForwardToGL)
    {
        #if !defined(ORION_GLES)
        glMatrixMode(GL_MODELVIEW);
        #endif
        #if !defined(ORION_GLES)
        glTranslatef(x, y, z);
        #endif
    }
}
//----------------------------------------------------------------------------------
void CGLMatrixStack::Rotate(float degrees, float x, float y, float z)
{
    const float radians = degrees * (float)M_PI / 180.0f;
    const float c = cosf(radians);
    const float s = sinf(radians);

    const float length = sqrtf(x * x + y * y + z * z);
    if (length > 0.0f)
    {
        x /= length;
        y /= length;
        z /= length;
    }

    const float oneMinusC = 1.0f - c;

    float rotation[16] = {};
    rotation[0] = x * x * oneMinusC + c;
    rotation[1] = y * x * oneMinusC + z * s;
    rotation[2] = x * z * oneMinusC - y * s;
    rotation[4] = x * y * oneMinusC - z * s;
    rotation[5] = y * y * oneMinusC + c;
    rotation[6] = y * z * oneMinusC + x * s;
    rotation[8] = x * z * oneMinusC + y * s;
    rotation[9] = y * z * oneMinusC - x * s;
    rotation[10] = z * z * oneMinusC + c;
    rotation[15] = 1.0f;

    Multiply(m_ModelView, rotation, m_ModelView);

    if (m_ForwardToGL)
    {
        #if !defined(ORION_GLES)
        glMatrixMode(GL_MODELVIEW);
        #endif
        #if !defined(ORION_GLES)
        glRotatef(degrees, x, y, z);
        #endif
    }
}
//----------------------------------------------------------------------------------
void CGLMatrixStack::Ortho(
    float left, float right, float bottom, float top, float nearZ, float farZ)
{
    SetIdentity(m_Projection);

    const float width = right - left;
    const float height = top - bottom;
    const float depth = farZ - nearZ;

    if (width == 0.0f || height == 0.0f || depth == 0.0f)
        return;

    m_Projection[0] = 2.0f / width;
    m_Projection[5] = 2.0f / height;
    m_Projection[10] = -2.0f / depth;
    m_Projection[12] = -(right + left) / width;
    m_Projection[13] = -(top + bottom) / height;
    m_Projection[14] = -(farZ + nearZ) / depth;
    m_Projection[15] = 1.0f;

    if (m_ForwardToGL)
    {
        #if !defined(ORION_GLES)
        glMatrixMode(GL_PROJECTION);
        #endif
        #if !defined(ORION_GLES)
        glLoadIdentity();
        #endif
        #if !defined(ORION_GLES)
        glOrtho(left, right, bottom, top, nearZ, farZ);
        #endif
        #if !defined(ORION_GLES)
        glMatrixMode(GL_MODELVIEW);
        #endif
    }
}
//----------------------------------------------------------------------------------
void CGLMatrixStack::Transform(float *out) const
{
    Multiply(m_Projection, m_ModelView, out);
}
//----------------------------------------------------------------------------------
