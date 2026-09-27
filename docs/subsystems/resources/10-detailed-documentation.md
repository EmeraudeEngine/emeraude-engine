## Detailed Documentation

For complete resources system architecture:
- [`../../docs/resource-management.md`](../../resource-management.md) - Fail-safe, dependencies, detailed lifecycle, thread safety, future suggestions

Related systems:
- [`../Net/AGENTS.md`](../../../src/Net/AGENTS.md) - Resource download from URLs (`"Source": "ExternalData"`): the manager's contract, the `FileDownloaded`-on-main-thread rule, the console check of the whole chain
- [`../Graphics/AGENTS.md`](../../../src/Graphics/AGENTS.md) - Geometry, Material, Texture as resources
- [`../Audio/AGENTS.md`](../../../src/Audio/AGENTS.md) - SoundResource, MusicResource
- [`../../dependencies/emeraude-base/src/AGENTS.md`](../../../dependencies/emeraude-base/src/AGENTS.md) - Observer/Observable pattern
