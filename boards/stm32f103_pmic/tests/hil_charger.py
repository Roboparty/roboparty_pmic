#!/usr/bin/env python3
"""HIL smoke test for the RoboParty USB-PD charger board.

The board exposes an ASCII command interface on USART3 (115200 8N1):

    NAME:value;

Read-only status commands are always tested.  LED/fan/charge/save commands
are gated behind explicit flags so a normal run cannot change the charger
state or wear the configuration flash.

Examples:
    python3 tests/hil_charger.py --port /dev/ttyUSB0
    python3 tests/hil_charger.py --port /dev/ttyUSB0 --allow-write
    python3 tests/hil_charger.py --self-test
"""

import argparse
import sys
import time

SERIAL_DEFAULT = "/dev/ttyUSB0"
BAUD_DEFAULT = 115200

READ_ONLY_CASES = [
    ("CHRG_INFO:1;", ["CHRG_V:", "CHRG_I:", "BAT_V:", "ICTRL:", "CHG_EN:"]),
    ("BAT_INST:1;", ["BAT_INST:"]),
    ("IR_REC:1;", ["IR_REC:"]),
    ("BAT_SOC:1;", ["BMS_SOC:"]),
    ("BAT_SOH:1;", ["BMS_SOH:"]),
    ("SYS_INFO:1;", ["UPTIME:", "CHARGE_STACK:", "BMS_STACK:", "UI_STACK:", "HOST_STACK:"]),
]

WRITE_CASES = [
    ("R:10;", None),
    ("G:10;", None),
    ("B:10;", None),
    ("FAN:49;", None),
]

CHARGE_CASES = [
    ("CHG_EN:0;", None),
]

SAVE_CASES = [
    ("SAVE:1;", None),
]


class FakeTransport:
    """Offline responder used by --self-test."""

    def __init__(self):
        self.commands = []

    def exchange(self, command, timeout):
        self.commands.append(command)
        name = command.split(":", 1)[0]
        replies = {
            "CHRG_INFO": (
                "CHRG_SN:HIL-TEST;SW_DATE:2026-11-02-A;"
                "R:10;G:10;B:10;FAN:49;BAT_INST:1;IR_REC:0;"
                "CHRG_V:20.0V;CHRG_I:1.0A;BAT_V:24.0V;"
                "ICTRL:14;CHG_EN:1;BMS_SOC:80;BMS_SOH:99;"
            ),
            "BAT_INST": "BAT_INST:1",
            "IR_REC": "IR_REC:0",
            "BAT_SOC": "BMS_SOC:80;",
            "BAT_SOH": "BMS_SOH:99;",
            "SYS_INFO": (
                "UPTIME:12345;CHARGE_STACK:120/1024;BMS_STACK:96/1024;"
                "UI_STACK:144/1024;HOST_STACK:200/1280;"
            ),
        }
        return replies.get(name, command)

    def close(self):
        pass


class SerialTransport:
    def __init__(self, port, baud, timeout):
        try:
            import serial
        except ImportError as exc:
            raise SystemExit("pyserial is required for HIL runs: pip install pyserial") from exc

        self.timeout = timeout
        self.serial = serial.serial_for_url(port, baudrate=baud, timeout=0.05)

    def exchange(self, command, timeout):
        self.serial.reset_input_buffer()
        self.serial.write(command.encode("ascii"))
        self.serial.flush()

        deadline = time.monotonic() + timeout
        last_rx = time.monotonic()
        data = bytearray()

        while time.monotonic() < deadline:
            chunk = self.serial.read(self.serial.in_waiting or 1)
            if chunk:
                data.extend(chunk)
                last_rx = time.monotonic()
                if b";" in chunk or b"\n" in chunk:
                    break
            elif data and (time.monotonic() - last_rx) >= 0.05:
                break

        return data.decode("ascii", errors="replace")

    def close(self):
        self.serial.close()


def run_case(transport, command, expected, timeout):
    response = transport.exchange(command, timeout)
    if expected is None:
        ok = True
    else:
        ok = all(item in response for item in expected)

    status = "PASS" if ok else "FAIL"
    print(f"[{status}] {command}")
    if response:
        print(f"       <- {response.strip()}")
    elif not ok:
        print("       <- <empty response>")
    return ok


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default=SERIAL_DEFAULT)
    parser.add_argument("--baud", type=int, default=BAUD_DEFAULT)
    parser.add_argument("--timeout", type=float, default=1.0)
    parser.add_argument("--allow-write", action="store_true",
                        help="exercise LED and fan commands")
    parser.add_argument("--allow-charge", action="store_true",
                        help="exercise CHG_EN:0 (disable only)")
    parser.add_argument("--allow-save", action="store_true",
                        help="exercise SAVE:1 (writes the config flash page)")
    parser.add_argument("--dry-run", action="store_true",
                        help="print the command plan without opening the port")
    parser.add_argument("--self-test", action="store_true",
                        help="run against an offline fake responder")
    args = parser.parse_args()

    cases = list(READ_ONLY_CASES)
    if args.allow_write:
        cases += WRITE_CASES
    if args.allow_charge:
        cases += CHARGE_CASES
    if args.allow_save:
        cases += SAVE_CASES

    if args.dry_run:
        for command, _ in cases:
            print(command)
        return 0

    transport = FakeTransport() if args.self_test else SerialTransport(args.port, args.baud, args.timeout)
    failed = 0

    try:
        for command, expected in cases:
            if not run_case(transport, command, expected, args.timeout):
                failed += 1
    finally:
        transport.close()

    total = len(cases)
    print(f"\n{total - failed}/{total} passed")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
