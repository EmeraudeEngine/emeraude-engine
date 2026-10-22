---
id: tracer-truncates-a-journal-another-process-writes
title: The tracer truncates a journal that another process is still writing when it cannot rotate it
status: open
priority: high
scope: Tracer
opened: 2026-10-22
tags: [tracer, logging, crash-report, windows, defect]
---

# The tracer truncates a journal that another process is still writing when it cannot rotate it

## Why

`Tracer::enableLogger()` (`src/Tracer.cpp:528`) renames the existing journal to `<name>.previous.<ext>` before
opening a new one. When the rename fails it only warns ("Unable to keep the previous log file as '…' (…), it will be
overwritten.") and `TracerLogger` opens the same path, truncating it. On Windows the rename fails precisely when
another process of the same application is still writing that journal (a sharing violation): the second process
destroys the head of the running instance's journal, whose later lines are then appended after the second process's
own.

Measured 2026-10-22 on Windows (a downstream application, two instances that did not share a single-instance lock —
an elevated instance and a normal one, or two versions): `journal-main.log` of the running instance went from 54 KB
to 27 KB, its head lost. The downstream crash reports carry these journals: of 115 reports of one cause, 80 held "the
journal of another run" — this truncation is a likely source. Linux/macOS rename an open file without error, so the
running instance keeps writing into the renamed `.previous` file and a crash report of the new run reads the wrong one
there too, more quietly.

## What remains

- When the journal cannot be rotated (or is held by another live process), write this run's journal to a distinct
  name (e.g. with the process id) instead of truncating the other one; never open a journal in truncate mode over a
  file the rename could not move. Decide how a reader (the downstream crash reporter, which zips `journal-*.log`)
  finds it.
- Check the same on Linux/macOS: detect a journal another live process holds (an advisory lock on the file?) rather
  than relying on the rename failing.

## ⚠️ Traps

- The downstream crash reporter collects the journals by name (app_system `docs/crash-report.md` § Traps): a new
  naming must be agreed with it in the same delivery.

## References

- app_system `docs/todo/crash-exit-code-1-after-cefinitializationfailed-0b1244.md` (the measurement, the reports).
- app_system `docs/plans/crashreport-solucius.md` (wave W0: the instrument).
