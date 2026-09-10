#!/usr/bin/env python3
"""Ask a UO login server what it thinks of a login, without the client.

When the client hangs on "Verifying account" or shows "There is some problem
communicating with Origin", that dialog is the client's rendering of packet
0x82 (Login Denied). The reason byte says why, but the client throws it away.
This sends the same seed + 0x80 sequence the client does and prints the reason.

    ./tools/login-probe.py uo.jmaul.co.uk 2593 --account name --password pw

Credentials are optional; without them a throwaway name is used, which is
enough to tell a server-side refusal from a credential problem. If a made-up
account and a real one come back with the identical reason, the server is not
checking credentials at all and the problem is server-side.
"""

import argparse
import socket
import struct
import sys
import time

REASONS = {
    0x00: "incorrect name or password",
    0x01: "someone is already using this account",
    0x02: "your account has been blocked",
    0x03: "account credentials are invalid",
    0x04: "communication problem (generic; server refused before checking)",
    0x05: "IGR concurrency limit met",
    0x06: "IGR time limit met",
    0x07: "general IGR authentication failure",
}


def probe(host, port, account, password, version, timeout):
    seed = struct.pack(">BI", 0xEF, 0x0A000001) + struct.pack(">IIII", *version)
    login = (b"\x80" + account.encode().ljust(30, b"\x00")
             + password.encode().ljust(30, b"\x00") + b"\x5d")

    started = time.time()
    sock = socket.create_connection((host, port), timeout=timeout)
    sock.settimeout(timeout)
    try:
        sock.sendall(seed)
        time.sleep(0.25)
        sock.sendall(login)
        try:
            data = sock.recv(4096)
        except socket.timeout:
            return "connection held open, no reply (server ignored the login)"
        if not data:
            return "server closed the connection without replying (FIN)"
        if data[0] == 0x82:
            reason = data[1]
            return "0x82 Login Denied, reason 0x%02X - %s" % (
                reason, REASONS.get(reason, "unknown"))
        if data[0] == 0xA8:
            return "0xA8 Server List - login ACCEPTED (%d bytes)" % len(data)
        return "0x%02X, %d bytes: %s" % (data[0], len(data), data[:32].hex())
    finally:
        sock.close()
        sys.stderr.write("  (%.0f ms)\n" % ((time.time() - started) * 1000))


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("host")
    parser.add_argument("port", nargs="?", type=int, default=2593)
    parser.add_argument("-a", "--account", default="probeaccount")
    parser.add_argument("-p", "--password", default="probepassword")
    parser.add_argument("-t", "--timeout", type=float, default=8.0)
    parser.add_argument(
        "-c", "--client-version", default="7.0.20.0",
        help="version reported in the seed packet (default: 7.0.20.0)")
    args = parser.parse_args()

    try:
        version = tuple(int(part) for part in args.client_version.split("."))
        if len(version) != 4:
            raise ValueError
    except ValueError:
        parser.error("--client-version must look like 7.0.20.0")

    print("%s:%d as %r" % (args.host, args.port, args.account))
    print("  %s" % probe(args.host, args.port, args.account, args.password,
                         version, args.timeout))


if __name__ == "__main__":
    main()
