## Video Capture System

Cross-platform webcam/video capture via `VideoCaptureDevice`.

### Architecture

| File | Purpose |
|------|---------|
| `VideoCaptureDevice.hpp` | Common interface: `enumerateDevices()`, `open()`, `captureFrame()`, `close()` |
| `VideoCaptureDevice.cpp` | Shared code: `width()`, `height()`, `convertYUYVtoRGBA()` |
| `VideoCaptureDevice.linux.cpp` | V4L2 implementation (synchronous mmap) |
| `VideoCaptureDevice.mac.mm` | AVFoundation implementation (async delegate → sync copy) |
| `VideoCaptureDevice.windows.cpp` | Media Foundation implementation (synchronous IMFSourceReader) |

### Platform-Specific Details

**Linux (V4L2):**
- Direct `open()` → `ioctl(VIDIOC_*)` → `mmap` buffer → `select()` for frame readiness
- Format: YUYV (converted via shared `convertYUYVtoRGBA()`)

**macOS (AVFoundation):**
- `FrameCaptureDelegate` (ObjC class in .mm) receives frames asynchronously via `AVCaptureVideoDataOutputSampleBufferDelegate`
- Latest frame stored in `std::mutex`-protected buffer with `std::condition_variable` for first-frame synchronization
- `MacCaptureContext` struct (stored via `void* m_platformHandle`) holds `AVCaptureSession`, input, output, delegate, dispatch queue
- `open()` waits up to 3s for first frame arrival (AVFoundation's `startRunning` is async)
- Format: BGRA (converted to RGBA by swapping B↔R per pixel)
- Uses `AVCaptureDeviceDiscoverySession` for device enumeration (handles deprecation of `devicesWithMediaType:`)
- All ObjC code wrapped in `@autoreleasepool`

**Windows (Media Foundation):**
- `MFCaptureContext` struct (stored via `void* m_platformHandle`) holds `IMFSourceReader`, `IMFMediaSource`, format/COM flags
- `open()` tries RGB32 first, falls back to YUY2
- RGB32 (BGRA) → RGBA via B↔R swap; YUY2 via shared `convertYUYVtoRGBA()`
- COM lifecycle: `CoInitializeEx`/`CoUninitialize` with `RPC_E_CHANGED_MODE` handling
- MF lifecycle: `MFStartup`/`MFShutdown` tracked per context
- String conversions use `convertWideToUTF8()`/`convertUTF8ToWide()` from `Helpers.hpp` (**no local duplicates**)

### CMake Dependencies (`SetupVideoCapture.cmake`)

| Platform | Linked Libraries |
|----------|-----------------|
| macOS | `AVFoundation`, `CoreMedia`, `CoreVideo` frameworks |
| Windows | `Mfplat.lib`, `Mfreadwrite.lib`, `Mf.lib`, `Mfuuid.lib` |
| Linux | None (V4L2 uses kernel headers) |

---
