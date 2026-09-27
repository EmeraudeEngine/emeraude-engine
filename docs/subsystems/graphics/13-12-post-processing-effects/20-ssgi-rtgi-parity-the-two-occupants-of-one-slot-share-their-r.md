## 12. Post-Processing Effects

### SSGI / RTGI parity — the two occupants of one slot share their range and their range fade (Sep 2026)

The two occupants of the `IndirectDiffuse` slot are an A/B of ONE concept on the same frame. Two
things that were neither "screen-space limitation" nor technique used to differ, and the A/B
compared them along with the technique:

| | RTGI | SSGI, before | SSGI, now |
|---|---|---|---|
| bounce range default (was `IndirectDiffuse/<lane>/MaxDistance`, now ONE concept-level `IndirectDiffuse/MaxDistance`) | 8 m | **5 m** | 8 m, same key |
| range fade | `1 - smoothstep(0.8·max, max, hitT)` — last fifth only | **`1 - hitDist / max`** — linear from the surface | last fifth only |

- The range key has the same meaning and unit in both lanes, so it carries the same default (the
  rule `ContactShadows` already follows). In a 6 m corridor a wall could not receive the opposite
  wall's light in the screen-space lane at all — a setting that read as a technique gap.
- The linear fade halves a bounce found at mid-range. RTGI retired it (Jul 2026: "a fade
  proportional to the distance is NOT physical … cost about a factor two of indirect energy against
  the screen-space path"), SSGI kept it: the fix had landed on one lane. The fade now only smooths
  the last fifth, whose sole purpose is to keep geometry from popping at the range boundary.

Measured on `global-illumination` (same pose, left wall bit-stable as the exposure control), before
the parity: the slot added **+34.1** of mean luminance in the ray-traced lane and **+1.2** in the
screen-space one; the shadowed right wall read 84 under RTGI and **0.0** under SSGI. What remains
after the parity is the structural screen-space ceiling — an off-screen surface carries no light —
and nothing else. ⚠️ SSGI still has **no sky term**, on purpose (§ *Indirect-diffuse OWNERSHIP*
above): a screen-space miss is "no information", not "open sky". Do not close the residual gap by
adding one.

> [!CAUTION]
> ⚠️ **projet-alpha does not reset its settings on a version bump.** A `settings.json` written under
> the old tree keeps `IndirectDiffuse/ScreenSpace/MaxDistance = 5` as an ORPHAN (nothing reads it any
> more) and gets the concept-level `IndirectDiffuse/MaxDistance` at its default; the owner's file was
> migrated by hand (Sep 2026). Any machine whose screen-space GI still stops short of the room checks
> the concept-level key first, then looks for stale per-lane entries.
