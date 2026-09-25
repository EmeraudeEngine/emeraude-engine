#!/usr/bin/env python3
"""
glTF conformance bench — drives the engine over the remote console and captures the Khronos
sample models under the framing each test's README imposes.

    ./bench.py --plan                     # print the framing plan, no engine needed
    ./bench.py                            # capture every model
    ./bench.py NormalTangentTest          # capture one, or a few

Harness. The engine's own `+ModelViewer` scene is the bench: `Core.openFiles()` opens it, and it
brings the three things a conformance capture needs — framing computed from the real world
extents, a fixed MANUAL sunny-sixteen exposure, and (since Aug 2026) a sky supplying the
background and the IBL. The camera is then placed explicitly on the `ViewerCamera` node, which
the orbit controller only reclaims on a pointer event — none happens over TCP.

    Launch the engine WITHOUT --load-demo. A running demo scene is never disturbed by the
    dropped-files pipeline: `openFiles` answers "a scene is running, the file was ignored" and
    the bench captures nothing.

Reading the results is NOT this script's job. Each Khronos model ships its README and, more
importantly, its NAMED failure images (`incorrect-flipped-y.png`, `supplied-tangents-ignored.png`,
`OrientationTestFail.png`, `BlendFail.jpg`...). Those images are the diagnostic instrument:
comparing against `screenshot.png` alone tells you something is wrong but never what.
"""

import argparse
import json
import math
import re
import shutil
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from emeraude_console import DEFAULT_HOST, DEFAULT_PORT, Console  # noqa: E402
from gltf_bounds import image_encodings, scene_bounds  # noqa: E402
from png_compare import compare as compare_captures, read_png_size  # noqa: E402


# --- Optics -----------------------------------------------------------------------------------
#
# Replicated from Scenes::Viewers::ModelViewer so an imposed view sits at the same scale as the
# viewer's own default framing. Keep in lockstep with ViewerFocalLength / FramingMargin there.

SENSOR_WIDTH_MM = 36.0                          # Camera::m_sensorWidth, full frame.
SENSOR_HEIGHT_MM = SENSOR_WIDTH_MM * 2.0 / 3.0  # Camera::sensorHeight().
FOCAL_LENGTH_MM = 50.0                          # ModelViewer::ViewerFocalLength.
FRAMING_MARGIN = 1.2                            # ModelViewer::FramingMargin.

FIELD_OF_VIEW = 2.0 * math.atan(SENSOR_HEIGHT_MM / (2.0 * FOCAL_LENGTH_MM))
DISTANCE_FACTOR = FRAMING_MARGIN / math.sin(0.5 * FIELD_OF_VIEW)

# The near plane, and the history that makes this constant look strange.
#
# It USED to be hard-coded in ViewMatrices2DUBO as `0.1 / sqrt(1 + tan²(fov/2)·(aspect²+1))` —
# about 0.089 m WHATEVER the scene scale. Sub-decimetre assets (BoomBox r=0.010 m,
# MetalRoughSpheresNoTextures r=0.0038 m, authentically authored in millimetres) sat entirely
# inside it at their computed framing distance and rendered NOTHING, so the bench pushed the
# camera out until the bounding sphere cleared the plane and reported the frame coverage it lost
# rather than hiding a capture that was quietly empty.
#
# Since 2026-08-28 the engine derives it from the subject: `+ModelViewer` tells the camera that
# the nearest object is 1 % of the subject's radius, so the near plane is now PROPORTIONAL and
# the clamp below can essentially never bite — a framing distance of DISTANCE_FACTOR × radius
# clears radius × (1 + 0.01 × clearance) by a wide margin for any DISTANCE_FACTOR > 1.02.
#
# ⚠️ The guard is KEPT, and expressed relatively rather than deleted: it is what turned an empty
# capture into a reported number once, and if the viewer's rule ever changes it must complain
# again instead of silently producing black frames. If `near_plane_clamped` starts coming back
# true, the engine and this file have drifted apart — read ModelViewer's
# `setNearestObjectDistance()` call before touching anything here.
NEAR_PLANE_SUBJECT_FRACTION = 0.01   # mirrors ModelViewer: nearestObject = radius * 0.01
NEAR_CLEARANCE = 1.5

# --- Timings ----------------------------------------------------------------------------------

VIEWER_BUDGET_S = 40.0   # Waiting for +ModelViewer to come up with at least one entity.
LOAD_SETTLE_S = 4.5      # ⚠️ Must exceed Notifier::DefaultDuration (3000 ms), or the "Viewing
                         # <file>" toast is still on screen and pollutes the capture.
VIEW_SETTLE_S = 1.2      # After moving the camera, before capturing.

# ⚠️ The path may contain SPACES — macOS captures land in "~/Library/Application Support/…" — and the
# engine quotes it: take the quoted form whole, the bare token otherwise (found by the macOS session,
# 2026-09-22: every view failed with "screenshot never landed on disk: /Users/…/Library/Application").
SCREENSHOT_PATH = re.compile(r"Screenshot saved:\s*(\"[^\"]+\"|\S+)")

# --- Views ------------------------------------------------------------------------------------
#
# (azimuth degrees around +Y measured from +Z, elevation degrees above the horizon).
# ⚠️ Never a pure 90 degree elevation: straight down is the lookAt gimbal singularity.

VIEWS = {
    "front":     (0.0, 0.0),
    "three-qtr": (35.0, 20.0),
    # The mirror of three-qtr, for a model that carries a second set of features on the three
    # OPPOSITE faces — OrientationTest's CMY arrows (matrix encodings) against its RGB ones
    # (quaternions). Reading only one of the two halves is how a half-tested verdict is written.
    "three-qtr-rear": (215.0, -20.0),
    "back":      (180.0, 0.0),
    "top-down":  (0.0, 70.0),
    "top":       (0.0, 80.0),
    "bottom":    (0.0, -80.0),
    "right":     (90.0, 0.0),
    "left":      (-90.0, 0.0),
}

