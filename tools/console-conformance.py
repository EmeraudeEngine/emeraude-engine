#!/usr/bin/env python3
"""
Emeraude Engine - Remote Console conformance bench.

Drives a RUNNING engine over the remote console and checks the whole contract end to end: the wire
protocol (one JSON line per request, in order, even for a failure; the banner; the transport
limits) and the typed command contract (every command declared, arguments validated BEFORE the
command runs). It is what makes "the console answers correctly" a measured claim rather than a
belief — run it after any change to src/Console/ or to a command, on every OS.

    python3 tools/console-conformance.py [--host H] [--port P] [--verbose]

It is NON-DESTRUCTIVE by construction: a command is only EXECUTED when it declares the
`readOnly` hint and needs no argument; every other command is only sent calls its validation
must refuse before anything runs (a wrong type, one argument too many). The transport tests end
with a disconnection of their OWN connection only.

Exit status: 0 when every check passes, 1 otherwise.
"""

import argparse
import json
import socket
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from emeraude_console import DEFAULT_HOST, DEFAULT_PORT, PROTOCOL_VERSION, Console, ProtocolError, Response  # noqa: E402

SEVERITIES = {"Debug", "Success", "Info", "Warning", "Error", "Fatal"}
KINDS = {"text", "json", "binary"}
TYPES = {"boolean", "integer", "float", "string", "any"}
ARITIES = {"required", "optional", "variadic"}
HINTS = {"readOnly", "destructive", "idempotent"}

#: Mirrors RemoteListener::MaxLineLength / MaxPendingCommandsPerClient.
MAX_LINE_LENGTH = 8192
MAX_PENDING_PER_CLIENT = 32


class Bench:
    """Collects check results."""

    def __init__(self, verbose: bool) -> None:
        self.verbose = verbose
        self.passed = 0
        self.failures = []

    def check(self, condition: bool, name: str, detail: str = "") -> bool:
        if condition:
            self.passed += 1

            if self.verbose:
                print(f"  PASS  {name}")
        else:
            self.failures.append(f"{name}: {detail}")
            print(f"  FAIL  {name}: {detail}")

        return condition


def validate_response(bench: Bench, response: Response, context: str) -> None:
    """Checks the shape of one response document."""
    document = response.document
    bench.check(isinstance(document.get("ok"), bool), f"{context}: 'ok' is a boolean", repr(document)[:200])
    bench.check(isinstance(document.get("outputs"), list), f"{context}: 'outputs' is a list", repr(document)[:200])

    for index, output in enumerate(response.outputs):
        where = f"{context}: output #{index}"
        bench.check(output.get("severity") in SEVERITIES, f"{where} severity", repr(output.get("severity")))
        bench.check(output.get("kind") in KINDS, f"{where} kind", repr(output.get("kind")))
        bench.check(isinstance(output.get("message"), str), f"{where} message is a string", repr(output)[:200])

        if output.get("kind") == "json":
            try:
                json.loads(output["message"])
                bench.check(True, f"{where} JSON parses")
            except (json.JSONDecodeError, KeyError) as exception:
                bench.check(False, f"{where} JSON parses", f"{exception}: {output.get('message', '')[:200]}")

        if output.get("kind") == "binary":
            bench.check(bool(output.get("mimeType")) and isinstance(output.get("data"), str), f"{where} binary has mimeType and data", repr(output)[:200])


def invalid_call(command: dict):
    """
    Builds a call the command's validation must refuse before it runs, or None.

    First choice: a non-numeric token at the first required boolean/integer/float parameter (the
    string parameters before it get a placeholder; conversion stops at the first failure, in order).
    Fallback: one argument more than the command accepts (checked before any conversion).
    """
    parameters = command.get("parameters", [])

    for index, parameter in enumerate(parameters):
        if parameter["arity"] != "required":
            break

        if parameter["type"] in ("boolean", "integer", "float"):
            arguments = ["conformance"] * index + ["not_a_number"]

            return f"{command['path']}({', '.join(arguments)})", parameter["name"]

    if parameters and parameters[-1]["arity"] == "variadic":
        return None

    arguments = ["0"] * (len(parameters) + 1)

    return f"{command['path']}({', '.join(arguments)})", "Too many arguments"


