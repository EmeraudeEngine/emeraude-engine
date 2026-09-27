## Critical: Descriptor pools are GROWABLE, and a working NVIDIA run proves nothing about their sizes (Sep 2026)

`DescriptorPool` adds a PAGE of the same sizes when one is exhausted (`VK_ERROR_OUT_OF_POOL_MEMORY` /
`VK_ERROR_FRAGMENTED_POOL`) and frees each set into the page it came from (`pageCount()`, one trace per page
added). Pattern: "DescriptorAllocatorGrowable", Victor Blanco, vkguide.dev (MIT). The declared sizes are
therefore a PAGE, not a ceiling — but **every descriptor TYPE a pool serves must be in its sizes**, or no page
can ever serve it.

⚠️⚠️ Measured 2026-09-22, found by the peer sessions: the renderer's main pool declared **64 combined image
samplers for 4096 sets and no storage image at all** (RTR's cone image has been one since 2026-08-30). Every
Linux run passed — the NVIDIA driver tolerates an overdraw of a pool, and the Debian validation layer does not
check the type. The Windows SDK 1.4.357 layer refused the RTR trace set
(`WARNING-CoreValidation-AllocateDescriptorSets-WrongType`, then `VK_ERROR_VALIDATION_FAILED_EXT`: `relief`
did not load), and the AMD iGPU returned `VK_ERROR_OUT_OF_POOL_MEMORY` for the screen-space lane's sets and
crashed. The page is now 1024 combined image samplers and 128 storage images (`Renderer.cpp`). Validate a
descriptor change on AMD or on the Windows layer, never on the NVIDIA run alone.
