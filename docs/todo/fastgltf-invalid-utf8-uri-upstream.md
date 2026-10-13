---
id: fastgltf-invalid-utf8-uri-upstream
title: Offer fastgltf the fix for a percent-escaped URI that decodes to invalid UTF-8
status: open
priority: unranked
scope: ext-deps fastgltf (upstream)
opened: 2026-10-13
tags: [fastgltf, upstream, windows, utf-8]
---

# Offer fastgltf the fix for a percent-escaped URI that decodes to invalid UTF-8

## Why

fastgltf 0.9.1 percent-decodes a buffer `uri` and hands the bytes to `std::filesystem::u8path()` in
`Parser::loadFileFromUri()` (and `URIView::fspath()` through the `char8_t` path constructor). MSVC's STL throws
`std::system_error` on invalid UTF-8, so a glTF with `"uri": "%FF.bin"` aborts a build without exceptions
(`0xC0000409`, measured on Windows 2026-10-12) and would reach `std::terminate()` in a build with them (a `noexcept`
function). POSIX only answers `MissingExternalBuffer`. Found during rule C step 3 (the Windows path-encoding pass,
closed 2026-10-13; record in `docs/caution-points.md` § "On MSVC, `std::filesystem::path::string()` is ANSI").

## What remains

One step, the owner's act: file the issue on the fastgltf GitHub repository (https://github.com/spnda/fastgltf, the
source ext-deps-generator builds from) with the prepared text, and attach or propose the patch as a pull request:

- `docs/upstream/fastgltf-invalid-utf8-uri/upstream-issue.md` — the issue text (version, reproduction, stack,
  proposed fix);
- `docs/upstream/fastgltf-invalid-utf8-uri/fastgltf.patch` — a strict RFC 3629 `fastgltf::detail::isValidUtf8()` used
  by `loadFileFromUri()` (`Error::InvalidURI`) and `URIView::fspath()` (an empty path), against commit `705564d4`
  (v0.9.1).

The item is done once the issue (or the pull request) is filed: record its URL in `docs/caution-points.md` (§ "On MSVC,
`std::filesystem::path::string()` is ANSI", step 3), then delete this file. Keep
`docs/upstream/fastgltf-invalid-utf8-uri/` until the engine's ext-deps carry a fastgltf with an equivalent fix.

## ⚠️ Traps

- **The engine's own pre-check stays, whatever upstream answers** (owner decision 2026-10-12, rule C): `GLTFLoader::load()`
  refuses the asset BEFORE fastgltf through `findNonUTF8URI()` (`src/Scenes/Loaders/GLTFLoader.cpp:185`, called at
  `:1994`): every `buffers[]` and `images[]` uri, percent-decoded (`Base::Network::PercentEncoding::decode()`), must pass
  `Base::String::isValidUTF8()`, else a TraceError names the uri. An upstream fix is a second layer, never a reason to
  remove it (an older fastgltf, or another path API, would bring the abort back).
- The JSON is valid UTF-8 (simdjson checks it): only a percent-escape smuggles an invalid byte in, so the reproduction
  needs `%FF`, not a raw byte.

## References

- `docs/cpp-conventions.md` § "Paths and Text Encoding — rule C" (untrusted text bound for a third-party path API).
- `docs/caution-points.md` § "On MSVC, `std::filesystem::path::string()` is ANSI" (step 3, the fastgltf abort).
