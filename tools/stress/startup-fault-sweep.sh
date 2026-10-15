#!/bin/bash
#
# tools/stress/startup-fault-sweep.sh
# This file is part of Emeraude-Engine (LGPLv3).
#
# The startup fault-injection sweep (Ave Robustus II P3): runs EmeraudeStartupFaultInjection once per service name, the
# startup of that service failing on purpose, and prints one line per run: the exit code, the duration, whether the
# injection hit, the validation messages, the ASan errors and LeakSanitizer reports. Every run must exit cleanly (1, 97, or
# 0 when the engine goes on without the service), quickly (no hang), without a leak or a VUID.
#
# Usage: startup-fault-sweep.sh <Release directory of an ASan + EMERAUDE_FAULT_INJECTION build> <scratch directory> [jobs]
# The service names come from a reference run (every "service up" line of the main process), resource containers
# excepted (their initialize() cannot fail). Linux: lavapipe (VK_DRIVER_FILES) keeps the run deterministic and the
# NVIDIA driver's own exit-time allocation out of LeakSanitizer. Procedure: engine
# docs/subsystems/source-tree/08-startup-fault-injection.md.

set -u

RELEASE=${1:?the Release directory of the fault-injection build}
SCRATCH=${2:?a scratch directory}
JOBS=${3:-4}
TOOL="$RELEASE/EmeraudeStartupFaultInjection"
LVP=/usr/share/vulkan/icd.d/lvp_icd.json

[ -x "$TOOL" ] || { echo "No $TOOL (configure with -DEMERAUDE_FAULT_INJECTION=On)." >&2; exit 2; }
mkdir -p "$SCRATCH"

run () {
	# run <tag> [extra arguments...]: one run in its own directories; prints its log path.
	local tag=$1; shift
	local dir="$SCRATCH/runs/$tag"
	mkdir -p "$dir/config" "$dir/cache"
	(
		cd "$RELEASE" || exit 2
		VK_DRIVER_FILES=$LVP ALSOFT_DRIVERS=null \
		ASAN_OPTIONS="detect_leaks=1:halt_on_error=1" \
		LSAN_OPTIONS="suppressions=$RELEASE/lsan_suppressions.txt:print_suppressions=0" \
			timeout -s KILL 120 "$TOOL" --config-directory="$dir/config" --cache-directory="$dir/cache" \
			--set-vk-layers=VK_LAYER_KHRONOS_validation "$@" > "$dir/run.log" 2>&1
		echo $? > "$dir/exit"
	)
	echo "$dir"
}

# The reference run: every service up, a clean exit, and the list of names.
REF=$(run reference)
if [ "$(cat "$REF/exit")" != 0 ]; then
	echo "The reference run failed (exit $(cat "$REF/exit")): $REF/run.log" >&2
	exit 1
fi
sed 's/\x1b\[[0-9;]*m//g' "$REF/run.log" | grep -oE '\]\[[A-Za-z]+\] *[A-Za-z0-9]+Service (primary |secondary |user )?service up' \
	| sed -E 's/^\]\[[A-Za-z]+\] *//; s/ (primary |secondary |user )?service up$//' | awk '!seen[$0]++' > "$SCRATCH/services.txt"
echo "$(wc -l < "$SCRATCH/services.txt") services to inject." >&2

one () {
	local name=$1
	local start end dir
	start=$(date +%s.%N)
	dir=$(run "$name" "--inject-startup-failure=$name")
	end=$(date +%s.%N)
	local log="$dir/run.log"
	LC_NUMERIC=C printf "%-34s exit=%-4s %6.2fs hit=%s validation=%s asan=%s lsan=%s\n" "$name" "$(cat "$dir/exit")" \
		"$(echo "$end - $start" | bc)" "$(grep -c 'fails on purpose' "$log")" \
		"$(grep -c -F -e '[VALIDATION][ERROR]' -e '[VALIDATION][WARNING]' "$log")" \
		"$(grep -c 'ERROR: AddressSanitizer' "$log")" "$(grep -c 'ERROR: LeakSanitizer' "$log")"
}
export -f run one
export RELEASE SCRATCH TOOL LVP

tr '\n' '\0' < "$SCRATCH/services.txt" | xargs -0 -P "$JOBS" -I{} bash -c 'one "$1"' _ {}
