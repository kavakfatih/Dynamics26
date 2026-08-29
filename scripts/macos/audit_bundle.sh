#!/usr/bin/env bash
set -euo pipefail
skip_signature=0
if [[ "${1:-}" == "--skip-signature" ]]; then
  skip_signature=1
  shift
fi
app="${1:-}"
if [[ -z "$app" || ! -d "$app" ]]; then
  echo "usage: $0 [--skip-signature] /path/to/FEMCAE.app" >&2
  exit 2
fi
main="$app/Contents/MacOS/FEMCAE"
plist="$app/Contents/Info.plist"
test -x "$main"
test -f "$plist"

plutil -lint "$plist"
bundle_id="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$plist")"
short_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")"
bundle_version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' "$plist")"
[[ "$bundle_id" == "org.femcae.app" ]] || { echo "ERROR: unexpected CFBundleIdentifier: $bundle_id" >&2; exit 1; }
[[ -n "$short_version" && -n "$bundle_version" ]] || { echo "ERROR: bundle version metadata missing" >&2; exit 1; }

echo "FEMCAE bundle metadata: id=$bundle_id short=$short_version build=$bundle_version"

fail=0
while IFS= read -r -d '' item; do
  if file -L "$item" | grep -q 'Mach-O'; then
    echo "[Mach-O] $item"
    archs="$(lipo -archs "$item" 2>/dev/null || true)"
    if [[ "$archs" != "arm64" ]]; then
      echo "ERROR: bundle is arm64-only by contract, got '$archs': $item" >&2
      fail=1
    fi

    deps="$(otool -L "$item" || true)"
    echo "$deps"
    if echo "$deps" | grep -Eq '/opt/homebrew|/usr/local|/Users/runner/work|/private/var/folders/.*/build-|/tmp/.*/build-'; then
      echo "ERROR: external build/Homebrew dependency remains in bundle: $item" >&2
      fail=1
    fi

    rpaths="$(otool -l "$item" | awk '/cmd LC_RPATH/{getline; getline; print $2}' || true)"
    if [[ -n "$rpaths" ]]; then
      echo "LC_RPATH: $rpaths"
      if echo "$rpaths" | grep -Eq '^/opt/homebrew|^/usr/local|/Users/runner/work|/private/var/folders|/tmp/'; then
        echo "ERROR: external absolute LC_RPATH remains in bundle: $item" >&2
        fail=1
      fi
    fi
  fi
done < <(find "$app/Contents" -type f -print0)

if [[ "$fail" -ne 0 ]]; then exit 1; fi

# A process reaching main() proves the dyld dependency closure can be loaded.
"$main" --bundle-smoke

if [[ "$skip_signature" -eq 0 ]]; then
  codesign --verify --deep --strict --verbose=2 "$app"
fi

echo "FEMCAE macOS bundle audit PASS"
