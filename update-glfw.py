#!/usr/bin/env python3
"""update-glfw.py

Syncs the GLFW fork (EmeraudeEngine/glfw) with the official upstream
repository and rebases the custom branch on top of a chosen reference.

Steps:
  1. fetch upstream (https://github.com/glfw/glfw.git) and origin, tags included
  2. resolve the reference to rebase onto (--onto, default: latest upstream tag)
  3. reconcile the local `em/customization` with origin's
  4. fast-forward the local `master` mirror to `upstream/master`
  5. rebase `em/customization` onto the target reference
  6. verify the patch survived (the marker, not the build) — refuses to push otherwise
  7. push `master` and force-push (with lease) `em/customization` to origin

The default target is the LATEST UPSTREAM TAG, i.e. a release. It is deliberately
not `upstream/master`: right after a release, upstream's tip is the next version's
development branch ('Start 3.6' bumps the project version to 3.6.0), which is never
what a submodule pin wants. Pass `--onto upstream/master` for the bleeding edge, or
`--onto <committish>` for anything else.

Works whether dependencies/glfw is a plain clone or a git submodule of the
engine (detached HEAD). In the submodule case the script ends on the rebased
`em/customization` tip, ready for the parent repo to re-pin the gitlink.
Cross-platform: Windows, Linux, macOS.
"""

import argparse
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
GLFW_DIR = SCRIPT_DIR / "dependencies" / "glfw"
UPSTREAM_URL = "https://github.com/glfw/glfw.git"
MIRROR_BRANCH = "master"
CUSTOM_BRANCH = "em/customization"
CUSTOM_HEADER = "include/GLFW/glfw3.h"
CUSTOM_MARKER = "GLFW_EM_CUSTOM_VERSION"


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


def ref_exists(ref: str) -> bool:
    """Tell whether a reference resolves in the GLFW repository."""
    return git("rev-parse", "--verify", "--quiet", f"{ref}^{{commit}}", capture=True, check=False).returncode == 0


def remote_exists(remote: str) -> bool:
    """Tell whether a remote is configured in the GLFW repository."""
    return git("remote", "get-url", remote, capture=True, check=False).returncode == 0


def fail(message: str) -> None:
    """Report an error and terminate."""
    print(f"Error: {message}", file=sys.stderr)
    raise SystemExit(1)


def parse_arguments() -> argparse.Namespace:
    """Parse the command line."""
    parser = argparse.ArgumentParser(description="Update the GLFW fork and rebase the custom patch on top of it.")
    parser.add_argument(
        "--onto",
        default=None,
        metavar="REF",
        help=f"reference to rebase '{CUSTOM_BRANCH}' onto: a tag ('3.5.1') pins a release, a branch "
             f"follows it ('upstream/{MIRROR_BRANCH}' = bleeding edge). Default: the latest upstream tag",
    )
    parser.add_argument(
        "--from",
        dest="source",
        choices=("local", "origin"),
        help=f"which '{CUSTOM_BRANCH}' to rebase when the local branch and origin's have diverged",
    )
    parser.add_argument(
        "--no-push",
        action="store_true",
        help="do everything locally and push nothing (review before publishing)",
    )
    return parser.parse_args()


def repoint_branch(branch: str, target: str) -> None:
    """Move a local branch onto a target, whether or not it is the checked out one.

    'git branch --force' refuses to move the current branch, which is exactly the
    state a submodule is left in by a previous run of this script. The working tree
    was checked for tracked modifications before anything ran, so the reset is safe.
    """
    if git_output("branch", "--show-current") == branch:
        git("reset", "--hard", target)
    else:
        git("branch", "--force", branch, target)


def latest_upstream_tag() -> str | None:
    """Return the most recent tag reachable from the upstream tip, if any."""
    result = git("describe", "--tags", "--abbrev=0", f"upstream/{MIRROR_BRANCH}", capture=True, check=False)
    return result.stdout.strip() if result.returncode == 0 else None


def resolve_target(onto: str | None) -> str:
    """Resolve, and validate, the reference the custom patch is rebased onto.

    Done before anything is moved: a typo must not leave the repository displaced.
    """
    if onto is not None:
        if not ref_exists(onto):
            fail(f"'{onto}' does not resolve to a commit in {GLFW_DIR}.")
        return onto

    tag = latest_upstream_tag()
    if tag is None:
        fail(f"no tag is reachable from upstream/{MIRROR_BRANCH}; pass --onto explicitly.")

    print(f"Target: {tag} (latest upstream tag).")
    return tag


