## macOS: the Vulkan loader is pinned to the bundle's driver (Sep 2026)

**Rule.** On macOS, `PlatformManager::onInitialize()` calls
`PlatformSpecific::pinVulkanLoaderToBundledDriver()` (`Helpers.mac.cpp`) **before `glfwInit()`**. When the
executable is a bundle carrying `Contents/Resources/vulkan/icd.d/MoltenVK_icd.json`, it sets
`VK_DRIVER_FILES` to that manifest, so the loader loads that driver and no other. It changes nothing
when `VK_DRIVER_FILES` or `VK_ICD_FILENAMES` is already set (an explicit choice wins, which is how a
developer points the engine at another driver), and nothing outside a bundle or in a bundle without
its own manifest.

**Why.** The Vulkan loader loads **one driver per manifest it finds**, in this search order (measured
with `VK_LOADER_DEBUG=driver`, loader 1.4.357): the bundle's `Contents/Resources/vulkan/icd.d`, then
`~/.config/vulkan/icd.d`, `/etc/xdg/vulkan/icd.d`, `/etc/vulkan/icd.d`, `~/.local/share/vulkan/icd.d`,
`/usr/local/share/vulkan/icd.d`, `/usr/share/vulkan/icd.d`. The LunarG SDK installs a MoltenVK manifest
and a KosmicKrisp manifest in `/usr/local/share/vulkan/icd.d`. Measured on an Apple M2 with the SDK,
2026-09-30, before this rule:

- MoltenVK was dlopened **twice** in the main process, from `Contents/Frameworks/libMoltenVK.dylib` and
  `/usr/local/lib/libMoltenVK.dylib`. The files were byte-identical with the same install name
  (`@rpath/libMoltenVK.dylib`), but dyld keys on the path. The Objective-C runtime then warned
  `Class MVKBlockObserver is implemented in both … This may cause spurious casting failures and
  mysterious crashes.`
- The engine enumerated **three** "Apple M2" physical devices, one per driver, and selected the one
  backed by the **SDK** copy. The bundle's copy was dead weight.
- The KosmicKrisp device logged `missing the required 'VK_KHR_portability_subset' extension`, a false
  alarm that comes from a non-portability driver.

A Mac without the SDK loaded only the bundle's driver already. The defect needs the SDK, so it hits
every development machine and any user who installed the SDK.

**⚠️ Traps.**

- **It has to run before the FIRST loader call.** `glfwInit()` / `glfwVulkanSupported()` already scan
  the drivers (a scan dlopens every manifest's library), and a macOS dylib holding Objective-C classes
  is never unloaded. So setting the variable in `Vulkan::Instance` would be too late.
- **`VK_LUNARG_direct_driver_loading` cannot replace it.** The loader only accepts a direct driver that
  negotiates Loader-Driver interface **version 7** or higher (`loader_add_direct_driver`, `interface_version
  < 7` → "skipping"). MoltenVK answers **5** whatever it is offered (`vk_icdNegotiateLoaderICDInterfaceVersion`,
  checked on MoltenVK `main`, 2026-09-30), even though it already exposes `vk_icdGetPhysicalDeviceProcAddr`
  through `vk_icdGetInstanceProcAddr`, which is what version 7 asks for. In exclusive mode, the instance
  would have no driver left (`VK_ERROR_INCOMPATIBLE_DRIVER`). Revisit if MoltenVK raises its version.
- **`VK_LOADER_DRIVERS_SELECT` cannot replace it either**: it matches manifest *file names*, and both
  MoltenVK manifests are called `MoltenVK_icd.json`.
- The variable is inherited by child processes. That is harmless for a consumer's CEF helpers: they link
  the engine but create no Vulkan instance.
- Keep the bundle manifest's `api_version` in step with the MoltenVK that ships (the consumer owns the
  manifest).

**Verify.** Launch with `VK_LOADER_DEBUG=driver`: `VK_DRIVER_FILES` replaces the default search, so the only
"Found ICD manifest file" line is the bundle's (`…/Contents/Resources/vulkan/icd.d/MoltenVK_icd.json`). No
`implemented in both` line, one "Apple M2" device, and `Using "Apple M2" with driver:
"…/Contents/Frameworks/libMoltenVK.dylib"`.
