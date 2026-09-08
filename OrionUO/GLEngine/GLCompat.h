/***********************************************************************************
**
** GLCompat.h
**
** Bridges the gap between desktop OpenGL and OpenGL ES for the Android port.
**
** Define ORION_GLES to build against GLES. The client targets GLES 1.x, which
** keeps the fixed function pipeline (matrix stack, client-side vertex arrays,
** texture environment) that the renderer is built on. What GLES drops entirely,
** at every version, is:
**
**   - immediate mode: handled by CGLVertexBatch, which every glBegin/glEnd
**     block in GLEngine.cpp now goes through.
**   - display lists: shimmed away below. Every call site is already behind
**     CConfigManager::GetUseGLListsForInterface(), which is forced off under
**     GLES, so the shims are never reached at runtime - they exist so the
**     surrounding code still compiles.
**   - the double-precision entry points (glOrtho, glTranslated): GLES only has
**     the float spellings, aliased below.
**
************************************************************************************
*/
//----------------------------------------------------------------------------------
#ifndef GLCOMPAT_H
#define GLCOMPAT_H
//----------------------------------------------------------------------------------
#ifdef ORION_GLES

#define GL_COMPILE 0x1300

// Display lists do not exist in GLES. Nothing reaches these, because
// GetUseGLListsForInterface() is compiled to return false; they keep the call
// sites valid without scattering #ifdefs through the gump code.
inline GLuint glGenLists(GLsizei /*range*/)
{
    return 0;
}
inline void glNewList(GLuint /*list*/, GLenum /*mode*/)
{
}
inline void glEndList()
{
}
inline void glCallList(GLuint /*list*/)
{
}
inline void glDeleteLists(GLuint /*list*/, GLsizei /*range*/)
{
}

// GLES has only the float spellings of the matrix and depth entry points.
inline void glClearDepth(GLdouble depth)
{
    glClearDepthf((GLclampf)depth);
}
// GLES has only the float spellings of the matrix and clipping entry points.
inline void glOrtho(
    GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    glOrthof(
        (GLfloat)left, (GLfloat)right, (GLfloat)bottom, (GLfloat)top, (GLfloat)zNear,
        (GLfloat)zFar);
}

#endif // ORION_GLES
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