# Per-model views, driven by what each README actually asks to be inspected — not by symmetry.
MODELS = {
    # Flat test charts read face-on. The back view guards against a chart facing -Z.
    "AlphaBlendModeTest":           ["front", "back"],
    "AnisotropyStrengthTest":       ["front", "back"],
    "ClearCoatTest":                ["front", "back"],
    "EmissiveStrengthTest":         ["front", "back"],
    # ⚠️ These two are 3-D GRIDS of spheres: a dead-on `front` view collapses them into
    # overlapping rows and reads nothing, which is how their failure went unattributed for three
    # runs. A three-quarter view is what their own references use, and what makes the IOR and
    # thickness axes separable.
    "IridescenceDielectricSpheres": ["three-qtr", "front", "back"],
    "IridescenceMetallicSpheres":   ["three-qtr", "front", "back"],
    "MetalRoughSpheres":            ["front", "back"],
    "MetalRoughSpheresNoTextures":  ["front", "back"],
    "SpecularTest":                 ["front", "back"],
    "TextureTransformTest":         ["front", "back"],
    "TransmissionTest":             ["front", "back"],
    "TransmissionRoughnessTest":    ["front", "back"],
    "VertexColorTest":              ["front", "back"],
    # Both normal/tangent READMEs impose exactly these three views, and say why.
    "NormalTangentTest":            ["front", "top-down", "back"],
    "NormalTangentMirrorTest":      ["front", "top-down", "back"],
    # Arrows on all six faces: RGB (quaternions) on three, CMY (matrices) on the others. One view
    # per axis per encoding, or the test is only half read.
    # ⚠️ The six axis views show an arrow but NOT the target it must point at: the targets sit on
    # the box's far rim, out of frame at this framing. The test's criterion is the arrow-to-target
    # ALIGNMENT, so it also gets the three-quarter view its own reference screenshot uses, where
    # arrow and same-colour target appear together. Judging it from an axis view alone cannot be
    # done — and inheriting a previous run's verdict instead is how a stale PASS survives.
    "OrientationTest":              ["three-qtr", "three-qtr-rear", "front", "back", "top", "bottom", "right", "left"],
    # Showcase assets: a face-on and a three-quarter, as the references show them.
    "BoomBox":                      ["front", "three-qtr"],
    "WaterBottle":                  ["front", "three-qtr"],
    "SheenCloth":                   ["front", "three-qtr"],
    # ⚠️ NOT a Khronos model — ours, generated by ./make-volume-probe.py, because no sample
    # asset exercises KHR_materials_volume's absorption: of the 60 glTF files in reach, 15
    # declare the extension, 2 declare an attenuation colour and ZERO declare an attenuation
    # distance, whose default is +infinity. The feature is therefore invisible on Khronos
    # content whether it works or not. Three identical transmissive balls differing only in
    # the volume they declare, so the control lives inside the single capture.
    "VolumeAbsorptionProbe":        ["front"],
}

# --- Compressed-variant A/B ---------------------------------------------------------------------
#
# Models captured TWICE — once from the plain glTF, once from its `glTF-Draco` sibling — and
# compared per pixel. Added 2026-09-18 with KHR_draco_mesh_compression support.
#
# ⚠️ A codec test asks a DIFFERENT question from the rest of this bench. Everywhere else the
# criterion is "does the renderer obey the spec"; here it is "does the compressed asset render the
# SAME thing as its uncompressed source". There is no reference image to read — the reference is
# the other capture, and it is only a reference if both are shot at an IDENTICAL framing.
#
# ⚠️⚠️ WHICH IS WHY THE FRAMING IS COMPUTED FROM THE PLAIN ASSET AND REUSED VERBATIM. Draco
# quantisation dilates the bounding box by half a quantum, measured at a constant **+0.0977 %** on
# the radius across Box/Avocado/BoomBox/WaterBottle. Letting each variant compute its own bounds
# moves the camera by that much, and the A/B then measures the camera, not the codec.
#
# The value is the view list, used only for a model that is NOT already in MODELS above; a model
# present in both inherits MODELS' views, so the two captures always have views to compare.
DRACO_MODELS = {
    # Showcase assets already benched uncompressed: they inherit their MODELS views.
    "BoomBox":              None,
    "WaterBottle":          None,
    # Draco-only entries. Two views, because a single face-on shot of a symmetric subject can hide
    # a decode error behind its own silhouette.
    "Avocado":              ["front", "three-qtr"],
    "BarramundiFish":       ["front", "three-qtr"],
    "Box":                  ["front", "three-qtr"],
    "Corset":               ["front", "three-qtr"],
    "Duck":                 ["front", "three-qtr"],
    "Lantern":              ["front", "three-qtr"],
    "CesiumMilkTruck":      ["front", "three-qtr"],
    "VirtualCity":          ["front", "three-qtr"],
    # The skinned set — JOINTS_0 and WEIGHTS_0 go through the decoder here, and a wrong influence
    # shreds the mesh rather than nudging it.
    "CesiumMan":            ["front", "three-qtr"],
    "BrainStem":            ["front", "three-qtr"],
    "RiggedFigure":         ["front", "three-qtr"],
    "RiggedSimple":         ["front", "three-qtr"],
    # Draco attributes sitting NEXT TO plain bufferView-bearing morph-target accessors — the mixed
    # primitive. This loader reads no morph target, so what is verified is the base geometry.
    "MorphPrimitivesTest":  ["front", "three-qtr"],
    # KTX2 *and* Draco on the same asset, 109 bitstreams: the two extension families together.
    "CarConcept":           ["front", "three-qtr"],
    # WebP *and* Draco on the same asset. Blocked until 2026-09-18, when EXT_texture_webp landed.
    "SunglassesKhronos":    ["front", "three-qtr"],
}

