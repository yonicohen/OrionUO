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

**The whole client cross-compiles and links.** All 247 translation units build
for `aarch64-linux-android`, and they link into a 3.0 MB `libmain.so` against
SDL2, SDL2_mixer, GLES 1.x, EGL, zlib and the NDK runtime. Every one of the 360
symbols the library still needs is provided by something it links against, so a
missing entry point fails at build time rather than when the phone tries to load
it.

```bash
./tools/android-build-deps.sh     # once: cross-builds SDL2 and SDL2_mixer
./tools/android-build.sh          # compiles, links, checks symbols
./tools/android-build-apk.sh      # packages and signs an installable APK
```

**It packages as an APK.** `tools/android-build-apk.sh` produces a signed 5.8 MB
`orionuo.apk` containing `libmain.so`, SDL2, SDL2_mixer and `libc++_shared.so`
for `arm64-v8a`. It drives `aapt2`, `d8` and `apksigner` directly rather than
going through Gradle, so the only thing needed besides the SDK is a JDK. Note
that Android's `d8` crashes on class files from very new JDKs; the script prefers
a JDK 17 if one is installed.

`android/` holds the Java side: SDL's `org.libsdl.app` classes and an
`OrionActivity` that names the libraries to load. No JNI shim was needed -
`OrionMain.cpp` includes `SDL.h`, which includes `SDL_main.h`, which redefines
`main` to `SDL_main` on platforms where SDL owns the entry point, so
`libmain.so` already exports exactly the symbol `SDLActivity` looks up.

`tests/gles/check.sh` is the narrower, faster check: it builds just the
compatibility layer and the renderer and verifies their GL entry points against
the device's `libGLESv1_CM.so`. It runs without the SDL2 build too, covering the
compat layer alone.

A few things had to give way to get there. FreeImage is not cross-built - it is
used only by `ScreenshotBuilder`, so screenshots are stubbed out on Android.
SDL2_mixer is built without FluidSynth, which is not available cross-compiled,
so MIDI goes through the bundled timidity.

The desktop build is unaffected by all of the above and still builds and renders.

## Left to do

Roughly in order:

1. **Run it on real hardware.** It has been run on an emulator and gets a long
   way (see below), but stalls in the emulator's own GLES 1.x texture path. A
   device with a native `libGLESv1_CM` driver is the next test and the one that
   matters.
2. **Asset delivery.** The UO data is ~2.6 GB and cannot be redistributed, so it
   cannot ship in the APK. It has to be side-loaded to external storage and
   located at runtime, replacing the `CustomPath` lookup in `uo_debug.cfg`.
3. **Touch input.** The client assumes a mouse with two buttons and a keyboard.
   Movement is right-button-hold, targeting is left-click, and there is no
   on-screen keyboard handling. This is a design problem, not a porting one.
4. **Hues, via a GLES 2.0 renderer.** Replaces the fixed function matrix stack
   and client-side vertex arrays as well, since GLES 2.0 has neither. This is
   the largest remaining piece.
5. **Sound on device.** `SoundBackend.cpp` and SDL2_mixer both build for
   Android, but no audio has been played.

## What happens when you run it

On an `arm64-v8a` emulator (API 34, AOSP image) the client:

- loads `libmain.so`, finds `SDL_main` and runs it;
- resolves its data path to app storage and reads the UO files from there;
- creates a **GLES 1.1 context** and initialises the renderer:

  ```
  GLES v(OpenGL ES-CM 1.1 (OpenGL ES 3.1.0 (ANGLE 2.1.1)))
  Graphics Successfully Initialized
  g_UseFrameBuffer = 0; CanUseBuffer = 0
  ```

- loads anim1-5, speech, tiledata, fonts, skills and the map block table;
- then **stalls in `glGenTextures`**, on the first texture it uploads:

  ```
  #00 read
  #01 qemu_pipe_read                           <- emulator's GL transport
  #02 QemuPipeStream::commitBufferAndReadFully
  #03 libGLESv1_enc.so  glGenTextures_enc
  #04 libmain.so  CGLEngine::GL1_BindTexture16
  #05 UOFileReader::ReadGump
  #06 COrion::GetGumpDimension  <- COrion::Install
  ```

The call that blocks is a plain `glGenTextures(1, &tex)`, and it blocks waiting
on the emulator's host GL translator rather than in any of our code. The
emulator also reports the same ANGLE-backed renderer whatever `-gpu` mode it is
started with, and its GL stack dies outright after a run - the emulator's GLES 1.x
support is emulated through ANGLE and is the weak link.

That GLES 1.x is the right target for this codebase was checked rather than
assumed: asking for a GLES 2.0 context instead gets a real ES 3.1 context, and
the client then segfaults immediately, because the renderer calls fixed function
entry points that do not exist there. The ES-CM 1.1 path gets orders of
magnitude further.

So the open question is whether a real device, with a vendor `libGLESv1_CM`
driver rather than an emulated one, gets past that call. That has not been tried.

## Caveats

- No frame has been drawn. The client starts, initialises a GLES 1.1 context
  and loads its data on an emulator, but stops at the first texture upload, so
  nothing has been rendered and no screenshot exists.
- The texture format conversions have therefore never executed. They are
  reasoned from the format definitions and verified only by the desktop build
  still rendering correctly.
- Touch input, sound and hues are all untested for the same reason.
- The `0xFACE` handshake, login crypto and networking are platform-independent
  and are already working on macOS, so they are not expected to need changes.
- UO data files are copyright and must never be bundled in an APK.
