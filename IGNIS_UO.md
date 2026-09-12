# Playing on Ignis UO

Setup for [Ignis UO](https://uo.jmaul.co.uk) (formerly JIMUO) using this client.
The shard permits only the Orion client, so a stock macOS UO client will not
work — this is why the port exists.

**You need:** a Mac, Linux box or Windows PC, and an Ultima Online installation
you already have. The shard address is baked into the package; there is nothing
to configure.

---

## Quick start

Download the archive for your platform from the
[releases page](https://github.com/yonicohen/OrionUO/releases) and run the setup
script. It asks where your Ultima Online folder is, wires everything up, and
starts the game.

**macOS**

```bash
mkdir orionuo && tar -xzf orionuo-macos-arm64.tar.gz -C orionuo
cd orionuo && ./setup.sh
```

macOS will refuse to open the binary the first time because it is not notarised.
Either right-click `OrionUO` → *Open* once, or run:

```bash
xattr -dr com.apple.quarantine .
```

**Linux**

```bash
mkdir orionuo && tar -xzf orionuo-linux-x86_64.tar.gz -C orionuo
cd orionuo && ./setup.sh
```

**Windows**

Unpack the archive and double-click **`setup.cmd`**.

Afterwards, start the game with `./play-ignis.sh` (macOS and Linux) or
`play-ignis.cmd` (Windows). Setup only has to be run once.

You can skip the prompt by passing the path:

```bash
./setup.sh "/path/to/Ultima Online"
```

---

## Where the UO data comes from

**This package contains no UO data and none is ever published with it.** The
art, maps, sounds and animations are EA/Broadsword copyright and cannot be
redistributed. `setup.sh` takes them from an installation on your own machine:

* **Your shard's package.** Ignis UO links `JimUO_May2025.zip` (1.4 GB) from its
  [Getting Started](https://uo.jmaul.co.uk/index.php?title=Getting_Started)
  page. Unzip it and point setup at the **`Ultima Online`** folder inside. This
  is the easiest route: it already contains a `Client.cuo`.
* **The free Classic Client** from <https://uo.com/client-download/>. Install it
  (the installer is Windows-only, and the data is fetched by its patcher on
  first run), then point setup at the resulting folder.

Setup does not copy the data. On macOS and Linux it creates a `data/` directory
of symlinks — a few kilobytes, not 2.6 GB — and on Windows it uses hard links
and junctions, falling back to copying if the package and your install are on
different drives. **Nothing in your UO folder is written to or modified.**

Some of the install is never read and is left out: `Models/`,
`Orion Launcher/`, the `.bik` intro videos, and the Windows `.exe`/`.dll` files.
`Music/` is included but only matters if you want in-game music.

---

## First time in the client

1. **Choose `Ignis UO` on the server list — not `DO NOT USE`.** The shard
   advertises two servers and the dead one is listed first. The client
   remembers your choice afterwards.
2. Enter any account name and password; the account is created on first login.
3. Create your character.
4. Use the **Starter Skills Selector Stone** before the starting gate. It wants
   **900 total skill points and 275 stats** — both, or the gate refuses you.

---

## Pointing the package at another shard

`shard.conf` in the package is the whole configuration:

```
SHARD_NAME=Ignis UO
SHARD_HOST=uo.jmaul.co.uk
SHARD_PORT=2593
ORION_VERSION=
```

The launcher turns that into `-login uo.jmaul.co.uk,2593` on the client's
command line. Edit the file to play elsewhere; nothing else needs changing.

---

## Troubleshooting

**macOS: "OrionUO cannot be opened because the developer cannot be verified".**
The build is not notarised. Run `xattr -dr com.apple.quarantine .` in the
package directory, or right-click the binary and choose *Open* once.

**macOS: missing Homebrew libraries.**
`setup.sh` checks the ones the binary actually links and prints the exact
command. It is normally:

```bash
brew install sdl2 sdl2_mixer glew
```

**Linux: missing shared libraries.** On Debian/Ubuntu:

```bash
sudo apt-get install libsdl2-2.0-0 libsdl2-image-2.0-0 libsdl2-mixer-2.0-0 \
    libglew2.2 libglu1-mesa
```

**Login hangs, or you are dropped immediately after the login packet.**
The server still has your previous session open. Wait 30–60 seconds before
retrying; reconnecting inside that window keeps re-triggering the refusal.

**Kicked about 6 seconds after entering the world**, with
*"This server requires the latest ORION version"*.
The shard checks the reported Orion version. Bump it in `shard.conf` — no
rebuild required:

```
ORION_VERSION=1.0.38.0
```

Current releases are listed at
`http://orionuo.online/Updates5152/BackupsList64.html`.

**`Failed to LoadLibrary .../OA/OrionAssistant.dll` at startup.**
Expected and harmless. The Orion Assistant is a Windows DLL and cannot load
natively, so scripting and macros are unavailable. Everything else works.

**`Client.cuo is missing!`**
`Client.cuo` names the UO version and login encryption, and is normally written
by the Windows-only Orion Launcher. If your install has one, setup links it. If
not, setup generates one for 7.0.20.0 with `make_client_cuo.py`, which needs
Python 3. Never overwrite a `Client.cuo` your shard shipped — that copy carries
login crypt keys a generated file zeroes out.

To generate one by hand:

```bash
python3 make_client_cuo.py -o data/Client.cuo \
    --client-version CV_70180 --encryption ET_TFISH --version-text 7.0.20.0
```

**Stuck on "Verifying account", or "There is some problem communicating with
Origin".**
That dialog is packet `0x82` (Login Denied); the client discards the reason byte
that says why. Ask the server directly with `tools/login-probe.py` from the
source tree:

```bash
./tools/login-probe.py uo.jmaul.co.uk 2593
```

Reason `0x00` means the name or password is wrong. Reason `0x01` means a previous
session is still open — wait 30-60 seconds. Reason `0x04` is generic and is sent
before the server looks at your credentials: if a made-up account and your real
one both return `0x04`, the shard is refusing all logins and nothing client-side
will fix it. Compare the two before assuming the client is at fault:

```bash
./tools/login-probe.py uo.jmaul.co.uk 2593 -a madeupname123
./tools/login-probe.py uo.jmaul.co.uk 2593 -a youraccount -p yourpassword
```

**No sound.**
MIDI music needs `uo_4mb_2.sf2`, which ships in the package. MP3 music and sound
effects need nothing extra.

**Starting over.** Delete `data/` and `uo_debug.cfg` from the package and run
`./setup.sh` again. Your UO installation is never touched, so there is nothing
to repair there.

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

## Building it yourself

Only needed if you want to change the client; the release archives are prebuilt.
See [README.md](README.md#building) for macOS, Windows and Linux.
