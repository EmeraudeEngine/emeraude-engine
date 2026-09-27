#!/usr/bin/env python3
"""
Emeraude Engine - MCP server conformance bench.

Drives a RUNNING engine's MCP endpoint (Core/MCP/Enabled, default http://127.0.0.1:7778/mcp) and checks
it against the Model Context Protocol, BOTH eras the server speaks: 2026-07-28 (stateless) and the
handshake era (2025-11-25: initialize). Run it after any change to src/Console/ (MCP/, the command
contract) or to a command, on every OS.

    python3 tools/mcp-conformance.py [--port 7778] [--verbose] [--trigger-list-change]

NON-DESTRUCTIVE by default: a tool is only EXECUTED when it declares readOnlyHint and needs no argument
(plus Renderer_screenshot, which only writes a capture file); every other tool is only sent calls its
validation must refuse before it runs. --trigger-list-change additionally presses F4 in projet-alpha
(unloads the active act) to check that `notifications/tools/list_changed` reaches both stream kinds.

It needs no MCP SDK: everything is plain HTTP, so a failure points at the server, not at a client
library. The official SDK clients were checked separately (docs/ai-runtime-control.md § MCP).

Exit status: 0 when every check passes, 1 otherwise.
"""

import argparse
import base64
import http.client
import json
import os
import re
import socket
import struct
import sys
import threading
import time

MODERN = "2026-07-28"
LEGACY = "2025-11-25"
TOOL_NAME = re.compile(r"^[A-Za-z0-9_-]{1,49}$")
META = {
    "io.modelcontextprotocol/protocolVersion": MODERN,
    "io.modelcontextprotocol/clientInfo": {"name": "mcp-conformance", "version": "1"},
    "io.modelcontextprotocol/clientCapabilities": {},
}


class Bench:
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


class Endpoint:
    def __init__(self, port: int) -> None:
        self.port = port
        self.host = f"127.0.0.1:{port}"

    def raw(self, method: str, body: bytes = b"", headers: dict = None, path: str = "/mcp"):
        """One request on a fresh connection: (status, headers, body text)."""
        connection = http.client.HTTPConnection("127.0.0.1", self.port, timeout=60)
        connection.request(method, path, body, headers or {})
        response = connection.getresponse()
        data = response.read().decode("utf-8", errors="replace")
        connection.close()

        return response.status, dict(response.getheaders()), data

    def modern(self, method: str, params: dict = None, headers: dict = None, request_id=1):
        params = dict(params or {})
        params["_meta"] = META
        all_headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream",
                       "MCP-Protocol-Version": MODERN, "Mcp-Method": method}

        if method == "tools/call":
            all_headers["Mcp-Name"] = params.get("name", "")

        all_headers.update(headers or {})
        status, _, data = self.raw("POST", json.dumps({"jsonrpc": "2.0", "id": request_id, "method": method, "params": params}).encode(), all_headers)

        return status, parse(data)

    def legacy(self, method: str, params: dict = None, request_id=1, version: str = LEGACY):
        headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}

        if version:
            headers["MCP-Protocol-Version"] = version

        body = {"jsonrpc": "2.0", "id": request_id, "method": method}

        if params is not None:
            body["params"] = params

        status, _, data = self.raw("POST", json.dumps(body).encode(), headers)

        return status, parse(data)

    def alive(self) -> bool:
        try:
            status, document = self.legacy("ping")
            return status == 200 and "result" in (document or {})
        except OSError:
            return False


def parse(text: str):
    try:
        return json.loads(text)
    except json.JSONDecodeError:
        return None


def error_code(document) -> int:
    return ((document or {}).get("error") or {}).get("code", 0)


