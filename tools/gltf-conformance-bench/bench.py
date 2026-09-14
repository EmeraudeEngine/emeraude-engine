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
from gltf_bounds import scene_bounds  # noqa: E402


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

SCREENSHOT_PATH = re.compile(r"Screenshot saved:\s*(\S+)")

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
        "Core/Viewers/AmbientIntensity": 0,
    },
    "AnisotropyStrengthTest": {
        "Core/Viewers/Background": "",
        "Core/Viewers/EnvironmentCubemap": "",
        "Core/Viewers/AmbientIntensity": 0,
    },
    # ⚠️ Its README asks for it in as many words: "It is recommended to have an environment with
    # distinctive bright light sources for testing." The coat's Fresnel is 0.04 at normal
    # incidence, so a band of coat present-vs-absent is a 4 % step in the REFLECTION — invisible
    # against the viewer's dark forest, plain against a bright sky. The `Partial coating` row is
    # unreadable without this, and reading it as a defect is the mistake to avoid.
    "ClearCoatTest": {
        "Core/Viewers/Background": "Kloppenheim05",
        "Core/Viewers/EnvironmentCubemap": "",
        "Core/Viewers/AmbientIntensity": 0,
    },
    "SheenCloth": {
        "Core/Viewers/Background": "",
        "Core/Viewers/EnvironmentCubemap": "",
        "Core/Viewers/AmbientIntensity": 0,
    },
}

ENVIRONMENT_KEYS = ("Core/Viewers/Background", "Core/Viewers/EnvironmentCubemap", "Core/Viewers/AmbientIntensity")


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

    return {key: viewers.get(key.rsplit("/", 1)[1]) for key in ENVIRONMENT_KEYS if key.rsplit("/", 1)[1] in viewers}


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


def pick_asset(assets: Path, model: str) -> Path:
    """
    Prefers the self-contained binary variant, falls back to the plain glTF.

    Our own assets/ directory is searched FIRST so a probe shadows nothing upstream and needs
    no entry anywhere else; the vendored Khronos tree is the fallback.
    """
    for root in (local_assets_directory(), assets):
        for folder, suffix in (("glTF-Binary", ".glb"), ("glTF", ".gltf")):
            candidate = root / model / folder / f"{model}{suffix}"

            if candidate.exists():
                return candidate

        if (root / model).is_dir():
            matches = sorted((root / model).rglob("*.gl[tb]*"))

            if matches:
                return matches[0]

    raise FileNotFoundError(f"No glTF asset found for {model} under {local_assets_directory()} or {assets}")


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


def build_plan(assets: Path, only: set) -> list:
    """
    Computes, for every selected model, its world bounds and one camera placement per view.

    ⚠️ The framing is CALCULATED, never guessed. The Khronos models span three orders of
    magnitude — radius 0.0038 m for MetalRoughSpheresNoTextures against 14.5 m for the
    iridescence grids, a factor of 3800. A bench built on fixed distances shoots half of them at
    a hundred pixels and the rest out of frame entirely.
    """
    plan = []

    for model, views in MODELS.items():
        if only and model not in only:
            continue

        asset = pick_asset(assets, model)
        bounds = scene_bounds(asset)
        radius = bounds["radius"]
        distance, clamped = clamp_distance(DISTANCE_FACTOR * max(radius, 1e-4), radius)

        plan.append({
            "model": model,
            "asset": str(asset),
            "bounds": bounds,
            "distance": distance,
            "near_plane_clamped": clamped,
            "frame_coverage": 2.0 * radius / distance,
            "views": [
                {"name": name, "position": camera_position(bounds["centre"], distance, *VIEWS[name])}
                for name in views
            ],
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
            bounds = entry["bounds"]
            centre = bounds["centre"]

            print(f"\n=== {model}  (radius {bounds['radius']:.4f} m, {bounds['primitives']} primitives)")

            # The environment this test declares, posed BEFORE the scene is built — and the
            # session's own values put back as soon as a model that wants none comes along, so a
            # run never leaves one test's environment on another's capture.
            environment = ENVIRONMENTS.get(model)

            if environment is not None:
                apply_environment(console, environment)
                posed_environment = True
                print(f"  environment: " + ", ".join(f"{k.rsplit('/', 1)[1]}={v!r}" for k, v in environment.items()))
            elif posed_environment:
                apply_environment(console, session_environment)
                posed_environment = False

            record = {key: entry[key] for key in ("model", "asset", "bounds", "distance")}
            record["views"] = {}

            if environment is not None:
                record["environment"] = environment

            answer = console.run(f'Core.openFiles("{entry["asset"]}")', timeout=90.0)

            if "ignored" in answer:
                print("  ! refused — a scene is running. Launch the engine without --load-demo.")
                record["error"] = "a scene was running, the file was ignored"
                report.append(record)
                continue

            try:
                record["entities"] = wait_for_viewer(console, model)


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

                destination = output / f"{model}_{name}.png"

                try:
                    capture(console, destination)
                except RuntimeError as error:
                    print(f"  ! {name}: {error}")
                    record["views"][name] = {"error": str(error)}
                    continue

                record["views"][name] = {"file": str(destination)}

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


def print_plan(plan: list) -> None:
    print(f"Vertical field of view {math.degrees(FIELD_OF_VIEW):.2f} deg "
          f"({FOCAL_LENGTH_MM:.0f} mm on {SENSOR_HEIGHT_MM:.0f} mm), "
          f"framing distance = {DISTANCE_FACTOR:.3f} x radius\n")
    print(f"{'model':<30} {'radius':>10} {'distance':>10} {'coverage':>9}  views")

    for entry in plan:
        flag = " *" if entry["near_plane_clamped"] else "  "
        print(f"{entry['model']:<30} {entry['bounds']['radius']:>10.4f} {entry['distance']:>10.4f} "
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
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    arguments = parser.parse_args()

    if not arguments.assets.is_dir():
        print(f"Assets directory not found: {arguments.assets}\n"
              "The Khronos samples are a sparse checkout under dependencies/glTF-Sample-Assets.",
              file=sys.stderr)
        return 1

    unknown = set(arguments.models) - set(MODELS)

    if unknown:
        print(f"Unknown model(s): {', '.join(sorted(unknown))}\n"
              f"Known: {', '.join(sorted(MODELS))}", file=sys.stderr)
        return 1

    plan = build_plan(arguments.assets, set(arguments.models))

    if arguments.plan:
        print_plan(plan)
        return 0

    try:
        report = run_bench(plan, arguments.out, arguments.host, arguments.port)
    except ConnectionRefusedError:
        print(f"Cannot connect to {arguments.host}:{arguments.port}. Is the engine running, "
              "and was it launched WITHOUT --load-demo?", file=sys.stderr)
        return 1

    arguments.out.mkdir(parents=True, exist_ok=True)
    # ⚠️ A partial run (`./bench.py OneModel`) MERGES into the existing report instead of replacing
    # it: the report carries the bounds and framing distance every later measurement reads back, and
    # a twenty-minute full run must not be erased by a one-model re-capture. Re-captured models
    # replace their own record, everything else is kept.
    report_path = arguments.out / "bench-report.json"
    merged = []

    if report_path.exists():
        previous = json.loads(report_path.read_text())
        captured = {record["model"] for record in report}
        merged = [record for record in previous if record["model"] not in captured]

    merged.extend(report)
    report_path.write_text(json.dumps(merged, indent=2))

    print(f"\n{sum(len(record['views']) for record in report)} capture(s) in {arguments.out}")
    print(f"Report: {report_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())