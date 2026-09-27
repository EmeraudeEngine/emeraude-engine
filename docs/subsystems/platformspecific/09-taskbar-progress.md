## Taskbar Progress

### Windows (`ITaskbarList3`)

| `ProgressMode` | Windows Flag | Visual |
|----------------|--------------|--------|
| `None` | `TBPF_NOPROGRESS` | Hidden |
| `Normal` | `TBPF_NORMAL` | Green bar |
| `Indeterminate` | `TBPF_INDETERMINATE` | Marquee animation |
| `Error` | `TBPF_ERROR` | Red bar |
| `Paused` | `TBPF_PAUSED` | Yellow bar |

### macOS (`NSDockTile`)

- Uses `NSProgressIndicator` overlay on dock icon
- Pass negative value to remove progress indicator
- Mode parameter is ignored (only normal progress supported)

### Linux

**Not supported** on standard Linux desktops. Would require `libunity` for Ubuntu Unity only.

---
