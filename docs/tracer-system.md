# Tracer System

> **FUNDAMENTAL RULE:** One trace = one complete log entry. A trace object creates a single entry with its own metadata (timestamp, thread, severity, source location). **NEVER use multiple traces to compose a single logical message.** Use `"\n"` within a single trace for multi-line content, or use a named trace variable to build the message across scopes.

The Tracer system provides logging with three distinct forms. **Choose the appropriate one based on your use case.**

The Tracer always appends a final newline. For multi-line content within a single trace, use `"\n"`.

## 1. Static Methods (Simple Messages)
Use `Tracer::level(tag, message)` for **simple messages without variables** (most performant):

```cpp
Tracer::info(ClassId, "Initialization complete.");
Tracer::warning(ClassId, "Feature not supported on this platform.");
Tracer::error(ClassId, "Failed to load configuration file.");
Tracer::fatal(ClassId, "Critical system failure.");
Tracer::debug(ClassId, "Entering function.");  // Only in DEBUG builds
```

## 2. Trace Classes — Inline (Formatted Messages)
Use `TraceLevel{tag} << ...` for **messages with variables or formatting**:

```cpp
TraceInfo{ClassId} << "Found " << count << " devices.";
TraceWarning{ClassId} << "Texture '" << name << "' not found, using fallback.";
TraceError{ClassId} << "Vulkan error: " << vkResultToCString(result);
TraceFatal{ClassId} << "Cannot allocate " << size << " bytes.";
TraceDebug{ClassId} << "Value: " << value;  // Only in DEBUG builds
```

## 3. Trace Classes — Named Variable (Multi-scope Messages)
Use a **named variable** when the message is built across multiple scopes, conditionals, or loops.
This produces **a single coherent trace entry** regardless of how many `<<` calls contribute to it:

```cpp
TraceWarning trace{ClassId};
trace <<
    "\n"
    "==============================" "\n"
    "  REPORT TITLE" "\n"
    "==============================" "\n";

if ( hasData )
{
    trace << "Data: " << data << "\n";
}

for ( const auto & item : items )
{
    trace << "  - " << item.name() << " (" << item.size() << " bytes)" "\n";
}

trace << "==============================";
// The trace is emitted as a single log entry when 'trace' goes out of scope.
```

## Critical Rules

1. **Always use braces `{}`** for Trace classes, never parentheses `()`:
   ```cpp
   TraceInfo{ClassId} << "Message";   // ✓ CORRECT
   TraceInfo(ClassId) << "Message";   // ✗ WRONG - compiles but inconsistent
   ```

2. **Use static methods for literal strings** (no variables):
   ```cpp
   Tracer::error(ClassId, "Connection failed.");  // ✓ CORRECT - more performant
   TraceError{ClassId} << "Connection failed.";   // ✗ Unnecessary overhead
   ```

3. **Use Trace classes when formatting is needed**:
   ```cpp
   TraceError{ClassId} << "Error code: " << code;  // ✓ CORRECT
   Tracer::error(ClassId, "Error code: " + std::to_string(code));  // ✗ Unnecessary string concatenation
   ```

4. **Each trace must be a coherent, self-contained message**:

   Each call to the Tracer creates an independent log entry with its own metadata (timestamp, thread ID, source location, severity). Therefore, **one trace = one complete piece of information**.

   ```cpp
   // ✗ WRONG - Multiple traces for one logical message
   TraceInfo{ClassId} << "========== Configuration ==========";
   TraceInfo{ClassId} << "Device: " << deviceName;
   TraceInfo{ClassId} << "Vendor: " << vendorName;
   TraceInfo{ClassId} << "====================================";

   // ✓ CORRECT - One trace, one-liner
   TraceInfo{ClassId} << "Device '" << deviceName << "' (Vendor: " << vendorName << ")";

   // ✓ CORRECT - One trace, multi-line via named variable
   TraceInfo trace{ClassId};
   trace << "Configuration:" "\n";
   trace << "  Device: " << deviceName << "\n";
   trace << "  Vendor: " << vendorName;
   ```

