## 4. Platform-Specific Recommendations

### Linux/NVIDIA/X11 (GNOME, KDE)

**Issue:** VSync causes micro-stuttering due to double synchronization (driver + compositor).

**Solution:**
```
Core/Video/EnableVSync = false
Core/Video/EnableTripleBuffering = true (optional)
Core/Video/FrameRateLimit = 60  (or monitor refresh rate)
```

The compositor handles display sync; the app limits its frame rate to avoid wasting resources.

**Details:** See [`src/Vulkan/AGENTS.md`](../../src/Vulkan/AGENTS.md) (Present Mode Selection) and [`src/Graphics/AGENTS.md`](../../src/Graphics/AGENTS.md) (Frame Rate Limiter).