def select_source_branch(source: str | None) -> bool:
    """Reconcile the local custom branch with origin's, and tell whether it moved.

    Returns True when the local branch was repointed to origin's tip.
    """
    remote = f"origin/{CUSTOM_BRANCH}"

    if not ref_exists(remote):
        return False

    if not ref_exists(CUSTOM_BRANCH):
        print(f"Local {CUSTOM_BRANCH} is missing, creating it from {remote}...")
        git("branch", CUSTOM_BRANCH, remote)
        return True

    local_tip = git_output("rev-parse", CUSTOM_BRANCH)
    remote_tip = git_output("rev-parse", remote)

    if local_tip == remote_tip:
        return False

    if git("merge-base", "--is-ancestor", CUSTOM_BRANCH, remote, check=False).returncode == 0:
        print(f"Local {CUSTOM_BRANCH} is behind {remote}, fast-forwarding it...")
        repoint_branch(CUSTOM_BRANCH, remote)
        return True

    # Ahead of origin is ordinary unpushed work, not a divergence: nothing to choose.
    if git("merge-base", "--is-ancestor", remote, CUSTOM_BRANCH, check=False).returncode == 0:
        ahead = git_output("rev-list", "--count", f"{remote}..{CUSTOM_BRANCH}")
        print(f"Local {CUSTOM_BRANCH} is ahead of {remote} ({ahead} unpushed commit(s)), rebasing it as it is.")
        return False

    if source is None:
        print(
            f"Error: local {CUSTOM_BRANCH} ({local_tip[:8]}) and {remote} ({remote_tip[:8]}) have diverged.\n"
            f"Compare them, then choose explicitly:\n"
            f"  --from local   rebase the local branch as it is\n"
            f"  --from origin  discard the local tip and rebase origin's\n"
            f"No local branch has been moved (only remote-tracking refs were refreshed).",
            file=sys.stderr,
        )
        raise SystemExit(1)

    if source == "origin":
        print(f"Discarding local {CUSTOM_BRANCH} tip {local_tip[:8]} in favour of {remote} ({remote_tip[:8]}).")
        print(f"  The abandoned tip stays reachable as {local_tip} until git prunes it.")
        repoint_branch(CUSTOM_BRANCH, remote)
        return True

    print(f"Rebasing the local {CUSTOM_BRANCH} ({local_tip[:8]}), ignoring {remote} ({remote_tip[:8]}).")
    return False


def update_mirror_branch() -> None:
    """Point the local mirror branch at the upstream tip, creating it when absent.

    A freshly initialised submodule sits on a detached HEAD with no local branch at
    all, so the branch is created rather than assumed.
    """
    remote = f"upstream/{MIRROR_BRANCH}"

    if not ref_exists(MIRROR_BRANCH):
        print(f"Local {MIRROR_BRANCH} is missing, creating it from {remote}...")
        git("branch", MIRROR_BRANCH, remote)
        return

    if git_output("rev-parse", MIRROR_BRANCH) == git_output("rev-parse", remote):
        print(f"Local {MIRROR_BRANCH} already matches {remote}.")
        return

    if git("merge-base", "--is-ancestor", MIRROR_BRANCH, remote, check=False).returncode != 0:
        fail(
            f"local {MIRROR_BRANCH} has diverged from {remote} and cannot be fast-forwarded.\n"
            f"It is meant to be a strict mirror of upstream: inspect it, then reset it with\n"
            f"  git -C {GLFW_DIR} branch --force {MIRROR_BRANCH} {remote}"
        )

    behind = git_output("rev-list", "--count", f"{MIRROR_BRANCH}..{remote}")
    print(f"Fast-forwarding {MIRROR_BRANCH} ({behind} new upstream commit(s))...")
    repoint_branch(MIRROR_BRANCH, remote)


def patch_id(commit: str) -> str | None:
    """Return the stable patch-id of a commit, or None when it carries no diff.

    A rebase rewrites the SHA of the patch every single time; its patch-id does not
    move. It is therefore the only identity worth printing or comparing.
    """
    show = git("show", commit, capture=True)
    result = subprocess.run(
        ["git", "-C", str(GLFW_DIR), "patch-id", "--stable"],
        input=show.stdout,
        capture_output=True,
        text=True,
        check=True,
    )
    fields = result.stdout.split()
    return fields[0] if fields else None


