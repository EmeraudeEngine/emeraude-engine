## Validation layers for one run: `--set-vk-layers` (Oct 2026)

The validation layers follow the settings (`Core/Video/VulkanInstance/EnableDebug` + `RequestedValidationLayers`).
`--set-vk-layers=LAYER_A,LAYER_B` (or `--set-vk-layers LAYER_A,LAYER_B`) overrides them **for one run** (owner
request, 2026-10-11): the run takes exactly those layers, and the validation settings are neither read nor written
(not even `AvailableValidationLayers`) — so a verification run never leaves a trace in the user's settings file.

| Command line | Result |
|---|---|
| `--set-vk-layers=VK_LAYER_KHRONOS_validation` | that layer, debug mode on, settings untouched |
| `--set-vk-layers=` (empty value) | no layer for this run, whatever the settings request |
| `--set-vk-layers` (no value) | error trace, the settings apply |
| a name outside `[A-Za-z0-9_]` or ≥ 256 characters, more than 32 layers | the whole value refused (trace), the settings apply |
| spaces, empty items, duplicates | trimmed / dropped (first occurrence kept) |

`--debug-vulkan` still forces debug mode on top of it. An unavailable layer name is reported by the usual "requested
validation layer is unavailable" warning. Verified on Linux 2026-10-11 (the five rows, LunarG layer 1.4.363, exit 0).

Implementation: `Instance::readSettings()` / `Instance::parseValidationLayersArgument()` (command line = trust boundary,
checked in every build). The requested names are owned by `Instance::m_requestedValidationLayers`, into which
`m_requiredValidationLayers` points — before 2026-10-11 they lived in a function-local `static`, read once per
process.