def test_lifecycle(bench: Bench, endpoint: Endpoint) -> None:
    print("Lifecycle (both eras)")

    status, document = endpoint.modern("server/discover")
    result = (document or {}).get("result", {})
    bench.check(status == 200 and MODERN in result.get("supportedVersions", []), "server/discover lists 2026-07-28", repr(document)[:200])
    bench.check(LEGACY in result.get("supportedVersions", []), "server/discover lists the handshake era too", repr(result.get("supportedVersions")))
    bench.check(result.get("resultType") == "complete", "a modern result carries resultType", repr(result)[:200])
    bench.check("io.modelcontextprotocol/serverInfo" in result.get("_meta", {}), "a modern result carries serverInfo", repr(result.get("_meta")))
    bench.check((result.get("capabilities", {}).get("tools") or {}).get("listChanged") is True, "tools.listChanged advertised", repr(result.get("capabilities")))

    status, document = endpoint.legacy("initialize", {"protocolVersion": LEGACY, "capabilities": {}, "clientInfo": {"name": "t", "version": "1"}}, version=None)
    result = (document or {}).get("result", {})
    bench.check(status == 200 and result.get("protocolVersion") == LEGACY, "initialize echoes a served version", repr(document)[:200])
    bench.check("resultType" not in result, "a handshake-era result has no resultType", repr(result)[:200])

    status, document = endpoint.legacy("initialize", {"protocolVersion": "2024-01-01", "capabilities": {}, "clientInfo": {"name": "t", "version": "1"}}, version=None)
    bench.check((document or {}).get("result", {}).get("protocolVersion") == LEGACY, "initialize proposes the newest era version for an unknown one", repr(document)[:200])

    status, headers, data = endpoint.raw("POST", json.dumps({"jsonrpc": "2.0", "method": "notifications/initialized"}).encode(), {"Content-Type": "application/json"})
    bench.check(status == 202 and data == "", "a notification is answered 202 with no body", f"{status} {data[:80]}")


def test_headers(bench: Bench, endpoint: Endpoint) -> None:
    print("Modern request validation")

    status, document = endpoint.modern("tools/list", headers={"MCP-Protocol-Version": ""})
    bench.check(status == 400 and error_code(document) == -32020, "modern _meta without the version header → 400 HeaderMismatch", f"{status} {document}")

    status, document = endpoint.modern("tools/list", headers={"MCP-Protocol-Version": LEGACY})
    bench.check(status == 400 and error_code(document) == -32020, "header version ≠ _meta version → 400 HeaderMismatch", f"{status} {document}")

    status, document = endpoint.modern("tools/list", headers={"Mcp-Method": "tools/call"})
    bench.check(status == 400 and error_code(document) == -32020, "Mcp-Method ≠ method → 400 HeaderMismatch", f"{status} {document}")

    status, document = endpoint.modern("tools/call", {"name": "Window_getState", "arguments": {}}, headers={"Mcp-Name": "Other"})
    bench.check(status == 400 and error_code(document) == -32020, "Mcp-Name ≠ params.name → 400 HeaderMismatch", f"{status} {document}")

    encoded = "=?base64?" + base64.b64encode(b"Window_getState").decode() + "?="
    status, document = endpoint.modern("tools/call", {"name": "Window_getState", "arguments": {}}, headers={"Mcp-Name": encoded})
    bench.check(status == 200 and "result" in (document or {}), "a Base64-sentinel Mcp-Name is decoded", f"{status} {repr(document)[:160]}")

    status, document = endpoint.legacy("tools/list", version="1999-01-01")
    data = (document or {}).get("error", {}).get("data", {})
    bench.check(status == 400 and error_code(document) == -32022 and MODERN in data.get("supported", []), "an unknown version → 400 UnsupportedProtocolVersion listing the supported ones", f"{status} {document}")

    status, document = endpoint.modern("no/such/method")
    bench.check(status == 404 and error_code(document) == -32601, "an unknown modern method → 404 -32601", f"{status} {document}")

    status, document = endpoint.legacy("no/such/method")
    bench.check(status == 200 and error_code(document) == -32601, "an unknown handshake-era method → -32601", f"{status} {document}")


