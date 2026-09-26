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
        response = console.call('Core.SceneManagerService.setNodePosition("ViewerCamera", 0, 2, 8)')
        if not response.ok:
            print(response.text)

The module name is underscored on purpose: `remote-console.py` carries a hyphen and cannot be
imported, so the shared code cannot live there.

Wire format (engine `src/Console/RemoteProtocol.hpp`, since 2026-09-27): a request is ONE command
line; every request gets exactly ONE response, which is ONE JSON object on ONE line:

    {"ok": true, "outputs": [{"severity": "Success", "kind": "text", "message": "..."}]}

`kind` is "text", "json" (the message is a JSON document) or "binary" (with "mimeType" and a
Base64 "data"). The welcome banner sent on connection is a response too, with a top-level
"protocol" version. Reads therefore end on the newline, never on a quiet period: the timeout is only
the longest a command may take, and a fast answer returns as soon as it arrives.
"""

import json
import socket
import time

DEFAULT_HOST = "localhost"
DEFAULT_PORT = 7777

#: Longest a command may take before the read gives up, in seconds. Generous on purpose: an answer
#: returns as soon as its line arrives, so a long timeout costs nothing to a fast command.
DEFAULT_TIMEOUT = 30.0

#: Timeout of the initial connection and of the welcome banner, in seconds.
CONNECT_TIMEOUT = 5.0

#: The protocol version this client speaks (the banner's "protocol" field).
PROTOCOL_VERSION = 1

BUFFER_SIZE = 65536


class ProtocolError(RuntimeError):
    """The server sent something that is not a response line of the expected protocol."""


class Response:
    """One response of the remote console: whether the command succeeded, and its outputs."""

    def __init__(self, document: dict) -> None:
        self.document = document
        self.ok = bool(document.get("ok", False))
        self.outputs = list(document.get("outputs", []))

    @property
    def text(self) -> str:
        """Every output message, one per line — what the console printed before the JSON framing."""
        return "\n".join(output.get("message", "") for output in self.outputs)

    def json(self):
        """
        Returns the first JSON output, parsed.

        :raises ProtocolError: No output of kind "json".
        """
        for output in self.outputs:
            if output.get("kind") == "json":
                return json.loads(output["message"])

        raise ProtocolError("the response carries no JSON output: " + self.text)

    def __repr__(self) -> str:
        return f"Response(ok={self.ok}, outputs={self.outputs!r})"


class Console:
    """
    A held connection to the remote console.

    Use this over :func:`send_command` as soon as a tool issues more than a handful of commands:
    the console keeps per-session state on the engine side (`targetActiveScene`, `targetNode`),
    and reconnecting for every command pays the TCP and welcome-banner cost each time.
    """

    def __init__(self, host: str = DEFAULT_HOST, port: int = DEFAULT_PORT, timeout: float = DEFAULT_TIMEOUT) -> None:
        """
        Connects and reads the welcome banner.

        :param host: Host running the engine.
        :param port: Remote console port.
        :param timeout: Default longest wait of :meth:`call` / :meth:`run`, in seconds.
        :raises ConnectionRefusedError: The engine is not listening — either it is not running, or the
            remote console is disabled (it is CLOSED BY DEFAULT: setting
            ``Core/Console/EnableRemoteListener`` must be true, and the bind address
            ``Core/Console/RemoteListenerAddress`` defaults to 127.0.0.1). Retrying does not help.
        :raises ProtocolError: The server does not speak this protocol (an engine built before the
            JSON framing answers raw text).
        """
        self.sock = socket.create_connection((host, port), timeout=CONNECT_TIMEOUT)
        self.timeout = timeout
        self._buffer = b""

        banner = self._read_response(CONNECT_TIMEOUT)

        #: The banner response; ``welcome`` is its text, for display.
        self.banner = banner
        self.welcome = banner.text
        self.protocol = banner.document.get("protocol")

        if self.protocol != PROTOCOL_VERSION:
            raise ProtocolError(f"the server speaks protocol {self.protocol!r}, this client speaks {PROTOCOL_VERSION}")

    def call(self, command: str, timeout: float = None) -> Response:
        """
        Sends one command and returns its response.

        :param command: The command line, without its trailing newline.
        :param timeout: Longest wait for this command only. Defaults to the session timeout; raise
            it for a command known to be slow (a synchronous asset import, a long temporal capture).
        :return: The response. A failed command is a Response with ``ok`` false, not an exception.
        :raises TimeoutError: No response line within the timeout. The connection is then out of
            step (a late answer would be read as the next one): close it.
        """
        line = command.strip()

        if "\n" in line or "\r" in line:
            raise ValueError("a command is one line")

        self.sock.sendall((line + "\n").encode("utf-8"))

        return self._read_response(self.timeout if timeout is None else timeout)

    def run(self, command: str, timeout: float = None) -> str:
        """
        Sends one command and returns the text of its response (every output message, one per line).

        :param command: The command line, without its trailing newline.
        :param timeout: Longest wait for this command only, in seconds.
        :return: The response text, possibly empty. Use :meth:`call` to know whether it succeeded.
        """
        return self.call(command, timeout).text

    def _read_response(self, timeout: float) -> Response:
        """Reads exactly one response line."""
        deadline = time.monotonic() + timeout

        while b"\n" not in self._buffer:
            remaining = deadline - time.monotonic()

            if remaining <= 0:
                raise TimeoutError(f"no response within {timeout:.1f} s")

            self.sock.settimeout(remaining)

            try:
                data = self.sock.recv(BUFFER_SIZE)
            except socket.timeout as exception:
                raise TimeoutError(f"no response within {timeout:.1f} s") from exception

            if not data:
                raise ConnectionError("the server closed the connection")

            self._buffer += data

        line, self._buffer = self._buffer.split(b"\n", 1)

        try:
            document = json.loads(line.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError) as exception:
            raise ProtocolError("not a response line: " + line[:200].decode("utf-8", errors="replace")) from exception

        if not isinstance(document, dict) or "ok" not in document:
            raise ProtocolError("not a response object: " + line[:200].decode("utf-8", errors="replace"))

        return Response(document)

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
    :return: The response text, possibly empty.
    """
    with Console(host, port) as console:
        return console.run(command)
