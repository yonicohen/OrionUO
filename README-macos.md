# OrionUO on macOS

A native macOS (Apple Silicon and Intel) build of the OrionUO client.

Upstream shipped a Windows client with a partial, never-finished Linux port. This
branch completes that port: the client builds, runs, renders, plays sound, takes
input, and connects to a live Sphere shard on macOS.

The upstream source has not been updated since ~2018 and has diverged from the
current Orion binary, which matters for one protocol handshake (see
[Known gaps](#known-gaps)).

Verified working against a live Sphere 0.56b shard: login, character creation,
world rendering, movement, sound, text input, vendors and gump interaction.

## Getting the code

```bash
git clone -b macos-port https://github.com/yonicohen/OrionUO.git
cd OrionUO
```

This is a fork of [Hotride/OrionUO](https://github.com/Hotride/OrionUO); the port
lives on the `macos-port` branch, `master` is upstream untouched.

---

## Building

### Dependencies

Homebrew, plus:

```bash
brew install cmake ninja sdl2 sdl2_image sdl2_mixer freeimage glew
```

### Build

```bash
cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Release
ninja -C build OrionUO
```

The binary lands at `build/OrionUO/OrionUO`. A debug build is the same with
`-DCMAKE_BUILD_TYPE=Debug`; use it if you need a usable backtrace, because the
release build compiles with `-fomit-frame-pointer` and stacks do not unwind.

There is also a one-shot script that installs dependencies, builds, and wires up
the runtime files:

```bash
./tools/setup-macos.sh /path/to/your/UltimaOnline
```

### Notes on the build

- `cotire` (the old precompiled-header helper) is unmaintained and fails under
  CMake 4.x. It is replaced by CMake's native `target_precompile_headers`, and
  kept behind `-DORION_USE_COTIRE=ON` only for the legacy unity build.
- The client links `sdl2-compat` rather than SDL2 proper, because Homebrew's
  `sdl2_image`/`sdl2_mixer` depend on it. This works; if you hit odd input or
  windowing behaviour, it is the first thing to rule out.
- macOS OpenGL is deprecated but functional. The renderer is fixed-function
  GL 2.x, so it runs in the legacy profile — 2.1 via Apple's Metal-backed GL.

---

## Running

You need a real Ultima Online installation. **No UO data files ship with this
repository**, and they cannot be redistributed. The client reads the `.mul` /
`.uop` files from an existing install.

### 1. Point it at your UO data

Create `uo_debug.cfg` next to the binary:

```
CustomPath=/path/to/your/UltimaOnline
```

### 2. Provide a `Client.cuo`

`Client.cuo` is mandatory and is normally produced by the Windows-only Orion
Launcher. It is not encrypted — on a non-Windows build the in-tree crypto reads
it as a plain binary record — so it can be generated:

```bash
python3 tools/make_client_cuo.py -o build/OrionUO/Client.cuo \
    --client-version CV_70180 --encryption ET_TFISH --version-text 7.0.20.0
```

If your shard's package already contains a `Client.cuo`, prefer that one: it
carries the shard's real client version and encryption type. The client looks in
the UO data directory first, then next to the binary.

### 3. Launch

```bash
cd build/OrionUO
./OrionUO "-login your.shard.host,2593"
```

### Command line

| Option | Meaning |
| --- | --- |
| `-login host,port` | Login server. Required — `Login.cfg` usually contains a placeholder. |
| `-orionversion a.b.c.d` | Override the Orion version reported to the server. |
| `-account <hex>,<hex>` | Account and password, hex-encoded bytes. |
| `-autologin 1` | Auto-select server and character. |
| `-fastlogin` | Actually initiate the auto-login. `-autologin` alone only ticks the box. |

`tools/version-sweep.sh` uses these to test candidate Orion versions unattended.

---

## What was fixed

Most of these were latent bugs that had simply never run on a 64-bit
little-endian target, not macOS-specific issues. Several are independently
confirmed by CrossUO, which reached the same conclusions separately.

### Crypto (all four broke the game connection)

- **Twofish compiled big-endian on a little-endian CPU.** `Crypt/platform.h`
  selected endianness with `#ifdef _M_IX86`, an MSVC-on-x86 macro that clang
  never defines — so `Bswap()` swapped every word, `ADDR_XOR` reversed byte
  extraction, and `ALIGN32` changed struct layouts. Now uses `__BYTE_ORDER__`.
- **`typedef unsigned long u32`** in `Crypt/aes.h` — 8 bytes on LP64, doubling
  every buffer in the Twofish key schedule and smashing the stack in `reKey()`.
- **Blowfish box overflow.** `p_box`/`s_box` were `unsigned long` but copied into
  `unsigned int` tables: 144→72 and 8192→4096 bytes, across 25 key tables.
- **`CTwofishCrypt::m_IP`** was an 8-byte `unsigned long` filled by a 4-byte
  `memcpy`, feeding uninitialised memory into the key.

### Networking

- **`select(h, …)` instead of `select(h + 1, …)`.** The client could send but
  never receive anything. Winsock ignores that argument, which is why it only
  broke off Windows. Same bug in both ICMP paths.
- **`tcp_connect` blocked the main thread** with no timeout, freezing the client
  for the full TCP timeout on an unreachable address. Now non-blocking with a
  bounded wait.
- **`SOCKADDR_IN` aliased to `in_addr`**, making `sizeof()` 4 instead of 16 where
  it is passed as `sendto`'s address length.
- **Login seed** was computed by passing a binary address to `inet_aton`, which
  expects a dotted-decimal string, and writing its 0/1 return as the seed.
- **NAT relay.** Shards behind NAT relay to their own LAN address; the client now
  stays on the host it successfully logged in to.

### Protocol

- **`ReadString` returned NUL-padded fixed-width fields.** An empty 30-byte
  character name came back as a 30-character string of NULs — non-empty to every
  "is this slot used?" test, but printing as blank. The client believed empty
  character slots held characters, logged in to one, and the server dropped the
  connection.
- **`0xBF` / `0xFACE` handshake.** Current Orion answers the version query under
  this subcommand rather than packet `0xFC`. Shards running the stock Sphere
  "orion exclusive" script disconnect clients that stay silent. The reply is
  repeated across the server's 5-second window because Sphere only populates
  `LOCAL.CHAR` once the character exists, and a reply landing earlier clears
  nothing.
- **Packet size table** (upstream PR #97): `== CV_6060` never matched newer
  clients, and `0xEE`/`0xEF` used a `0x2000` placeholder instead of real lengths.
- **Packet log timestamps** were disabled off Windows; `localtime_s` now shims to
  `localtime_r`.

### Runtime and platform

- **`glPixelStorei` during static initialisation.** `CConfigManager` is a global
  whose constructor created a GL texture before `main()` and before any GL
  context. Windows' `opengl32` ignores GL calls with no current context; macOS'
  `libGL` dereferences null and crashes.
- **Main loop only ran `OnMainLoop()` when an SDL event arrived**, so networking
  and game logic advanced only while the mouse moved — and it span at 100% CPU
  when idle.
- **`SetTimer`/`KillTimer` were no-ops**, so the update and auto-login timers
  never fired. Now backed by `SDL_AddTimer`, dispatched on the main thread.
- **`MidiInfoStruct` was `#pragma pack(1)`**, putting an 8-byte pointer at
  unaligned offsets — the arm64 linker refuses to emit chained fixups for that.
- **`WaveHeader` used `unsigned long`** (4 bytes on Windows, 8 on LP64), shifting
  every field after `chunkSize`.
- **`GetFileVersion` never set the numeric version**, so the client reported
  version 0 to servers that check it.

### Audio

Every `BASS_*` call was a no-op macro off Windows — the build was completely
silent. `Managers/SoundBackend.cpp` implements the used surface over SDL_mixer,
leaving `CSoundManager` unchanged. Sound effects become `Mix_Chunk`s, music a
`Mix_Music`; the MIDI soundfont (`bin/uo_4mb_2.sf2`) is copied to the build
directory automatically.

### Input

- **All 27 `OnTextInput` handlers were `NOT_IMPLEMENTED`** — no text entry
  anywhere: login, character creation, chat, books, options, bulletin boards.
  The platform-neutral handlers now compile on all platforms with the SDL events
  adapted onto them.
- **`SDL_StartTextInput()` was never called**, so SDL emitted no text events.
- **`VK_*` constants matched neither Win32 nor SDL.** `VK_BACK` was 32 (space)
  and `VK_UP` was 8 (backspace), so space deleted characters and backspace moved
  the caret. Only Return/Tab/Escape worked, by coincidence.
- **Gump dragging.** The SDL handler cleared `LeftButtonPressed` *before*
  `OnLeftMouseButtonUp()`, and `LeftDroppedOffset()` returns `(0,0)` when the
  button reads as up — so every drag measured zero distance and no gump moved.
- **`UnicodeTalk`/`EncodeUTF8`/`DecodeUTF8`** called unimplemented Win32 stubs,
  so all cliloc text, tooltips, item names and outgoing unicode chat were empty.

### Window

- **`SDL_WINDOWEVENT_*` subtypes were compared against `ev.type`**, so the entire
  window-event block was dead code: show/hide never reached the plugin or the
  sound/FPS handling, and resize was never noticed.
- The window is now created `SDL_WINDOW_RESIZABLE` with a 640×480 minimum, and
  the game view tracks the window size.
- **`GetSystemMetrics` returned 0**, and `SM_CYFRAME`/`SM_CYCAPTION`/
  `SM_CXSIZEFRAME` all aliased to `SM_CXSCREEN`.

### Misc

- `GetSystemDefaultLangID` returned 0, which is `LANG_RUSSIAN` — the client
  defaulted to Russian on macOS. Locale now comes from `SDL_GetPreferredLocales`.
- Clipboard (`OpenClipboard`/`GetClipboardData`/`GlobalLock`) implemented over
  SDL, `ShellExecuteA` over `SDL_OpenURL`, `GetLocalTime` over `localtime_r`.
- `CommandLineToArgvW` returned nothing, so no command-line option was ever
  parsed — including the login server. Its result is freed by the caller with
  `LocalFree`, so it must be a `malloc`'d block.
- Music streams leaked on every track change.

---

## Known gaps

### Plugins and the Orion Assistant cannot work

`PluginManager` loads Windows DLLs via `LoadLibrary`. The Orion Assistant ships
as `OrionAssistant.dll`. There is no native path for this, and the client logs a
harmless `dlopen` failure at startup. Scripting is unavailable.

### `0xFACE` subcommand `0x8001`

The shard pushes an extended feature-flag set under this subcommand that this
source predates — light filter, death screen, autoloot and similar Orion
conveniences. It is configuration pushed at the client, not a query, so ignoring
it is safe; those specific features are simply unavailable. Decoding it would
require knowing the current Orion flag layout.

### ICMP ping needs privileges

Server-list latency uses raw ICMP sockets, which need root on macOS. The
`select()` and `sizeof(SOCKADDR_IN)` bugs in that path are fixed, but it will
still fail unprivileged. Cosmetic only.

### Remaining stubs

Twelve Win32 stubs remain `NOT_IMPLEMENTED`, all of them dead on this platform —
they sit inside `#if USE_WISP` blocks or have working SDL equivalents:
`_beginthreadex`/`_endthreadex` (SDL threads are used), `WideCharToMultiByte`/
`MultiByteToWideChar` (`std::wstring_convert` is used), `WSAStartup`/`WSACleanup`,
`timeBeginPeriod`/`timeEndPeriod`, `DefWindowProc`, `AdjustWindowRectEx`,
`CloseHandle`.

---

## Work remaining

### If you contribute

- **Line endings.** Most upstream files are CRLF. Scripted edits silently
  rewrite them to LF and produce enormous diffs — a one-line change can appear
  as thousands. Always compare `git diff --stat` against
  `git diff --stat --ignore-cr-at-eol` before committing.
- **Watch for Windows type-width assumptions.** Four separate bugs here came
  from them: `unsigned long` assumed to be 32-bit (three times) and `wchar_t`
  assumed to be 16-bit (once). If something crashes in crypto or text handling,
  look there first.

### Worth doing

- **`SetSize` on the window** is the last stub reachable at runtime.
- **`CPingThread` can be constructed with an empty host** when login fails
  before the server list arrives. Harmless, but it should be guarded.
- The server-list log line in `ServerList.cpp` is kept deliberately — it prints
  the names a shard advertises, which is how you notice you are connecting to
  the wrong entry.
- **Fold in the CrossUO fixes.** CrossUO is the maintained fork of this codebase
  and has years of additional portability work.
- **Stop depending on `sdl2-compat`** by building against SDL2 proper, or move to
  SDL3 outright.
- **Replace FreeImage** — it is unmaintained and the most awkward dependency to
  redistribute (FIPL/GPL dual licence).
- **Audio verification.** The SDL_mixer backend is verified for WAV sound effects
  and music loading, but MIDI playback through fluidsynth has not been confirmed
  against real UO music files.

### Not planned

- Windows and Linux builds are untouched and should still work, but neither has
  been tested since these changes.
- The plugin system is not portable and no attempt has been made to replace it.

---

## Licensing

OrionUO is MIT (see `LICENSE`) — a modified binary can be redistributed provided
the copyright notice is kept. SDL2, GLEW and zlib are permissive. **FreeImage is
dual-licensed FIPL/GPL and its terms should be read before publishing binaries.**

**Do not redistribute UO data files.** They are EA/Broadsword copyrighted content.
A bare executable is fine; a bundle containing game data is not.
