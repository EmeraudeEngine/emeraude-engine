#!/usr/bin/env python3
"""
Emeraude Engine - Deterministic demo capture bench.

Launches a demo, waits for the scene to CONVERGE, then captures. The convergence wait is the
whole point of this tool.

A freshly loaded scene is not stable: exposure adaptation, temporal accumulation (TAA, SVGF) and
animation start-up all move the image for seconds after the first frame. Capturing after a fixed
`sleep` therefore samples an arbitrary point of that transient, and two scripts whose delays
differ compare two different moments of the scene -- an artefact that reads exactly like a
rendering regression.

Measured on `reflexion-debug --demo-options=0,6,0`, mean luminance of the same crop:

    t~4s: 136.5    t~8s: 129.8    t~12s: 126.8    t~16s: 125.9    t~20s: 125.7

A bench capturing at t~4s against a reference captured at t~8s shows a uniform +7 offset over
the whole frame. That happened (Sep 2026) and cost a false regression report on the RTR UBO port
before the drift was identified. Waiting for convergence removes the coupling entirely: both runs
capture the same state, whatever else the script does in between.

Usage:
    # Capture a demo once the image stops moving
    python3 demo-capture-bench.py --exe ./projet-alpha --demo asset-loader \\
        --demo-options 7,0,1,0,0,0 --out /tmp/shot.png

    # Same, with a fixed camera pose and the per-pass GPU timings
    python3 demo-capture-bench.py --exe ./projet-alpha --demo reflexion-debug \\
        --demo-options 0,6,0 --camera 0,1.6,4 --look-at 0,1,0 \\
        --crop 300,200,1700,1400 --gpu-timings --out /tmp/rtr.png

    # Two comparable captures of the same run (the noise floor of a pixel diff)
    python3 demo-capture-bench.py --exe ./projet-alpha --demo asset-loader \\
        --demo-options 7,0,1,0,0,0 --out /tmp/a.png --control /tmp/b.png

⚠️ The remote console is CLOSED BY DEFAULT: `Core/Console/EnableRemoteListener` must be true in
the application's settings.json. A refused connection means the instance was launched without the
key -- enable it and relaunch, never poll a running instance.

⚠️ Run the app with the Vulkan validation layers ON for anything you intend to conclude from.
This tool reports the VUID count of the run; a clean-looking capture with the layers off proves
nothing.
"""

import argparse
import os
import re
import shutil
import signal
import socket
import subprocess
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from emeraude_console import Console, DEFAULT_HOST, DEFAULT_PORT

try:
	import numpy as np
	from PIL import Image
except ImportError as error:
	sys.exit("This bench needs numpy and pillow for the convergence metric: %s" % error)

DEFAULT_CAPTURES = Path.home() / ".local/share/LNIsle/projet-alpha/captures"


def wait_for_console(host, port, timeout):
	"""Block until the remote console accepts a connection, or give up."""

	deadline = time.monotonic() + timeout

	while time.monotonic() < deadline:
		with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as probe:
			probe.settimeout(0.5)

			if probe.connect_ex((host, port)) == 0:
				return True

		time.sleep(0.5)

	return False


def newest_capture(captures_dir):
	"""Path of the most recent PNG in the captures directory, or None."""

	shots = sorted(captures_dir.glob("*.png"), key=lambda item: item.stat().st_mtime)

	return shots[-1] if shots else None


def capture_signature(path):
	"""Identity of a capture FILE, not of its name.

	⚠️ The engine names a capture with a unix timestamp in SECONDS, so two screenshots taken
	within the same second write the SAME file and the second silently overwrites the first.
	Identifying a capture by its path alone therefore misses it entirely -- measured: a
	convergence probe followed immediately by the measurement capture returned "no capture
	produced" while the engine had written one. Compare mtime and size too.
	"""

	if path is None:
		return None

	stat = path.stat()

	return (path, stat.st_mtime_ns, stat.st_size)


def take_capture(console, captures_dir, previous_signature):
	"""Ask for a screenshot and return the file it produced.

	⚠️ Returns None when the engine produced nothing. Never fall back to "the newest file": a
	failed screenshot then re-serves the PREVIOUS capture, and two bit-identical images read as
	"the effect changed nothing" -- a trap that has already produced a wrong conclusion.
	"""

	console.run("Core.RendererService.screenshot()")

	deadline = time.monotonic() + 10.0

	while time.monotonic() < deadline:
		current = newest_capture(captures_dir)

		if current is not None and capture_signature(current) != previous_signature:
			return current

		time.sleep(0.25)

	return None


