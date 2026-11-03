import argparse
import json
import re
import socket
import time


def query(command, timeout):
    with socket.create_connection(("127.0.0.1", 22282), timeout=timeout) as sock:
        sock.settimeout(0.2)
        sock.sendall(command.encode("ascii"))
        deadline = time.monotonic() + timeout
        data = bytearray()
        final_field = b"HOST_STACK:" if command.startswith("SYS_INFO") else b"CHG_EN:"
        while time.monotonic() < deadline:
            try:
                chunk = sock.recv(512)
            except socket.timeout:
                continue
            if not chunk:
                break
            data.extend(chunk)
            if final_field in data and data.endswith(b";"):
                break
        return data.decode("ascii", errors="replace")


parser = argparse.ArgumentParser()
parser.add_argument("--duration", type=int, default=180)
parser.add_argument("--interval", type=float, default=2.0)
args = parser.parse_args()

start = time.monotonic()
next_at = start
counts = {"SYS_INFO": 0, "CHRG_INFO": 0, "empty": 0, "error": 0}
uptimes = []
stack_max = {}
charge_samples = []

while time.monotonic() - start < args.duration:
    kind = "SYS_INFO" if sum(counts.values()) % 2 == 0 else "CHRG_INFO"
    try:
        reply = query(kind + ":1;", 1.5)
        if not reply:
            counts["empty"] += 1
            print(f"{time.monotonic() - start:.1f}s {kind} EMPTY", flush=True)
        elif kind == "SYS_INFO" and "HOST_STACK:" in reply:
            counts[kind] += 1
            match = re.search(r"UPTIME:(\d+);", reply)
            if match:
                uptimes.append(int(match.group(1)))
            for name, used, total in re.findall(r"(\w+)_STACK:(\d+)/(\d+);", reply):
                stack_max[name] = max(stack_max.get(name, 0), int(used))
            print(f"{time.monotonic() - start:.1f}s {kind} {reply}", flush=True)
        elif kind == "CHRG_INFO" and "CHG_EN:" in reply:
            counts[kind] += 1
            charge_samples.append(reply)
            print(f"{time.monotonic() - start:.1f}s {kind} {reply}", flush=True)
        else:
            counts["error"] += 1
            print(f"{time.monotonic() - start:.1f}s {kind} PARTIAL {reply!r}", flush=True)
    except OSError as exc:
        counts["error"] += 1
        print(f"{time.monotonic() - start:.1f}s {kind} ERROR {exc}", flush=True)
    next_at += args.interval
    time.sleep(max(0, next_at - time.monotonic()))

print("SUMMARY " + json.dumps({
    "duration_s": round(time.monotonic() - start, 1),
    "counts": counts,
    "uptime_first_last_ms": [uptimes[0], uptimes[-1]] if uptimes else [],
    "uptime_monotonic": all(b > a for a, b in zip(uptimes, uptimes[1:])),
    "stack_max_bytes": stack_max,
    "charge_first": charge_samples[0] if charge_samples else "",
    "charge_last": charge_samples[-1] if charge_samples else "",
}, ensure_ascii=True), flush=True)
