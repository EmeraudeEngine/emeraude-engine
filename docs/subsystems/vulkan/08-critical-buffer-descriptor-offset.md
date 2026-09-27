## Critical: Buffer Descriptor Offset

> [!CRITICAL]
> **`Buffer::getDescriptorInfo(offset, range)` MUST use the offset parameter!**
>
> This function returns `VkDescriptorBufferInfo` for descriptor set binding.
> The `offset` parameter specifies where to start reading in the buffer.
>
> **Bug pattern (fixed Jan 2026):**
> ```cpp
> // BROKEN - ignores offset, all descriptors point to offset 0
> getDescriptorInfo (uint32_t /*offset*/, uint32_t range) {
>     descriptorInfo.offset = 0;  // WRONG!
> }
>
> // CORRECT - uses the provided offset
> getDescriptorInfo (uint32_t offset, uint32_t range) {
>     descriptorInfo.offset = static_cast<VkDeviceSize>(offset);
> }
> ```
>
> **Impact:** SharedUniformBuffer stores multiple materials. If offset is ignored,
> ALL materials read from offset 0, causing Material B to display Material A's properties.

### UniformBufferObject Element Index Conversion

`UniformBufferObject::getDescriptorInfo(elementOffset)` receives an **element index** (0, 1, 2...)
but must pass a **byte offset** to `Buffer::getDescriptorInfo()`:

```cpp
// In UniformBufferObject.cpp
VkDescriptorBufferInfo
UniformBufferObject::getDescriptorInfo (uint32_t elementOffset) const noexcept
{
    // Convert element index to byte offset
    return this->getDescriptorInfo(elementOffset * m_blockAlignedSize, m_blockAlignedSize);
}
```

**Files involved:**
- `Buffer.hpp:getDescriptorInfo()` - Must use offset parameter
- `UniformBufferObject.cpp:getDescriptorInfo()` - Must multiply by block size
