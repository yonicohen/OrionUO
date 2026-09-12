# OrionUO Client

An alternative, open source Ultima Online graphic client.

This is a fork of [Hotride/OrionUO](https://github.com/Hotride/OrionUO) that adds
a **native macOS build** (Apple Silicon and Intel). Upstream shipped a Windows
client with a partial, never-finished Linux port; this fork completes it — the
client builds, runs, renders, plays sound, takes input and connects to a live
Sphere shard on macOS.

* Platforms: **macOS**, Windows, Linux
* Rendering: OpenGL 2.0 and higher

Upstream has not been updated since ~2018 and has diverged from the current Orion
binary, which matters for one protocol handshake (see [Known gaps](#known-gaps)).

Verified working against a live Sphere 0.56b shard: login, character creation,
world rendering, movement, sound, text input, vendors and gump interaction.

> **Just want to play?** Don't build anything. Grab the archive for your
> platform from the [releases page](https://github.com/yonicohen/OrionUO/releases)
> and run `./setup.sh` (`setup.cmd` on Windows): it asks where your Ultima
> Online folder is and then starts the client on Ignis UO.
> [IGNIS_UO.md](IGNIS_UO.md) has the details, troubleshooting and
> controls.

---

## Getting the code

```bash
git clone https://github.com/yonicohen/OrionUO.git
cd OrionUO
```

The port is merged into `master`; the `macos-port` branch is kept for history.

---

## Building

Pick your platform. macOS is the one this fork was made for and is the most
exercised; Windows and Linux build from the same source and are covered by CI,
but have had far less use.

### macOS

Homebrew, plus:

```bash
brew install cmake ninja sdl2 sdl2_image sdl2_mixer freeimage glew
```

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

#### Notes on the macOS build

- `cotire` (the old precompiled-header helper) is unmaintained and fails under
  CMake 4.x. It is replaced by CMake's native `target_precompile_headers`, and
  kept behind `-DORION_USE_COTIRE=ON` only for the legacy unity build.
- The client links `sdl2-compat` - the SDL2 API reimplemented on SDL3 - because
  that is what Homebrew's `sdl2` formula now installs; `/opt/homebrew/opt/sdl2`
  is a symlink into the sdl2-compat cellar. `sdl2_mixer` resolves to the same
  library, so there is one SDL in the process, not two. Moving off it would mean
  building SDL2 from source or porting to SDL3.
- macOS OpenGL is deprecated but functional. The renderer is fixed-function
  GL 2.x, so it runs in the legacy profile — 2.1 via Apple's Metal-backed GL.

### Windows

```bat
md build
cd build
cmake -G "Visual Studio 2017" ..
```

> This builds a 32-bit executable; add `Win64` to the generator name for 64-bit.
> Plugins are unsupported in the 64-bit client.

> `ORION_WISP` selects the original pure-Win32 implementation. Disabling it uses
> the SDL path, which is what the macOS build runs on.

### Linux

```bash
mkdir build && cd build
cmake -G Ninja .. -DCMAKE_BUILD_TYPE=Release
ninja OrionUO -j8
./OrionUO/OrionUO
```

> As on macOS you need a `Client.cuo` and a `uo_debug.cfg` containing
> `CustomPath=/path/to/uo/data`.

---

## Running

You need a real Ultima Online installation. **No UO data files ship with this
repository or with any release**, and they cannot be redistributed - they are
EA/Broadsword copyright. The client reads the `.mul` / `.uop` files from an
install you already have: your shard's download, or the free Classic Client
from <https://uo.com/client-download/>.

The release archives do this for you. `packaging/setup.sh` (shipped in each
archive as `setup.sh`) symlinks the data into a `data/` directory beside the
binary, writes `uo_debug.cfg`, generates a `Client.cuo` if the install has none,
and launches the client. Nothing is written into your UO folder. The shard it
connects to lives in `packaging/shard.conf`, and `packaging/play-ignis.sh`
turns that into the client's `-login` argument.

What follows is the manual equivalent, for a build tree.

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

Each option must arrive as a **single** argument, quotes included:
`ParseCommandLine` tokenises every argument on spaces, commas and colons, so an
unquoted `-login host,port` reaches the client as two arguments and is dropped
without a word.

`tools/version-sweep.sh` uses these to test candidate Orion versions unattended.

---

## What this fork changes

The port is mostly renderer work. The short version:

- **Rendering.** Upstream drew through `glBegin`/`glEnd` immediate mode, which
  macOS's GL profile does not have. All of it goes through a batched vertex path
  now — a shader and a vertex buffer, with a fixed-function fallback — plus a
  matrix stack of the client's own instead of `glTranslatef`/`glOrtho`. On top of
  that: Retina-correct output, vsync, sharp filtering and 2x art upscaling.
- **It connects to a live shard.** Four crypto bugs, the uninitialised login
  seed, and several disconnect-handling faults had to be fixed before a modern
  Sphere server would talk to it.
- **The Windows build compiles again.** It had not since ~2018 — winsock header
  order, missing import libraries, and 64-bit detection using a retired CMake
  convention.
- **Releases are built by CI** on macOS, Linux and Windows, and packaged so a
  player can run one script instead of following a page of instructions.

An Android port lives on the `android-port` branch: it plays, with touch
controls and an on-screen movement stick, but has no hue colorisation yet.

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

### Not planned

- Windows and Linux builds are untouched and should still work, but neither has
  been tested since these changes.
- The plugin system is not portable and no attempt has been made to replace it.


---

## Orion Community

[Discord](https://discord.gg/xSnqcBU8a8)

[WIKI](https://github.com/Hotride/OrionUO/wiki)

[github issue tracking](https://github.com/Hotride/OrionUO/issues)

## Download

* Download [Orion Launcher](http://orionuo.online/Launcher.html) to set everything up and play right away!

### Other Orion Projects

* [Orion.dll](https://github.com/Hotride/OrionDLL) protocol cryptography
* [Orion Launcher](https://github.com/Hotride/OrionLauncher)

--------------

---

## Contributing

See the project planning [here](https://github.com/Hotride/OrionUO/projects) to find tasks on which you can help.

  > More detailed contribution documentation soon

## Contributors

[Hotride](https://github.com/Hotride/) (Author)

[AimedNuu](https://github.com/AimedNuu)

and [Others](https://github.com/Hotride/OrionUO/graphs/contributors)

--------------
## Support this project, make a donation!

[PayPal](https://www.paypal.me/Hotride)

WebMoney: R644829964694 Z983232789532 E400319624386

Patreon: https://www.patreon.com/hotride

---

## Licensing

OrionUO is MIT licensed (see `LICENSE`) — a modified binary may be redistributed
provided the copyright notice is kept. SDL2, GLEW and zlib are permissive.
**FreeImage is dual-licensed FIPL/GPL and its terms should be read before
publishing binaries.**

**Do not redistribute UO data files.** They are EA/Broadsword copyrighted
content. A bare executable is fine; a bundle containing game data is not.
