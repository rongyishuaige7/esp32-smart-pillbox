#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$(mktemp -d /tmp/esp32-smart-pillbox-verify.XXXXXX)"
PYCACHE="$(mktemp -d /tmp/esp32-smart-pillbox-pycache.XXXXXX)"
cleanup() {
  rm -rf -- "$WORK" "$PYCACHE"
}
trap cleanup EXIT

if git -C "$ROOT" rev-parse --verify HEAD >/dev/null 2>&1; then
  git -C "$ROOT" archive HEAD | tar -x -C "$WORK"
else
  tar -C "$ROOT" \
    --exclude=.git --exclude=.pio --exclude=.dart_tool --exclude=.gradle --exclude=.idea \
    --exclude=.vscode --exclude=build --exclude=dist --exclude=ephemeral --exclude=__pycache__ \
    -cf - . | tar -x -C "$WORK"
fi

export PYTHONPYCACHEPREFIX="$PYCACHE"
python3 "$WORK/scripts/secret_scan.py" --root "$WORK"
python3 "$WORK/scripts/check_repo.py" --root "$WORK"
(cd "$WORK" && python3 -m unittest discover -s tests -v)

PIO_WORK="$WORK/firmware"
pio run -d "$PIO_WORK"

(
  cd "$WORK/app"
  flutter pub get
  flutter test
  flutter analyze
  flutter build web --release
  # Flutter writes platform-local metadata during validation.  It is ignored by
  # Git and must not be included in the final archive rescan.
  rm -f -- .flutter-plugins-dependencies android/local.properties
  rm -f -- ios/Flutter/Generated.xcconfig ios/Flutter/flutter_export_environment.sh
  rm -rf -- ios/Flutter/ephemeral
)

python3 "$WORK/scripts/secret_scan.py" --root "$WORK"
python3 "$WORK/scripts/check_repo.py" --root "$WORK"
echo 'Verification: PASS'