def test_http(bench: Bench, endpoint: Endpoint) -> None:
    print("HTTP and security")

    body = json.dumps({"jsonrpc": "2.0", "id": 1, "method": "ping"}).encode()
    json_headers = {"Content-Type": "application/json"}

    status, _, _ = endpoint.raw("POST", body, {**json_headers, "Origin": "http://evil.example"})
    bench.check(status == 403, "a foreign Origin is refused (DNS rebinding)", str(status))

    status, _, _ = endpoint.raw("POST", body, {**json_headers, "Origin": f"http://127.0.0.1:{endpoint.port}"})
    bench.check(status == 200, "the server's own Origin is accepted", str(status))

    status, _, _ = endpoint.raw("POST", body, {**json_headers, "Host": f"evil.example:{endpoint.port}"})
    bench.check(status == 403, "a foreign Host is refused (loopback binding)", str(status))

    status, _, _ = endpoint.raw("POST", body, json_headers, path="/other")
    bench.check(status == 404, "another path → 404", str(status))

    status, headers, _ = endpoint.raw("GET", b"", {"Accept": "application/json"})
    bench.check(status == 405, "GET without text/event-stream → 405", str(status))

    status, _, _ = endpoint.raw("DELETE")
    bench.check(status == 405, "DELETE → 405 (no sessions)", str(status))

    status, _, _ = endpoint.raw("POST", body, {"Content-Type": "text/plain"})
    bench.check(status == 415, "a non-JSON content type → 415", str(status))

    # NOTE: the server refuses on the declared length, before reading the body — so only the head is sent.
    reply = raw_exchange(endpoint, b"POST /mcp HTTP/1.1\r\nHost: " + endpoint.host.encode() + b"\r\nContent-Type: application/json\r\nContent-Length: 1048586\r\n\r\n")
    bench.check(reply.startswith(b"HTTP/1.1 413"), "a body over 1 MiB → 413, refused on the declared length", reply[:40].decode(errors="replace"))

    reply = raw_exchange(endpoint, b"POST /mcp HTTP/1.1\r\nHost: " + endpoint.host.encode() + b"\r\nContent-Type: application/json\r\nTransfer-Encoding: chunked\r\n\r\n0\r\n\r\n")
    bench.check(reply.startswith(b"HTTP/1.1 501"), "chunked encoding → 501", reply[:40].decode(errors="replace"))

    reply = raw_exchange(endpoint, b"POST /mcp HTTP/1.1\r\nHost: " + endpoint.host.encode() + b"\r\nContent-Type: application/json\r\n\r\n")
    bench.check(reply.startswith(b"HTTP/1.1 411"), "a POST without Content-Length → 411", reply[:40].decode(errors="replace"))

    reply = raw_exchange(endpoint, b"POST /mcp HTTP/1.1\r\nHost: " + endpoint.host.encode() + b"\r\nContent-Length: 2\r\nContent-Length: 20\r\n\r\n{}")
    bench.check(reply.startswith(b"HTTP/1.1 400"), "duplicated Content-Length → 400 (request smuggling)", reply[:40].decode(errors="replace"))


def raw_exchange(endpoint: Endpoint, request: bytes) -> bytes:
    sock = socket.create_connection(("127.0.0.1", endpoint.port), timeout=10)
    sock.sendall(request)
    data = b""

    try:
        while b"\r\n\r\n" not in data:
            chunk = sock.recv(4096)

            if not chunk:
                break

            data += chunk
    except socket.timeout:
        pass

    sock.close()

    return data


