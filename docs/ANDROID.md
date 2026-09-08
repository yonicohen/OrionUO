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
GLES also has no `GLdouble` type at all, so those shims take plain `double` -
caught by the cross-compile check below, not by reading the spec.

**Shaders stubbed.** `GLShader.cpp` is written against the ARB extension entry
points, which GLES does not provide under any name. Under `ORION_GLES` the
shader classes become stubs whose `Init()` returns false. Every caller already
copes with that - `Use()` returns false and drawing falls back to fixed function
- so the client renders, but **without hue colorisation**. Hues are not cosmetic
in UO, so this is a stopgap, not a finished state.

**Textures converted.** UO stores 16-bit pixels as ARGB1555 and 32-bit as the
word `B<<24|G<<16|R<<8|A`, uploaded on desktop with `GL_BGRA` and the `_REV`
packed types. GLES 1.x has neither `GL_BGRA` nor any `_REV` type, and requires
the internal format to equal the format. `GL1_BindTexture16` now rotates the
five-bit fields into `GL_RGBA`/`GL_UNSIGNED_SHORT_5_5_5_1`, and
`GL1_BindTexture32` unpacks to four bytes explicitly rather than reordering the
word, which would depend on host endianness.

**The GL2 path is compiled out.** It binds vertex buffer objects with `GL_INT`
arrays, which GLES does not accept as a vertex array type. It is dead code on
every platform - `CanUseBuffer` is hardcoded `false` - so it is removed for
Android rather than ported.

**Cross-compiled and symbol-checked.** `tests/gles/check.sh` builds the GLES
layer *and the renderer* with the NDK for `aarch64-linux-android` against the
real Android GLES 1.x headers, then checks every GL entry point the objects
reference against the `libGLESv1_CM.so` the device ships - so a call that links
on the host but does not exist on the phone fails here instead of at runtime:

```bash
./tools/android-build-sdl2.sh     # once; the renderer's headers need SDL
./tests/gles/check.sh
```

`GLEngine.cpp` compiles clean for ARM64 and all 42 GL symbols it needs resolve.
Without the SDL2 build the check still runs, covering the compatibility layer
alone.

The desktop build is unaffected by all of the above and still builds and renders.

## Left to do

Roughly in order:

1. **Cross-compile the rest of the tree.** The renderer (`GLEngine.cpp`), the
   vertex batch and the compat layer all build for `aarch64-linux-android` and
   pass the symbol check. The other ~200 translation units have not been tried;
   they are mostly platform-independent game logic, but `ScreenshotBuilder`
   needs FreeImage, which is not cross-built (it is the only user, so stubbing
   it is the likely answer), and `SoundBackend` needs SDL_mixer for Android.
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

- No Android device or emulator has run this. The renderer is compiled and
  symbol-checked for ARM64, but compiling is not running: nothing has drawn a
  frame on a phone. The texture conversions in particular are reasoned from the
  format definitions and verified only by the desktop build still rendering -
  the GLES paths themselves have never executed.
- The `0xFACE` handshake, login crypto and networking are platform-independent
  and are already working on macOS, so they are not expected to need changes.
- UO data files are copyright and must never be bundled in an APK.
