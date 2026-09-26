#!/usr/bin/env python3
"""
Emeraude Engine - Remote Console Client
Cross-platform TCP client for the AI Remote Console (port 7777).

Usage:
    # Interactive mode
    python remote-console.py

    # Send a single command and print the response
    python remote-console.py "Core.RendererService.screenshot()"

    # Pipe commands
    echo "Core.SceneManagerService.getSceneInfo()" | python remote-console.py

    # Print the raw JSON response lines instead of their text
    python remote-console.py --json "Core.WindowService.getState()"

It prints the text of each response (the output messages, one per line). The exit status is 1 when
a command failed (the response's "ok" is false), in single-command and pipe modes.

The wire protocol lives in `emeraude_console.py` and is shared with every other Python tool
here — this file is the command-line front end, nothing more.
"""


import socket
import sys
from pathlib import Path

# The shared client sits next to this script; called through an absolute path or a symlink, the
# interpreter's search path does not include that directory.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from emeraude_console import DEFAULT_HOST, DEFAULT_PORT, Console, ProtocolError, Response  # noqa: E402


def show(response: Response, raw: bool) -> None:
    """Prints a response: its text, or the line exactly as received when raw."""
    if raw:
        print(response.raw)
    elif response.text.strip():
        print(response.text.strip())


def interactive_mode(host: str, port: int, raw: bool) -> None:
    """Interactive REPL mode."""
    with Console(host, port) as console:
        print(console.welcome.strip())
        print(f"Connected to {host}:{port}. Type 'quit' to exit.\n")

        while True:
            try:
                command = input("> ").strip()
            except (EOFError, KeyboardInterrupt):
                print()
                break

            if not command:
                continue

            if command.lower() in ("quit", "exit"):
                break

            show(console.call(command), raw)


def main() -> None:
    host = DEFAULT_HOST
    port = DEFAULT_PORT

    # Parse optional --host and --port
    args = sys.argv[1:]
    filtered_args = []
    raw = False
    succeeded = True
    i = 0
    while i < len(args):
        if args[i] == "--json":
            raw = True
            i += 1
        elif args[i] == "--host" and i + 1 < len(args):
            host = args[i + 1]
            i += 2
        elif args[i] == "--port" and i + 1 < len(args):
            port = int(args[i + 1])
            i += 2
        else:
            filtered_args.append(args[i])
            i += 1

    try:
        if filtered_args:
            # Single command mode
            with Console(host, port) as console:
                response = console.call(" ".join(filtered_args))

            show(response, raw)
            succeeded = response.ok
        elif not sys.stdin.isatty():
            # Pipe mode. One held connection for the whole stream: the console keeps per-session
            # state (targetActiveScene, targetNode), so a piped sequence that targets a scene and
            # then acts on it only works if the connection is the same throughout.
            with Console(host, port) as console:
                for line in sys.stdin:
                    line = line.strip()

                    if not line:
                        continue

                    response = console.call(line)

                    show(response, raw)
                    succeeded = succeeded and response.ok
        else:
            # Interactive mode
            interactive_mode(host, port, raw)
    except ConnectionRefusedError:
        print(f"Error: Cannot connect to {host}:{port}. Is the engine running?", file=sys.stderr)
        print("Note: the remote console is CLOSED BY DEFAULT. It starts only when the setting "
              "Core/Console/EnableRemoteListener is true (bind address: Core/Console/RemoteListenerAddress, "
              "default 127.0.0.1). Enable it in the application's settings.json and relaunch; do not retry "
              "against a running instance that never opened the port.", file=sys.stderr)
        sys.exit(1)
    except (socket.timeout, TimeoutError) as exception:
        print(f"Error: {host}:{port} timed out ({exception}).", file=sys.stderr)
        sys.exit(1)
    except ProtocolError as exception:
        print(f"Error: {host}:{port} does not speak this protocol ({exception}). An engine built before "
              "2026-09-27 answers raw text: rebuild it, or use an older copy of this tool.", file=sys.stderr)
        sys.exit(1)

    if not succeeded:
        sys.exit(1)


if __name__ == "__main__":
    main()
