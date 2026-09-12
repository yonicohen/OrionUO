# Running a local shard

A server you control, so the client can be changed on both ends at once. This
is what makes anything beyond UO's 1997 protocol possible: you cannot add a
message to a shard somebody else runs.

## What this needs

- SphereServer X, built from source (Apache-2.0, C++20, CMake)
- `mariadb-connector-c` — the libraries bundled with Sphere are Windows only
- A UO installation for the `.mul` data. Nothing is copied and nothing in it is
  modified: the shard links the files read-only.

## Building the server

```
git clone https://github.com/Sphereserver/Source-X.git sphere-x
brew install mariadb-connector-c
cd sphere-x
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -G Ninja
cmake --build build -j
```

Upstream builds with `-Werror` and a newer clang finds warnings theirs does
not, so that flag has to come out of `cmake/CompilerFlagsBase.cmake` first.

## Setting it up

The shard directory needs `sphere.ini`, `sphereCrypt.ini` (both from
`sphere-x/src/`), the script pack from
[Scripts-X](https://github.com/Sphereserver/Scripts-X) in `scripts/`, and the
built binary. In `sphere.ini`:

```
MulFiles=muls/     # a directory of symlinks to the UO data
AccApp=2           # let accounts be created on first login
UseNoCrypt=1       # this client does not encrypt
```

## Logging in without clicking

`-account` takes its values **hex encoded**, two digits per character:
`796F6E69` is `yoni`. That is not documented anywhere and looks exactly like a
broken argument if you pass plain text.

```
./OrionUO "-login 127.0.0.1,2593" "-account 796F6E69,796F6E69" -autologin -fastlogin
```

A fresh account has no characters and character creation needs a human, so
`scripts/custom/bootstrap_test_char.scp` makes one on first login from
`f_onaccount_login`. A bare `c_man` has no stats and dies as soon as the world
ticks, so it is given some.

## Talking to the client about things UO has no opcode for

This is the part that matters. The client already has a private packet - `0xFC`
carries Orion's own messages - and Sphere can hook any packet id into a script:

```
PACKET252=f_orion_packet        # in sphere.ini; 252 is 0xFC
```

The script is handed the raw bytes and can answer on the same socket:

```
[FUNCTION f_orion_packet]
SERV.LOG ORION in: packet=<ARGN1> bytes=<LOCAL.NUM> account='<LOCAL.ACCOUNT>'
ARGO.SENDPACKET B252 W9 W50 D3735928559
RETURN 1
```

`W50` is `OCT_ORION_FEATURES`, which the client handles in
`PACKET_HANDLER(OrionMessages)`. Sending `0xDEADBEEF` through it arrives in the
client intact:

```
server: ORION in: packet=252 bytes=09 account='yoni'
client: ORION features received from server: 0xDEADBEEF
```

So a custom client/server message costs a few lines of script and no C++ on
either side. Everything that needs the two ends to agree on something new -
structured item data, server-driven interface, actions that resolve while the
player is away - starts here.
