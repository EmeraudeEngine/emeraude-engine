## 7. Frame Rate Limiter

Optional software frame rate limiter for precise FPS control.

**Settings key:** `Core/Video/FrameRateLimit` (default: 0)

| Value | Behavior |
|-------|----------|
| `0` | Disabled (unlimited FPS) |
| `60` | Limit to 60 FPS |
| `144` | Limit to 144 FPS |
| etc. | Target FPS value |

**Implementation:**
- Hybrid sleep + busy-wait for precision
- Sleep for bulk of remaining time (saves CPU)
- Busy-wait for final ~1ms (timing accuracy)

**When to use:**
- Linux with compositor (GNOME/KDE): VSync OFF + FrameRateLimit ON
- Reduce GPU power consumption without VSync
- Consistent frame pacing for recording/streaming

**Code references:**
- `Renderer.cpp:renderFrame()` - Frame limiting logic at end of function
- `Renderer.cpp:onInitialize()` - Setting initialization
- `Renderer.hpp:m_frameRateLimit`, `m_frameDuration`, `m_frameStartTime`
- `SettingKeys.hpp:VideoFrameRateLimitKey`
