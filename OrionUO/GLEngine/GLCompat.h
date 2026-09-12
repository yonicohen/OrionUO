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

// Framebuffers, blend equations and mipmap generation are all core in GLES 2.0,
// so nothing here has to be aliased onto an extension the way the 1.x port did.

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

// The ARB spellings the desktop build uses for shader objects. GLES 2.0 has the
// same calls under their core names, so these are aliases rather than stubs.
#define glUniform1iARB glUniform1i
#define glUniform1fvARB glUniform1fv
#define glUseProgramObjectARB glUseProgram

// Fixed function state that GLES 2.0 removed outright. The renderer draws
// through a shader and a software matrix stack, so nothing is lost by making
// these inert - but the call sites are spread through drawing code shared with
// the desktop build, and an #ifdef at each one would be worse than this.
#define GL_ALPHA_TEST 0x0BC0
#define GL_TEXTURE_2D_ENABLE_STANDIN 0x0DE1

inline void glAlphaFunc(GLenum /*func*/, GLclampf /*ref*/)
{
}
inline void glLightModeli(GLenum /*pname*/, GLint /*param*/)
{
}
inline void glClearDepth(double depth)
{
    glClearDepthf((GLclampf)depth);
}

// GL_TEXTURE_2D is a texture target in GLES 2.0, not a capability: enabling and
// disabling it is meaningless, and the driver rejects it. Whether a draw is
// textured is a uniform on the shader instead - see CGLVertexBatch - and these
// keep that decision in one place.
void OrionGLSetTextured(bool textured);
bool OrionGLTextured();

// GL_LIGHTING is fixed function state the land tile path switches on around its
// draw, and which the batch reads back to decide whether to shade. GLES 2.0 has
// neither, so the flag is kept here and read by CGLVertexBatch instead.
#define GL_LIGHTING 0x0B50

void OrionGLSetLighting(bool lighting);
bool OrionGLLightingEnabled();

inline void OrionGLEnable(GLenum cap)
{
    if (cap == GL_TEXTURE_2D)
    {
        OrionGLSetTextured(true);
        return;
    }

    if (cap == GL_LIGHTING)
    {
        OrionGLSetLighting(true);
        return;
    }

    if (cap == GL_ALPHA_TEST)
        return;

    glEnable(cap);
}

inline void OrionGLDisable(GLenum cap)
{
    if (cap == GL_TEXTURE_2D)
    {
        OrionGLSetTextured(false);
        return;
    }

    if (cap == GL_LIGHTING)
    {
        OrionGLSetLighting(false);
        return;
    }

    if (cap == GL_ALPHA_TEST)
        return;

    glDisable(cap);
}

#define glEnable OrionGLEnable
#define glDisable OrionGLDisable

#endif // ORION_GLES
//----------------------------------------------------------------------------------
#endif
//----------------------------------------------------------------------------------
