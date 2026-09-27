## External-Memory Image Import (zero-copy CEF accelerated paint)

Imports a GPU texture owned by another API/process as a `VkImage` — built for the CEF
`OnAcceleratedPaint` path (Windows/D3D11 and macOS/IOSurface implemented; the consumer side lives
in the consumer's `WebView`, plan in the consumer's own migration documentation).

**Components:**
- `ExternalImageDescriptor.hpp` — platform-neutral hand-off struct (`HandleType`:
  `Win32D3D11Texture` | `DmaBuf` | `IOSurface`; Win32 and IOSurface are implemented, DmaBuf is a
  placeholder). The embedder fills it (e.g. CEF BGRA_8888 → `VK_FORMAT_B8G8R8A8_UNORM`), the
  engine imports it.
- `Instance.cpp` (optional-extension block of `createGraphicsDevice`) — enables the device
  extension `VK_KHR_external_memory_win32` when present (Windows only, literal string —
  `vulkan_win32.h` is not included there) and `VK_EXT_metal_objects` when present (macOS only,
  MoltenVK). `VK_KHR_external_memory`(+capabilities) are core 1.1.
- `Device::externalMemoryWin32Enabled()` / `Device::metalObjectsEnabled()` — mirror
  `rayTracingEnabled()` detection.
- `PhysicalDevice::supportsExternalImageImport(format, type, tiling, usage, handleType)` — core
  1.1 `vkGetPhysicalDeviceImageFormatProperties2` query; checks `IMPORTABLE_BIT`. Callers should
  check once (not per frame) and fall back to the CPU path (`OnPaint`).
- `Image::importFromWin32Handle(device, descriptor)` (`#if IS_WINDOWS`) — mirror of
  `createFromSwapChain`: creates the `VkImage` with `VkExternalMemoryImageCreateInfo{D3D11_TEXTURE_BIT}`,
  allocates a **dedicated** import (`VkMemoryDedicatedAllocateInfo` → `VkImportMemoryWin32HandleInfoKHR`),
  binds, names, `setCreated()`. Usage is `TRANSFER_SRC` only (single GPU→GPU copy toward an
  engine-owned overlay image).
- `Image::importFromIOSurface(device, descriptor)` (`#if IS_MACOS`) — same shape, but through
  **`VK_EXT_metal_objects`**, NOT Vulkan external memory: `VkImportMetalIOSurfaceInfoEXT` chained
  to `VkImageCreateInfo` makes MoltenVK build the backing `MTLTexture` directly from the
  IOSurface at image creation. The dedicated allocation (no import struct — none exists) only
  satisfies the binding contract. Pure Vulkan, zero Objective-C. Validated end-to-end on
  Apple M2 (2026-07-08).
- `DeviceMemory` — new constructor overload taking a borrowed `pNext` chain for
  `VkMemoryAllocateInfo` (read only during `createOnHardware()`).

> [!WARNING]
> **This is the one sanctioned exception to "VMA mandatory".** VMA cannot import external
> memory — the import allocates with raw `vkAllocateMemory` + import info. The `Image` carries
> `m_isImportedImage`, which routes destruction through the manual path (never `vmaDestroyImage`)
> and blocks `createOnHardware()` re-entry (factory-built, like swap-chain images).

**Handle contract (CEF):** the D3D11 shared HANDLE is **borrowed, never closed** by the engine,
and (CEF 126) is **pool-managed — only valid during the `OnAcceleratedPaint` callback**. The
imported image and every GPU read of it must be complete (fence) before the callback returns.
CEF 126 creates the texture **without a keyed mutex**: synchronization relies on that
copy-during-callback contract, not `VK_KHR_win32_keyed_mutex`. Layout starts `UNDEFINED` — record
an acquire barrier (`VK_QUEUE_FAMILY_EXTERNAL` → graphics queue family) before the copy.