# Draco variants deliberately NOT benched, each with the reason. Listed rather than silently
# omitted: an absent row reads as "never tried", which is not the same claim.
#
# Empty since 2026-09-18, when EXT_texture_webp support unblocked `SunglassesKhronos` — its only
# occupant, and one that had nothing to do with Draco. The table stays because the next asset to be
# blocked will need it, and because `main()` answers with the reason instead of "unknown model".
DRACO_BLOCKED = {}

# The environment every compressed-variant A/B is shot in.
#
# ⚠️⚠️ NOT a style choice — it is what makes the comparison reproducible. The viewer's environment
# cubemap STREAMS, and the scene logs its choice before the texture is resident, so one capture of a
# pair can be drawn with the default while its twin gets the landscape. Measured 2026-09-18: that
# moved `RiggedSimple` by +28.39/255 of mean and `CarConcept` by -1.78 with the subject
# pixel-identical — indistinguishable from a codec defect, and the entity count matched so the
# structural control stayed silent.
#
# ⚠️ Cropping to the subject does NOT fix it, and was tried first: a tight box around a slender
# model is still mostly background, and removing the identical sky only concentrates the difference
# — `RiggedSimple` went from 28.43 to 38.40. The cure is to have nothing to stream.
#
# "" on both keys is the bit-exact black backdrop already used by the SpecularTest/SheenCloth rows.
# The viewer's own key light still lights the subject, so it stays readable; what disappears is the
# environment reflection, which a GEOMETRY codec test has no business measuring anyway.
DRACO_AB_ENVIRONMENT = {
    "Core/Viewers/Background": "",
    "Core/Viewers/EnvironmentCubemap": "",
}

# --- Per-model viewer environment ------------------------------------------------------------
#
# ⚠️ A test must be posed in the environment IT declares, and three of these cannot be judged in
# the viewer's default daylight sky. `+ModelViewer` reads these three settings when it builds its
# scene, and `Core.SettingsService.set()` reaches them over the console, so the bench poses each
# test and restores the session's values afterwards.
#
#   Core/Viewers/Background          what is BEHIND the subject ("" = a bit-exact black backdrop)
#   Core/Viewers/EnvironmentCubemap  what the subject REFLECTS ("" = whatever the background set)
#   Core/Viewers/AmbientIntensity    flat ambient floor, in lux
#
# ⚠️ The first two are INDEPENDENT axes, and black is not the universal answer — measured:
#   - `SpecularTest` is too DARK, not washed out: baseColor [0,0,0,1], metallic 0, roughness 0, a
#     dielectric mirror whose F0 never exceeds 0.04. On the default sky its 35 spheres span
#     19.5/255 mean; under Kloppenheim05 with no flat ambient they span 89.5 and the seven rows
#     separate. It needs a BRIGHT REFLECTION, not a darker backdrop.
#   - `AnisotropyStrengthTest` needs a DISTINCT source. In an outdoor environment a sphere's
#     brightest pixels are an image of the sky, and every shape metric reads noise — five were
#     confounded that way. With no background, no environment cubemap and no ambient, the only
#     specular source left is the viewer's own key light, the lobe becomes a lobe, and the
#     elongation runs 1.11 -> 9.26 monotonically up the anisotropy axis.
#   - `SheenCloth` is shot on black by Khronos; its rim-to-backdrop contrast goes 8.1x -> 390x.
ENVIRONMENTS = {
    "SpecularTest": {
        "Core/Viewers/Background": "Kloppenheim05",
        "Core/Viewers/EnvironmentCubemap": "",
        "Core/Viewers/AmbientIlluminance": 0,
    },
    "AnisotropyStrengthTest": {
        "Core/Viewers/Background": "",
        "Core/Viewers/EnvironmentCubemap": "",
        "Core/Viewers/AmbientIlluminance": 0,
    },
    # ⚠️ Its README asks for it in as many words: "It is recommended to have an environment with
    # distinctive bright light sources for testing." The coat's Fresnel is 0.04 at normal
    # incidence, so a band of coat present-vs-absent is a 4 % step in the REFLECTION — invisible
    # against the viewer's dark forest, plain against a bright sky. The `Partial coating` row is
    # unreadable without this, and reading it as a defect is the mistake to avoid.
    "ClearCoatTest": {
        "Core/Viewers/Background": "Kloppenheim05",
        "Core/Viewers/EnvironmentCubemap": "",
        "Core/Viewers/AmbientIlluminance": 0,
    },
    "SheenCloth": {
        "Core/Viewers/Background": "",
        "Core/Viewers/EnvironmentCubemap": "",
        "Core/Viewers/AmbientIlluminance": 0,
    },
}

ENVIRONMENT_KEYS = ("Core/Viewers/Background", "Core/Viewers/EnvironmentCubemap", "Core/Viewers/AmbientIlluminance")

# The engine's defaults for those keys (SettingKeys.hpp: DefaultViewerBackground,
# DefaultViewerEnvironmentCubemap, DefaultViewerAmbientIntensity). A key ABSENT from the session's settings
# is restored to its default: +ModelViewer creates it with that very value on its first scene, so leaving
# the last posed value behind would change the user's viewer for good. ⚠️ Keep them in sync with the engine.
ENVIRONMENT_DEFAULTS = {
    "Core/Viewers/Background": "GreenLandscape",
    "Core/Viewers/EnvironmentCubemap": "",
    "Core/Viewers/AmbientIlluminance": 80.72,  # the engine default (renamed 2026-09-25; 200 x a dimming colour before)
}


