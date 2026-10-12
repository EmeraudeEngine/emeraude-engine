#!/usr/bin/env python3
"""
Emeraude Engine - path encoding guard (rule C, engine docs/todo/windows-path-string-ansi-pass.md).

On Windows a std::filesystem::path is UTF-16 and its NARROW conversions go through the ANSI code page: path::string()
THROWS on a character outside it (an abort, exceptions are off) and writes invalid UTF-8 for the others; a path BUILT
from a narrow string decodes that string as ANSI, not UTF-8. The cascade converts explicitly: Base::IO::u8path() in,
Base::IO::toU8String() / toGenericU8String() out, the native / wide string to a Windows or CEF API. This tool finds
every other conversion.

    python3 tools/check-path-encoding.py --build-dir <release build> --build-dir <debug build> [--root <src dir>]... [--grep-only] [--verbose]

Two layers:
  1. clang-query over <build>/compile_commands.json (the AUTHORITY: it sees implicit conversions, `dir / text`):
     - narrow-in: a std::filesystem::path constructed from std::string / std::string_view / char * / char[];
       a string literal and a constexpr constant are exempt (author-controlled compile-time text, see layer 2);
     - narrow-out: path::string() / generic_string() / u8string() / generic_u8string(), and std's
       operator<< (ostream, path);
     a POSIX-only file (`*.linux.*`, `*.mac.*`, a `linux/` or `mac/` directory) is skipped: its native narrow string
     IS UTF-8.
  2. A grep gate (fast, no build needed): `.string()`-family calls and `std::filesystem::path{...}` / `(...)` on a
     non-literal in the source files no database compiles (another OS: POSIX-only files are skipped, their native
     string IS UTF-8), and a NON-ASCII narrow string literal on a path line in every file (layer 1 exempts literals).
     With --grep-only it scans every non-POSIX file: a coarse quick net, layer 1 decides.
The roots default to the engine's src/ and emeraude-base's src/; an application adds its own with --root.

A legitimate site goes in tools/path-encoding-allowlist.txt, one per line:
    <path suffix> | <text of the source line> | <one-line reason>

Run it before every push that touches C++ (the documented pre-push step, like tools/mcp-conformance.py).
Exit status: 0 when no finding is left outside the allowlist, 1 otherwise, 2 on a usage error.
"""

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ENGINE = os.path.dirname(HERE)
DEFAULT_ROOTS = [os.path.join(ENGINE, "src"), os.path.join(ENGINE, "dependencies", "emeraude-base", "src")]
DEFAULT_ALLOWLIST = os.path.join(HERE, "path-encoding-allowlist.txt")
SOURCE_EXTENSIONS = (".cpp", ".hpp", ".h", ".cc", ".mm", ".inl")
UNIT_EXTENSIONS = (".cpp", ".cc", ".mm")
# A file compiled on Linux / macOS only: its native narrow string IS UTF-8, path::string() is right there.
POSIX_ONLY = re.compile(r"\.(?:linux|mac)\.|[\\/](?:linux|mac)[\\/]", re.IGNORECASE)


def _type(inner: str) -> str:
    return f"hasType(hasCanonicalType({inner}))"


# NOTE: anyOf() INSIDE hasCanonicalType() silently drops the pointer / array branches (clang-query 23, dynamic matchers):
# one hasType(hasCanonicalType(...)) per branch, under an expression-level anyOf() (engine docs/caution-points.md).
NARROW = "anyOf(" + ", ".join([
    _type('hasDeclaration(classTemplateSpecializationDecl(anyOf(hasName("::std::basic_string"), hasName("::std::basic_string_view")), '
        'hasTemplateArgument(0, refersToType(asString("char")))))'),
    _type('pointerType(pointee(asString("const char")))'),
    _type('pointerType(pointee(asString("char")))'),
    _type('arrayType(hasElementType(asString("const char")))'),
    _type('arrayType(hasElementType(asString("char")))'),
]) + ")"
EXEMPT = "anyOf(stringLiteral(), declRefExpr(to(varDecl(isConstexpr()))))"
PATH_CLASS = 'cxxRecordDecl(hasName("::std::filesystem::path"))'
MATCHERS = {
    "narrow-in": (
        f'cxxConstructExpr(hasDeclaration(cxxConstructorDecl(ofClass(hasName("::std::filesystem::path")))), '
        f'hasArgument(0, ignoringImplicit(expr({NARROW}, unless({EXEMPT})))))'
    ),
    # NOTE: path's member templates convert a narrow Source too, without any constructor call: `p = text`,
    # `p /= text`, `p += text`, assign / append / concat / replace_filename / replace_extension.
    "narrow-assign": (
        f'cxxOperatorCallExpr(hasAnyOverloadedOperatorName("=", "/=", "+="), callee(cxxMethodDecl(ofClass({PATH_CLASS}))), '
        f'hasArgument(1, ignoringImplicit(expr({NARROW}, unless({EXEMPT})))))'
    ),
    "narrow-member": (
        f'cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("assign", "append", "concat", "replace_filename", "replace_extension"), '
        f'ofClass({PATH_CLASS}))), hasArgument(0, ignoringImplicit(expr({NARROW}, unless({EXEMPT})))))'
    ),
    "narrow-out": (
        f'cxxMemberCallExpr(callee(cxxMethodDecl(hasAnyName("string", "generic_string", "u8string", "generic_u8string"), '
        f'ofClass({PATH_CLASS}))))'
    ),
    # NOTE: std's operator<< (ostream, path) is a hidden friend of path, in std::filesystem: isInStdNamespace() does not
    # match a nested namespace. Every `<<` of a path is flagged but emeraude-base's BlobTrait overload (UTF-8, quoted),
    # which the Trace* helpers inherit.
    "std-operator<<": (
        f'cxxOperatorCallExpr(hasOverloadedOperatorName("<<"), '
        f'unless(callee(cxxMethodDecl(ofClass(hasName("::EmEn::Base::BlobTrait"))))), '
        f'hasArgument(1, ignoringImplicit(expr({_type("hasDeclaration(" + PATH_CLASS + ")")}))))'
    ),
}
DIAGNOSTIC = re.compile(r"^(?P<file>.+?)[(:](?P<line>\d+)[,:](?P<column>\d+)\)?: note: \"(?P<kind>[^\"]+)\" binds here")

