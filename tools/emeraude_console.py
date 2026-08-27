#!/usr/bin/env python3
"""
Emeraude Engine - Remote Console client library.

The single implementation of the AI Remote Console wire protocol (TCP, port 7777) for every
Python tool in this directory. Import it, do not re-implement it:

    from emeraude_console import Console, send_command

    # One-shot, opens and closes a connection.
    print(send_command("localhost", 7777, "Core.RendererService.screenshot()"))

    # Held session, for tools that issue many commands (a bench, a capture sequence).
    with Console() as console:
        console.run("Core.SceneManagerService.targetActiveScene()")
        console.run('Core.SceneManagerService.setNodePosition("ViewerCamera", 0, 2, 8)')

The module name is underscored on purpose: `remote-console.py` carries a hyphen and cannot be
imported, so the shared code cannot live there.

The protocol has no framing and no end-of-response marker — the server simply writes text and
stops. Reads therefore end on a quiet period, never on a delimiter, which is why every read
takes a timeout and why the deadline is EXTENDED whenever new bytes arrive: a long answer
(`help`, a scene dump) reaches us in several TCP segments, and a fixed deadline would truncate
it mid-stream.
"""

import select
import socket
import time

DEFAULT_HOST = "localhost"
DEFAULT_PORT = 7777

#: Quiet period after which a response is considered complete, in seconds.
RECV_TIMEOUT = 3.0

#: Extension granted on every chunk received, in seconds. Long dumps arrive segmented.
CHUNK_GRACE = 0.5

#: Timeout of the initial connection, in seconds.
CONNECT_TIMEOUT = 5.0

BUFFER_SIZE = 65536


def recv_all(sock: socket.socket, timeout: float = RECV_TIMEOUT, grace: float = CHUNK_GRACE) -> str:
    """
    Reads everything the server sends until it falls quiet.

    :param sock: A connected socket. Left in non-blocking mode on return.
    :param timeout: Quiet period ending the read, in seconds.
    :param grace: Deadline extension granted on each chunk, in seconds.
    :return: The decoded response, possibly empty.
    """
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
            except (BlockingIOError, ConnectionError):
                break

            if not data:
                break

            chunks.append(data.decode("utf-8", errors="replace"))

            # A long answer arrives segmented: every chunk buys more time.
            deadline = time.monotonic() + grace
        elif chunks:
            # Quiet, and something was already received: the answer is complete.
            break

    return "".join(chunks)


class Console:
    """
    A held connection to the remote console.

    Use this over :func:`send_command` as soon as a tool issues more than a handful of commands:
    the console keeps per-session state on the engine side (`targetActiveScene`, `targetNode`),
    and reconnecting for every command pays the TCP and welcome-banner cost each time.
    """

    def __init__(self, host: str = DEFAULT_HOST, port: int = DEFAULT_PORT, timeout: float = RECV_TIMEOUT) -> None:
        """
        Connects and swallows the welcome banner.

        :param host: Host running the engine.
        :param port: Remote console port.
        :param timeout: Default quiet period for :meth:`run`, in seconds.
        :raises ConnectionRefusedError: The engine is not listening — either it is not running, or the
            remote console is disabled (it is CLOSED BY DEFAULT: setting
            ``Core/Console/EnableRemoteListener`` must be true, and the bind address
            ``Core/Console/RemoteListenerAddress`` defaults to 127.0.0.1). Retrying does not help.
        """
        self.sock = socket.create_connection((host, port), timeout=CONNECT_TIMEOUT)
        self.timeout = timeout
        self.welcome = recv_all(self.sock, timeout=1.0)

    def run(self, command: str, timeout: float = None) -> str:
        """
        Sends one command and returns the response.

        :param command: The command line, without its trailing newline.
        :param timeout: Quiet period for this command only. Defaults to the session timeout;
            raise it for a command known to be slow (a synchronous asset import, a screenshot).
        :return: The decoded response, possibly empty.
        """
        self.sock.setblocking(True)
        self.sock.sendall((command.strip() + "\n").encode("utf-8"))

        return recv_all(self.sock, timeout=self.timeout if timeout is None else timeout)

    def close(self) -> None:
        """Closes the connection. Safe to call more than once."""
        try:
            self.sock.close()
        except OSError:
            pass

    def __enter__(self) -> "Console":
        return self

    def __exit__(self, *_exception) -> None:
        self.close()


def send_command(host: str = DEFAULT_HOST, port: int = DEFAULT_PORT, command: str = "") -> str:
    """
    Sends a single command over a connection opened and closed for it.

    :param host: Host running the engine.
    :param port: Remote console port.
    :param command: The command line.
    :return: The decoded response, possibly empty.
    """
    with Console(host, port) as console:
        return console.run(command)