def mean_luminance(path, crop):
	"""Rec.709 mean luminance of a capture, over the whole frame or a crop."""

	image = Image.open(path).convert("RGB")

	if crop is not None:
		image = image.crop(crop)

	pixels = np.asarray(image).astype(np.float64)

	return float(0.2126 * pixels[:, :, 0].mean() + 0.7152 * pixels[:, :, 1].mean() + 0.0722 * pixels[:, :, 2].mean())


def wait_for_convergence(console, captures_dir, crop, interval, epsilon, window, max_probes):
	"""Probe the image until it stops moving. Returns (converged, trace, last_capture).

	⚠️ The test is the PEAK-TO-PEAK spread of the last `window` probes, NOT the delta between
	two consecutive ones. Exposure adaptation moves in steps: the luminance sits perfectly still
	for a probe, then jumps again. A pairwise test latches onto that false plateau -- measured on
	reflexion-debug, a pairwise criterion converged two runs of the SAME binary at 128.40 and
	127.19 (delta 1.2), because run 2 saw `delta 0.000` followed by `delta 0.483`. The windowed
	test rejects it: a plateau only counts once the whole window is flat.
	"""

	trace = []
	history = []
	previous_signature = capture_signature(newest_capture(captures_dir))
	probes = []
	last = None

	for _ in range(max_probes):
		time.sleep(interval)

		shot = take_capture(console, captures_dir, previous_signature)

		if shot is None:
			print("  !! the engine produced no capture -- aborting the convergence wait", file=sys.stderr)
			break

		previous_signature = capture_signature(shot)
		probes.append(shot)
		last = shot

		luminance = mean_luminance(shot, crop)
		history.append(luminance)
		history = history[-window:]
		spread = max(history) - min(history) if len(history) == window else None
		trace.append((round(time.monotonic(), 2), luminance, spread))

		print("    probe: luminance %7.3f%s" % (
			luminance, "" if spread is None else "   spread(%d) %.3f" % (window, spread)))

		if spread is not None and spread < epsilon:
			# The probes were instrumentation, not results: do not leave them behind.
			for probe in probes[:-1]:
				probe.unlink(missing_ok=True)

			return True, trace, last

	for probe in probes[:-1]:
		probe.unlink(missing_ok=True)

	return False, trace, last


def parse_triplet(text):
	values = [float(item) for item in text.split(",")]

	if len(values) != 3:
		raise argparse.ArgumentTypeError("expected three comma-separated numbers, got %r" % text)

	return values