GREP_RULES = [
    ("grep:string()", re.compile(r"\.(?:generic_)?(?:u8)?string\(\)")),
    ("grep:path-ctor", re.compile(r"std::filesystem::path\s*[{(]\s*(?![\"')}]|u8\")")),
]
NON_ASCII_LITERAL = re.compile(rb'(?<![uUL8R])"(?:[^"\\\n]|\\.)*[\x80-\xff](?:[^"\\\n]|\\.)*"')
PATH_LINE = re.compile(rb"filesystem::path|fs::path|u8path\(|/ \"")


def normalized(path: str) -> str:
    return os.path.normcase(os.path.realpath(path))


def load_allowlist(path: str):
    entries = []

    if not os.path.isfile(path):
        return entries

    with open(path, encoding="utf-8") as allowlist:
        for number, raw in enumerate(allowlist, 1):
            line = raw.strip()

            if not line or line.startswith("#"):
                continue

            parts = [part.strip() for part in line.split("|")]

            if len(parts) != 3 or not all(parts):
                print(f"{path}:{number}: an entry is '<path suffix> | <source line text> | <reason>'", file=sys.stderr)
                sys.exit(2)

            entries.append((parts[0].replace("\\", "/").lower(), parts[1], parts[2]))

    return entries


def allowed(entries, file: str, text: str) -> bool:
    unix = file.replace("\\", "/").lower()

    return any(unix.endswith(suffix) and fragment in text for suffix, fragment, _ in entries)


def source_line(file: str, number: int) -> str:
    try:
        with open(file, encoding="utf-8", errors="replace") as source:
            for index, line in enumerate(source, 1):
                if index == number:
                    return line.strip()
    except OSError:
        pass

    return ""


def find_clang_query(explicit: str):
    if explicit:
        return explicit

    for candidate in ("clang-query", "clang-query-21", "clang-query-20", "clang-query-19"):
        if found := shutil.which(candidate):
            return found

    windows = r"C:\Program Files\LLVM\bin\clang-query.exe"

    return windows if os.path.isfile(windows) else None


def compiled_units(builds, roots):
    """The translation units of every build's compile_commands.json under the roots: {unit: build directory}."""
    units = {}

    for build in builds:
        with open(os.path.join(build, "compile_commands.json"), encoding="utf-8") as database:
            for entry in json.load(database):
                unit = normalized(os.path.join(entry["directory"], entry["file"]))

                if any(unit.startswith(root) for root in roots):
                    units.setdefault(unit, []).append(build)

    return units


