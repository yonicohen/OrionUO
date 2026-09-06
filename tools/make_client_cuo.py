#!/usr/bin/env python3
"""Generate a Client.cuo for OrionUO.

Client.cuo is normally produced by the Windows-only Orion Launcher, which makes
it impossible to start the client on macOS or Linux. It is not encrypted: on a
build with the in-tree crypto (ORION_DLL_STATIC=ON, i.e. every non-Windows
build) Crypt/CryptEntry.cpp reads it as a plain binary record. This script
writes that record directly.

Layout, per ApplyInstall() in OrionUO/Crypt/CryptEntry.cpp:

    u8            format version
    u8            encryption type   (ENCRYPTION_TYPE)
    u8            client version    (CLIENT_VERSION enum index)
    u8            length of the version string
    bytes         version string
    14 bytes      crypt keys and seed (read past, unused by the in-tree crypto)
    u8            map count         (format version >= 4)
    N x (u16,u16) map width/height, little endian
    u8            client flag
    u8            use verdata
    u8            plugin count

Note the map count must stay 6: LoadPlugins() seeks past this section with a
hardcoded 39-byte offset (14 crypt bytes + 1 count + 6*4 map bytes).
"""

import argparse
import struct
import sys

# CLIENT_VERSION, in declaration order - the file stores the enum's index.
CLIENT_VERSIONS = [
    "CV_OLD", "CV_200", "CV_305D", "CV_306E", "CV_308D", "CV_308J", "CV_308Z",
    "CV_400B", "CV_405A", "CV_4011D", "CV_500A", "CV_5020", "CV_5090",
    "CV_6000", "CV_6013", "CV_6017", "CV_6040", "CV_6060", "CV_60142",
    "CV_60144", "CV_7000", "CV_7090", "CV_70130", "CV_70160", "CV_70180",
    "CV_70240", "CV_70331",
]

# ENCRYPTION_TYPE, in declaration order.
ENCRYPTION_TYPES = [
    "ET_NOCRYPT", "ET_OLD_BFISH", "ET_1_25_36", "ET_BFISH", "ET_203", "ET_TFISH",
]

# Felucca, Trammel, Ilshenar, Malas, Tokuno, TerMur.
DEFAULT_MAPS = [
    (7168, 4096),
    (7168, 4096),
    (2304, 1600),
    (2560, 2048),
    (1448, 1448),
    (1280, 4096),
]

MAP_COUNT = 6


def build(client_version, encryption, version_text, maps, use_verdata, client_flag):
    if len(maps) != MAP_COUNT:
        raise ValueError(
            "map count must be %d; LoadPlugins() seeks past this section with a "
            "hardcoded offset that assumes it" % MAP_COUNT
        )

    text = version_text.encode("ascii")
    if len(text) > 255:
        raise ValueError("version string too long")

    out = bytearray()
    out.append(4)                                   # format version
    out.append(encryption)                          # ENCRYPTION_TYPE
    out.append(client_version)                      # CLIENT_VERSION
    out.append(len(text))
    out += text
    out += b"\x00" * 14                             # crypt keys and seed
    out.append(MAP_COUNT)
    for width, height in maps:
        out += struct.pack("<HH", width, height)
    out.append(1 if client_flag else 0)
    out.append(1 if use_verdata else 0)
    out.append(0)                                   # plugin count
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(
        description="Generate a Client.cuo for OrionUO",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "-o", "--output", default="Client.cuo", help="output path (default: Client.cuo)")
    parser.add_argument(
        "-c", "--client-version", default="CV_70331", choices=CLIENT_VERSIONS,
        help="UO client version to speak (default: CV_70331)")
    parser.add_argument(
        "-e", "--encryption", default="ET_NOCRYPT", choices=ENCRYPTION_TYPES,
        help="login/game encryption (default: ET_NOCRYPT, what most free shards use)")
    parser.add_argument(
        "-t", "--version-text", default="7.0.33.1",
        help="version string reported to the server (default: 7.0.33.1)")
    parser.add_argument(
        "--use-verdata", action="store_true", help="load verdata.mul")
    parser.add_argument(
        "--client-flag", action="store_true", help="set the client flag byte")
    parser.add_argument(
        "--map", action="append", metavar="INDEX:WIDTHxHEIGHT", default=[],
        help="override one map's size, e.g. --map 0:6144x4096 for an old Felucca")
    args = parser.parse_args()

    maps = list(DEFAULT_MAPS)
    for override in args.map:
        try:
            index, size = override.split(":", 1)
            width, height = size.lower().split("x", 1)
            maps[int(index)] = (int(width), int(height))
        except (ValueError, IndexError):
            parser.error("bad --map value %r; expected INDEX:WIDTHxHEIGHT" % override)

    data = build(
        client_version=CLIENT_VERSIONS.index(args.client_version),
        encryption=ENCRYPTION_TYPES.index(args.encryption),
        version_text=args.version_text,
        maps=maps,
        use_verdata=args.use_verdata,
        client_flag=args.client_flag,
    )

    with open(args.output, "wb") as handle:
        handle.write(data)

    print("wrote %s (%d bytes)" % (args.output, len(data)), file=sys.stderr)
    print("  client version : %s (%s)" % (args.client_version, args.version_text), file=sys.stderr)
    print("  encryption     : %s" % args.encryption, file=sys.stderr)
    print("  maps           : %s" % ", ".join("%dx%d" % m for m in maps), file=sys.stderr)


if __name__ == "__main__":
    main()
