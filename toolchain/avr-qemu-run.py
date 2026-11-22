#!/usr/bin/env python3
"""Run an AVR program under QEMU's mega2560 machine and return its exit code.

QEMU's AVR machines have no semihosting, so a program cannot make QEMU exit
with a status. Programs print a final line "OSEK_EXIT <code>" on USART0
instead (see test/common/avr/console.c). This script runs QEMU with USART0
on a pipe, passes the output through, stops QEMU when the marker appears
and exits with the code from it.

Usage: avr-qemu-run.py [--timeout SECONDS] [--icount SHIFT] program.elf [qemu options...]

Without --icount the guest runs as fast as the host allows while its timers
follow host time. With --icount every guest instruction takes 2^SHIFT ns of
virtual time, which makes runs reproducible; 6 gives about 16 million
instructions per second, close to a 16 MHz AVR.

Exits with 124 on timeout and 125 if QEMU ends without the marker.
"""

import argparse
import os
import re
import subprocess
import sys
import threading

MARKER = re.compile(rb"^OSEK_EXIT (-?\d+)\s*$")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--timeout", type=float,
                        default=float(os.environ.get("AVR_QEMU_TIMEOUT", "50")),
                        help="seconds before giving up (default 50, or $AVR_QEMU_TIMEOUT)")
    parser.add_argument("--icount", type=int, metavar="SHIFT",
                        help="run with -icount shift=SHIFT,align=off,sleep=off")
    parser.add_argument("--qemu", default=os.environ.get("AVR_QEMU", "qemu-system-avr"))
    parser.add_argument("program")
    parser.add_argument("qemu_args", nargs=argparse.REMAINDER)
    args = parser.parse_args()

    cmd = [args.qemu, "-M", "mega2560", "-display", "none", "-monitor", "none",
           "-serial", "stdio", "-bios", args.program]
    if args.icount is not None:
        cmd += ["-icount", "shift=%d,align=off,sleep=off" % args.icount]
    cmd += args.qemu_args

    proc = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE)

    timer = threading.Timer(args.timeout, proc.kill)
    timer.start()
    code = None
    try:
        for line in proc.stdout:
            line = line.replace(b"\r", b"")
            match = MARKER.match(line)
            if match:
                code = int(match.group(1))
                break
            sys.stdout.buffer.write(line)
            sys.stdout.flush()
    finally:
        timed_out = not timer.is_alive() and code is None
        timer.cancel()
        proc.kill()
        proc.wait()

    if code is not None:
        return code & 0xFF
    if timed_out:
        sys.stderr.write("avr-qemu-run: timed out after %g s\n" % args.timeout)
        return 124
    sys.stderr.write("avr-qemu-run: QEMU exited (%d) without OSEK_EXIT\n" % proc.returncode)
    return 125


if __name__ == "__main__":
    sys.exit(main())
