// Minimal stand-in for the client's stdafx.h, so GLVertexBatch.cpp can be built
// on its own for this test without dragging in the whole client. Placed first on
// the include path, which is why GLVertexBatch.cpp's #include "stdafx.h" lands
// here instead of on OrionUO/stdafx.h.
#pragma once

#define GL_SILENCE_DEPRECATION 1
#include <cstdio>

// The client routes LOG to stdout; the shader code uses it for diagnostics.
#define LOG(...) fprintf(stdout, " LOG: " __VA_ARGS__)

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

// The shader consults this to choose texel-space filtering over plain bilinear.
// The client defines it in GLArtUpscale.cpp, which this test does not build.
extern bool g_SharpFilter;

#include "GLMatrixStack.h"
#include "GLVertexBatchShader.h"
#include "GLVertexBatch.h"