def test_malformed(bench: Bench, endpoint: Endpoint) -> None:
    print("Malformed JSON never kills the engine")

    cases = [
        b"not json", b"[1,2]", b"{}", b'{"jsonrpc":{}}', b'{"jsonrpc":"2.0","method":{}}', b'{"jsonrpc":"2.0","id":{},"method":"ping"}',
        b'{"jsonrpc":"2.0","id":1,"method":"tools/call","params":[1]}',
        b'{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":{},"arguments":[]}}',
        b'{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"Window_resize","arguments":{"width":[1],"height":{}}}}',
        b'{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"Window_resize","arguments":"x"}}',
        b'{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"Window_resize","arguments":{"width":1e400,"height":99999999999}}}',
        b'{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":{}}}',
        b'{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{"_meta":[1]}}',
        b'{"jsonrpc":"2.0","id":1,"method":"tools/list","params":{"_meta":{"io.modelcontextprotocol/protocolVersion":7}}}',
        b'{"jsonrpc":"2.0","id":1,"method":"subscriptions/listen","params":{"notifications":"all"}}',
        b"[" * 5000 + b"]" * 5000,
    ]

    for case in cases:
        status, _, _ = endpoint.raw("POST", case, {"Content-Type": "application/json"})
        bench.check(status in (200, 400, 404) and endpoint.alive(), f"malformed {case[:40]!r} answered {status}, engine alive", str(status))


def test_tools(bench: Bench, endpoint: Endpoint) -> list:
    print("Tools")

    status, document = endpoint.modern("tools/list")
    result = (document or {}).get("result", {})
    tools = result.get("tools", [])
    bench.check(status == 200 and len(tools) > 0, f"tools/list returns tools ({len(tools)})", repr(document)[:200])
    bench.check("ttlMs" in result and "cacheScope" in result, "a modern tools/list carries ttlMs and cacheScope", repr(list(result.keys())))

    status, legacy_document = endpoint.legacy("tools/list")
    bench.check([tool.get("name") for tool in (legacy_document or {}).get("result", {}).get("tools", [])] == [tool.get("name") for tool in tools], "both eras list the same tools, in the same order", "")

    _, again = endpoint.modern("tools/list")
    bench.check([tool.get("name") for tool in (again or {}).get("result", {}).get("tools", [])] == [tool.get("name") for tool in tools], "the order is deterministic", "")

    names = [tool.get("name", "") for tool in tools]
    bench.check(len(names) == len(set(names)), "tool names are unique", "duplicates")

    for tool in tools:
        name = tool.get("name", "?")
        schema = tool.get("inputSchema", {})
        properties = schema.get("properties", {})
        bench.check(bool(TOOL_NAME.match(name)), f"{name}: valid name (A-Za-z0-9_-, ≤ 49)", name)
        bench.check(bool(tool.get("description")), f"{name}: has a description", "")
        bench.check(schema.get("type") == "object" and schema.get("additionalProperties") is False, f"{name}: closed object schema", repr(schema)[:120])
        bench.check(set(schema.get("required", [])) <= set(properties), f"{name}: required ⊆ properties", repr(schema)[:120])
        bench.check(all(isinstance(tool.get("annotations", {}).get(key), bool) for key in ("readOnlyHint", "destructiveHint", "idempotentHint")), f"{name}: boolean annotations", repr(tool.get("annotations")))

        # Validation BEFORE execution: a wrong type at the first required non-string parameter.
        required = schema.get("required", [])
        target = next((parameter for parameter in required if properties.get(parameter, {}).get("type") in ("integer", "number", "boolean")), None)

        if target is not None:
            arguments = {parameter: "conformance" for parameter in required}
            arguments[target] = "not_a_number"
            status, document = endpoint.modern("tools/call", {"name": name, "arguments": arguments})
            outcome = (document or {}).get("result", {})
            text = " ".join(item.get("text", "") for item in outcome.get("content", []))
            bench.check(outcome.get("isError") is True and f"'{target}'" in text, f"{name}: a wrong '{target}' is refused before running", text[:160])

        status, document = endpoint.modern("tools/call", {"name": name, "arguments": {"no_such_argument": 1}})
        bench.check((document or {}).get("result", {}).get("isError") is True, f"{name}: an unknown argument is refused", repr(document)[:160])

        annotations = tool.get("annotations", {})

        if annotations.get("readOnlyHint") and not required:
            status, document = endpoint.modern("tools/call", {"name": name, "arguments": {}})
            outcome = (document or {}).get("result", {})
            bench.check(status == 200 and isinstance(outcome.get("content"), list) and outcome.get("resultType") == "complete", f"{name}: executed (read-only), well-formed result", repr(document)[:160])

    status, document = endpoint.modern("tools/call", {"name": "No_such_tool", "arguments": {}})
    bench.check(error_code(document) == -32602, "an unknown tool → -32602", repr(document)[:160])

    return names


