// Compile-only check that the display list shims stand in for the desktop calls.
// GLES has no display lists in any version; the gump code still spells these
// out, guarded at runtime by GetUseGLListsForInterface(), which is compiled to
// return false under ORION_GLES.
#include "stdafx.h"

void ExerciseDisplayListShims()
{
    GLuint list = glGenLists(1);
    glNewList(list, GL_COMPILE);
    glEndList();
    glCallList(list);
    glDeleteLists(list, 1);
}

void ExerciseEntryPointAliases()
{
    // Desktop spellings; GLES only has the float ones, which GLCompat maps.
    glOrtho(0, 640, 480, 0, -150, 150);
    glClearDepth(1.0);
}
