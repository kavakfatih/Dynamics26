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

app="$(cd "$app" && pwd -P)"
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

resolve_internal_rpath() {
  local item="$1"
  local rpath="$2"
  local base suffix candidate resolved

  case "$rpath" in
    @loader_path)
      base="$(cd "$(dirname "$item")" && pwd -P)"
      suffix=""
      ;;
    @loader_path/*)
      base="$(cd "$(dirname "$item")" && pwd -P)"
      suffix="${rpath#@loader_path/}"
      ;;
    @executable_path)
      base="$app/Contents/MacOS"
      suffix=""
      ;;
    @executable_path/*)
      base="$app/Contents/MacOS"
      suffix="${rpath#@executable_path/}"
      ;;
    *)
      return 1
      ;;
  esac

  candidate="$base"
  if [[ -n "$suffix" ]]; then
    candidate="$base/$suffix"
  fi
  [[ -d "$candidate" ]] || return 1
  resolved="$(cd "$candidate" && pwd -P)"
  [[ "$resolved" == "$app" || "$resolved" == "$app/"* ]]
}

fail=0
while IFS= read -r -d '' item; do
  if ! file -L "$item" | grep -q 'Mach-O'; then
    continue
  fi

  echo "[Mach-O] $item"
  archs="$(lipo -archs "$item" 2>/dev/null || true)"
  if [[ "$archs" != "arm64" ]]; then
    echo "ERROR: bundle is arm64-only by contract, got '$archs': $item" >&2
    fail=1
  fi

  # otool -L'nin ilk satırı incelenen dosyanın kendi dosya yoludur; dependency
  # değildir. Önceki grep bu başlıkta /Users/runner/work gördüğünde false-positive
  # üretiyordu. Burada yalnızca gerçek LC_LOAD_* kayıtlarını denetliyoruz.
  deps_raw="$(otool -L "$item" || true)"
  echo "$deps_raw"
  deps="$(printf '%s\n' "$deps_raw" | sed -n '2,$p' | sed -E 's/^[[:space:]]*//; s/[[:space:]]+\(compatibility version.*$//')"
  while IFS= read -r dep; do
    [[ -z "$dep" ]] && continue
    case "$dep" in
      /System/Library/*|/usr/lib/*)
        ;;
      @executable_path/*|@loader_path/*|@rpath/*)
        ;;
      /*)
        echo "ERROR: external absolute Mach-O dependency remains: $item -> $dep" >&2
        fail=1
        ;;
      *)
        echo "ERROR: unsupported non-relocatable Mach-O dependency remains: $item -> $dep" >&2
        fail=1
        ;;
    esac
  done <<< "$deps"

  # Bundled dylib/framework kimliği de Homebrew/Cellar yoluna bağlı kalamaz.
  install_id="$(otool -D "$item" 2>/dev/null | sed -n '2p' | sed -E 's/^[[:space:]]*//; s/[[:space:]]+$//' || true)"
  if [[ -n "$install_id" && "$install_id" == /* ]]; then
    echo "ERROR: absolute LC_ID_DYLIB remains in bundle: $item -> $install_id" >&2
    fail=1
  fi

  rpaths="$(otool -l "$item" | awk '/cmd LC_RPATH/{getline; getline; print $2}' || true)"
  if [[ -n "$rpaths" ]]; then
    while IFS= read -r rpath; do
      [[ -z "$rpath" ]] && continue
      echo "LC_RPATH: $rpath"
      if [[ "$rpath" == /* ]]; then
        echo "ERROR: absolute LC_RPATH remains in bundle: $item -> $rpath" >&2
        fail=1
      elif ! resolve_internal_rpath "$item" "$rpath"; then
        echo "ERROR: LC_RPATH does not resolve to an existing directory inside the app: $item -> $rpath" >&2
        fail=1
      fi
    done <<< "$rpaths"
  fi
done < <(find "$app/Contents" -type f -print0)

if [[ "$fail" -ne 0 ]]; then
  exit 1
fi

# A process reaching main() proves the dyld dependency closure can be loaded.
"$main" --bundle-smoke

if [[ "$skip_signature" -eq 0 ]]; then
  codesign --verify --deep --strict --verbose=2 "$app"
fi

echo "FEMCAE macOS bundle audit PASS"
