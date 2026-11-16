#!/bin/bash
# Builds Prisma-<version>.pkg from dist/ (Prisma.vst3, Prisma.component).
# The installer writes into the current user's home only:
#   ~/Library/Audio/Plug-Ins/VST3, ~/Library/Audio/Plug-Ins/Components
# and replaces any version already there.
#   installer/build_pkg.sh <version> [dist dir] [output dir]
set -euo pipefail
VERSION="$1"; DIST="${2:-dist}"; OUT="${3:-.}"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

component() {   # name, bundle, install location, identifier
  local name="$1" bundle="$2" loc="$3" id="$4"
  mkdir -p "$WORK/root-$name"
  cp -R "$DIST/$bundle" "$WORK/root-$name/"
  pkgbuild --analyze --root "$WORK/root-$name" "$WORK/$name.plist" >/dev/null
  plutil -replace 0.BundleIsRelocatable -bool NO "$WORK/$name.plist"
  plutil -replace 0.BundleOverwriteAction -string upgrade "$WORK/$name.plist"
  pkgbuild --root "$WORK/root-$name" --component-plist "$WORK/$name.plist" --install-location "$loc" \
           --identifier "$id" --version "$VERSION" --scripts "$HERE/scripts-plugins" "$WORK/$name.pkg"
}
component vst3 Prisma.vst3      "/Library/Audio/Plug-Ins/VST3"       com.dannyshark.prisma.vst3
component au   Prisma.component "/Library/Audio/Plug-Ins/Components" com.dannyshark.prisma.au

sed "s/@VERSION@/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"
mkdir -p "$OUT"
productbuild --distribution "$WORK/distribution.xml" --resources "$HERE/resources" --package-path "$WORK" "$OUT/Prisma-$VERSION.pkg"
echo "built $OUT/Prisma-$VERSION.pkg"