def test_commands(bench: Bench, console: Console) -> None:
    print("Command contract")

    response = console.call("describeCommands()")
    validate_response(bench, response, "describeCommands()")

    if not bench.check(response.ok, "describeCommands() succeeds", response.text[:200]):
        return

    commands = response.json()
    bench.check(len(commands) > 0, "describeCommands() lists commands", "empty list")

    for command in commands:
        path = command.get("path", "?")
        bench.check(command.get("typed") is True, f"{path} is typed", "bound without a declared signature")
        bench.check(bool(command.get("help")), f"{path} has a description", "empty help")

        parameters = command.get("parameters", [])
        seen_omittable = False

        for index, parameter in enumerate(parameters):
            where = f"{path} parameter '{parameter.get('name')}'"
            bench.check(parameter.get("type") in TYPES, f"{where} type", repr(parameter.get("type")))
            bench.check(parameter.get("arity") in ARITIES, f"{where} arity", repr(parameter.get("arity")))
            bench.check(bool(parameter.get("description")), f"{where} has a description", "empty")

            if parameter.get("arity") == "variadic":
                bench.check(index == len(parameters) - 1, f"{where} variadic is last", "not last")

            if parameter.get("arity") == "required":
                bench.check(not seen_omittable, f"{where} required after optional", "a required parameter follows an optional one")
            else:
                seen_omittable = True

        bench.check(set(command.get("hints", [])) <= HINTS, f"{path} hints", repr(command.get("hints")))

        # Validation BEFORE execution: never runs the command.
        call = invalid_call(command)

        if call is not None:
            line, expected = call
            refused = console.call(line)
            validate_response(bench, refused, line)
            bench.check(not refused.ok and expected in refused.text, f"{line} is refused naming '{expected}'", refused.text[:200])

        # Execution: only for read-only commands that need no argument.
        needs_argument = any(parameter["arity"] == "required" for parameter in parameters)

        if "readOnly" in command.get("hints", []) and not needs_argument:
            executed = console.call(f"{path}()")
            validate_response(bench, executed, f"{path}()")


def test_protocol(bench: Bench, host: str, port: int) -> None:
    print("Wire protocol")

    with Console(host, port) as console:
        bench.check(console.protocol == PROTOCOL_VERSION, "banner announces the protocol version", repr(console.protocol))

        unknown = console.call("Core.NoSuchService.noSuchCommand()")
        validate_response(bench, unknown, "unknown object")
        bench.check(not unknown.ok, "an unknown object answers ok=false", unknown.text[:200])

        invalid = console.call("not a command at all")
        validate_response(bench, invalid, "garbage line")
        bench.check(not invalid.ok, "a garbage line answers ok=false", invalid.text[:200])

        quoted = console.call('Core.WindowService.resize("12,34", 600)')
        bench.check(not quoted.ok and "12,34" in quoted.text, "a quoted comma stays inside ONE argument", quoted.text[:200])

        # Pipelining: many requests in one write, answered one line each, IN ORDER.
        good, bad = "Core.remoteConsoleStatus()", "Core.WindowService.resize(abc, 600)"
        batch = [good if index % 2 == 0 else bad for index in range(20)]
        console.sock.sendall(("\n".join(batch) + "\n").encode("utf-8"))
        answers = [console._read_response(10.0) for _ in batch]
        expected = [index % 2 == 0 for index in range(20)]
        bench.check([answer.ok for answer in answers] == expected, "20 pipelined requests answered in order", repr([answer.ok for answer in answers]))

    # A line longer than the limit: one last error line, then the connection is closed.
    with Console(host, port) as console:
        console.sock.sendall(b"x" * (MAX_LINE_LENGTH + 100) + b"\n")
        try:
            last = console._read_response(10.0)
            bench.check(not last.ok and "Line too long" in last.text, "an over-long line answers a last error line", last.text[:200])
            closed = False

            try:
                console._read_response(5.0)
            except ConnectionError:
                closed = True
            except (TimeoutError, ProtocolError):
                closed = False

            bench.check(closed, "an over-long line closes the connection", "the connection stayed open")
        except (ConnectionError, TimeoutError) as exception:
            bench.check(False, "an over-long line answers a last error line", str(exception))

    # A flood: more pending requests than a client's share disconnects THAT client only.
    with Console(host, port) as console:
        console.sock.sendall(("Core.remoteConsoleStatus()\n" * 2000).encode("utf-8"))
        disconnected = False
        refusal_seen = False
        answers = 0
        deadline = time.monotonic() + 30.0

        while time.monotonic() < deadline:
            try:
                response = console._read_response(10.0)
            except ConnectionError:
                disconnected = True
                break
            except TimeoutError:
                break

            if not response.ok and "Too many pending commands" in response.text:
                refusal_seen = True
            else:
                answers += 1

        bench.check(refusal_seen and disconnected, f"a flood gets a last refusal then a disconnection (after {answers} answers)", f"refusal={refusal_seen} disconnected={disconnected}")

    with Console(host, port) as console:
        after = console.call("Core.remoteConsoleStatus()")
        bench.check(after.ok, "the console still serves a new client after the flood", after.text[:200])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--verbose", action="store_true", help="print passing checks too")
    arguments = parser.parse_args()

    bench = Bench(arguments.verbose)

    try:
        with Console(arguments.host, arguments.port) as console:
            test_commands(bench, console)

        test_protocol(bench, arguments.host, arguments.port)
    except ConnectionRefusedError:
        print(f"Cannot connect to {arguments.host}:{arguments.port}: the remote console is closed by default "
              "(Core/Console/EnableRemoteListener).", file=sys.stderr)
        sys.exit(1)
    except ProtocolError as exception:
        print(f"Protocol error: {exception}", file=sys.stderr)
        sys.exit(1)

    print(f"\n{bench.passed} checks passed, {len(bench.failures)} failed.")

    if bench.failures:
        sys.exit(1)


if __name__ == "__main__":
    main()
