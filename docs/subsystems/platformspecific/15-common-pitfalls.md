## Common Pitfalls

| Pitfall | Solution |
|---------|----------|
| Platform macros in implementation files | Remove them - CMake handles file selection |
| Wrong UTF-8 conversion on Windows | Always use `convertUTF8ToWide()` |
| `defaultCStringEncoding` on macOS | Use `stringWithUTF8String:` instead |
| File filters with folder selection | Skip filters when `m_selectFolder = true` |
| More than 3 buttons with kdialog | Fall back to zenity |
| Blocking shell commands on Linux | Use `executeCommand()` which handles `popen`/`pclose` |
| Duplicating string conversions on Windows | Use `Helpers.hpp` functions (`convertUTF8ToWide`, `convertWideToUTF8`). **Never** write local `MultiByteToWideChar`/`WideCharToMultiByte` wrappers |

---