def asset_directory_candidates() -> tuple:
    """
    Every place the vendored Khronos corpus can legitimately be, most specific first.

    ⚠️ It is NOT in the engine tree. Test data belongs to the testbed (owner decision,
    2026-08-31), so `glTF-Sample-Assets` is a submodule of the CONSUMER — `projet-alpha` — while
    this script lives in the engine. Three layouts are real and all three are supported:

    1. the engine checked out (or symlinked) at `<consumer>/dependencies/emeraude-engine`, which
       is how the cascade is documented — reached from the UNRESOLVED script path, because
       `resolve()` follows the symlink straight out of the consumer's tree and loses it;
    2. the engine and its consumer as SIBLING directories, the symlink's actual target on the
       owner's workstation — found by scanning the resolved engine root's siblings;
    3. the corpus cloned inside the engine itself, for a standalone engine checkout with no
       consumer at all.

    ⚠️ The corpus is a PARTIAL clone (`blob:none`, 201 MB against 1.7 GB upstream) — never
    re-clone it plainly.
    """
    relative = Path("dependencies") / "glTF-Sample-Assets" / "Models"

    unresolved_engine_root = Path(__file__).absolute().parents[2]
    resolved_engine_root = Path(__file__).resolve().parents[2]

    candidates = [
        unresolved_engine_root.parents[1] / relative,
        resolved_engine_root.parents[1] / relative,
    ]

    # The sibling layout: no arithmetic can name the consumer, so ask the filesystem which
    # neighbour carries the corpus. Sorted, so the answer never depends on directory order.
    for sibling in sorted(resolved_engine_root.parent.iterdir()):
        if sibling.is_dir() and sibling != resolved_engine_root:
            candidates.append(sibling / relative)

    candidates.append(resolved_engine_root / relative)

    return tuple(candidates)


def console_literal(value) -> str:
    """A value as the console expression parser wants it: strings quoted, numbers bare."""
    return f'"{value}"' if isinstance(value, str) else str(value)


def read_environment(console) -> dict:
    """
    The session's current viewer environment, so the bench can put it back.

    ⚠️ It MUST be put back: `Core.shutdown()` saves the settings on the way out, so a value this
    script leaves behind lands in the user's settings.json permanently.
    """
    import json as _json

    answer = console.run("Core.SettingsService.getJson()", timeout=20.0)
    start = answer.find("{")

    if start < 0:
        return {}

    try:
        tree = _json.loads(answer[start:answer.rfind("}") + 1])
    except ValueError:
        return {}

    viewers = tree.get("Core", {}).get("Viewers", {})

    # ⚠️ Every key, present or not: a key absent from settings.json (a session that never opened a viewer)
    # used to be left out, so nothing was restored and Core.shutdown() saved the last posed environment
    # (found by the macOS session, 2026-09-22). Absent means "the engine default".
    return {key: viewers.get(key.rsplit("/", 1)[1], ENVIRONMENT_DEFAULTS[key]) for key in ENVIRONMENT_KEYS}


def apply_environment(console, environment: dict) -> None:
    """Poses the viewer environment. Read by +ModelViewer when it builds its scene, so BEFORE openFiles()."""
    for key, value in environment.items():
        console.run(f'Core.SettingsService.set("{key}", {console_literal(value)})', timeout=20.0)


def default_assets_directory() -> Path:
    """
    The first candidate layout that actually holds the corpus.

    Falls back to the documented one so a failure names the place the assets are SUPPOSED to be
    rather than the last place that was tried.
    """
    for candidate in asset_directory_candidates():
        if candidate.is_dir():
            return candidate

    return asset_directory_candidates()[0]


def local_assets_directory() -> Path:
    """
    Our OWN probe assets, laid out like the Khronos ones so pick_asset() needs no special case.

    They exist for the gaps Khronos does not cover — see make-volume-probe.py for why a
    home-made asset is the only way to regression-test KHR_materials_volume at all. Keep this
    directory small and every asset in it generated by a versioned script, never hand-dropped:
    a binary blob nobody can regenerate is worse than no test.
    """
    return Path(__file__).resolve().parent / "assets"


# Folder/suffix pairs searched for a variant, in order of preference.
#
# ⚠️ The default prefers the self-contained .glb; the Draco variants are shipped as loose .gltf
# only, and `CarConcept` carries its Draco bitstreams in the KTX2 folder rather than a plain
# `glTF-Draco` one — hence two entries, not one.
VARIANT_FOLDERS = {
    None:    (("glTF-Binary", ".glb"), ("glTF", ".gltf")),
    "draco": (("glTF-Draco", ".gltf"), ("glTF-KTX-BasisU-Draco", ".gltf")),
}


def pick_asset(assets: Path, model: str, variant: str = None) -> Path:
    """
    Prefers the self-contained binary variant, falls back to the plain glTF.

    Our own assets/ directory is searched FIRST so a probe shadows nothing upstream and needs
    no entry anywhere else; the vendored Khronos tree is the fallback.

    ⚠️ The `rglob` last resort applies to the DEFAULT variant only. A variant asks for one precise
    encoding of a model, so "any glTF under this directory" would happily hand back the plain file
    and the A/B would then compare an asset with itself — reporting a perfect match for a variant
    that was never loaded. A missing variant must raise instead.
    """
    for root in (local_assets_directory(), assets):
        for folder, suffix in VARIANT_FOLDERS[variant]:
            candidate = root / model / folder / f"{model}{suffix}"

            if candidate.exists():
                return candidate

        if variant is None and (root / model).is_dir():
            matches = sorted((root / model).rglob("*.gl[tb]*"))

            if matches:
                return matches[0]

    label = model if variant is None else f"{model} [{variant}]"

    raise FileNotFoundError(f"No glTF asset found for {label} under {local_assets_directory()} or {assets}")


def clamp_distance(distance: float, radius: float) -> tuple:
    """
    Pushes the camera out until the bounding sphere clears the near plane.

    The near plane is now proportional to the subject (see the constants above), so this is a
    guard rather than the workaround it used to be. It returns the flag as well as the distance
    because a clamp that fires is a finding — it means the engine's rule and this file disagree.
    """
    minimum = radius * (1.0 + NEAR_PLANE_SUBJECT_FRACTION * NEAR_CLEARANCE)

    return (max(distance, minimum), distance < minimum)


