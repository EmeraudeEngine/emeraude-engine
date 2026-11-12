---
id: listening-sockets-inherited-by-child-processes
title: The engine's listening sockets are inherited by the processes it launches
status: open
priority: high
scope: Console / Net (RemoteListener, TCPServer, the MCP server) and PlatformSpecific (process launch)
opened: 2026-11-11
tags: [network, process, linux, posix, robustness]
---

# The engine's listening sockets are inherited by the processes it launches

## Why

Measured 2026-11-11 (Linux main, projet-alpha Debug, `--enable-remote-console=7780 --enable-mcp=17779`): the engine
showed its native dialog (`zenity`, through `PlatformSpecific::executeCommandPumpingEvents()` → `popen()`), the
engine process was then killed, and `ss -ltnp` showed the console port 7780 and the MCP port 17779 still LISTENING —
held by `zenity` and its `sh` (fds 7 and 11). The next launch could not bind them ("Address already in use") until
the dialog was closed.

A listening socket created without `SOCK_CLOEXEC` / `FD_CLOEXEC` survives `exec()`: every child process the engine
starts (`popen()` for the dialogs and the Wi-Fi scan, `posix_spawnp()` for desktop commands) inherits the console,
MCP and TCP server sockets, and keeps the ports while it lives — after the engine exited or crashed. A child could
also accept a connection on them.

## What remains

- Confirm which sockets (`Console/RemoteListener.cpp`, `Net/TCPServer.cpp`, the MCP server's HTTP server) and which
  launch paths (`Helpers.linux.cpp` `popen()`, `Process.posix.cpp` `posix_spawnp()`, `Net/WiFiScanner.linux.cpp`)
  are affected; macOS the same (POSIX); Windows: handle inheritance (`bInheritHandles`, `HANDLE_FLAG_INHERIT`).
- Decide the fix (owner): mark every engine socket close-on-exec at creation, and/or make the launch paths close
  what is not theirs (`posix_spawn_file_actions_addclosefrom_np`, `close_range()`), keeping only the stdio pipes.
- Proof: a launched child holds none of the engine's descriptors (`/proc/<pid>/fd`), the port is free once the
  engine exits while the child lives.

## References

- `docs/subsystems/vulkan/26-swap-chain-failure-paths.md` (the run that showed it).
