#!/usr/bin/env bash
#
# src/Fuzzing/build-fuzzers.sh — build the emeraude-engine libFuzzer targets (Ave Robustus II P-W, gap 7, 2026-10-18).
#
# Each target compiles the ANALYSIS half of a loader (the parse, split from its resource creation) together with its
# fuzz entry point, with clang, libFuzzer, ASan and UBSan; the rest comes from the engine and base libraries of a
# regular build. The compile flags of the analysis TU are taken from that build's compile_commands.json (its include
# paths and definitions), the GCC-only flags removed. A third-party library the analysis calls directly, hidden inside
# libEmeraude.so, links from the external dependencies of that build (EXTERNAL below).
#
# Prerequisites: a built engine (libEmeraude.so) and base (libEmeraudeBase.a) and a compile_commands.json, in the build
# directory given by BUILD_DIR (a projet-alpha build: .claude-build-release).
#
# Usage:   BUILD_DIR=/path/to/build src/Fuzzing/build-fuzzers.sh
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${BUILD_DIR:?set BUILD_DIR to a build directory holding compile_commands.json, libEmeraude.so and libEmeraudeBase.a}"
CXX="${CXX:-clang++}"
OUT_DIR="${OUT_DIR:-${ROOT}/src/Fuzzing/build}"

LIBRARY_DIR="$(dirname "$(find "${BUILD_DIR}" -name 'libEmeraude.so' -print -quit)")"
BASE_LIBRARY="$(find "${BUILD_DIR}" -name 'libEmeraudeBase.a' -print -quit)"

if [ -z "${LIBRARY_DIR}" ] || [ -z "${BASE_LIBRARY}" ] || [ ! -f "${BUILD_DIR}/compile_commands.json" ]; then
	echo "error: ${BUILD_DIR} lacks libEmeraude.so, libEmeraudeBase.a or compile_commands.json." >&2
	exit 1
fi

# The analysis TUs of each target: their flags come from the regular build (the first one's).
declare -A ANALYSIS=(
	[fuzz_wad]="src/Scenes/Loaders/WADParser.cpp"
	[fuzz_fbx]="src/Scenes/Loaders/FBXParser.cpp"
	[fuzz_usd]="src/Scenes/Loaders/USDParser.cpp src/Scenes/Loaders/USDZArchive.cpp"
)

# The external static libraries of each target, as <header>:<library>: lib<library>.a is found in the lib directory
# beside the include directory holding <header>. LIB_<library>=/path/lib<library>.a in the environment links that one
# instead (an instrumented build of the library, so the fuzzer sees inside it: README.md).
declare -A EXTERNAL=(
	[fuzz_fbx]="ufbx/ufbx.h:ufbx"
	[fuzz_usd]="lightusd/lightusd.hh:lightusd_static"
)

# Extra flags of a target. lightusd is built WITHOUT RTTI (its LIGHTUSD_CXX_RTTI option, off): UBSan's vptr check needs
# RTTI in every object it inspects and reports each lightusd shared_ptr as an "invalid vptr" — a false positive.
declare -A EXTRA_FLAGS=(
	[fuzz_usd]="-fno-sanitize=vptr"
)

flagsOf ()
{
	python3 - "$1" "${BUILD_DIR}/compile_commands.json" <<'PYEOF'
import json, shlex, sys
source, database = sys.argv[1], sys.argv[2]
entry = next(e for e in json.load(open(database)) if e['file'].endswith(source))
arguments = shlex.split(entry['command']) if 'command' in entry else entry['arguments']
keep = []
skip = False
for argument in arguments[1:]:
	if skip:
		keep.append(argument)
		skip = False
		continue
	if argument in ('-isystem', '-include'):
		keep.append(argument)
		skip = True
	elif argument.startswith(('-D', '-I', '-std=')):
		keep.append(argument)
print(' '.join(shlex.quote(a) for a in keep))
PYEOF
}

mkdir -p "${OUT_DIR}"

for target in "${!ANALYSIS[@]}"; do
	read -r -a sources <<< "${ANALYSIS[${target}]}"
	source="${sources[0]}"
	eval "flags=($(flagsOf "${source}"))"

	externals=()

	for external in ${EXTERNAL[${target}]:-}; do
		header="${external%%:*}"
		name="${external##*:}"
		override="LIB_${name}"
		library="${!override:-}"

		if [ -n "${library}" ]; then
			externals+=("${library}")
			continue
		fi

		for (( i = 0; i < ${#flags[@]} - 1; ++i )); do
			if [ "${flags[i]}" = "-isystem" ] && [ -f "${flags[i + 1]}/${header}" ] && [ -f "${flags[i + 1]}/../lib/lib${name}.a" ]; then
				library="${flags[i + 1]}/../lib/lib${name}.a"
				break
			fi
		done

		if [ -z "${library}" ]; then
			echo "error: lib${name}.a not found beside the include paths of ${source}." >&2
			exit 1
		fi

		externals+=("${library}")
	done

	echo "building ${target} (${sources[*]})"

	read -r -a extraFlags <<< "${EXTRA_FLAGS[${target}]:-}"

	"${CXX}" "${flags[@]}" -fno-exceptions -g -O1 -fsanitize=fuzzer,address,undefined "${extraFlags[@]}" \
		"${sources[@]/#/${ROOT}/}" "${ROOT}/src/Fuzzing/${target}.cpp" \
		-L"${LIBRARY_DIR}" -lEmeraude -Wl,-rpath,"${LIBRARY_DIR}" "${BASE_LIBRARY}" "${externals[@]}" \
		-o "${OUT_DIR}/${target}"
done

echo "built: ${OUT_DIR}"