def parse_box(text):
	values = [int(item) for item in text.split(",")]

	if len(values) != 4:
		raise argparse.ArgumentTypeError("expected x0,y0,x1,y1, got %r" % text)

	return tuple(values)


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("--exe", required=True, help="Path to the application binary.")
	parser.add_argument("--demo", required=True, help="Demo class id (see --list-builtin-demos).")
	parser.add_argument("--demo-options", default=None, help="Demo options string, e.g. 7,0,1,0,0,0.")
	parser.add_argument("--extra-arg", action="append", default=[], help="Extra argument forwarded to the app (repeatable).")
	parser.add_argument("--out", required=True, type=Path, help="Where to write the measurement capture.")
	parser.add_argument("--control", type=Path, default=None,
		help="Take a SECOND capture of the same run here. Its difference to --out is the noise "
		     "floor of any pixel diff you draw from this scene -- shoot it before reading one.")
	parser.add_argument("--camera", type=parse_triplet, default=None, help="Body position x,y,z.")
	parser.add_argument("--look-at", type=parse_triplet, default=None, help="lookAt target x,y,z.")
	parser.add_argument("--crop", type=parse_box, default=None,
		help="x0,y0,x1,y1 the convergence metric is measured over. Exclude animated subjects: "
		     "they never converge and would hold the wait open until --max-probes.")
	parser.add_argument("--gpu-timings", action="store_true", help="Dump the per-pass GPU timings after convergence.")
	parser.add_argument("--captures-dir", type=Path, default=DEFAULT_CAPTURES, help="Engine captures directory.")
	parser.add_argument("--host", default=DEFAULT_HOST)
	parser.add_argument("--port", type=int, default=DEFAULT_PORT)
	parser.add_argument("--boot-timeout", type=float, default=120.0, help="Seconds to wait for the console to open.")
	parser.add_argument("--settle", type=float, default=4.0, help="Seconds after the camera pose before probing starts.")
	parser.add_argument("--interval", type=float, default=2.0, help="Seconds between convergence probes.")
	parser.add_argument("--epsilon", type=float, default=0.15, help="Luminance delta below which a probe counts as stable.")
	parser.add_argument("--window", type=int, default=4,
		help="Number of consecutive probes whose peak-to-peak spread must stay under --epsilon. "
		     "Below 3 the test latches onto the plateaus of a stepped exposure adaptation.")
	parser.add_argument("--max-probes", type=int, default=30, help="Give up after this many probes.")
	parser.add_argument("--log", type=Path, default=None, help="Where to write the application log.")
	args = parser.parse_args()

	if not args.captures_dir.is_dir():
		sys.exit("Captures directory not found: %s" % args.captures_dir)

	command = [str(args.exe), "--load-demo", args.demo]

	if args.demo_options is not None:
		command += ["--demo-options", args.demo_options]

	command += args.extra_arg

	log_path = args.log or Path("/tmp/demo-capture-bench-%s.log" % args.demo)
	converged = False
	print("Launching: %s" % " ".join(command))

	with open(log_path, "w", encoding="utf-8") as log_file:
		app = subprocess.Popen(command, stdout=log_file, stderr=subprocess.STDOUT)

		try:
			if not wait_for_console(args.host, args.port, args.boot_timeout):
				sys.exit("The remote console never opened on %s:%d. It is CLOSED BY DEFAULT: set "
					"Core/Console/EnableRemoteListener to true in settings.json and relaunch." % (args.host, args.port))

			with Console(args.host, args.port) as console:
				if args.camera is not None or args.look_at is not None:
					console.run("Core.SceneManagerService.targetActiveScene()")

				if args.camera is not None:
					console.run("Core.SceneManagerService.Act.setPosition(%f, %f, %f)" % tuple(args.camera))

				if args.look_at is not None:
					console.run("Core.SceneManagerService.Act.lookAt(%f, %f, %f)" % tuple(args.look_at))

				time.sleep(args.settle)

				print("Waiting for the image to converge (spread of %d probes under %.3f):" % (args.window, args.epsilon))
				converged, trace, _ = wait_for_convergence(
					console, args.captures_dir, args.crop,
					args.interval, args.epsilon, args.window, args.max_probes)

				if converged:
					print("  converged after %d probes (~%.1f s of probing)" % (len(trace), len(trace) * args.interval))
				else:
					print("  !! NOT CONVERGED after %d probes. The capture is on a moving image and is NOT\n"
						"     comparable to another run. An animated subject inside --crop does this;\n"
						"     exclude it, or raise --epsilon knowing what you are trading." % len(trace),
						file=sys.stderr)

				shot = take_capture(console, args.captures_dir, capture_signature(newest_capture(args.captures_dir)))

				if shot is None:
					sys.exit("The engine produced no capture.")

				shutil.copy(shot, args.out)
				print("Capture -> %s (luminance %.3f)" % (args.out, mean_luminance(args.out, args.crop)))

				if args.control is not None:
					time.sleep(args.interval)
					control = take_capture(console, args.captures_dir, capture_signature(shot))

					if control is None:
						sys.exit("The engine produced no control capture.")

					shutil.copy(control, args.control)
					print("Control -> %s (luminance %.3f)" % (args.control, mean_luminance(args.control, args.crop)))

				if args.gpu_timings:
					print("--- GPU timings ---")
					print(re.sub(r"\x1b\[[0-9;]*m", "", console.run("Core.RendererService.getGPUTimings()")))

				console.run("Core.shutdown()")

		finally:
			try:
				app.wait(timeout=30)
			except subprocess.TimeoutExpired:
				# A single TERM is ignored by the application on purpose.
				app.send_signal(signal.SIGINT)

				try:
					app.wait(timeout=10)
				except subprocess.TimeoutExpired:
					app.kill()

	text = log_path.read_text(encoding="utf-8", errors="replace")
	vuids = len(re.findall(r"VUID-", text))
	errors = len(re.findall(r"\[Error\]", text))
	print("Run: %d VUID, %d [Error] (log: %s)" % (vuids, errors, log_path))

	if vuids:
		print("  !! Validation errors in the run: read the FIRST VUID of a burst, the rest are consequences.", file=sys.stderr)

	return 0 if converged else 1


if __name__ == "__main__":
	sys.exit(main())
