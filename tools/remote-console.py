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
"""

import socket
import sys
import select
import time

DEFAULT_HOST = "localhost"
DEFAULT_PORT = 7777
RECV_TIMEOUT = 3.0
BUFFER_SIZE = 65536


def recv_all(sock: socket.socket, timeout: float = RECV_TIMEOUT) -> str:
    """Receive all available data from socket with timeout."""
    sock.setblocking(False)
    chunks = []
    deadline = time.monotonic() + timeout

    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            break

        ready = select.select([sock], [], [], min(remaining, 0.1))
        if ready[0]:
            try:
                data = sock.recv(BUFFER_SIZE)
                if not data:
                    break
                chunks.append(data.decode("utf-8", errors="replace"))
                # Reset deadline on new data (response may come in chunks)
                deadline = time.monotonic() + 0.5
            except BlockingIOError:
                break
            except ConnectionError:
                break
        elif chunks:
            # No more data and we already have some - done
            break

    return "".join(chunks)


def send_command(host: str, port: int, command: str) -> str:
    """Connect, send a command, return the response."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(5.0)
        sock.connect((host, port))

        # Read welcome message
        welcome = recv_all(sock, timeout=1.0)

        # Send command
        sock.sendall((command.strip() + "\n").encode("utf-8"))

        # Read response
        response = recv_all(sock, timeout=RECV_TIMEOUT)

    return response


def interactive_mode(host: str, port: int) -> None:
    """Interactive REPL mode."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(5.0)
        sock.connect((host, port))

        welcome = recv_all(sock, timeout=1.0)
        print(welcome.strip())
        print(f"Connected to {host}:{port}. Type 'quit' to exit.\n")

        while True:
            try:
                cmd = input("> ").strip()
            except (EOFError, KeyboardInterrupt):
                print()
                break

            if not cmd:
                continue
            if cmd.lower() in ("quit", "exit"):
                break

            sock.sendall((cmd + "\n").encode("utf-8"))
            response = recv_all(sock, timeout=RECV_TIMEOUT)
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
            command = " ".join(filtered_args)
            response = send_command(host, port, command)
            if response.strip():
                print(response.strip())
        elif not sys.stdin.isatty():
            # Pipe mode
            for line in sys.stdin:
                line = line.strip()
                if line:
                    response = send_command(host, port, line)
                    if response.strip():
                        print(response.strip())
        else:
            # Interactive mode
            interactive_mode(host, port)
    except ConnectionRefusedError:
        print(f"Error: Cannot connect to {host}:{port}. Is the engine running?", file=sys.stderr)
        sys.exit(1)
    except socket.timeout:
        print(f"Error: Connection to {host}:{port} timed out.", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()