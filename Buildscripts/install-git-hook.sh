#!/usr/bin/env bash
set -euo pipefail

HOOK_DIR=".git/hooks"
HOOK_FILE="${HOOK_DIR}/pre-commit"

cat > "${HOOK_FILE}" <<'HOOK'
#!/usr/bin/env bash
set -euo pipefail

# Run the generator so generated files are updated before commit.
if [ -f Buildscripts/generate-boards.py ]; then
    python3 Buildscripts/generate-boards.py
fi

# If generator changed files, add them so they are included in the commit.
# Optionally you can abort instead of auto-adding.
if ! git diff --quiet || git ls-files --others --exclude-standard | grep -q .; then
    echo "Auto-staging generated files..."
    git add -A
fi
HOOK

chmod +x "${HOOK_FILE}"
echo "Installed pre-commit hook at ${HOOK_FILE}"
echo "Remove or modify it if you prefer the hook to fail instead of auto-staging."