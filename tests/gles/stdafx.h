// Stand-in for the client's stdafx.h when cross-compiling the GLES layer on its
// own. Real Android GLES 1.x headers first, then our compatibility layer, so the
// check exercises what the device actually provides.
#pragma once

#include <GLES/gl.h>
#include <GLES/glext.h>

#include "GLCompat.h"
#include "GLVertexBatch.h"
