// Verifies CGLVertexBatch draws exactly what immediate mode drew.
//
// Immediate mode does not exist in OpenGL ES, so the Android port replaces every
// glBegin/glEnd block with the batch. This renders the same scenes both ways
// into an offscreen buffer and compares the pixels: if the batch is a faithful
// replacement, the two images are identical.
//
// Build and run via tests/vertexbatch/run.sh.

#define GL_SILENCE_DEPRECATION 1
#include <SDL.h>
#include <OpenGL/gl.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#include "GLVertexBatchShader.h"
#include "GLVertexBatch.h"

// Sharp filtering only differs from bilinear under magnification, and these
// scenes render at 1:1, so either setting compares equally. On matches the client.
bool g_SharpFilter = true;

static const int WIDTH = 256;
static const int HEIGHT = 256;

typedef void (*SceneFn)(bool useBatch);

// Each scene mirrors one of the converted call sites in GLEngine.cpp.

static void SceneQuad(bool useBatch)
{
    const float w = 180.0f, h = 120.0f;
    if (useBatch)
    {
        g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
        g_GLBatch.TexCoord(0, 1); g_GLBatch.Vertex(0, h);
        g_GLBatch.TexCoord(1, 1); g_GLBatch.Vertex(w, h);
        g_GLBatch.TexCoord(0, 0); g_GLBatch.Vertex(0, 0);
        g_GLBatch.TexCoord(1, 0); g_GLBatch.Vertex(w, 0);
        g_GLBatch.End();
    }
    else
    {
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2i(0, 1); glVertex2f(0, h);
        glTexCoord2i(1, 1); glVertex2f(w, h);
        glTexCoord2i(0, 0); glVertex2f(0, 0);
        glTexCoord2i(1, 0); glVertex2f(w, 0);
        glEnd();
    }
}

static void SceneLines(bool useBatch)
{
    glDisable(GL_TEXTURE_2D);
    if (useBatch)
    {
        g_GLBatch.Begin(GL_LINES, false);
        g_GLBatch.Vertex(10, 10); g_GLBatch.Vertex(240, 200);
        g_GLBatch.Vertex(10, 200); g_GLBatch.Vertex(240, 10);
        g_GLBatch.End();
    }
    else
    {
        glBegin(GL_LINES);
        glVertex2i(10, 10); glVertex2i(240, 200);
        glVertex2i(10, 200); glVertex2i(240, 10);
        glEnd();
    }
    glEnable(GL_TEXTURE_2D);
}

// The circle is the one site that changes colour part way through a batch, and
// the only one whose first vertex must keep the colour that was current before.
static void SceneCircleGradient(bool useBatch)
{
    glDisable(GL_TEXTURE_2D);
    glColor4f(1.0f, 0.25f, 0.5f, 1.0f);
    glTranslatef(128.0f, 128.0f, 0.0f);

    const float radius = 100.0f;
    const float pi = (float)M_PI * 2.0f;

    if (useBatch)
    {
        g_GLBatch.Begin(GL_TRIANGLE_FAN, false);
        g_GLBatch.Vertex(0, 0);
        g_GLBatch.Color(0.0f, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i <= 360; i++)
        {
            float a = (i / 180.0f) * pi;
            g_GLBatch.Vertex(cosf(a) * radius, sinf(a) * radius);
        }
        g_GLBatch.End();
    }
    else
    {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2i(0, 0);
        glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
        for (int i = 0; i <= 360; i++)
        {
            float a = (i / 180.0f) * pi;
            glVertex2f(cosf(a) * radius, sinf(a) * radius);
        }
        glEnd();
    }

    glTranslatef(-128.0f, -128.0f, 0.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
}

// Land tiles carry a per-vertex normal with lighting on.
static void SceneNormals(bool useBatch)
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    if (useBatch)
    {
        g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
        g_GLBatch.Normal(0.0f, 0.0f, 1.0f);  g_GLBatch.TexCoord(0, 0); g_GLBatch.Vertex(22, 0);
        g_GLBatch.Normal(0.5f, 0.0f, 0.86f); g_GLBatch.TexCoord(0, 1); g_GLBatch.Vertex(0, 22);
        g_GLBatch.Normal(0.0f, 0.5f, 0.86f); g_GLBatch.TexCoord(1, 0); g_GLBatch.Vertex(44, 22);
        g_GLBatch.Normal(0.3f, 0.3f, 0.9f);  g_GLBatch.TexCoord(1, 1); g_GLBatch.Vertex(22, 44);
        g_GLBatch.End();
    }
    else
    {
        glBegin(GL_TRIANGLE_STRIP);
        glNormal3f(0.0f, 0.0f, 1.0f);  glTexCoord2i(0, 0); glVertex2i(22, 0);
        glNormal3f(0.5f, 0.0f, 0.86f); glTexCoord2i(0, 1); glVertex2i(0, 22);
        glNormal3f(0.0f, 0.5f, 0.86f); glTexCoord2i(1, 0); glVertex2i(44, 22);
        glNormal3f(0.3f, 0.3f, 0.9f);  glTexCoord2i(1, 1); glVertex2i(22, 44);
        glEnd();
    }
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHTING);
}

