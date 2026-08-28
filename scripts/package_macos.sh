#!/usr/bin/env bash
set -euo pipefail

# Pacchettizza gli artefatti macOS gia' buildati (VST3/AU/Standalone) in
# uno zip pronto per una distribuzione ad-hoc (issue #7).
#
# NON firma con un Developer ID reale e NON notarizza: usa la firma ad-hoc
# che CMake/JUCE applica gia' in fase di build (vedi CMakeLists.txt,
# COPY_PLUGIN_AFTER_BUILD). Uno zip cosi' si installa senza problemi sulla
# stessa macchina, ma su un'altra macchina con Gatekeeper attivo l'utente
# dovra' approvarlo manualmente (clic destro > Apri, o rimuovere la
# quarantena con xattr) finche' non viene firmato con un Developer ID reale
# e notarizzato — vedi scripts/notarize_macos.sh.
#
# Uso:
#   cmake --build build --config Release
#   ./scripts/package_macos.sh [build_dir]

cd "$(dirname "$0")/.."

BUILD_DIR="${1:-build}"
ARTEFACTS_DIR="$BUILD_DIR/Livellatore_artefacts"

if [ ! -d "$ARTEFACTS_DIR" ]; then
    echo "Artefatti non trovati in $ARTEFACTS_DIR" >&2
    echo "Builda prima con: cmake --build $BUILD_DIR --config Release" >&2
    exit 1
fi

VERSION=$(grep -m1 'project(Livellatore VERSION' CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/')
PACKAGE_NAME="Livellatore-${VERSION}-macOS"
OUTPUT_ZIP="${PACKAGE_NAME}.zip"
STAGING_DIR=$(mktemp -d)
STAGING_PACKAGE_DIR="$STAGING_DIR/$PACKAGE_NAME"

mkdir -p "$STAGING_PACKAGE_DIR"

found_any=0
for item in "VST3/Livellatore.vst3" "AU/Livellatore.component" "Standalone/Livellatore.app"; do
    src="$ARTEFACTS_DIR/$item"
    if [ -e "$src" ]; then
        cp -R "$src" "$STAGING_PACKAGE_DIR/"
        found_any=1
    fi
done

if [ "$found_any" -eq 0 ]; then
    echo "Nessun artefatto trovato (VST3/AU/Standalone) in $ARTEFACTS_DIR" >&2
    rm -rf "$STAGING_DIR"
    exit 1
fi

cp README.md "$STAGING_PACKAGE_DIR/"

rm -f "$OUTPUT_ZIP"
(cd "$STAGING_DIR" && zip -r -q -y "$OLDPWD/$OUTPUT_ZIP" "$PACKAGE_NAME")
rm -rf "$STAGING_DIR"

echo "Creato $OUTPUT_ZIP"
unzip -l "$OUTPUT_ZIP"
