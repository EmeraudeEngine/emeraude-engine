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

The wire protocol lives in `emeraude_console.py` and is shared with every other Python tool
here — this file is the command-line front end, nothing more.
"""

import socket
import sys
from pathlib import Path

# The shared client sits next to this script; called through an absolute path or a symlink, the
# interpreter's search path does not include that directory.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from emeraude_console import DEFAULT_HOST, DEFAULT_PORT, Console  # noqa: E402


def interactive_mode(host: str, port: int) -> None:
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

            response = console.run(command)

            if response.strip():
                print(response.strip())


def main() -> None:
    host = DEFAULT_HOST
    port = DEFAULT_PORT

    # Parse optional --host and --port
    args = sys.argv[1:]
    filtered_args = []
    i = 0
    while i < len(args):
        if args[i] == "--host" and i + 1 < len(args):
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
                response = console.run(" ".join(filtered_args))

            if response.strip():
                print(response.strip())
        elif not sys.stdin.isatty():
            # Pipe mode. One held connection for the whole stream: the console keeps per-session
            # state (targetActiveScene, targetNode), so a piped sequence that targets a scene and
            # then acts on it only works if the connection is the same throughout.
            with Console(host, port) as console:
                for line in sys.stdin:
                    line = line.strip()

                    if not line:
                        continue

                    response = console.run(line)

                    if response.strip():
                        print(response.strip())
        else:
            # Interactive mode
            interactive_mode(host, port)
    except ConnectionRefusedError:
        print(f"Error: Cannot connect to {host}:{port}. Is the engine running?", file=sys.stderr)
        sys.exit(1)
    except (socket.timeout, TimeoutError):
        print(f"Error: Connection to {host}:{port} timed out.", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()