def camera_position(centre, distance: float, azimuth_deg: float, elevation_deg: float) -> list:
    """
    Camera position on the framing sphere.

    +Y up, azimuth measured from +Z toward +X. Since the Y-up migration the glTF frame IS the
    engine frame (the import is the identity), so these are directly world coordinates.
    """
    azimuth = math.radians(azimuth_deg)
    elevation = math.radians(elevation_deg)

    return [
        centre[0] + distance * math.cos(elevation) * math.sin(azimuth),
        centre[1] + distance * math.sin(elevation),
        centre[2] + distance * math.cos(elevation) * math.cos(azimuth),
    ]


def build_plan(assets: Path, only: set, with_draco: bool = True) -> list:
    """
    Computes, for every selected model, its world bounds and one camera placement per view.

    ⚠️ The framing is CALCULATED, never guessed. The Khronos models span three orders of
    magnitude — radius 0.0038 m for MetalRoughSpheresNoTextures against 14.5 m for the
    iridescence grids, a factor of 3800. A bench built on fixed distances shoots half of them at
    a hundred pixels and the rest out of frame entirely.
    """
    plan = []

    def plain_entry(model: str, views: list) -> dict:
        asset = pick_asset(assets, model)
        bounds = scene_bounds(asset)
        radius = bounds["radius"]
        distance, clamped = clamp_distance(DISTANCE_FACTOR * max(radius, 1e-4), radius)

        return {
            "model": model,
            "label": model,
            "variant": None,
            "asset": str(asset),
            "bounds": bounds,
            "distance": distance,
            "near_plane_clamped": clamped,
            "frame_coverage": 2.0 * radius / distance,
            "views": [
                {"name": name, "position": camera_position(bounds["centre"], distance, *VIEWS[name])}
                for name in views
            ],
        }

    for model, views in MODELS.items():
        if only and model not in only:
            continue

        plan.append(plain_entry(model, views))

    if not with_draco:
        return plan

    planned = {entry["model"]: entry for entry in plan}

    for model, fallback_views in DRACO_MODELS.items():
        if only and model not in only:
            continue

        # A model already benched uncompressed keeps ITS views; a Draco-only one uses the list
        # declared next to it in DRACO_MODELS.
        reference = planned.get(model)

        if reference is None:
            reference = plain_entry(model, fallback_views)
            plan.append(reference)

        # ⚠️⚠️ bounds, distance and view POSITIONS are the plain entry's, copied verbatim and never
        # recomputed from the Draco file. Quantisation dilates the bounding sphere by ~0.0977 %, so
        # a recomputed framing moves the camera and the A/B measures that move instead of the codec.
        draco_asset = pick_asset(assets, model, "draco")

        # ⚠️ The A/B only measures the GEOMETRY codec if both variants are textured identically,
        # and several Khronos models break that: SunglassesKhronos is PNG against WebP, CarConcept
        # PNG against KTX2. Their delta then carries two codecs at once, and reading it as a Draco
        # number is simply wrong -- SunglassesKhronos comes out at 98 % of pixels that way. Detected
        # rather than hand-listed, so a corpus update cannot silently invalidate a row.
        plain_encodings = image_encodings(Path(reference["asset"]))
        draco_encodings = image_encodings(draco_asset)

        plan.append({
            "model": model,
            "label": f"{model}[draco]",
            "variant": "draco",
            "compare_with": reference["label"],
            "texture_encodings": sorted(plain_encodings | draco_encodings) if plain_encodings != draco_encodings else None,
            "asset": str(draco_asset),
            "bounds": reference["bounds"],
            "distance": reference["distance"],
            "near_plane_clamped": reference["near_plane_clamped"],
            "frame_coverage": reference["frame_coverage"],
            "views": [dict(view) for view in reference["views"]],
        })

    return plan


def wait_for_viewer(console: Console, model: str, budget: float = VIEWER_BUDGET_S) -> int:
    """Waits until +ModelViewer is active AND holds at least one entity."""
    deadline = time.monotonic() + budget

    while time.monotonic() < deadline:
        answer = console.run("Core.SceneManagerService.getSceneInfo()", timeout=6.0)

        if "+ModelViewer" in answer:
            entities = re.search(r"Static entities:\s*(\d+)", answer)

            if entities and int(entities.group(1)) > 0:
                return int(entities.group(1))

        time.sleep(0.5)

    raise TimeoutError(f"{model}: the +ModelViewer scene never came up")


def capture(console: Console, destination: Path) -> Path:
    """
    Captures the framebuffer and renames it.

    ⚠️ The engine names screenshots after a ONE-SECOND timestamp, so two captures inside the same
    second collide. The file is copied out immediately, from the path the console reports back.
    """
    answer = console.run("Core.RendererService.screenshot()", timeout=25.0)
    match = SCREENSHOT_PATH.search(answer)

    if match is None:
        raise RuntimeError(f"screenshot refused: {answer.strip()[:300]}")

    source = Path(match.group(1).strip("\"'"))

    for _ in range(40):
        if source.exists() and source.stat().st_size > 0:
            break

        time.sleep(0.1)
    else:
        raise RuntimeError(f"screenshot never landed on disk: {source}")

    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)

    return destination


