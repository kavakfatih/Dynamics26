#!/usr/bin/env bash
set -euo pipefail
# Developer-ID release helper.
# Notarization authentication options:
#   A) FEMCAE_NOTARY_PROFILE (pre-created notarytool keychain profile)
#   B) FEMCAE_NOTARY_KEY_PATH + FEMCAE_NOTARY_KEY_ID + FEMCAE_NOTARY_ISSUER
app="${FEMCAE_APP:-${1:-}}"
identity="${FEMCAE_SIGN_IDENTITY:-}"
profile="${FEMCAE_NOTARY_PROFILE:-}"
notary_key="${FEMCAE_NOTARY_KEY_PATH:-}"
notary_key_id="${FEMCAE_NOTARY_KEY_ID:-}"
notary_issuer="${FEMCAE_NOTARY_ISSUER:-}"
out_dir="${FEMCAE_RELEASE_DIR:-$(pwd)/dist}"
version="${FEMCAE_VERSION:-1.0.2}"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [[ -z "$app" || ! -d "$app" ]]; then echo "FEMCAE_APP/.app gerekli" >&2; exit 2; fi
if [[ -z "$identity" ]]; then echo "FEMCAE_SIGN_IDENTITY (Developer ID Application: ...) gerekli" >&2; exit 2; fi

notary_args=()
if [[ -n "$profile" ]]; then
  notary_args=(--keychain-profile "$profile")
elif [[ -n "$notary_key" && -n "$notary_key_id" && -n "$notary_issuer" ]]; then
  test -f "$notary_key"
  notary_args=(--key "$notary_key" --key-id "$notary_key_id" --issuer "$notary_issuer")
else
  echo "FEMCAE_NOTARY_PROFILE veya API key notary credentials gerekli" >&2
  exit 2
fi

mkdir -p "$out_dir"

# Fail before signing/notarization when the bundle is not self-contained.
"$script_dir/audit_bundle.sh" --skip-signature "$app"

# Nested Mach-O code is signed first. Framework bundles are re-signed afterwards
# so their bundle seals describe the newly signed binaries; outer app is last.
while IFS= read -r -d '' item; do
  if file -L "$item" | grep -q 'Mach-O'; then
    codesign --force --options runtime --timestamp --sign "$identity" "$item"
  fi
done < <(find "$app/Contents" -type f -print0)

while IFS= read -r -d '' framework; do
  codesign --force --options runtime --timestamp --sign "$identity" "$framework"
done < <(find "$app/Contents/Frameworks" -type d -name '*.framework' -print0 2>/dev/null || true)

codesign --force --options runtime --timestamp --sign "$identity" "$app"
codesign --verify --deep --strict --verbose=2 "$app"
"$script_dir/audit_bundle.sh" "$app"

zip_path="$out_dir/FEMCAE-${version}-macos-arm64.zip"
dmg_path="$out_dir/FEMCAE-${version}-macos-arm64.dmg"
rm -f "$zip_path" "$dmg_path"
ditto -c -k --sequesterRsrc --keepParent "$app" "$zip_path"
xcrun notarytool submit "$zip_path" "${notary_args[@]}" --wait
xcrun stapler staple "$app"
xcrun stapler validate "$app"

hdiutil create -volname "FEMCAE ${version}" -srcfolder "$app" -ov -format UDZO "$dmg_path"
xcrun notarytool submit "$dmg_path" "${notary_args[@]}" --wait
xcrun stapler staple "$dmg_path"
xcrun stapler validate "$dmg_path"
spctl --assess --type execute --verbose=4 "$app"
shasum -a 256 "$zip_path" "$dmg_path" | tee "$out_dir/SHA256SUMS-macos.txt"
echo "FEMCAE signed/notarized release ready: $out_dir"
