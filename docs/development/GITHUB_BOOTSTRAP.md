# GitHub Repository Bootstrap — V1.0.2

FEMCAE V1.0.2 can be published to a new GitHub repository without copying build
artifacts or machine-local credentials into the first commit.

## Repository identity

Recommended repository name: `FEMCAE`.

The ChatGPT GitHub connector used during V1.0.2 hardening did not expose an
existing FEMCAE repository and does not provide repository-creation capability.
Therefore no remote repository is claimed to have been created by this source
release.

## First publication from macOS

```bash
unzip FEMCAE-v1.0.2-source-macos-arm64.zip
cd FEMCAE-v1.0.2

git config --global user.name "Your Name"
git config --global user.email "you@example.com"

# If GitHub CLI is authenticated and the repository does not exist:
./scripts/github/bootstrap_repo.sh OWNER/FEMCAE --create-public
```

For a repository already created in the GitHub UI:

```bash
./scripts/github/bootstrap_repo.sh OWNER/FEMCAE
```

Use `--no-push` to inspect the initial commit locally first.

## Required repository checks

Before protecting `main`, run at least one native Apple Silicon execution of:

- `macOS arm64 build and verification / build-test (Debug)`
- `macOS arm64 build and verification / build-test (Release)`
- `macOS arm64 build and verification / gui-build`

For pull requests, require these checks and require branches to be up to date
before merge. Do not require the signed/notarized workflow for every PR: that
workflow needs protected Apple credentials and is intended for release tags or
manual release approval.

## Repository hygiene

The source tree includes:

```bash
python3 scripts/github/verify_repository_hygiene.py --root .
```

It rejects build/stage/dist directories, compiled objects/libraries, archives,
Apple signing key material and common credential filenames.

## Reproducible source archive

```bash
python3 scripts/release/make_source_archive.py \
  --source . \
  --output /tmp/FEMCAE-v1.0.2-source-macos-arm64.zip \
  --version 1.0.2
```

Archive entry order, timestamps and permissions are normalized. Running the
command twice on identical source bytes must produce identical SHA256 values.