5. **Use `"\n"` for multi-line content**, never multiple trace calls:
   ```cpp
   // ✓ CORRECT - Single trace with newlines
   TraceInfo{ClassId} <<
       "Pipeline stats:" "\n"
       "  Shaders: " << shaderCount << "\n"
       "  Passes: " << passCount;

   // ✗ WRONG - Multiple traces for what should be one message
   TraceInfo{ClassId} << "Pipeline stats:";
   TraceInfo{ClassId} << "  Shaders: " << shaderCount;
   TraceInfo{ClassId} << "  Passes: " << passCount;
   ```

## Available Severity Levels
| Static Method | Trace Class | Use Case |
|---------------|-------------|----------|
| `Tracer::debug()` | `TraceDebug{}` | Development debugging (DEBUG builds only) |
| `Tracer::info()` | `TraceInfo{}` | General information |
| `Tracer::success()` | `TraceSuccess{}` | Successful operations |
| `Tracer::warning()` | `TraceWarning{}` | Non-critical issues |
| `Tracer::error()` | `TraceError{}` | Recoverable errors |
| `Tracer::fatal()` | `TraceFatal{}` | Unrecoverable errors |
| N/A | `TraceAPI{}` | External API calls (Vulkan, OpenAL, etc.) |

## Console Output Encoding (Windows)

Traces are written as **raw bytes** to `std::cout`. On Windows the console renders those
bytes through its *output code page*, which defaults to an OEM one (437, 850, 1252, …) —
never UTF-8. A perfectly valid UTF-8 message therefore displayed as mojibake: an accented
file path traced with `TraceInfo` came out as `Ã©` or `├⌐`, which reads exactly like an
encoding bug in the traced data and sends you hunting in the wrong place.

The `Tracer` constructor now calls `enableConsoleUTF8Output()` (`#if IS_WINDOWS`), which
switches the attached console to `CP_UTF8` **before the first trace is emitted**. The
previous code page is saved and restored by `restoreConsoleOutputCodePage()` in the
destructor — the console object is shared with the parent shell, so leaving it on UTF-8
after exit would change the shell's behaviour behind the user's back.

| Situation | Behaviour |
|---|---|
| Development build (CONSOLE subsystem) | The console is switched to UTF-8, then restored at exit. |
| Public release build (WINDOWS subsystem, no console) | `GetConsoleOutputCP()` returns 0 → no-op, nothing to restore. |
| Several CEF sub-processes sharing one console | The first `Tracer` switches it; the others see `CP_UTF8` and take the early return, so they never save UTF-8 as a "previous" value to restore. |
| Output redirected to a pipe or a file (IDE run window, `> log.txt`) | **The code page is irrelevant** — the bytes are UTF-8 and it is the reader's decoding that matters (IDE console encoding, text editor). |

> [!IMPORTANT]
> This fixes the **rendering** only. A trace showing a wrong character can still come from
> genuinely corrupted data upstream — typically a `std::filesystem::path` built implicitly
> from a UTF-8 `std::string` or `const char *`, which goes through the ANSI code page on
> Windows. Always build such paths with `Base::IO::u8path()` and convert back with
> `Base::IO::toU8String()`. Note that streaming a `std::filesystem::path` directly into a
> trace (`<< somePath`) uses `path::operator<<`, i.e. a lossy ANSI conversion on Windows —
> stream `IO::toU8String(somePath)` instead. The decisive test to tell the two apart: if
> the *behaviour* is also wrong (`IO::fileExists()` failing on a file that exists), the
> data is corrupted; if only the display is wrong, it is the console.

## Implementation Details

The Trace classes use the **CRTP pattern** (`T_TraceHelperBase`) to avoid code duplication while maintaining zero overhead. The message is accumulated internally via `operator<<` and flushed as a single log entry when the object is destroyed (RAII).