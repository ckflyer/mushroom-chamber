#!/usr/bin/env bash
# Cut a release.
#
#   ./release.sh 1.2.0  "fixed the thing"
#
# Bumps FW_VERSION in src/config.h so the dashboard shows the right version,
# commits everything, pushes, tags, and pushes the tag. GitHub Actions then
# builds chamber-firmware.bin and attaches it to the release.
#
# Re-tagging an existing version is handled: the old tag is removed first.

set -e

VERSION="${1:-}"
MESSAGE="${2:-release v${VERSION}}"

if [ -z "$VERSION" ]; then
  echo "usage: ./release.sh <version> [message]"
  echo "   eg: ./release.sh 1.2.0 \"presets and timezone dropdown\""
  exit 1
fi

if ! echo "$VERSION" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+$'; then
  echo "version should look like 1.2.0, got: $VERSION"
  exit 1
fi

echo "==> setting FW_VERSION to $VERSION"
sed -i.bak "s/#define FW_VERSION \".*\"/#define FW_VERSION \"$VERSION\"/" src/config.h
rm -f src/config.h.bak
grep -n FW_VERSION src/config.h

echo "==> committing"
git add -A
if git diff --cached --quiet; then
  echo "    nothing changed, skipping commit"
else
  git commit -m "$MESSAGE"
fi

echo "==> pushing"
git push

echo "==> tagging v$VERSION"
git tag -d "v$VERSION" 2>/dev/null || true
git push origin ":refs/tags/v$VERSION" 2>/dev/null || true
git tag "v$VERSION"
git push origin "v$VERSION"

REPO=$(git remote get-url origin | sed -e 's|.*github.com[:/]||' -e 's|\.git$||')
echo
echo "==> done"
echo "    build:   https://github.com/$REPO/actions"
echo "    release: https://github.com/$REPO/releases/tag/v$VERSION"
echo
echo "    When the build goes green, download chamber-firmware.bin and drop it"
echo "    into the Firmware tab of the chamber dashboard."
