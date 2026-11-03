#!/usr/bin/env python3
"""Send a firmware image to the RP1.3 PMIC over USART3.

Protocol (ASCII):
    OTA_BEGIN:<size>,<crc32>;
    OTA_DATA:<offset>,<hex>;
    OTA_END:1;

The running application writes the image to the external W25Q128 staging
area and resets.  The bootloader validates the CRC and copies it to the
application region.

Examples:
    python3 tests/ota_tool.py --port /dev/ttyUSB0 --bin build/Release/ChargeBAT.bin
    python3 tests/ota_tool.py --self-test
"""

import argparse
import sys
import time
import zlib

DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_BAUD = 115200
DEFAULT_CHUNK = 64


def firmware_crc(data):
    return zlib.crc32(data) & 0xFFFFFFFF


def read_until(serial, token, timeout):
    deadline = time.monotonic() + timeout
    data = bytearray()

    while time.monotonic() < deadline:
        chunk = serial.read(serial.in_waiting or 1)
        if chunk:
            data.extend(chunk)
            if token.encode("ascii") in data:
                return data.decode("ascii", errors="replace")
        else:
            time.sleep(0.01)

    return data.decode("ascii", errors="replace")


def send_command(serial, command, expect, timeout):
    serial.write(command.encode("ascii"))
    serial.flush()
    response = read_until(serial, expect, timeout)
    if expect not in response:
        raise RuntimeError(f"expected {expect!r}, got {response!r}")
    return response


def send_image(serial, data, chunk_size, timeout):
    crc = firmware_crc(data)
    print(f"image: {len(data)} bytes, crc32=0x{crc:08X}")

    send_command(serial, f"OTA_BEGIN:{len(data)},{crc};", "OTA_READY;", timeout)

    offset = 0
    while offset < len(data):
        block = data[offset:offset + chunk_size]
        command = f"OTA_DATA:{offset},{block.hex()};"
        send_command(serial, command, f"OTA_ACK:{offset + len(block)};", timeout)
        offset += len(block)
        print(f"\r{offset}/{len(data)} bytes", end="", flush=True)

    print()
    send_command(serial, "OTA_END:1;", "OTA_DONE;", timeout)
    print("OTA done, device is rebooting")


def self_test():
    if firmware_crc(b"123456789") != 0xCBF43926:
        print("CRC32 self-test FAILED")
        return 1
    print("CRC32 self-test passed")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default=DEFAULT_PORT)
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--bin", dest="image", required=False)
    parser.add_argument("--chunk", type=int, default=DEFAULT_CHUNK)
    parser.add_argument("--timeout", type=float, default=3.0)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        return self_test()

    if not args.image:
        parser.error("--bin is required unless --self-test is used")
    if not 1 <= args.chunk <= 128:
        parser.error("--chunk must be 1..128 (hex line must fit the host parser)")

    try:
        import serial
    except ImportError as exc:
        raise SystemExit("pyserial is required: pip install pyserial") from exc

    with open(args.image, "rb") as f:
        data = f.read()

    with serial.Serial(port=args.port, baudrate=args.baud, timeout=0.05) as port:
        try:
            send_image(port, data, args.chunk, args.timeout)
        except RuntimeError as exc:
            print(f"\nOTA failed: {exc}")
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
