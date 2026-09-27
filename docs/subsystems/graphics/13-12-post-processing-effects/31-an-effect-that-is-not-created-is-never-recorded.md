## 12. Post-Processing Effects

### An effect that is not CREATED is never recorded (Aug 2026)

> [!CAUTION]
> **A post-process effect holds per-frame containers that its recording code indexes without
> checking, so an un-created effect in the chain is a SEGFAULT, not a misbehaviour.** RTGI's
> `m_tracePerFrame[frameIndex]` on an empty vector is an out-of-bounds read — no null test would
> have caught it, and the stack trace points at `std::unique_ptr::operator->` with nothing to say.
> It happened twice, for two unrelated reasons:
> - a stack **installed without `createAll()`** — a demo factored its post-process setup and the
>   new code path called `Scene::setPostProcessStack()` while the old one created the effects
>   first (projet-alpha `AbstractDemo::create()`, Aug 2026);
> - a **`resize()` whose `create()` half failed** after its `destroy()` half had already run,
>   leaving the effect in the chain with its resources gone.
>
> **The contract.** `IndirectPostProcessEffect::isCreated()` says whether the effect holds its
> resources; `PostProcessStack` is the ONLY writer (`setCreatedFlag()`), because it drives the
> whole lifecycle — `createAll()`, `resizeAll()`, `destroyAll()`, and the four photographic
> effects it materializes itself in `syncCameraEffects()`. `PostProcessor` skips a
> non-created effect exactly as it skips a disabled one.
>
> - ⚠️ The skip is **silent by design**: the failure is already traced loudly where it happens
>   (`createAll()`/`resizeAll()` name the effect), and repeating it in the executor would print one
>   line per effect per frame.
> - ⚠️ `createAll()` and `resizeAll()` no longer **return on the first failure**: they used to
>   leave every remaining effect un-created, and those were recorded exactly like the one that
>   failed. They now attempt them all and report the aggregate.
> - ⚠️⚠️ **The photographic effects are created in `syncCameraEffects()`, never by `createAll()`.**
>   Forgetting their flag there does not crash — it silently skips the TONE MAPPING and leaves the
>   whole frame in linear HDR.