def test_screenshot(bench: Bench, endpoint: Endpoint, names: list) -> None:
    print("Screenshot")

    if "Renderer_screenshot" not in names:
        bench.check(False, "Renderer_screenshot is exposed", "missing")
        return

    status, document = endpoint.modern("tools/call", {"name": "Renderer_screenshot", "arguments": {}})
    content = (document or {}).get("result", {}).get("content", [])
    images = [item for item in content if item.get("type") == "image"]

    if not bench.check(len(images) == 1 and images[0].get("mimeType") == "image/png", "one PNG image is returned inline", repr(content)[:200]):
        return

    png = base64.b64decode(images[0]["data"])
    bench.check(png[:8] == b"\x89PNG\r\n\x1a\n", "the image is a PNG", repr(png[:8]))
    width, height = struct.unpack(">II", png[16:24])
    bench.check(max(width, height) <= 1568, f"the image is reduced to ≤ 1568 px ({width}×{height})", f"{width}x{height}")

    text = " ".join(item.get("text", "") for item in content if item.get("type") == "text")
    match = re.search(r'"([^"]+\.png)"', text)
    bench.check(match is not None and os.path.exists(match.group(1)), "the full-resolution file path is given and exists", text[:200])


def test_streams(bench: Bench, endpoint: Endpoint, trigger: bool) -> None:
    print("Notification streams")

    received = {"modern": b"", "legacy": b""}

    def reader(kind: str, request: bytes) -> None:
        sock = socket.create_connection(("127.0.0.1", endpoint.port), timeout=12)
        sock.sendall(request)

        try:
            while True:
                chunk = sock.recv(65536)

                if not chunk:
                    break

                received[kind] += chunk
        except socket.timeout:
            pass

        sock.close()

    body = json.dumps({"jsonrpc": "2.0", "id": "sub-7", "method": "subscriptions/listen", "params": {"_meta": META, "notifications": {"toolsListChanged": True, "promptsListChanged": True}}}).encode()
    modern = (f"POST /mcp HTTP/1.1\r\nHost: {endpoint.host}\r\nContent-Type: application/json\r\nAccept: text/event-stream\r\n"
              f"MCP-Protocol-Version: {MODERN}\r\nMcp-Method: subscriptions/listen\r\nContent-Length: {len(body)}\r\n\r\n").encode() + body
    legacy = f"GET /mcp HTTP/1.1\r\nHost: {endpoint.host}\r\nAccept: text/event-stream\r\n\r\n".encode()

    threads = [threading.Thread(target=reader, args=("modern", modern)), threading.Thread(target=reader, args=("legacy", legacy))]

    for thread in threads:
        thread.start()

    time.sleep(1.5)

    if trigger:
        endpoint.modern("tools/call", {"name": "InputManager_keyPress", "arguments": {"key": 293}})

    for thread in threads:
        thread.join()

    modern_text = received["modern"].decode(errors="replace")
    legacy_text = received["legacy"].decode(errors="replace")
    bench.check("text/event-stream" in modern_text and "notifications/subscriptions/acknowledged" in modern_text, "subscriptions/listen opens an acknowledged SSE stream", modern_text[:200])

    acknowledged = next((json.loads(line[6:]) for line in modern_text.splitlines() if line.startswith("data: ") and "acknowledged" in line), {})
    filter_ = acknowledged.get("params", {}).get("notifications", {})
    bench.check(filter_ == {"toolsListChanged": True}, "the acknowledgment honours only what the server supports", repr(filter_))
    bench.check(acknowledged.get("params", {}).get("_meta", {}).get("io.modelcontextprotocol/subscriptionId") == "sub-7", "the acknowledgment carries the subscription id", repr(acknowledged)[:200])
    bench.check("text/event-stream" in legacy_text, "GET opens the handshake era's SSE stream", legacy_text[:200])

    if trigger:
        bench.check("notifications/tools/list_changed" in modern_text and '"io.modelcontextprotocol/subscriptionId":"sub-7"' in modern_text, "list_changed reaches the modern stream, tagged", modern_text[-300:])
        bench.check("notifications/tools/list_changed" in legacy_text, "list_changed reaches the handshake era's stream", legacy_text[-300:])


