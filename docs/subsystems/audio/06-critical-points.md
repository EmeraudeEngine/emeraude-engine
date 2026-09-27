## Critical Points

- **Device detection**: OpenAL-Soft latest can be finicky on laptops
- **Sound format**: MONO mandatory for 3D spatial positioning
- **Music format**: STEREO supported (no spatialization)
- **Thread safety**: OpenAL handles threading internally
- **Coordinate system**: Always Y-up in Audio API
- **`AL_ORIENTATION` takes the engine vectors VERBATIM.** OpenAL follows the OpenGL convention — right-handed, `+Y` up, `-Z` forward — which is exactly the engine world convention. `HardwareOutput::updateDeviceFromCoordinates()` therefore passes `forwardVector()` and `upwardVector()` unmodified. **Any negation added there is a bug, not an OpenAL quirk.**
  ⚠️ It passed `downwardVector()` until Aug 2026 — a Y-down compensation the flip should have removed, leaving the vertical axis of the audio field INVERTED. It survived that long because it is **invisible to every visual check**: stereo/HRTF panning disagrees with the picture with nothing on screen to show for it. To re-verify, play a positional sound ABOVE the listener and confirm which way it comes from.
- **Resource integration**: Never load directly, go through Resources
- **Microphone privacy**: `Core/Audio/Capture/Enable` must be explicitly `true` before any microphone access. Default is `false`. Voice-over (`Core/RushMaker/EnableVoiceOver`) depends on this gate.
- **Available devices in settings**: Both playback (`Core/Audio/AvailableDevices`) and capture (`Core/Audio/Capture/AvailableDevices`) device lists are written to settings on each startup for easy device switching via settings file editing
