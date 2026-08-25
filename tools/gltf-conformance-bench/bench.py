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

# ⚠️ The near plane is HARD-CODED in ViewMatrices2DUBO::updatePerspectiveViewProperties() as
# `0.1 / sqrt(1 + tan²(fov/2)·(aspect²+1))` — about 0.089 m WHATEVER the scene scale, with no
# link to the camera or the content. Sub-decimetre assets (BoomBox r=0.010 m,
# MetalRoughSpheresNoTextures r=0.0038 m, authentically authored in millimetres) sit entirely
# inside it at their computed framing distance and render NOTHING. The bench pushes the camera
# out until the bounding sphere clears the plane and REPORTS the frame coverage it loses, rather
# than hiding a capture that is quietly empty.
NEAR_PLANE_M = 0.0886
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
    "IridescenceDielectricSpheres": ["front", "back"],
    "IridescenceMetallicSpheres":   ["front", "back"],
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
    "OrientationTest":              ["front", "back", "top", "bottom", "right", "left"],
    # Showcase assets: a face-on and a three-quarter, as the references show them.
    "BoomBox":                      ["front", "three-qtr"],
    "WaterBottle":                  ["front", "three-qtr"],
    "SheenCloth":                   ["front", "three-qtr"],
}


def default_assets_directory() -> Path:
    """The vendored Khronos sample assets, relative to this script inside the engine tree."""
    return Path(__file__).resolve().parents[2] / "dependencies" / "glTF-Sample-Assets" / "Models"


def pick_asset(assets: Path, model: str) -> Path:
    """Prefers the self-contained binary variant, falls back to the plain glTF."""
    for folder, suffix in (("glTF-Binary", ".glb"), ("glTF", ".gltf")):
        candidate = assets / model / folder / f"{model}{suffix}"

        if candidate.exists():
            return candidate

    matches = sorted((assets / model).rglob("*.gl[tb]*"))

    if matches:
        return matches[0]

    raise FileNotFoundError(f"No glTF asset found for {model} under {assets}")


def clamp_distance(distance: float, radius: float) -> tuple:
    """Pushes the camera out until the bounding sphere clears the fixed near plane."""
    minimum = NEAR_PLANE_M * NEAR_CLEARANCE + radius

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
        for entry in plan:
            model = entry["model"]
            bounds = entry["bounds"]
            centre = bounds["centre"]

            print(f"\n=== {model}  (radius {bounds['radius']:.4f} m, {bounds['primitives']} primitives)")

            record = {key: entry[key] for key in ("model", "asset", "bounds", "distance")}
            record["views"] = {}

            answer = console.run(f'Core.openFiles("{entry["asset"]}")', timeout=90.0)

            if "ignored" in answer:
                print("  ! refused — a scene is running. Launch the engine without --load-demo.")
                record["error"] = "a scene was running, the file was ignored"
                report.append(record)
                continue

            try:
                record["entities"] = wait_for_viewer(console, model)
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
        print("\n* pushed back to clear the fixed ~0.089 m near plane; the coverage shown is what is left.")


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
    report_path = arguments.out / "bench-report.json"
    report_path.write_text(json.dumps(report, indent=2))

    print(f"\n{sum(len(record['views']) for record in report)} capture(s) in {arguments.out}")
    print(f"Report: {report_path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())