def test_concurrency(bench: Bench, endpoint: Endpoint) -> None:
    print("Keep-alive, pipelining and concurrency")

    connection = http.client.HTTPConnection("127.0.0.1", endpoint.port, timeout=30)
    order = []

    for index in range(10):
        body = json.dumps({"jsonrpc": "2.0", "id": index, "method": "ping"})
        connection.request("POST", "/mcp", body, {"Content-Type": "application/json"})
        response = connection.getresponse()
        order.append((parse(response.read().decode(errors="replace")) or {}).get("id"))

    connection.close()
    bench.check(order == list(range(10)), "10 requests on one keep-alive connection, answered in order", repr(order))

    pipelined = b"".join(
        (f"POST /mcp HTTP/1.1\r\nHost: {endpoint.host}\r\nContent-Type: application/json\r\nContent-Length: {len(body)}\r\n\r\n").encode() + body
        for body in (json.dumps({"jsonrpc": "2.0", "id": index, "method": "ping"}).encode() for index in range(5))
    )
    sock = socket.create_connection(("127.0.0.1", endpoint.port), timeout=10)
    sock.sendall(pipelined)
    data = b""
    deadline = time.monotonic() + 10

    while data.count(b'"jsonrpc"') < 5 and time.monotonic() < deadline:
        chunk = sock.recv(65536)

        if not chunk:
            break

        data += chunk

    sock.close()
    ids = [int(value) for value in re.findall(rb'"id":(\d+)', data)]
    bench.check(ids == list(range(5)), "5 pipelined requests answered in order", repr(ids))

    results = []

    def worker() -> None:
        try:
            status, document = endpoint.modern("tools/list")
            results.append(status == 200 and len((document or {}).get("result", {}).get("tools", [])) > 0)
        except OSError:
            results.append(False)

    threads = [threading.Thread(target=worker) for _ in range(12)]

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    bench.check(results.count(True) == 12, "12 concurrent tools/list all answered", repr(results))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--port", type=int, default=7778)
    parser.add_argument("--verbose", action="store_true")
    parser.add_argument("--trigger-list-change", action="store_true", help="press F4 (projet-alpha: unload the act) to check list_changed")
    arguments = parser.parse_args()

    bench = Bench(arguments.verbose)
    endpoint = Endpoint(arguments.port)

    if not endpoint.alive():
        print(f"No MCP server on 127.0.0.1:{arguments.port} (Core/MCP/Enabled is false by default).", file=sys.stderr)
        sys.exit(1)

    test_lifecycle(bench, endpoint)
    test_headers(bench, endpoint)
    test_http(bench, endpoint)
    test_malformed(bench, endpoint)
    names = test_tools(bench, endpoint)
    test_screenshot(bench, endpoint, names)
    test_concurrency(bench, endpoint)
    test_streams(bench, endpoint, arguments.trigger_list_change)

    print(f"\n{bench.passed} checks passed, {len(bench.failures)} failed.")

    if bench.failures:
        sys.exit(1)


if __name__ == "__main__":
    main()
