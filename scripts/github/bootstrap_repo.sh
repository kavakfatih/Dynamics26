#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage:
  bootstrap_repo.sh OWNER/REPO [--create-public|--create-private] [--no-push]

Initializes the current FEMCAE source tree as a Git repository, performs hygiene
checks, creates an initial commit, optionally creates the GitHub repository via
GitHub CLI, and pushes main.

Examples:
  ./scripts/github/bootstrap_repo.sh kavakfatih/FEMCAE --create-public
  ./scripts/github/bootstrap_repo.sh kavakfatih/FEMCAE --no-push
EOF
}

[[ $# -ge 1 ]] || { usage; exit 2; }
repo="$1"; shift
create_mode=""
push=1
while [[ $# -gt 0 ]]; do
  case "$1" in
    --create-public) create_mode="public" ;;
    --create-private) create_mode="private" ;;
    --no-push) push=0 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage; exit 2 ;;
  esac
  shift
done

[[ "$repo" == */* ]] || { echo "OWNER/REPO format required" >&2; exit 2; }
command -v git >/dev/null
python3 scripts/github/verify_repository_hygiene.py --root .
git diff --check --no-index /dev/null /dev/null >/dev/null 2>&1 || true

if [[ ! -d .git ]]; then
  if git init -b main >/dev/null 2>&1; then :; else
    git init >/dev/null
    git branch -M main
  fi
fi

git config user.name >/dev/null || {
  echo "git user.name is not configured. Configure it before bootstrap." >&2
  exit 3
}
git config user.email >/dev/null || {
  echo "git user.email is not configured. Configure it before bootstrap." >&2
  exit 3
}

git add --all
python3 scripts/github/verify_repository_hygiene.py --root .
git diff --cached --check
if git diff --cached --quiet; then
  echo "No staged source changes; initial commit not created."
else
  git commit -m "release: FEMCAE $(python3 - <<'PY'
import re
s=open('CMakeLists.txt', encoding='utf-8').read()
print(re.search(r'VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)', s).group(1))
PY
) source baseline"
fi

remote_url="https://github.com/${repo}.git"
if [[ -n "$create_mode" ]]; then
  command -v gh >/dev/null || { echo "GitHub CLI (gh) is required for repository creation" >&2; exit 4; }
  if ! gh repo view "$repo" >/dev/null 2>&1; then
    gh repo create "$repo" "--${create_mode}" --source . --remote origin
  fi
fi

if ! git remote get-url origin >/dev/null 2>&1; then
  git remote add origin "$remote_url"
fi

if [[ $push -eq 1 ]]; then
  git push -u origin main
else
  echo "Bootstrap complete without push. Remote: $(git remote get-url origin)"
fi
