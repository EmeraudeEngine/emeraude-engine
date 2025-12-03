# Tracer System

The Tracer system provides logging with two distinct APIs. **Choose the appropriate one based on your use case:**

## 1. Static Methods (Simple Messages)
Use `Tracer::level(tag, message)` for **simple messages without variables**:

```cpp
Tracer::info(ClassId, "Initialization complete.");
Tracer::warning(ClassId, "Feature not supported on this platform.");
Tracer::error(ClassId, "Failed to load configuration file.");
Tracer::fatal(ClassId, "Critical system failure.");
Tracer::debug(ClassId, "Entering function.");  // Only in DEBUG builds
```

## 2. Trace Classes (Formatted Messages)
Use `TraceLevel{tag} << ...` for **messages with variables or formatting**:

```cpp
TraceInfo{ClassId} << "Found " << count << " devices.";
TraceWarning{ClassId} << "Texture '" << name << "' not found, using fallback.";
TraceError{ClassId} << "Vulkan error: " << vkResultToCString(result);
TraceFatal{ClassId} << "Cannot allocate " << size << " bytes.";
TraceDebug{ClassId} << "Value: " << value;  // Only in DEBUG builds
```

## Critical Rules

1. **Always use braces `{}`** for Trace classes, never parentheses `()`:
   ```cpp
   TraceInfo{ClassId} << "Message";   // ✓ CORRECT
   TraceInfo(ClassId) << "Message";   // ✗ WRONG - compiles but inconsistent
   ```

2. **Use static methods for literal strings** (no variables):
   ```cpp
   Tracer::error(ClassId, "Connection failed.");  // ✓ CORRECT
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

   // ✓ CORRECT - One trace per meaningful information
   TraceInfo{ClassId} << "Device '" << deviceName << "' (Vendor: " << vendorName << ")";
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

The Trace classes use the **CRTP pattern** (`T_TraceHelperBase`) to avoid code duplication while maintaining zero overhead.