def run_bench(plan: list, output: Path, host: str, port: int) -> list:
    report = []

    with Console(host, port) as console:
        session_environment = read_environment(console)
        posed_environment = False

        for entry in plan:
            model = entry["model"]
            # The capture name and the report key; `model` stays the ENVIRONMENT key, because a
            # compressed variant must be posed exactly like the asset it is compared against.
            label = entry.get("label", model)
            bounds = entry["bounds"]
            centre = bounds["centre"]

            print(f"\n=== {label}  (radius {bounds['radius']:.4f} m, {bounds['primitives']} primitives)")

            # The environment this test declares, posed BEFORE the scene is built — and the
            # session's own values put back as soon as a model that wants none comes along, so a
            # run never leaves one test's environment on another's capture.
            environment = ENVIRONMENTS.get(model)

            # A compressed-variant pair is shot in a deterministic environment unless the model
            # already declares one of its own, which then wins: a test posed for a reason keeps it.
            if environment is None and model in DRACO_MODELS:
                environment = DRACO_AB_ENVIRONMENT

            if environment is not None:
                apply_environment(console, environment)
                posed_environment = True
                print(f"  environment: " + ", ".join(f"{k.rsplit('/', 1)[1]}={v!r}" for k, v in environment.items()))
            elif posed_environment:
                apply_environment(console, session_environment)
                posed_environment = False

            record = {key: entry[key] for key in ("model", "asset", "bounds", "distance")}
            record["label"] = label
            record["variant"] = entry.get("variant")
            record["views"] = {}

            if entry.get("compare_with"):
                record["compareWith"] = entry["compare_with"]

            if entry.get("texture_encodings"):
                record["textureEncodingsDiffer"] = entry["texture_encodings"]

            if environment is not None:
                record["environment"] = environment

            answer = console.run(f'Core.openFiles("{entry["asset"]}")', timeout=90.0)

            if "ignored" in answer:
                print("  ! refused — a scene is running. Launch the engine without --load-demo.")
                record["error"] = "a scene was running, the file was ignored"
                report.append(record)
                continue

            try:
                record["entities"] = wait_for_viewer(console, label)


                # ⚠️⚠️ THE STRUCTURAL CONTROL, and the cheapest one this bench has: the engine builds
                # one static entity per mesh-bearing NODE, so anything below that count is geometry
                # that never reached the scene. It is what a per-pixel criterion cannot see —
                # `TransmissionTest` was marked PASS for weeks with two of its twelve spheres absent,
                # because its three nodes named `BlueTransWithMask` collided in a registry keyed by
                # name and the reading criterion only asked whether the four declared hues appeared
                # somewhere in the frame. Run this before reading a single pixel.
                # The viewer adds a couple of entities of its own, so only a DEFICIT is a defect.
                expected = bounds.get("meshNodes", 0)
                record["meshNodes"] = expected

                if expected > 0 and record["entities"] < expected:
                    record["missingEntities"] = expected - record["entities"]
                    print(f"  ⚠️ MISSING GEOMETRY: {record['entities']} static entities for {expected} "
                          f"mesh-bearing nodes — {record['missingEntities']} node(s) never reached the scene")
                else:
                    print(f"  entities {record['entities']} / {expected} mesh-bearing nodes")
            except TimeoutError as error:
                print(f"  ! {error}")
                record["error"] = str(error)
                report.append(record)
                continue

            console.run("Core.SceneManagerService.targetActiveScene()", timeout=6.0)

            # ⚠️ Forces the rest pose before any capture, so a stray space bar on the window cannot
            # freeze the subject mid-stride and pass the result off as a codec difference (it did,
            # 2026-09-18: one keypress took a model's A/B from a mean of 0.10/255 to 9.22). This
            # command draws NO notification, unlike Core.cycleAnimation() whose toast would be burnt
            # into the frame -- which is exactly why it exists. Harmless on a static asset: an asset
            # with no clip is already at rest and the call succeeds.
            console.run("Core.resetAnimation()", timeout=6.0)

            # Long enough for the textures to upload AND for the "Viewing <file>" toast to expire.
            time.sleep(LOAD_SETTLE_S)

            for view in entry["views"]:
                name = view["name"]
                position = view["position"]

                console.run(
                    'Core.SceneManagerService.setNodePosition("ViewerCamera", '
                    f'{position[0]:.6f}, {position[1]:.6f}, {position[2]:.6f})', timeout=6.0)
                console.run(
                    'Core.SceneManagerService.setNodeLookAt("ViewerCamera", '
                    f'{centre[0]:.6f}, {centre[1]:.6f}, {centre[2]:.6f})', timeout=6.0)

                time.sleep(VIEW_SETTLE_S)

                destination = output / f"{label}_{name}.png"

                try:
                    capture(console, destination)
                except RuntimeError as error:
                    print(f"  ! {name}: {error}")
                    record["views"][name] = {"error": str(error)}
                    continue

                record["views"][name] = {"file": str(destination), "position": position}

                flag = "  [CLAMPED by the fixed near plane]" if entry["near_plane_clamped"] else ""
                print(f"  {name:<10} d={entry['distance']:8.4f} m  "
                      f"coverage~{entry['frame_coverage'] * 100:5.1f}% of frame height{flag}")

            report.append(record)

        # ⚠️ Always put the session's environment back: `Core.shutdown()` SAVES the settings, so a
        # value left behind here ends up in the user's settings.json for good.
        if posed_environment and session_environment:
            apply_environment(console, session_environment)
            print(f"\nviewer environment restored: " + ", ".join(f"{k.rsplit('/', 1)[1]}={v!r}" for k, v in session_environment.items()))

    return report


def dot(a, b) -> float:
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b) -> tuple:
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def normalise(v) -> tuple:
    length = math.sqrt(dot(v, v))

    return (v[0] / length, v[1] / length, v[2] / length) if length > 1e-12 else (0.0, 0.0, 1.0)


# How much larger than the subject the comparison box is drawn.
#
# ⚠️ Not decoration: the silhouette is exactly where a lossy geometry codec differs, and a box cut
# flush to the bounding sphere would clip the pixels the measurement is about. 15 % also absorbs the
# small framing drift between two builds of the same scene.
SUBJECT_BOX_MARGIN = 1.15

