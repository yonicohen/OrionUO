# Android port

Status of making this client run on Android, and what is left.

The macOS port is the prerequisite and is done: see the top-level README. This
document covers only the Android-specific work.

## What is blocking, and why

Android has no desktop OpenGL. It has OpenGL ES, which comes in two flavours,
and this renderer is written against features that neither flavour has in full:

| | GLES 1.x | GLES 2.0+ |
| --- | --- | --- |
| `glBegin`/`glEnd` immediate mode | absent | absent |
| Display lists | absent | absent |
| Fixed function matrix stack (`glTranslatef`, `glOrtho`) | present | absent |
| Client-side vertex arrays (`glVertexPointer`) | present | absent |
| Shaders | absent | present |

The renderer uses the fixed function pipeline *and* shaders, so neither profile
covers it as written. The chosen route is **GLES 1.x first**: it keeps the matrix
stack and vertex arrays the whole renderer is built on, so only immediate mode
and display lists have to go. That gets pixels on screen. Hues then need GLES 2.0,
which is the larger second phase.

Build with `-DORION_GLES` to select the GLES paths.

## Done

**Immediate mode removed.** All 14 `glBegin`/`glEnd` blocks in `GLEngine.cpp` now
go through `CGLVertexBatch` (`GLEngine/GLVertexBatch.{h,cpp}`), which accumulates
vertices and submits them with `glDrawArrays`. The call shape deliberately
mirrors immediate mode so the drawing code still reads the same way.

Two sites carried state that needed care:

- `DrawCircle` changes colour part way through the fan. Immediate mode gives the
  first vertex whatever colour was already current, so `Color()` reads
  `GL_CURRENT_COLOR` and backfills the vertices already emitted, rather than
  letting the new colour apply retroactively to the whole batch.
- `GL1_DrawLandTexture` sets a normal per vertex, so normals are tracked the same
  sticky way, with the same backfill.

Verified by `tests/vertexbatch`, which renders five scenes - one per converted
call shape - through both immediate mode and the batch into an offscreen buffer
and compares the pixels. All five are identical on GL 2.1:

```bash
./tests/vertexbatch/run.sh
```

**Display lists shimmed.** Every call site was already behind
`CConfigManager::GetUseGLListsForInterface()`, which has a working non-list
fallback. That getter is compiled to return `false` under `ORION_GLES`, and
`GLEngine/GLCompat.h` provides inert `glGenLists`/`glNewList`/`glCallList`/
`glEndList`/`glDeleteLists` so the surrounding gump code still compiles without
`#ifdef`s scattered through it.

**Desktop-only entry points aliased.** `glOrtho` and `glClearDepth` have only
float spellings in GLES (`glOrthof`, `glClearDepthf`); `GLCompat.h` maps them.

**Shaders stubbed.** `GLShader.cpp` is written against the ARB extension entry
points, which GLES does not provide under any name. Under `ORION_GLES` the
shader classes become stubs whose `Init()` returns false. Every caller already
copes with that - `Use()` returns false and drawing falls back to fixed function
- so the client renders, but **without hue colorisation**. Hues are not cosmetic
in UO, so this is a stopgap, not a finished state.

The desktop build is unaffected by all of the above and still builds and renders.

## Left to do

Roughly in order:

1. **Cross-compile.** Build the tree with the NDK for `arm64-v8a` against GLES.
   Nothing here has been compiled by an Android toolchain yet - the GLES paths
   are written but unproven, which is the single biggest caveat in this document.
2. **SDL2 Android bootstrap.** SDL supports Android natively; the client needs
   the Java activity, the JNI entry point and a Gradle project around it.
3. **Asset delivery.** The UO data is ~2.6 GB and cannot be redistributed, so it
   cannot ship in the APK. It has to be side-loaded to external storage and
   located at runtime, replacing the `CustomPath` lookup in `uo_debug.cfg`.
4. **Touch input.** The client assumes a mouse with two buttons and a keyboard.
   Movement is right-button-hold, targeting is left-click, and there is no
   on-screen keyboard handling. This is a design problem, not a porting one.
5. **Hues, via a GLES 2.0 renderer.** Replaces the fixed function matrix stack
   and client-side vertex arrays as well, since GLES 2.0 has neither. This is
   the largest remaining piece.
6. **Sound.** `SoundBackend.cpp` uses SDL_mixer, which builds for Android, so
   this is expected to be mostly a build-system matter.

## Caveats

- No Android device or emulator has run this. Nothing below step 1 is verified.
- The `0xFACE` handshake, login crypto and networking are platform-independent
  and are already working on macOS, so they are not expected to need changes.
- UO data files are copyright and must never be bundled in an APK.
