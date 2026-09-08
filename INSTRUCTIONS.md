# Playing on Ignis UO from a Mac

Step-by-step setup for [Ignis UO](https://uo.jmaul.co.uk) (formerly JIMUO) using
this client. The shard permits only the Orion client, so a stock macOS UO client
will not work — this is why the port exists.

**You need:** a Mac (Apple Silicon or Intel), [Homebrew](https://brew.sh), and
about 4 GB of free disk.

---

## 1. Get the shard's client package

Download `JimUO_May2025.zip` (1.4 GB) from the link on the shard's
[Getting Started](https://uo.jmaul.co.uk/index.php?title=Getting_Started) page
and unzip it.

You want the **`Ultima Online`** folder inside — that is the game data. The
`Orion Launcher` folder is Windows-only and is not needed.

> UO data files cannot be redistributed, which is why they are not in this
> repository. You must get them from the shard.

## 2. Install dependencies

```bash
brew install cmake ninja sdl2 sdl2_image sdl2_mixer freeimage glew
```

## 3. Build the client

```bash
git clone https://github.com/yonicohen/OrionUO.git
cd OrionUO
cmake -G Ninja -S . -B build -DCMAKE_BUILD_TYPE=Release
ninja -C build OrionUO
```

## 4. Point it at the game data

```bash
cd build/OrionUO
echo "CustomPath=/full/path/to/Ultima Online" > uo_debug.cfg
```

Use the real path to the folder you unzipped. No quotes are needed in the file
even though the path contains a space.

## 5. Run it

```bash
./OrionUO "-login uo.jmaul.co.uk,2593" "-orionversion 1.0.35.1"
```

The `cd` in step 4 matters — the client resolves `uo_debug.cfg` and `Client.cuo`
relative to its working directory.

Alternatively, `./tools/setup-macos.sh "/full/path/to/Ultima Online"` does steps
2 through 4 in one go.

---

## 6. First time in the client

1. **Choose `Ignis UO` on the server list — not `DO NOT USE`.** The shard
   advertises two servers and the dead one is listed first. The client
   remembers your choice afterwards.
2. Enter any account name and password; the account is created on first login.
3. Create your character.
4. Use the **Starter Skills Selector Stone** before the starting gate. It wants
   **900 total skill points and 275 stats** — both, or the gate refuses you.

---

## Troubleshooting

**Login hangs, or you are dropped immediately after the login packet.**
The server still has your previous session open. Wait 30–60 seconds before
retrying; reconnecting inside that window keeps re-triggering the refusal.

**Kicked about 6 seconds after entering the world**, with
*"This server requires the latest ORION version"*.
The shard checks the reported Orion version. Bump it — no rebuild required:

```bash
./OrionUO "-login uo.jmaul.co.uk,2593" "-orionversion 1.0.37.0"
```

Current releases are listed at
`http://orionuo.online/Updates5152/BackupsList64.html`.

**`Failed to LoadLibrary .../OA/OrionAssistant.dll` at startup.**
Expected and harmless. The Orion Assistant is a Windows DLL and cannot load
natively, so scripting and macros are unavailable. Everything else works.

**`Client.cuo is missing!`**
It normally ships in the shard's package and is found automatically. To generate
one:

```bash
python3 tools/make_client_cuo.py -o Client.cuo \
    --client-version CV_70180 --encryption ET_TFISH --version-text 7.0.20.0
```

**No sound.**
MIDI music needs `uo_4mb_2.sf2`, which the build copies automatically. MP3 music
and sound effects need nothing extra.

---

## Controls worth knowing

| | |
| --- | --- |
| Move | Hold **right mouse button**; further from your character = faster |
| Run | Same, cursor well away from your character (2× walking) |
| Ride | Mounts are another 2× on top — a mounted run is 4× walking |
| Shard chat | **Yell** (the server broadcasts it); `.chat` toggles world chat |
| Resurrect | As a ghost, **toggle war mode** to manifest, then approach a healer |
| Use a skill | Paperdoll → skills scroll → blue button beside the skill |
| Buy | Say `buy` to a vendor, or double-click them; stand adjacent |

---

See [README.md](README.md) for build details, what was fixed in this port, and
known limitations.
