#!/usr/bin/env python3
"""update-glfw.py

Syncs the GLFW fork (EmeraudeEngine/glfw) with the official upstream
repository and rebases the custom branch on top.

Steps:
  1. fetch upstream (https://github.com/glfw/glfw.git)
  2. fast-forward local `master` to `upstream/master`
  3. rebase `em/customization` onto `master`
  4. push `master` and force-push (with lease) `em/customization` to origin

Works whether dependencies/glfw is a plain clone or a git submodule of the
engine (detached HEAD). In the submodule case the script ends on the rebased
`em/customization` tip, ready for the parent repo to re-pin the gitlink.
Cross-platform: Windows, Linux, macOS.
"""

import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
GLFW_DIR = SCRIPT_DIR / "dependencies" / "glfw"
UPSTREAM_URL = "https://github.com/glfw/glfw.git"
CUSTOM_BRANCH = "em/customization"


def git(*args: str, capture: bool = False, check: bool = True) -> subprocess.CompletedProcess:
    """Run a git command inside the GLFW directory."""
    return subprocess.run(
        ["git", "-C", str(GLFW_DIR), *args],
        capture_output=capture,
        text=True,
        check=check,
    )


def git_output(*args: str) -> str:
    """Run a git command and return its stripped stdout."""
    return git(*args, capture=True).stdout.strip()


def main() -> int:
    # .git is a directory for a plain clone, but a gitlink FILE when glfw is
    # a submodule of the engine — accept both by asking git itself.
    probe = git("rev-parse", "--git-dir", capture=True, check=False)
    if probe.returncode != 0:
        print(f"Error: {GLFW_DIR} is not a git repository.", file=sys.stderr)
        return 1

    # Ensure the upstream remote exists.
    if git("remote", "get-url", "upstream", capture=True, check=False).returncode != 0:
        print("Adding upstream remote...")
        git("remote", "add", "upstream", UPSTREAM_URL)

    print("Fetching upstream...")
    git("fetch", "upstream")

    # Empty when HEAD is detached — the normal state when glfw is checked out
    # as a submodule pinned to a commit; in that case we end on CUSTOM_BRANCH
    # (the tip the parent repo should re-pin to).
    original_branch = git_output("branch", "--show-current")

    behind = git_output("rev-list", "--count", "master..upstream/master")
    print(f"Updating master ({behind} new upstream commit(s))...")
    git("checkout", "master")
    git("merge", "--ff-only", "upstream/master")

    print(f"Rebasing {CUSTOM_BRANCH} onto master...")
    git("checkout", CUSTOM_BRANCH)
    rebase = git("rebase", "master", check=False)
    if rebase.returncode != 0:
        print(
            f"Error: rebase of {CUSTOM_BRANCH} hit conflicts.\n"
            f"Resolve them in {GLFW_DIR} then run 'git rebase --continue',\n"
            f"or abort with 'git rebase --abort'. Nothing has been pushed.",
            file=sys.stderr,
        )
        return 1

    print("Pushing to origin...")
    git("push", "origin", "master")
    git("push", "--force-with-lease", "origin", CUSTOM_BRANCH)

    # Restore the original branch if there was one and it differs.
    if original_branch and original_branch != CUSTOM_BRANCH:
        git("checkout", original_branch)

    tip = git_output("log", "--oneline", "-1", CUSTOM_BRANCH)
    print(f"Done. {CUSTOM_BRANCH} tip: {tip}")
    if not original_branch:
        print(
            "Note: glfw is checked out as a submodule — remember to commit the "
            "updated gitlink in the parent repository."
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())