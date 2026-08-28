#!/usr/bin/env bash
set -euo pipefail

# Template di firma con Developer ID reale + notarization Apple (issue #7).
#
# NON eseguito da Claude in nessuna sessione: richiede un account Apple
# Developer Program (a pagamento) e le credenziali personali dell'utente,
# che non vanno MAI condivise con un LLM o incollate in una chat. Va
# eseguito a mano dall'utente, sulla propria macchina, quando si arriva al
# punto di distribuire pubblicamente il plugin.
#
# Prerequisiti (una tantum):
#  1. Un certificato "Developer ID Application: <Nome> (<TEAMID>)" nel
#     portachiavi (Xcode > Settings > Accounts > Manage Certificates, o
#     scaricato da developer.apple.com).
#  2. Una app-specific password per il tuo Apple ID (appleid.apple.com >
#     Sicurezza > Password per app), salvata nel portachiavi locale:
#       xcrun notarytool store-credentials "AC_PASSWORD" \
#           --apple-id "tuo@id.apple.com" --team-id "TEAMID" \
#           --password "xxxx-xxxx-xxxx-xxxx"
#
# Uso (un bundle alla volta: VST3, AU, o l'app Standalone):
#   DEVELOPER_ID="Developer ID Application: Nome Cognome (TEAMID)" \
#       ./scripts/notarize_macos.sh build/Livellatore_artefacts/VST3/Livellatore.vst3

: "${DEVELOPER_ID:?Imposta DEVELOPER_ID, es. \"Developer ID Application: Nome Cognome (TEAMID)\"}"
TARGET="${1:?Uso: $0 <path-al-bundle-.vst3-o-.component-o-.app>}"

if [ ! -e "$TARGET" ]; then
    echo "Non trovato: $TARGET" >&2
    exit 1
fi

echo "Firma $TARGET con \"$DEVELOPER_ID\"..."
codesign --force --deep --options runtime --timestamp --sign "$DEVELOPER_ID" "$TARGET"

echo "Verifica firma..."
codesign --verify --deep --strict --verbose=2 "$TARGET"

ZIP_PATH="$(basename "$TARGET").zip"
echo "Comprimo per l'upload a notarytool..."
ditto -c -k --keepParent "$TARGET" "$ZIP_PATH"

echo "Invio a notarytool (richiede credenziali salvate come profilo 'AC_PASSWORD')..."
xcrun notarytool submit "$ZIP_PATH" --keychain-profile "AC_PASSWORD" --wait

echo "Staple del ticket di notarization sul bundle..."
xcrun stapler staple "$TARGET"

rm -f "$ZIP_PATH"
echo "Fatto: $TARGET firmato con Developer ID e notarizzato."