# ⚠️ A FLOOR in pixels, because the margin above is RELATIVE and a flat model projects to a LINE.
# `MorphPrimitivesTest` seen face-on gave the box (223, 360, 1057, 360) — zero height, so a relative
# margin of zero, so an empty box and a row that could not be compared at all. Every other subject
# clears this floor on its own; it exists for the degenerate projection.
MIN_SUBJECT_BOX_MARGIN_PIXELS = 8.0


def subject_box(capture, bounds: dict, camera) -> tuple:
    """
    Projects the subject's world AABB into pixels, as the comparison box.

    ⚠️⚠️ WHY THIS EXISTS. The BACKGROUND is not deterministic: the viewer's environment cubemap can
    still be streaming when a frame is drawn, so one capture of a pair shows the default and the
    other the landscape. Measured 2026-09-18, that moved `RiggedSimple` by +28.39/255 of mean and
    `CarConcept` by -1.78 while the subject stayed pixel-identical — a perfect imitation of a codec
    defect, with the entity count matching so the structural control said nothing. Comparing only
    the subject removes the failure mode at its source rather than re-running until it goes away.

    ⚠️ The AABB, NOT the bounding sphere. The framing makes every subject subtend the same angle, so
    a sphere-based box is the SAME SIZE for every model and leaves a tall thin subject swimming in
    background: tried first, it took `RiggedSimple` from 28.43 to 27.81 — it barely moved, because
    the sphere around a slender cylinder is mostly empty. The projected AABB is as narrow as the
    model is.

    @param capture Path to one of the two captures, for the frame size.
    @param bounds The entry's world bounds, with its `min` and `max`.
    @param camera The camera position this view was shot from.
    @return (x0, y0, x1, y1)
    """
    width, height = read_png_size(capture)

    low = bounds["min"]
    high = bounds["max"]
    centre = bounds["centre"]

    forward = normalise([centre[i] - camera[i] for i in range(3)])

    # ⚠️ The world up is safe here because VIEWS never uses a pure ±90 degree elevation — that is
    # the lookAt gimbal singularity, and the table says so.
    right = normalise(cross(forward, (0.0, 1.0, 0.0)))
    up = cross(right, forward)

    tangent = math.tan(FIELD_OF_VIEW / 2.0)
    aspect = width / height

    xs = []
    ys = []

    for corner in ((low[0], low[1], low[2]), (high[0], low[1], low[2]),
                   (low[0], high[1], low[2]), (high[0], high[1], low[2]),
                   (low[0], low[1], high[2]), (high[0], low[1], high[2]),
                   (low[0], high[1], high[2]), (high[0], high[1], high[2])):
        view = [corner[i] - camera[i] for i in range(3)]
        depth = dot(view, forward)

        if depth <= 1e-6:
            # Behind the camera: cannot be projected, and a subject that straddles the eye is not a
            # framing this bench produces. Fall back to the whole frame rather than invent a box.
            return (0, 0, width, height)

        xs.append(width * 0.5 * (1.0 + (dot(view, right) / depth) / (tangent * aspect)))
        ys.append(height * 0.5 * (1.0 - (dot(view, up) / depth) / tangent))

    half_width = max((max(xs) - min(xs)) * 0.5 * (SUBJECT_BOX_MARGIN - 1.0), MIN_SUBJECT_BOX_MARGIN_PIXELS)
    half_height = max((max(ys) - min(ys)) * 0.5 * (SUBJECT_BOX_MARGIN - 1.0), MIN_SUBJECT_BOX_MARGIN_PIXELS)

    return (
        max(0, int(min(xs) - half_width)),
        max(0, int(min(ys) - half_height)),
        min(width, int(math.ceil(max(xs) + half_width))),
        min(height, int(math.ceil(max(ys) + half_height))),
    )


def annotate_draco_deltas(report: list) -> None:
    """
    Fills each compressed-variant record with its per-view difference against the plain capture.

    ⚠️ THE NUMBERS ARE NOT A VERDICT, and this function deliberately writes none. Draco is LOSSY:
    measured 2026-09-18, `Box` usually comes out bit-identical (its quantised positions land exactly,
    though an intermittent single-LSB residual on ~0.44 % of its pixels appears from run to run --
    one LSB is the CAPTURE's noise floor, never a codec signal) while
    `Avocado` differs on 10.3 % of pixels for a mean of 0.18/255 and `CesiumMan` on 3.3 % for
    0.11/255 — a sparse scatter along silhouettes, background untouched. Read the three numbers
    together: a tiny mean with a high maximum confined to edges is the quantisation signature; a
    mean that moves, or a maximum spread over a region, is a defect. Picking a pass threshold here
    would freeze one asset's quantisation grid into a rule for all of them.

    ⚠️⚠️ A row flagged `textureEncodingsDiffer` is NOT a geometry-codec measurement: the two
    variants of that model are textured differently (SunglassesKhronos ships PNG against WebP,
    CarConcept PNG against KTX2), so its delta carries two codecs at once. Read as a Draco number
    it is meaningless -- SunglassesKhronos comes out at 98 % of pixels that way. The row is kept
    because the LOAD is still worth exercising; only the pixel comparison is confounded.

    ⚠️ The ENTITY COUNT is the exception — it is a hard, non-arbitrary criterion, and the same
    structural control the rest of the bench already runs: the two variants declare the same nodes,
    so a deficit on the compressed side means a primitive failed to decode and silently vanished.
    That one is flagged.
    """
    by_label = {record.get("label", record["model"]): record for record in report}

    for record in report:
        reference_label = record.get("compareWith")

        if reference_label is None:
            continue

        reference = by_label.get(reference_label)

        if reference is None:
            record["dracoDeltaError"] = f"the reference capture '{reference_label}' is not in this run"
            continue

        if "entities" in record and "entities" in reference and record["entities"] != reference["entities"]:
            record["dracoEntityMismatch"] = {
                "variant": record["entities"],
                "reference": reference["entities"],
            }
            print(f"  ⚠️ {record['label']}: {record['entities']} entities against "
                  f"{reference['entities']} for the plain variant — geometry lost in the decode")

        deltas = {}

        for name, view in record["views"].items():
            other = reference["views"].get(name)

            if "file" not in view or other is None or "file" not in other:
                continue

            try:
                box = subject_box(view["file"], record["bounds"], view["position"])

                deltas[name] = compare_captures(other["file"], view["file"], box)
            except (OSError, ValueError) as error:
                deltas[name] = {"error": str(error)}

        if deltas:
            record["dracoDelta"] = deltas

            confounded = record.get("textureEncodingsDiffer")

            if confounded:
                print(f"  ⚠️ {record['label']}: the two variants are textured DIFFERENTLY "
                      f"({', '.join(confounded)}) — the delta below measures the texture codec as "
                      f"well as Draco and is NOT a geometry-codec number")

            for name, delta in deltas.items():
                if "error" in delta:
                    print(f"  ! {record['label']} {name}: {delta['error']}")
                else:
                    mark = "  [texture codec differs]" if confounded else ""
                    print(f"  {record['label']:<28} {name:<10} "
                          f"differ {delta['differingPercent']:7.4f} %  "
                          f"mean {delta['meanAbsDelta']:7.4f}/255  max {delta['maxAbsDelta']:3d}/255{mark}")


