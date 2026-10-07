---
id: apiclient-logs-query-credentials
title: Net::APIClient traces the whole URL, query credentials included
status: open
priority: unranked
scope: Net/APIClient
opened: 2026-10-07
tags: [network, privacy, diagnostics]
---

# Net::APIClient traces the whole URL, query credentials included

## Why

Every trace of `APIClient.cpp` prints the full URL (`"Calling PUT '<url>'"`, `"'<url>' answered HTTP …"`, the refusal
and failure lines). A presigned URL carries its credentials in the query: app_system's crash report PUTs to an S3 URL
holding `X-Amz-Credential`, `X-Amz-Security-Token` (an STS session token) and `X-Amz-Signature`. Reported by the
Windows validation of 2026-10-07: those land in the journals — which app_system's crash reports collect and upload —
and in the user's log files. Valid for 60 s only, but a credential in a log is a credential in a log.

## What remains

1. Trace the URL without its query (or with each query value masked) in every `APIClient` trace line; keep the scheme,
   host and path, which are what a diagnosis needs.
2. Check `Net::Manager` (downloads) and the console `request`/`get`/`post` echo for the same pattern.
3. Record the rule in `docs/` (Net subsystem): a URL is traced without its query.

## References

- app_system `docs/crash-report.md` § Sending (the presigned PUT).
- `src/Net/APIClient.cpp` lines ~266–518 (every `<< url`).
