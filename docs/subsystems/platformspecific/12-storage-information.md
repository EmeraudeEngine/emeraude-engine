## Storage Information (`PlatformSpecific::StorageInfo`)

**Files**: `StorageInfo.hpp` + `StorageInfo.{linux,mac,windows}.cpp`

Cross-platform mounted drive enumeration with space usage and removable detection.

**API**:
```cpp
namespace EmEn::PlatformSpecific::StorageInfo
{
    struct DriveInfo {
        std::string filesystem;      // Device path ("/dev/sda1") or description
        std::string mounted;         // Mount point ("/", "C:")
        std::string fsType;          // "ext4", "NTFS", "apfs", "exfat"
        uint64_t totalBytes{0};
        uint64_t usedBytes{0};
        uint64_t availableBytes{0};
        bool removable{false};       // USB, SD card, etc.
    };

    [[nodiscard]]
    std::vector< DriveInfo > listDrives () noexcept;
}
```

**Platform details**:
| Platform | Source | Space API | Removable detection |
|----------|--------|-----------|---------------------|
| Linux | `/proc/mounts` | `statvfs()` | `/sys/block/{dev}/removable` |
| macOS | `getmntinfo()` | `statfs` struct | DiskArbitration framework |
| Windows | `GetLogicalDriveStringsW()` | `GetDiskFreeSpaceExW()` | `GetDriveTypeW()` (DRIVE_REMOVABLE/DRIVE_CDROM) |

**Filtering**: Virtual/pseudo filesystems excluded. Linux skips `/dev/loop*` (snaps). Windows skips network/unknown drives.

---
