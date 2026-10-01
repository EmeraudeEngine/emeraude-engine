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

## Frame checks (triad 14, 2026-10-01)

- `convertYUYVtoRGBA()` returns `bool`: it refuses an odd width (YUYV packs pixel pairs; an odd pixel count wrote past
  the output), a row stride below the packed row, and source data shorter than the frame. It honours the stride
  (V4L2 `bytesperline`), so a padded row no longer skews the image.
- Linux refuses a device whose negotiated format is not YUYV (an MJPEG-only webcam answered `S_FMT` with another format,
  decoded as garbage). A short frame makes `captureFrame()` fail with a warning instead of returning stale data.
- Windows: the RGB32 path checks the buffer length (an over-read before), the frame size is read again on
  `MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED`, and `MFGetAttributeSize()`'s result is checked.
- macOS: the frame callback checks the lock, the base address, the BGRA non-planar format and the row length; `close()`
  detaches the delegate and drains the capture queue before freeing it.
- The platform objects live in `std::unique_ptr< PlatformContext > m_platformContext` (it was an owning `void *`).
- Measured on Linux (`/dev/video0`): YUYV 640×480 captured through projet-alpha's KeyP, 0 VUID.
