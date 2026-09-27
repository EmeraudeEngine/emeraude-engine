## UTF-8 Encoding Practices

All internal strings use **UTF-8 encoding**. Platform-specific conversions are required at boundaries.

### Windows
Always use `convertUTF8ToWide()` before passing strings to Windows APIs:
```cpp
const std::wstring wsTitle = convertUTF8ToWide(this->title());
const std::wstring wsMessage = convertUTF8ToWide(m_message);
MessageBoxW(parentWindow, wsMessage.data(), wsTitle.data(), flags);
```

### macOS
Use `stringWithUTF8String:` for NSString conversion:
```cpp
NSString* title = [NSString stringWithUTF8String:this->title().c_str()];
NSString* message = [NSString stringWithUTF8String:m_message.c_str()];
```

**IMPORTANT**: Do NOT use `stringWithCString:encoding:defaultCStringEncoding` - it may not handle UTF-8 correctly.

### Linux
Shell commands receive UTF-8 directly (modern Linux systems are UTF-8 native). Use `escapeShellArg()` to safely quote arguments.

---
