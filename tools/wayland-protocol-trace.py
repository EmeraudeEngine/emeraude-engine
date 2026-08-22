#!/usr/bin/env python3
"""
Emeraude Engine - Wayland protocol trace capture & analysis
Linux/Wayland only. Finds out WHO violated the Wayland protocol when the compositor kills the
window, which surfaces to the engine as VK_ERROR_SURFACE_LOST_KHR at present or acquire.

A compositor protocol error destroys the wl_display connection, so every Wayland object of the
process dies with it and nothing is recoverable: the engine can only shut down. The culprit is
therefore never in the shutdown path -- it is the request logged just before the error, and this
tool extracts it.

Usage:
    # Capture: run until a protocol error reproduces, keep only the failing log
    python3 wayland-protocol-trace.py --capture --runs 12 -- ./projet-alpha --load-demo game-logic --disable-cef

    # Analyse a log captured earlier (or any WAYLAND_DEBUG=1 output)
    python3 wayland-protocol-trace.py --analyse /tmp/wayland-trace-7.log

    # Widen the request window shown before the error
    python3 wayland-protocol-trace.py --analyse /tmp/wl.log --window 80

Notes:
    - WAYLAND_DEBUG=1 logs every request and event, which slows the client down a lot. A race can
      stop reproducing under trace: 12 clean runs is a RESULT (a tight race), not a failed attempt.
    - Requests (client -> compositor) are the lines carrying "->"; everything else is an event.
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile

DEFAULT_RUNS = 12
DEFAULT_TIMEOUT = 60
DEFAULT_WINDOW = 40

# [ 1234.567] wl_display@1.error(wp_linux_drm_syncobj_surface_v1@94, 3, "Release or Acquire ...")
ERROR_EVENT = re.compile(
    r"wl_display@\d+\.error\((?P<interface>[a-zA-Z0-9_]+)@(?P<id>\d+),\s*(?P<code>\d+),\s*\"(?P<message>[^\"]*)\""
)
# [ 1234.567]  -> wp_linux_drm_syncobj_manager_v1@42.get_surface(new id ...@94, wl_surface@23)
GET_SURFACE = re.compile(r"get_surface\(new id [a-zA-Z0-9_]+@(?P<object>\d+),\s*wl_surface@(?P<surface>\d+)\)")
REQUEST = re.compile(r"->\s*(?P<interface>[a-zA-Z0-9_]+)@(?P<id>\d+)\.(?P<name>[a-zA-Z0-9_]+)\((?P<args>.*)\)")

# Requests that decide whether a commit is legal with respect to explicit synchronisation.
ATTACH = "attach"
COMMIT = "commit"
SYNC_POINTS = ("set_acquire_point", "set_release_point")


def capture(command: list[str], runs: int, timeout: int) -> int:
    """Run the command under WAYLAND_DEBUG until a protocol error shows up. Keeps the failing log."""
    if not command:
        print("--capture needs a command after '--'.", file=sys.stderr)
        return 2

    environment = dict(os.environ, WAYLAND_DEBUG="1")

    for run in range(1, runs + 1):
        path = os.path.join(tempfile.gettempdir(), f"wayland-trace-{run}.log")

        with open(path, "w", encoding="utf-8", errors="replace") as log:
            process = subprocess.Popen(command, env=environment, stdout=log, stderr=subprocess.STDOUT)

            try:
                process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()

        size = os.path.getsize(path) / (1024 * 1024)

        with open(path, encoding="utf-8", errors="replace") as log:
            reproduced = any(ERROR_EVENT.search(line) for line in log)

        if reproduced:
            print(f"run {run}/{runs}: REPRODUCED -> {path} ({size:.0f} MiB)")
            print(f"\nNow analyse it:\n  python3 {sys.argv[0]} --analyse {path}")

            return 0

        os.remove(path)
        print(f"run {run}/{runs}: clean ({size:.0f} MiB discarded)")

    print(
        f"\n{runs} clean runs. That is a result, not a failure: under WAYLAND_DEBUG the client is "
        "slow enough that a tight race can stop reproducing. Try the same build WITHOUT the trace to "
        "confirm the defect is still there, and compare libdecor plugins (LIBDECOR_PLUGIN_DIR) since "
        "that changes behaviour without changing timing."
    )

    return 1


def describe_request(match: re.Match) -> str:
    """Annotates the requests that matter for the explicit-synchronisation contract."""
    name = match.group("name")
    args = match.group("args")

    if name == ATTACH:
        empty = args.startswith("nil") or args.startswith("(nil)") or args.startswith("0,")

        return "   <-- ATTACH WITHOUT A BUFFER" if empty else "   <-- attach (buffer)"

    if name == COMMIT:
        return "   <-- COMMIT"

    if name in SYNC_POINTS:
        return "   <-- sync point"

    return ""


def analyse(path: str, window: int) -> int:
    """Extract the error, the surface it belongs to, and the requests that led to it."""
    with open(path, encoding="utf-8", errors="replace") as log:
        lines = log.read().splitlines()

    error_index = None
    error = None

    for index, line in enumerate(lines):
        found = ERROR_EVENT.search(line)

        if found:
            error_index, error = index, found
            break

    if error is None:
        print(f"No Wayland protocol error in {path}. Nothing to attribute.")

        return 1

    interface, object_id = error.group("interface"), error.group("id")

    print("=" * 100)
    print(f"PROTOCOL ERROR at line {error_index + 1}")
    print(f"  object    : {interface}@{object_id}")
    print(f"  code      : {error.group('code')}")
    print(f"  message   : {error.group('message')}")

    # The offending object is rarely the surface itself: resolve the wl_surface it wraps.
    surface_id = object_id if interface == "wl_surface" else None

    if surface_id is None:
        for line in lines[:error_index]:
            found = GET_SURFACE.search(line)

            if found and found.group("object") == object_id:
                surface_id = found.group("surface")
                print(f"  wraps     : wl_surface@{surface_id}")
                break

    if surface_id is None:
        print("  wraps     : UNRESOLVED (no get_surface request found for that object)")

    print("=" * 100)

    # Requests touching that surface, and the sync-point requests on the offending object.
    targets = {f"wl_surface@{surface_id}", f"{interface}@{object_id}"} if surface_id else {f"{interface}@{object_id}"}
    history = []

    for index, line in enumerate(lines[:error_index]):
        match = REQUEST.search(line)

        if match and f"{match.group('interface')}@{match.group('id')}" in targets:
            history.append((index + 1, line.strip(), describe_request(match)))

    print(f"\nLAST {min(window, len(history))} REQUESTS ON THAT SURFACE (of {len(history)} total)\n")

    for number, text, note in history[-window:]:
        print(f"  {number:>8}  {text}{note}")

    # The contract: acquire/release points are legal only when a non-null buffer is attached in the
    # SAME commit. A "commit" here means everything staged since the PREVIOUS commit, up to and
    # including the last one -- which is the double-buffered state the compositor rejected.
    commits = [index for index, (_, text, _) in enumerate(history) if f".{COMMIT}(" in text]

    if not commits:
        print("\nNo commit request on that surface before the error: the violation is elsewhere.")

        return 0

    previous = commits[-2] if len(commits) > 1 else -1
    frame = [text for _, text, _ in history[previous + 1 : commits[-1] + 1]]

    attached = any(f".{ATTACH}(" in text and not re.search(rf"\.{ATTACH}\(\s*nil", text) for text in frame)
    pointed = any(any(f".{point}(" in text for point in SYNC_POINTS) for text in frame)

    print("\n" + "=" * 100)
    print("READING")
    print(f"  the last commit before the error carried an attach : {'yes' if attached else 'NO'}")
    print(f"  ... and acquire/release sync points                : {'yes' if pointed else 'no'}")

    if pointed and not attached:
        print("  => sync points without a buffer in the same commit: this commit is the violation.")
    elif not pointed and not attached:
        print("  => a commit carrying neither: the sync points were set on an EARLIER commit, so")
        print("     whoever issued this bare commit is the culprit. Look at the requests around it:")
        print("     GLFW batches set_opaque_region / set_buffer_scale / xdg_surface.set_window_geometry")
        print("     and libdecor's own surfaces, while the Vulkan WSI commits alone next to its attach.")
    else:
        print("  => nothing obviously illegal on the last commit; widen with --window and read the")
        print("     interleaving of the two writers by hand.")

    print("=" * 100)

    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Capture and analyse a Wayland protocol violation.")
    parser.add_argument("--capture", action="store_true", help="run a command under WAYLAND_DEBUG until it fails")
    parser.add_argument("--analyse", metavar="LOG", help="analyse an existing WAYLAND_DEBUG log")
    parser.add_argument("--runs", type=int, default=DEFAULT_RUNS, help=f"capture attempts (default {DEFAULT_RUNS})")
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT, help=f"seconds per run (default {DEFAULT_TIMEOUT})")
    parser.add_argument("--window", type=int, default=DEFAULT_WINDOW, help=f"requests shown (default {DEFAULT_WINDOW})")
    parser.add_argument("command", nargs=argparse.REMAINDER, help="after '--', the command to run")

    arguments = parser.parse_args()

    if arguments.capture:
        command = arguments.command[1:] if arguments.command[:1] == ["--"] else arguments.command

        return capture(command, arguments.runs, arguments.timeout)

    if arguments.analyse:
        return analyse(arguments.analyse, arguments.window)

    parser.print_help()

    return 2


if __name__ == "__main__":
    sys.exit(main())
