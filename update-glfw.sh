#!/bin/bash
# update-glfw.sh
# Syncs the GLFW fork with the official upstream repository
# and rebases the em/customization branch on top.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GLFW_DIR="$SCRIPT_DIR/dependencies/glfw"
UPSTREAM_URL="https://github.com/glfw/glfw.git"
CUSTOM_BRANCH="em/customization"

if [ ! -d "$GLFW_DIR/.git" ]; then
	echo "Error: $GLFW_DIR is not a git repository."
	exit 1
fi

cd "$GLFW_DIR"

# Ensure upstream remote exists.
if ! git remote get-url upstream &>/dev/null; then
	echo "Adding upstream remote..."
	git remote add upstream "$UPSTREAM_URL"
fi

# Fetch upstream.
echo "Fetching upstream..."
git fetch upstream

# Save current branch to restore it later.
ORIGINAL_BRANCH="$(git branch --show-current)"

# Update master from upstream.
echo "Updating master..."
git checkout master
git merge upstream/master

# Rebase custom branch.
echo "Rebasing $CUSTOM_BRANCH onto master..."
git checkout "$CUSTOM_BRANCH"
git rebase master

# Push both branches.
echo "Pushing to origin..."
git push origin master
git push --force-with-lease origin "$CUSTOM_BRANCH"

# Restore original branch if different.
if [ "$ORIGINAL_BRANCH" != "$CUSTOM_BRANCH" ]; then
	git checkout "$ORIGINAL_BRANCH"
fi

echo "Done."
