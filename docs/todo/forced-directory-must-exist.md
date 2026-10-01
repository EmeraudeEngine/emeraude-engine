---
id: forced-directory-must-exist
title: A --cache-directory (or --config-directory) that does not exist stops the engine at startup
status: open
priority: unranked
scope: FileSystem (the forced directories of the command line)
opened: 2026-10-01
tags: [filesystem, command-line, startup, robustness]
---

# A --cache-directory (or --config-directory) that does not exist stops the engine at startup

## Why

Reported by the Windows peer (2026-10-01, re-checking the non-ASCII settings paths), reproducible on every OS, with an
ASCII path or not: a `--cache-directory` that points at a directory that does not exist yet stops the start.

```
[Error][FileSystemService] Unable to reach a valid cache directory !
[Fatal][Core] Unable to initialize primary services !
```

The process exits with code 2. `FileSystem::checkCacheDirectory()` registers a forced path with
`registerDirectory(path, /* createDirectory */ false, /* writable */ true, …)`. `--config-directory` does the same. The
default locations (next to the binary, the user's directories) are created when missing. The help asks for an
existing directory, so this may be intended, but the result is a hard stop where creating the directory, or falling
back to the default one, would also be possible.

## What remains

- [ ] Owner decision, for `--cache-directory` and `--config-directory` alike:
  - keep the hard stop, but name the path and say "create it, or remove the argument";
  - create the directory (its parent must exist), as the default locations do;
  - or warn and fall back to the default directory.
  `--data-directory` stays as it is: a read-only data root that does not exist is a real error.
- [ ] Apply the decision. Test a missing directory, a missing parent, a file at that path, and a read-only directory
  on the three OS; a non-ASCII path too (the `IO::u8path()` route of 2026-10-01).

## References

- `src/FileSystem.cpp`: the `--cache-directory` / `--config-directory` branches, `registerDirectory()`,
  `checkDirectoryRequirements()`.
- projet-alpha `docs/plans/triad-engine-pass-report.md` (section 15, the Windows re-check).