def run_clang_query(clang_query: str, units, roots, jobs: int, verbose: bool):
    script = ["set output diag"] + [f"match {matcher}.bind(\"{kind}\")" for kind, matcher in MATCHERS.items()]
    # NOTE: one run per (unit, build): a Release and a Debug database see different #if branches.
    jobs_list = [(unit, build) for unit, builds in sorted(units.items()) for build in builds]

    def query(job):
        unit, build = job
        command = [clang_query, "-p", build, "--extra-arg=-Wno-everything", "--extra-arg=-Wno-error"]

        for line in script:
            command += ["-c", line]

        result = subprocess.run(command + [unit], capture_output=True, text=True, errors="replace")
        hits = []

        for line in result.stdout.splitlines():
            # NOTE: clang-query also binds every match as "root": that line repeats the named one.
            if (match := DIAGNOSTIC.match(line)) and match["kind"] != "root":
                hits.append((normalized(match["file"]), int(match["line"]), match["kind"]))

        return unit, hits, result.returncode, result.stderr

    findings = {}
    failed = []

    with ThreadPoolExecutor(max_workers=jobs) as pool:
        for unit, hits, code, error in pool.map(query, jobs_list):
            if code != 0 and not hits:
                failed.append((unit, error.strip().splitlines()[-1:] if error.strip() else ["no output"]))

            for file, line, kind in hits:
                # NOTE: A POSIX-only file is compiled by a Linux / macOS database only: its native narrow string IS
                # UTF-8, the same exemption as the grep gate's.
                if any(file.startswith(root) for root in roots) and not POSIX_ONLY.search(file):
                    findings[(file, line, kind)] = None

    if verbose:
        print(f"clang-query: {len(jobs_list)} runs over {len(units)} translation units, {len(failed)} failed")

    return findings, failed


def run_grep(roots, compiled):
    """The quick net. The `.string()` and path-constructor rules cover the SOURCE files no database compiles (another
    OS, an optional module) and skip the POSIX-only ones (their native narrow string IS UTF-8); with --grep-only, every
    non-POSIX file (coarse: it cannot see types). The non-ASCII literal rule covers every file."""
    findings = {}

    for root in roots:
        for directory, _, files in os.walk(root):
            for name in files:
                if not name.endswith(SOURCE_EXTENSIONS):
                    continue

                file = normalized(os.path.join(directory, name))
                structural = not POSIX_ONLY.search(file) and file not in compiled and (not compiled or name.endswith(UNIT_EXTENSIONS))

                with open(file, "rb") as source:
                    for number, raw in enumerate(source, 1):
                        text = raw.decode("utf-8", errors="replace")
                        stripped = text.lstrip()

                        if stripped.startswith(("//", "*", "/*")):
                            continue

                        if structural:
                            for kind, rule in GREP_RULES:
                                if rule.search(text):
                                    findings[(file, number, kind)] = None

                        if NON_ASCII_LITERAL.search(raw) and PATH_LINE.search(raw):
                            findings[(file, number, "grep:non-ascii-literal")] = None

    return findings


def main() -> int:
    parser = argparse.ArgumentParser(description="Path encoding guard (rule C).")
    parser.add_argument("--build-dir", action="append", default=[], help="A build directory holding compile_commands.json (repeatable: Release AND Debug; needed unless --grep-only).")
    parser.add_argument("--root", action="append", default=[], help="An extra source directory to check (repeatable).")
    parser.add_argument("--allowlist", default=DEFAULT_ALLOWLIST, help="The allowlist file.")
    parser.add_argument("--clang-query", default="", help="The clang-query executable (default: found on PATH).")
    parser.add_argument("--jobs", type=int, default=os.cpu_count() or 4, help="Parallel clang-query processes.")
    parser.add_argument("--grep-only", action="store_true", help="Only the fast grep gate (no build needed; coarse).")
    parser.add_argument("--verbose", action="store_true", help="Also print the allowlisted findings.")
    arguments = parser.parse_args()

    roots = [normalized(root) for root in DEFAULT_ROOTS + arguments.root if os.path.isdir(root)]
    allowlist = load_allowlist(arguments.allowlist)
    units = {}
    failed = []

    if not arguments.grep_only:
        builds = [build for build in arguments.build_dir if os.path.isfile(os.path.join(build, "compile_commands.json"))]

        if not builds or len(builds) != len(arguments.build_dir):
            print("every --build-dir must hold compile_commands.json (CMAKE_EXPORT_COMPILE_COMMANDS=ON), or use --grep-only.", file=sys.stderr)
            return 2

        clang_query = find_clang_query(arguments.clang_query)

        if clang_query is None:
            print("clang-query not found: install LLVM, pass --clang-query, or use --grep-only.", file=sys.stderr)
            return 2

        units = compiled_units(builds, roots)

    findings = run_grep(roots, set(units))

    if not arguments.grep_only:
        semantic, failed = run_clang_query(clang_query, units, roots, arguments.jobs, arguments.verbose)
        findings.update(semantic)

    open_findings = 0
    allowlisted = 0

    for (file, line, kind) in sorted(findings):
        text = source_line(file, line)

        if allowed(allowlist, file, text):
            allowlisted += 1

            if arguments.verbose:
                print(f"allowed  {file}:{line}: {kind}: {text}")

            continue

        open_findings += 1
        print(f"{file}:{line}: {kind}: {text}")

    for unit, error in failed:
        print(f"clang-query FAILED on {unit}: {' '.join(error)}")

    print(f"{open_findings} finding(s), {allowlisted} allowlisted, {len(failed)} translation unit(s) failed.")

    return 0 if open_findings == 0 and not failed else 1


if __name__ == "__main__":
    sys.exit(main())
