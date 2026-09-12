# The server half

Scripts for a local SphereServer X shard, so the client can be changed on both
ends at once. `docs/LOCAL_SHARD.md` has the full setup; this is what goes in it.

| file | what it is |
|---|---|
| `start.sh` | starts the shard with a pipe on its stdin, so console commands can be sent while it runs |
| `play.sh` | connects the desktop client and logs straight in |
| `scripts/bootstrap_test_char.scp` | gives an account a character on first login, dressed in known hues |
| `scripts/orion_protocol.scp` | the private channel: hooks the client's own packet and answers on it |

Both scripts go in the shard's `scripts/custom/` and are listed in
`spheretables.scp`. `orion_protocol.scp` also needs `PACKET252=f_orion_packet`
in `sphere.ini`.

The character is dressed deliberately: five different hues, so a renderer can
be checked against a known answer rather than an opinion. That is how the GLES
2.0 colouriser was verified - and how a magenta torch flame was shown to be
faithful rather than broken, by logging what the desktop client was handed for
the same light and finding the same magenta palette.
