# A percent-escaped URI that decodes to invalid UTF-8 throws from `u8path` on MSVC

**fastgltf version:** v0.9.1 (705564d4d54500ae88befb0db9e361e447923655)
**Platform:** Windows, MSVC 2022 (19.44), C++20

## Summary

With `Options::LoadExternalBuffers`, a glTF whose buffer `uri` percent-decodes to bytes that are not UTF-8 (for
example `"uri": "%FF.bin"`) makes `Parser::loadFileFromUri()` call `std::filesystem::u8path()` on those bytes. MSVC's
STL validates the UTF-8 and throws `std::system_error` (`_Convert_narrow_to_wide` → `_Throw_system_error_from_std_win_error`).

- In a build without exceptions (`/EHs-c-`, `_HAS_EXCEPTIONS=0`), the process fails fast (`0xC0000409`). This was measured.
- In a build with exceptions, the exception escapes a `noexcept` function, so `std::terminate()` runs. This follows from
  the language rule and was not measured.

`URIView::fspath()` has the same issue through its `char8_t` path constructor.

The JSON itself is valid UTF-8 (simdjson checks it); only a percent-escape can smuggle an invalid byte in. On POSIX,
`u8path` does not validate, so the same file only gives `MissingExternalBuffer`. On Windows, any untrusted glTF can
therefore terminate the host application.

## Reproduction

```json
{
  "asset": { "version": "2.0" },
  "buffers": [ { "byteLength": 4, "uri": "%FF.bin" } ]
}
```

```cpp
fastgltf::Parser parser;
auto data = fastgltf::GltfDataBuffer::FromPath("ff-uri.gltf");
auto asset = parser.loadGltf(data.get(), ".", fastgltf::Options::LoadExternalBuffers); // terminates on MSVC
```

Stack (MSVC, Debug):

```
std::filesystem::_Convert_narrow_to_wide            [filesystem:47]
std::filesystem::u8path<std::string_view>           [filesystem:1413]
fastgltf::Parser::loadFileFromUri                   [io.cpp:420]
fastgltf::Parser::parseBuffers                      [fastgltf.cpp:2039]
fastgltf::Parser::parse                             [fastgltf.cpp:1586]
fastgltf::Parser::loadGltfJson                      [fastgltf.cpp:4963]
fastgltf::Parser::loadGltf                          [fastgltf.cpp:4930]
```

## Proposed fix

Validate the decoded path as strict UTF-8 (RFC 3629) before converting it:

- `loadFileFromUri()` returns `Error::InvalidURI` on invalid UTF-8;
- `URIView::fspath()` returns an empty path.

The attached patch adds a small `fastgltf::detail::isValidUtf8()` (no overlong forms, no encoded surrogates, nothing
past U+10FFFF) and uses it at both call sites. It does not change behaviour for valid input. It lets every platform
refuse the file the same way, instead of terminating on Windows only.

An alternative would be to keep `u8path` but catch the exception, which does not work in builds without exceptions.