def verify_customization(target: str) -> None:
    """Prove the custom patch survived the rebase, before anything is published.

    The engine consumes the patch through '#ifdef CUSTOM_MARKER' and falls back to
    per-key polling when it is absent, so a successful build proves NOTHING here —
    the marker is the only evidence. A run that lost the patch (a mis-repointed local
    branch, a rebase turned no-op) must never reach origin, nor a submodule pin.
    """
    count = git_output("rev-list", "--count", f"{target}..{CUSTOM_BRANCH}")
    header = git("show", f"{CUSTOM_BRANCH}:{CUSTOM_HEADER}", capture=True, check=False)

    if header.returncode != 0 or CUSTOM_MARKER not in header.stdout:
        fail(
            f"the rebased {CUSTOM_BRANCH} does not define {CUSTOM_MARKER} in {CUSTOM_HEADER}\n"
            f"({count} commit(s) above {target}): the customisation is GONE. Nothing was pushed.\n"
            f"The engine would still compile — it falls back to per-key polling — so do not\n"
            f"take a successful build as proof. Inspect the branch before retrying."
        )

    print(f"Customisation verified: {CUSTOM_MARKER} present, {count} commit(s) above {target}.")
    for commit in git_output("log", "--reverse", "--format=%H", f"{target}..{CUSTOM_BRANCH}").splitlines():
        identity = patch_id(commit)
        subject = git_output("log", "--format=%s", "-1", commit)
        print(f"  {commit[:8]}  patch-id {identity or '(no diff)'}  {subject}")


def main() -> int:
    arguments = parse_arguments()

    # .git is a directory for a plain clone, but a gitlink FILE when glfw is
    # a submodule of the engine — accept both by asking git itself.
    if git("rev-parse", "--git-dir", capture=True, check=False).returncode != 0:
        fail(f"{GLFW_DIR} is not a git repository.")

    # Branches get checked out and repointed below: a tracked modification would
    # either block the checkout or be carried across branches. Untracked files are
    # ignored, exactly like git itself does.
    if git_output("status", "--porcelain", "--untracked-files=no"):
        fail(f"{GLFW_DIR} has uncommitted changes to tracked files. Commit or clean them first.")

    if not remote_exists("upstream"):
        print("Adding upstream remote...")
        git("remote", "add", "upstream", UPSTREAM_URL)

    print("Fetching upstream...")
    git("fetch", "--tags", "upstream")

    # origin/em/customization is authoritative for the patch, and the push below
    # leases against it: deciding on a stale remote-tracking ref is not an option.
    if remote_exists("origin"):
        print("Fetching origin...")
        git("fetch", "--tags", "origin")

    target = resolve_target(arguments.onto)

    # Empty when HEAD is detached — the normal state when glfw is checked out
    # as a submodule pinned to a commit; in that case we end on CUSTOM_BRANCH
    # (the tip the parent repo should re-pin to).
    original_branch = git_output("branch", "--show-current")

    source_moved = select_source_branch(arguments.source)
    update_mirror_branch()

    print(f"Rebasing {CUSTOM_BRANCH} onto {target} ({git_output('log', '--oneline', '-1', target)})...")
    git("checkout", CUSTOM_BRANCH)
    rebase = git("rebase", target, check=False)
    if rebase.returncode != 0:
        print(
            f"Error: rebase of {CUSTOM_BRANCH} hit conflicts.\n"
            f"Resolve them in {GLFW_DIR} then run 'git rebase --continue',\n"
            f"or abort with 'git rebase --abort'. Nothing has been pushed.",
            file=sys.stderr,
        )
        return 1

    verify_customization(target)

    if arguments.no_push:
        print("Skipping the push (--no-push).")
    else:
        print("Pushing to origin...")
        git("push", "origin", MIRROR_BRANCH)
        git("push", "--force-with-lease", "origin", CUSTOM_BRANCH)

    # Restore the original branch if there was one and it differs.
    if original_branch and original_branch != CUSTOM_BRANCH:
        git("checkout", original_branch)

    tip = git_output("log", "--oneline", "-1", CUSTOM_BRANCH)
    print(f"Done. {CUSTOM_BRANCH} tip: {tip}")
    if source_moved and arguments.no_push:
        print(f"Note: the local {CUSTOM_BRANCH} was repointed before the rebase and nothing was pushed.")
    if not original_branch:
        print(
            "Note: glfw is checked out as a submodule — remember to commit the "
            "updated gitlink in the parent repository."
        )
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as error:
        # Every unforeseen git failure lands here instead of a Python traceback.
        print(f"Error: '{' '.join(error.cmd)}' exited with code {error.returncode}.", file=sys.stderr)
        if error.stderr:
            print(error.stderr.strip(), file=sys.stderr)
        sys.exit(1)
