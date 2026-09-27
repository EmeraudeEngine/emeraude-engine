## 12. Post-Processing Effects

### SSContactShadows — the screen-space lane of the contact shadow (Sep 2026)

The depth buffer marched toward the light: for each pixel a ray steps in **view space** by a fixed
**world-space** length (`maxDistance / stepCount`, metres), each step projected back to the screen
to read the depth buffer. The pixel is shadowed when the ray passes behind a depth sample by less
than `thickness`. This is the original technique — UE (`r.ContactShadows`), Unity HDRP, Frostbite —
and the ray-queried `RTContactShadows` is the newer of the two.

**Why a world-space step and not a pixel-uniform one.** The two occupants of a slot exist to be
compared on the same framing, and that requires `maxDistance` to mean the same thing in both. A
pixel-uniform DDA is cheaper and never misses a thin occluder, but its "length" would vary with
depth and the A/B would compare two different distances. A world step **clamped** to a screen-space
range (what UE does) is the documented refinement — after this version has been measured, never
before.

**Full resolution**, unlike its half-res sibling: a contact shadow exists for the fine detail at
the point of contact, which is exactly what a half-res march destroys; the sibling halves it to
amortise ray traversal, which a depth march does not pay. The shared denoise pass partitions its
group by extent, so cohabiting with a half-res SSGI/SSAO is already handled.

Both occupants emit the **same signal** — R = shadow factor, G = normalized contact distance — the
same PCSS-lite denoise kernel and the same combine snippet. That is a requirement, not a
coincidence: the chain must not know which occupant of a slot produced its input.

**Measured on Sponza** (2880×1620, zero VUID), against the same frame with the concept disabled and
with the auto-exposure shift cancelled:

| | pixels darkened | mean darkening (sum RGB) |
|---|---|---|
| `SSContactShadows` | 570 742 | 23.35 |
| `RTContactShadows` | 1 474 150 | 21.37 |

**Correlation of the two darkening maps: +0.302** — positive, so the screen-space lane shadows the
*same places* as the traced ground truth (this is the check that would have caught a view-space
sign error, which is the failure this effect is most exposed to). It finds **~39 % as many**
shadowed pixels, which is the structural ceiling, not a tuning problem, and the darkening it does
produce is of the right magnitude.

> [!CAUTION]
> **Three traps, all inherent.**
> - **An occluder that is not on screen casts nothing.** Same ceiling as SSR, where 43.3 % of the
>   rays were measured leaving the screen and `maxSteps` bought nothing. Do not try to buy the
>   missing 61 % back with more steps.
> - **The depth buffer is a heightfield with no thickness.** `thickness` is the assumption that
>   fills that in and it is THE knob: too thin leaks light through thin geometry, too thick trails
>   a halo behind every occluder.
> - ⚠️⚠️ **The view-space forward sign is read from the DATA, not assumed** (`forwardSign` in the
>   shader): the shaded pixel is in front of the camera by construction, so the sign of its
>   view-space z *is* the convention. Guessing it would invert every depth comparison and shadow
>   exactly the pixels that should stay lit — a failure that reads as a tuning problem, not as a
>   sign error, which is why the correlation check above is part of the verification and not a
>   nicety.
> - ⚠️ The bias is an offset on the **normal**, never a raised start distance. Same lesson as the
>   sibling's faceted terminator: at the terminator the light is grazing, so advancing along it
>   never leaves the surface, and raising the start skips the near occluders the effect exists for.
