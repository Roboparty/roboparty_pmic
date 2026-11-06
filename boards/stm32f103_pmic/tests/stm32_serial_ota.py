#!/usr/bin/env python3
"""Stream an STM32F103C8 PMIC application to its resident USART3 bootloader."""

import argparse
import re
import struct
import sys
import time
import zlib
from pathlib import Path


APP_START = 0x08001000
APP_END = 0x0800FC00
CHUNK = 64


def validate_image(data):
    if len(data) < 8 or len(data) > APP_END - APP_START:
        raise ValueError("application size exceeds the reserved Flash region")
    sp, pc = struct.unpack_from("<II", data)
    if not (0x20000000 <= sp <= 0x20005000 and sp % 4 == 0):
        raise ValueError("invalid STM32F103C8 initial stack pointer")
    if not (APP_START <= pc < APP_END and pc & 1):
        raise ValueError("image is not linked at 0x08001000")


def read_until(port, token, timeout):
    deadline = time.monotonic() + timeout
    received = bytearray()
    while time.monotonic() < deadline:
        received.extend(port.read(128))
        if token in received:
            return bytes(received)
        if len(received) > 512:
            received = received[-512:]
    raise TimeoutError(f"expected {token!r}, received {bytes(received)!r}")


def command(port, payload, expected, timeout):
    port.write(payload.encode("ascii"))
    port.flush()
    return read_until(port, expected.encode("ascii"), timeout)


def update(port, data, timeout, recover=False, uid=None):
    validate_image(data)
    if not recover:
        port.reset_input_buffer()
        response = command(port, "SN_GET:1;", "CHRG_SN:", timeout)
        match = re.search(rb"CHRG_SN:([0-9A-F]{24});", response)
        if not match:
            response += read_until(port, b";", timeout)
            match = re.search(rb"CHRG_SN:([0-9A-F]{24});", response)
        if not match:
            raise RuntimeError("PMIC UID response is incomplete")
        found = match.group(1).decode("ascii")
        if uid and found != uid.upper():
            raise RuntimeError(f"UID mismatch: expected {uid}, got {found}")
        print(f"PMIC UID: {found}")
        entry_reply = command(port, "OTA_ENTER:1;", "OTA_BOOT;", timeout)
        if b"OTA_BOOT:1;" not in entry_reply:
            read_until(port, b"OTA_BOOT:1;", timeout)
    else:
        port.reset_input_buffer()
        command(port, "OTA_PING:1;", "OTA_BOOT:1;", timeout)
        print("STM32 bootloader recovery mode confirmed")

    crc = zlib.crc32(data) & 0xFFFFFFFF
    command(port, f"OTA_BEGIN:{len(data)},{crc};", "OTA_READY;", timeout)
    for offset in range(0, len(data), CHUNK):
        block = data[offset:offset + CHUNK]
        command(port, f"OTA_DATA:{offset},{block.hex()};",
                f"OTA_ACK:{offset + len(block)};", timeout)
        if (offset // CHUNK) % 64 == 0:
            print(f"{offset + len(block)}/{len(data)} bytes", flush=True)
    command(port, "OTA_END:1;", "OTA_DONE;", max(timeout, 10.0))
    print(f"Written {len(data)} bytes, CRC32 {crc:08X}; bootloader reports OTA_DONE")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True)
    parser.add_argument("--bin", required=True, type=Path)
    parser.add_argument("--uid", help="expected 24-character chip UID")
    parser.add_argument("--recover", action="store_true",
                        help="start at bootloader after an interrupted update")
    parser.add_argument("--timeout", type=float, default=3.0)
    args = parser.parse_args(argv)
    if args.timeout <= 0 or (args.uid and not re.fullmatch(r"[0-9A-Fa-f]{24}", args.uid)):
        parser.error("timeout must be positive and UID must be 24 hex characters")
    data = args.bin.read_bytes()
    validate_image(data)
    import serial
    with serial.serial_for_url(args.port, baudrate=115200, timeout=0.05,
                               write_timeout=args.timeout) as port:
        port.dtr = False
        port.rts = False
        update(port, data, args.timeout, args.recover, args.uid)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, RuntimeError, TimeoutError, ValueError) as exc:
        print(f"OTA failed: {exc}", file=sys.stderr)
        sys.exit(1)
