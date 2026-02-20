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

## Implementation Details

The Trace classes use the **CRTP pattern** (`T_TraceHelperBase`) to avoid code duplication while maintaining zero overhead. The message is accumulated internally via `operator<<` and flushed as a single log entry when the object is destroyed (RAII).