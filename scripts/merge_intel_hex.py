#!/usr/bin/env python3
"""Merge Intel HEX files into one image and preserve record addresses."""

import sys


def parse(path):
    memory = {}
    base = 0
    with open(path, "r", encoding="ascii") as stream:
        for line in stream:
            line = line.strip()
            if not line.startswith(":"):
                continue
            raw = bytes.fromhex(line[1:])
            size = raw[0]
            address = (raw[1] << 8) | raw[2]
            record_type = raw[3]
            data = raw[4:4 + size]
            if record_type == 0:
                for offset, value in enumerate(data):
                    memory[base + address + offset] = value
            elif record_type == 2:
                base = ((data[0] << 8) | data[1]) << 4
            elif record_type == 4:
                base = ((data[0] << 8) | data[1]) << 16
            elif record_type == 1:
                break
    return memory


def record(record_type, address, data):
    body = bytes([len(data), (address >> 8) & 0xFF, address & 0xFF, record_type]) + data
    checksum = (-sum(body)) & 0xFF
    return ":" + body.hex().upper() + f"{checksum:02X}"


def main(argv):
    if len(argv) != 4:
        print("usage: merge_intel_hex.py <input1.hex> <input2.hex> <output.hex>", file=sys.stderr)
        return 2

    merged = {}
    for path in argv[1:3]:
        for address, value in parse(path).items():
            if address in merged:
                print(f"overlap at 0x{address:08X}", file=sys.stderr)
                return 1
            merged[address] = value

    if not merged:
        print("no data records found", file=sys.stderr)
        return 1

    addresses = sorted(merged)
    lines = []
    upper = None
    index = 0
    while index < len(addresses):
        start = addresses[index]
        chunk = []
        while (
            index < len(addresses)
            and addresses[index] == start + len(chunk)
            and len(chunk) < 16
        ):
            chunk.append(merged[addresses[index]])
            index += 1
        current_upper = start >> 16
        if current_upper != upper:
            lines.append(record(4, 0, bytes([(current_upper >> 8) & 0xFF, current_upper & 0xFF])))
            upper = current_upper
        lines.append(record(0, start & 0xFFFF, bytes(chunk)))
    lines.append(":00000001FF")

    with open(argv[3], "w", encoding="ascii") as stream:
        stream.write("\n".join(lines) + "\n")
    print(f"merged {len(addresses)} bytes: 0x{addresses[0]:08X}-0x{addresses[-1]:08X}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
