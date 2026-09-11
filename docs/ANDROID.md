# Android port

The client builds, installs and plays on Android: it reaches the game world on
an emulator, connects to a shard and renders the map, mobiles and the whole
interface, and taps, drags and press-to-walk drive it. What it does not yet have
is hue colorisation and a single run on real hardware. Details below.

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

**The whole client cross-compiles and links.** All 250 translation units build
for `aarch64-linux-android`, and they link into `libmain.so` against
SDL2, SDL2_mixer, GLES 1.x, EGL, zlib and the NDK runtime. Every one of the 360
symbols the library still needs is provided by something it links against, so a
missing entry point fails at build time rather than when the phone tries to load
it.

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

1. **Run it on real hardware.** Everything below was done on an emulator. A
   device with a vendor `libGLESv1_CM` driver is the next test and the one that
   matters - several of the workarounds below exist only because the emulator's
   GLES 1.1 translator is incomplete, and they should be harmless there but have
   never been confirmed against a real driver.
2. **More of a touch control scheme.** The gestures below cover clicking,
   dragging and walking, which is enough to play, but there is no pinch-zoom, no
   two-finger gesture for a plain right click (context menus), and the on-screen
   keyboard is whatever SDL raises for a text field - it covers the lower half of
   the screen while it is up.
3. **Hues, via a GLES 2.0 renderer.** The shader classes are stubs under GLES,
   so nothing is hue-colorised: every mobile, item and piece of clothing draws in
   its base palette. Replacing them means replacing the fixed function matrix
   stack and client-side vertex arrays too, since GLES 2.0 has neither. This is
   the largest remaining piece.
4. **Sound on device.** `SoundBackend.cpp` and SDL2_mixer both build for Android,
   but no audio has been heard.
5. **Asset delivery.** The UO data is ~2.6 GB and cannot be redistributed, so it
   cannot ship in the APK; today it is pushed to external storage with `adb`.
   Something friendlier - an in-app copy from a user-chosen folder - is needed
   before anyone but a developer can install this.

## Running it

```bash
./tools/android-build-deps.sh     # once: cross-builds SDL2 and SDL2_mixer
./tools/android-build.sh          # compiles, links, checks symbols
./tools/android-build-apk.sh      # packages and signs an installable APK
adb install -r build-android/apk/orionuo.apk
```

Then push the UO data - which is **not** in the APK and must never be put there -
into the app's external files directory:

```bash
adb push "/path/to/Ultima Online/." /sdcard/Android/data/uk.co.jmaul.orionuo/files/
```

The client takes its shard address from the command line (`-login host,port`),
and Android has no command line. `OrionActivity.getArguments()` supplies one,
from either an intent extra or a file next to the data:

```bash
printf -- '-login uo.example.com,2593\n' > orion_args.txt
adb push orion_args.txt /sdcard/Android/data/uk.co.jmaul.orionuo/files/
# or, per launch:
adb shell am start -n uk.co.jmaul.orionuo/.OrionActivity --esa args "-login uo.example.com,2593"
```

One argument per line; `#` and `;` start a comment. Without either the client
falls back to `login.cfg`, which in a stock UO install points at `127.0.0.1`.

`LOG()` goes to logcat on Android, so `adb logcat -s OrionUO` is the client's log.

On an `arm64-v8a` emulator (API 34, AOSP image, started with `-gpu host`) the
client gets a GLES 1.1 context, draws the login screen, connects to a shard,
logs in, picks a character and **renders the game world** - map, statics,
mobiles, paperdoll, status bar, minimap, journal and chat all draw. The emulator
must be started with `-gpu host`; SwiftShader's GLES 1.x emulation was the
original blocker and never got a frame out.

## Touch input

The client is written for a two-button mouse: the left button selects, drags and
double-clicks, and walking is the right button held down in the direction to
move. A touch screen has neither button, so `CWindow` translates gestures into
the mouse events the rest of the client already understands - it synthesises
them and runs them through its own `OnWindowProc`, so nothing above the window
layer knows the difference:

| gesture | becomes |
| --- | --- |
| tap | left click; a second tap inside the double-click window is a double click, which is how items are used |
| drag | left button held, for moving gumps and items |
| press and hold | right button held: walk towards the finger, steering by moving it, until it lifts |

A press only becomes a hold once 350 ms have passed with the finger still within
16 px of where it landed, which is what keeps a tap and the start of a drag
distinguishable from the start of a walk. Because a finger resting on the screen
generates no events, the hold is noticed from the frame loop rather than from an
event.

SDL's own touch-to-mouse synthesis is turned off (`SDL_HINT_TOUCH_MOUSE_EVENTS`):
it only ever produces a left button, so it would deliver a left click at the
start of every attempt to walk.

Verified on the emulator by `adb shell input`: taps drive the login and shard
screens, and a 2.5 s press in the world produces `Client:: Walk Request` and
`Server:: Confirm Walk` pairs, with the view scrolling.

## Android-specific workarounds

Things that are correct desktop GL but do not survive the trip, each found by
bisecting a frame on the emulator rather than by reading a spec:

- **The alpha test is ignored.** Every piece of UO art is 1-bit alpha masked by
  `glAlphaFunc(GL_GREATER, 0)`, and the emulator's GLES 1.1 translator does not
  discard those fragments - it writes them. The full-screen frame gump therefore
  wiped every gump drawn before it back to transparent black, which is why the
  pre-game screens showed nothing but their topmost artwork on a black field.
  `CGLEngine::Install` now also enables blending under GLES, and `GLCompat.h`
  turns `glDisable(GL_BLEND)` into "put the standard blend function back" so the
  mask survives the client's own blend sections.
- **`glDeleteFramebuffers` crashes.** Inside the emulator's GLES encoder, in
  `GLClientState::removeFramebuffers`. `CGLFrameBuffer::Init` keeps the
  framebuffer object and re-attaches a new colour texture instead of deleting and
  regenerating it, which is what a gump resize used to do on the way into the
  world.
- **`glIsEnabled` crashes** in the same encoder, and `glGetFloatv` of
  `GL_CURRENT_COLOR`/`GL_CURRENT_NORMAL` returns `GL_INVALID_ENUM` with the
  buffer untouched. `CGLVertexBatch` used to read the current colour back from GL
  to restore it after submitting a colour array; it now tracks the colour in
  software (`g_GLCurrentColor`, with `glColor4f`/`glColor4ub` redirected to it),
  which is both portable and cheaper.
- **NPOT textures.** GLES 1.1 has no `GL_OES_texture_npot`, so a texture whose
  dimensions are not powers of two is incomplete under `GL_REPEAT` and samples
  black. Tiled gump backgrounds relied on texture coordinates past 1.0; under
  GLES they are emitted as one quad per repeat with coordinates inside `[0, 1]`.
  Framebuffer colour textures are clamped and non-mipmapped for the same reason.
- **No `INTERNET` permission** meant `connect()` failed with the login screen
  reporting "There is some problem communicating with Origin". It is in the
  manifest now.

Two of these - the software colour tracking and keeping the framebuffer object
across a resize - are strictly better on the desktop too, and are not behind
`#ifdef`s.

## Caveats

- Only ever run on an emulator; see the first item under "Left to do".
- **No hues.** The shader classes are stubs, so everything draws in its base
  palette. This is very visible in the world.
- The touch scheme covers clicking, dragging and walking; it has no pinch-zoom,
  no gesture for a plain right click, and the soft keyboard covers half the
  screen while it is up.
- Sound has not been heard, and screenshots are stubbed out (FreeImage is not
  cross-built).
- The `0xFACE` handshake, login crypto and networking are platform-independent
  and needed no changes.
- UO data files are copyright and must never be bundled in an APK.