// A strip whose vertex count varies at runtime, like the sitting-character path.
static void SceneStrip(bool useBatch)
{
    if (useBatch)
    {
        g_GLBatch.Begin(GL_TRIANGLE_STRIP, true);
        for (int i = 0; i <= 8; i++)
        {
            float t = i / 8.0f;
            g_GLBatch.TexCoord(t, 0.0f); g_GLBatch.Vertex(t * 240.0f, 40);
            g_GLBatch.TexCoord(t, 1.0f); g_GLBatch.Vertex(t * 240.0f, 200);
        }
        g_GLBatch.End();
    }
    else
    {
        glBegin(GL_TRIANGLE_STRIP);
        for (int i = 0; i <= 8; i++)
        {
            float t = i / 8.0f;
            glTexCoord2f(t, 0.0f); glVertex2f(t * 240.0f, 40);
            glTexCoord2f(t, 1.0f); glVertex2f(t * 240.0f, 200);
        }
        glEnd();
    }
}

struct Scene { const char *name; SceneFn fn; };

static Scene SCENES[] = {
    { "textured quad (GL1_Draw)",        SceneQuad },
    { "lines (DrawLine)",                SceneLines },
    { "circle gradient (DrawCircle)",    SceneCircleGradient },
    { "normals + lighting (LandTexture)",SceneNormals },
    { "variable strip (DrawSitting)",    SceneStrip },
};

static GLuint MakeCheckerTexture()
{
    unsigned char pixels[16 * 16 * 4];
    for (int y = 0; y < 16; y++)
        for (int x = 0; x < 16; x++)
        {
            unsigned char v = ((x / 4 + y / 4) % 2) ? 230 : 40;
            unsigned char *p = pixels + (y * 16 + x) * 4;
            p[0] = v; p[1] = (unsigned char)(255 - v); p[2] = 128; p[3] = 255;
        }

    GLuint tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 16, 16, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    return tex;
}

static void Render(SceneFn fn, bool useBatch, std::vector<unsigned char> &out)
{
    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WIDTH, HEIGHT, 0, -150, 150);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    fn(useBatch);

    glFinish();
    out.resize(WIDTH * HEIGHT * 4);
    glReadPixels(0, 0, WIDTH, HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, &out[0]);
}

int main(int, char **)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 2;
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window *window = SDL_CreateWindow(
        "vertexbatch test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        WIDTH, HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    if (window == nullptr)
    {
        printf("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 2;
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == nullptr)
    {
        printf("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return 2;
    }

    printf("GL %s on %s\n\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

    GLuint tex = MakeCheckerTexture();
    glBindTexture(GL_TEXTURE_2D, tex);

    int failures = 0;
    const int sceneCount = (int)(sizeof(SCENES) / sizeof(SCENES[0]));

    const bool shaderReady = g_GLBatchShader.Init();
    printf("shader pipeline: %s\n\n", shaderReady ? "available" : "NOT available");

    for (int i = 0; i < sceneCount; i++)
    {
        std::vector<unsigned char> immediate, batched;
        Render(SCENES[i].fn, false, immediate);

        g_GLBatch.UseShaders = false;
        Render(SCENES[i].fn, true, batched);

        long differing = 0;
        int worst = 0;
        for (size_t p = 0; p < immediate.size(); p++)
        {
            int d = abs((int)immediate[p] - (int)batched[p]);
            if (d != 0)
            {
                differing++;
                if (d > worst)
                    worst = d;
            }
        }

        bool ok = (differing == 0);

        // And again through the shader and vertex buffer, which is what a Core
        // profile or GLES 2.0 will have to use.
        long shaderDiffering = 0;
        int shaderWorst = 0;
        if (shaderReady)
        {
            std::vector<unsigned char> shaded;
            g_GLBatch.UseShaders = true;
            Render(SCENES[i].fn, true, shaded);
            g_GLBatch.UseShaders = false;

            for (size_t p = 0; p < immediate.size(); p++)
            {
                int d = abs((int)immediate[p] - (int)shaded[p]);
                if (d != 0)
                {
                    shaderDiffering++;
                    if (d > shaderWorst)
                        shaderWorst = d;
                }
            }
        }

        // Reimplementing fixed function lighting in a shader will not round
        // identically - the interpolation happens at different precision - so a
        // whole-channel difference is a failure but one step of 255 is not. The
        // margin is reported either way rather than hidden.
        const bool shaderOk = !shaderReady || (shaderWorst <= 1);
        if (!ok || !shaderOk)
            failures++;

        const char *shaderVerdict = "skipped";
        if (shaderReady)
        {
            if (shaderDiffering == 0)
                shaderVerdict = "identical";
            else if (shaderOk)
                shaderVerdict = "within 1/255";
            else
                shaderVerdict = "DIFFERS";
        }

        printf("  %-38s arrays:%-10s shader:%s\n",
               SCENES[i].name,
               ok ? "identical" : "DIFFERS",
               shaderVerdict);
        if (!ok)
            printf("      arrays differ: %ld bytes, worst %d\n", differing, worst);
        if (shaderReady && shaderDiffering != 0)
            printf("      %ld byte(s) differ, worst %d\n", shaderDiffering, shaderWorst);
    }

    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
        printf("\n  warning: glGetError() = 0x%04X\n", err);

    printf("\n%d/%d scenes identical\n", sceneCount - failures, sceneCount);

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return failures == 0 ? 0 : 1;
}
