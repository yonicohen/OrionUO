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

// GLES defines no GLdouble at all, but the renderer stores one scale value as
// one. Nothing is submitted to GL at double precision, so this is only a type.
typedef double GLdouble;

// Framebuffers exist in GLES 1.x only under GL_OES_framebuffer_object, with
// every name suffixed. Availability is checked at runtime in CGLEngine::Install
// via the extension string, exactly as the desktop build checks GLEW.
#define glGenFramebuffers glGenFramebuffersOES
#define glBindFramebuffer glBindFramebufferOES
#define glDeleteFramebuffers glDeleteFramebuffersOES
#define glFramebufferTexture2D glFramebufferTexture2DOES
#define glCheckFramebufferStatus glCheckFramebufferStatusOES
#define GL_FRAMEBUFFER GL_FRAMEBUFFER_OES
#define GL_FRAMEBUFFER_BINDING GL_FRAMEBUFFER_BINDING_OES
#define GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE_OES
#define GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0_OES
#define glGenerateMipmap glGenerateMipmapOES

// Blend equations are GL_OES_blend_subtract in GLES 1.x, again all suffixed.
#define glBlendEquation glBlendEquationOES
#define GL_FUNC_ADD GL_FUNC_ADD_OES
#define GL_FUNC_REVERSE_SUBTRACT GL_FUNC_REVERSE_SUBTRACT_OES

// GLES has no sized internal formats and no BGRA. The framebuffer's colour
// texture is allocated with a null pixel pointer, so only the enums matter.
#define GL_RGBA8 GL_RGBA
#define GL_BGRA GL_RGBA
#define GL_UNSIGNED_INT_8_8_8_8 GL_UNSIGNED_BYTE

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

// The shader classes are stubbed out under GLES (see GLShader.cpp), but the
// uniform calls are scattered through the GUI drawing code. These make them
// inert rather than #ifdef'ing every call site; with no program bound there is
// no uniform to set.
inline void glUniform1iARB(GLint /*location*/, GLint /*v0*/)
{
}
inline void glUniform1fv(GLint /*location*/, GLsizei /*count*/, const GLfloat * /*value*/)
{
}

inline void glLightModeli(GLenum pname, GLint param)
{
    glLightModelf(pname, (GLfloat)param);
}

// GLES has only the float spellings of the matrix and depth entry points, and
// no GLdouble type at all - hence plain double in these signatures.
inline void glClearDepth(double depth)
{
    glClearDepthf((GLclampf)depth);
}
// GLES has only the float spellings of the matrix and clipping entry points.
inline void glOrtho(
    double left, double right, double bottom, double top, double zNear, double zFar)
{
    glOrthof(
        (GLfloat)left, (GLfloat)right, (GLfloat)bottom, (GLfloat)top, (GLfloat)zNear,
        (GLfloat)zFar);
}

#endif // ORION_GLES
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
