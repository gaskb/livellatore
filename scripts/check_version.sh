#!/usr/bin/env bash
set -uo pipefail

# Verifica che la versione dichiarata in CMakeLists.txt corrisponda a
# quella EFFETTIVAMENTE installata in ~/Library/Audio/Plug-Ins (quella che
# vede davvero un DAW come Reaper), non solo a quella nella cartella di
# build.
#
# Nato da un caso reale: dopo un bump di versione, `cmake -B build`
# rigenera subito l'Info.plist/moduleinfo.json nella cartella di build per
# OGNI target (e' un passo di configure, non di build) — ma la copia
# installata in ~/Library/Audio/Plug-Ins si aggiorna solo quando il target
# VST3/AU viene davvero ricompilato E il suo passo COPY_PLUGIN_AFTER_BUILD
# gira. Un `cmake --build build --target LivellatoreTests` (solo i test)
# lascia quindi la cartella di build gia' aggiornata ma il plugin
# installato ancora fermo alla versione precedente, senza nessun errore:
# e' esattamente quello che e' successo qui.
#
# Uso: ./scripts/check_version.sh
#   In caso di mismatch, il fix e': cmake --build build --config Release

cd "$(dirname "$0")/.."

EXPECTED_VERSION=$(grep -m1 'project(Livellatore VERSION' CMakeLists.txt | sed -E 's/.*VERSION ([0-9.]+).*/\1/')
echo "Versione dichiarata in CMakeLists.txt: $EXPECTED_VERSION"
echo

status=0

check_plist_version () {
    local label="$1" path="$2"
    if [ ! -f "$path" ]; then
        echo "WARN  $label: non trovato ($path) — mai buildato/installato su questa macchina?"
        return
    fi

    local installed
    installed=$(defaults read "$path" CFBundleShortVersionString 2>/dev/null || echo "?")
    if [ "$installed" = "$EXPECTED_VERSION" ]; then
        echo "OK    $label: $installed"
    else
        echo "FAIL  $label: $installed (atteso $EXPECTED_VERSION)"
        status=1
    fi
}

check_json_version () {
    local label="$1" path="$2"
    if [ ! -f "$path" ]; then
        echo "WARN  $label: non trovato ($path) — mai buildato/installato su questa macchina?"
        return
    fi

    local installed
    installed=$(grep -m1 '"Version"' "$path" | sed -E 's/.*"Version": *"([^"]+)".*/\1/')
    if [ "$installed" = "$EXPECTED_VERSION" ]; then
        echo "OK    $label: $installed"
    else
        echo "FAIL  $label: $installed (atteso $EXPECTED_VERSION)"
        status=1
    fi
}

check_json_version "VST3 installato" \
    "$HOME/Library/Audio/Plug-Ins/VST3/Livellatore.vst3/Contents/Resources/moduleinfo.json"
check_plist_version "AU installato  " \
    "$HOME/Library/Audio/Plug-Ins/Components/Livellatore.component/Contents/Info.plist"

echo
if [ "$status" -ne 0 ]; then
    echo "MISMATCH: ricompila e reinstalla per intero con:"
    echo "  cmake --build build --config Release"
else
    echo "Tutto allineato."
fi

exit $status
