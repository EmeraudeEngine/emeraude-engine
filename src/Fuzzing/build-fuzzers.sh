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

# The analysis TU of each target: its flags come from the regular build.
declare -A ANALYSIS=(
	[fuzz_wad]="src/Scenes/Loaders/WADParser.cpp"
	[fuzz_fbx]="src/Scenes/Loaders/FBXParser.cpp"
)

# The external static libraries of each target (found beside the include directory holding <name>/<name>.h).
declare -A EXTERNAL=(
	[fuzz_fbx]="ufbx"
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
	source="${ANALYSIS[${target}]}"
	eval "flags=($(flagsOf "${source}"))"

	externals=()

	for name in ${EXTERNAL[${target}]:-}; do
		library=""

		for (( i = 0; i < ${#flags[@]} - 1; ++i )); do
			if [ "${flags[i]}" = "-isystem" ] && [ -f "${flags[i + 1]}/${name}/${name}.h" ] && [ -f "${flags[i + 1]}/../lib/lib${name}.a" ]; then
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

	echo "building ${target} (${source})"

	"${CXX}" "${flags[@]}" -fno-exceptions -g -O1 -fsanitize=fuzzer,address,undefined \
		"${ROOT}/${source}" "${ROOT}/src/Fuzzing/${target}.cpp" \
		-L"${LIBRARY_DIR}" -lEmeraude -Wl,-rpath,"${LIBRARY_DIR}" "${BASE_LIBRARY}" "${externals[@]}" \
		-o "${OUT_DIR}/${target}"
done

echo "built: ${OUT_DIR}"