def print_plan(plan: list) -> None:
    print(f"Vertical field of view {math.degrees(FIELD_OF_VIEW):.2f} deg "
          f"({FOCAL_LENGTH_MM:.0f} mm on {SENSOR_HEIGHT_MM:.0f} mm), "
          f"framing distance = {DISTANCE_FACTOR:.3f} x radius\n")
    print(f"{'model':<30} {'radius':>10} {'distance':>10} {'coverage':>9}  views")
    # A `[draco]` row shows the radius and distance of the PLAIN entry it is compared against —
    # that is the point, not a display shortcut. See DRACO_MODELS.

    for entry in plan:
        flag = " *" if entry["near_plane_clamped"] else "  "
        print(f"{entry.get('label', entry['model']):<30} {entry['bounds']['radius']:>10.4f} {entry['distance']:>10.4f} "
              f"{entry['frame_coverage'] * 100:>8.1f}%{flag}{', '.join(v['name'] for v in entry['views'])}")

    if any(entry["near_plane_clamped"] for entry in plan):
        print("\n* pushed back to clear the near plane; the coverage shown is what is left.\n"
              "  ⚠️ Since the engine derives the near plane from the subject (2026-08-28) this should\n"
              "  never happen: a star here means ModelViewer's rule and this script have drifted.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("models", nargs="*", help="Models to capture. Default: all of them.")
    parser.add_argument("--assets", type=Path, default=default_assets_directory(),
                        help="glTF-Sample-Assets/Models directory.")
    parser.add_argument("--out", type=Path, default=Path.cwd() / "gltf-bench-captures",
                        help="Directory receiving the captures and the report.")
    parser.add_argument("--plan", action="store_true",
                        help="Print the framing plan and exit. Needs no running engine.")
    parser.add_argument("--no-draco", action="store_true",
                        help="Skip the compressed-variant A/B (DRACO_MODELS), which doubles the "
                             "capture count for the models it covers.")
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    arguments = parser.parse_args()

    if not arguments.assets.is_dir():
        print(f"Assets directory not found: {arguments.assets}\n"
              "The Khronos samples are a sparse checkout under dependencies/glTF-Sample-Assets.",
              file=sys.stderr)
        return 1

    known = set(MODELS) | set(DRACO_MODELS)
    unknown = set(arguments.models) - known

    if unknown:
        blocked = unknown & set(DRACO_BLOCKED)

        for model in sorted(blocked):
            print(f"{model} is deliberately not benched: {DRACO_BLOCKED[model]}", file=sys.stderr)

        if unknown - blocked:
            print(f"Unknown model(s): {', '.join(sorted(unknown - blocked))}\n"
                  f"Known: {', '.join(sorted(known))}", file=sys.stderr)

        return 1

    plan = build_plan(arguments.assets, set(arguments.models), not arguments.no_draco)

    if arguments.plan:
        print_plan(plan)
        return 0

    try:
        report = run_bench(plan, arguments.out, arguments.host, arguments.port)
    except ConnectionRefusedError:
        print(f"Cannot connect to {arguments.host}:{arguments.port}. Is the engine running, "
              "and was it launched WITHOUT --load-demo?", file=sys.stderr)
        return 1

    if any(record.get("compareWith") for record in report):
        print("\n=== compressed-variant A/B (Draco against the plain capture, identical framing)")
        annotate_draco_deltas(report)

    arguments.out.mkdir(parents=True, exist_ok=True)
    # ⚠️ A partial run (`./bench.py OneModel`) MERGES into the existing report instead of replacing
    # it: the report carries the bounds and framing distance every later measurement reads back, and
    # a twenty-minute full run must not be erased by a one-model re-capture. Re-captured models
    # replace their own record, everything else is kept.
    report_path = arguments.out / "bench-report.json"
    merged = []

    if report_path.exists():
        previous = json.loads(report_path.read_text())
        # ⚠️ Keyed on the LABEL, not the model: since 2026-09-18 a model can produce TWO records
        # (its plain capture and its `[draco]` twin) and keying on the model alone would let one
        # evict the other. `.get` keeps a pre-2026-09-18 report readable.
        captured = {record.get("label", record["model"]) for record in report}
        merged = [record for record in previous if record.get("label", record["model"]) not in captured]

    merged.extend(report)
    report_path.write_text(json.dumps(merged, indent=2))

    print(f"\n{sum(len(record['views']) for record in report)} capture(s) in {arguments.out}")
    print(f"Report: {report_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())