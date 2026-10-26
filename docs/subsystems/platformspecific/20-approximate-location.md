# The machine's approximate place — its time zone's city (Oct 2026)

`PlatformSpecific::approximateLocation()` (`src/PlatformSpecific/SystemLocation.*`) answers where the machine roughly is,
for a sky at the real time and place (`Scenes::Component::CelestialClock::observerPlace()`, which falls back to the
engine's birthplace, Liège, `Identification::Birthplace*`). Owner decision 2026-10-26: the TIME ZONE — offline, no
permission, a few hundred kilometres — rather than the OS location service (a permission prompt, three implementations)
or an IP service (network, privacy).

- **Linux / macOS** (`SystemLocation.posix.cpp`): the zone from `TZ` (a leading `:` and a zoneinfo path accepted), else
  the `/etc/localtime` link (Linux `/usr/share/zoneinfo/Europe/Brussels`, macOS `/var/db/timezone/zoneinfo/...`): what
  follows `zoneinfo/`. Located in the tz database's own table, `/usr/share/zoneinfo/zone1970.tab` then `zone.tab`
  (`findZone()`, `parseZoneCoordinates()`: ISO 6709 ±DDMM±DDDMM[SS]).
- **Windows** (`SystemLocation.windows.cpp`): `GetDynamicTimeZoneInformation()`'s key ("Romance Standard Time") and the
  user's region (`GetUserGeoID(GEOCLASS_NATION)` + `GetGeoInfoW(GEO_ISO2)`, "BE"), located in a GENERATED table: the
  Unicode CLDR `windowsZones` mapping (key, territory → IANA zone) crossed with the tz database's coordinates — 470
  pairs; the region picks Brussels over Paris for "Romance Standard Time". Regenerate with
  `tools/windows-zones/generate.py <windowsZones.xml> /usr/share/zoneinfo` (the CLDR licence in
  `docs/third-party-notices.md`).
- A zone without a city (`UTC`, `Etc/GMT+n`) answers nothing: the caller's fallback.
- Measured 2026-10-26 (Linux): `Europe/Brussels` → 50.8333°, 4.3333° (Brussels, 75 km from Liège). ⚠️ The Windows file
  is not compiled on Linux: its validation is the Windows peer's.
