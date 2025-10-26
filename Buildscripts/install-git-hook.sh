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
# Only stage the specific generated files, not all changes.
GENERATED_FILES=(
    "Firmware/Kconfig"
    "Firmware/Source/Boards.h"
    "Buildscripts/board.cmake"
)
for file in "${GENERATED_FILES[@]}"; do
    if [ -f "$file" ] && ! git diff --quiet -- "$file" 2>/dev/null; then
        echo "Auto-staging generated file: $file"
        git add "$file"
    fi
done
HOOK

chmod +x "${HOOK_FILE}"
echo "Installed pre-commit hook at ${HOOK_FILE}"
echo "Remove or modify it if you prefer the hook to fail instead of auto-staging."