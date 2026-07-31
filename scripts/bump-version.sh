#!/usr/bin/env bash
set -e

# 1. Find the latest tag safely
LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || git tag -l | grep -E '^v?[0-9]+\.[0-9]+' | sort -V | tail -n1)

if [ -z "$LAST_TAG" ]; then
    echo "version: no tags found, create an initial tag manually"
    exit 0
fi

# 2. Get commits since the last tag (hash|subject)
COMMITS=$(git log "${LAST_TAG}"..HEAD --format="%h|%s" 2>/dev/null || true)
if [ -z "$COMMITS" ]; then
    echo "version: no commits since $LAST_TAG"
    exit 0
fi

TMPFILE=$(mktemp)
printf '%s\n' "$COMMITS" > "$TMPFILE"

BUMP=""
CHANGELOG=""

# Helper function to derive all unique modules touched by a commit's files
get_modules_for_commit() {
    local commit_hash="$1"
    local files
    files=$(git diff-tree --no-commit-id --name-only -r "$commit_hash" 2>/dev/null || true)

    local found_modules=""

    while IFS= read -r file; do
        [ -z "$file" ] && continue

        local mod="core"
        if [[ "$file" =~ ^types/ ]]; then
            mod="types"
        elif [[ "$file" =~ src/plugins/([^/]+)/ ]]; then
            mod="${BASH_REMATCH[1]}"
        fi

        if [[ ! " $found_modules " =~ [[:space:]]"$mod"[[:space:]] ]]; then
            found_modules="${found_modules} ${mod}"
        fi
    done <<< "$files"

    echo "${found_modules# }"
}

# 3. Parse commits and normalize inline multi-prefixes
while IFS='|' read -r commit_hash raw_msg; do
    [ -z "$commit_hash" ] && continue

    normalized_msg=$(echo "$raw_msg" | sed -E 's/\b(feature|refactor|fix|docs|config|update):/\n\1:/g')

    while IFS= read -r msg; do
        [ -z "$msg" ] && continue

        is_bump=false
        MATCHED_TYPE=""

        if echo "$msg" | grep -qE '^(feature|refactor):'; then
            if [ "$BUMP" != "minor" ]; then
                BUMP="minor"
            fi
            if [[ "$msg" =~ ^feature: ]]; then MATCHED_TYPE="feature"; else MATCHED_TYPE="refactor"; fi
            is_bump=true
        elif echo "$msg" | grep -qE '^(fix|docs|config):'; then
            if [ -z "$BUMP" ]; then
                BUMP="patch"
            fi
            if [[ "$msg" =~ ^fix: ]]; then MATCHED_TYPE="fix"
            elif [[ "$msg" =~ ^docs: ]]; then MATCHED_TYPE="docs"
            else MATCHED_TYPE="config"
            fi
            is_bump=true
        fi

        if [ -z "$MATCHED_TYPE" ] && [[ "$msg" =~ ^(update): ]]; then
            if [ -z "$BUMP" ]; then
                BUMP="patch"
            fi
            MATCHED_TYPE="fix"
            is_bump=true
        fi

        if [ "$is_bump" = true ]; then
            if echo "$msg" | grep -qE '^[a-z]+\([a-zA-Z0-9_-]+\):'; then
                CHANGELOG="${CHANGELOG}- ${msg}"$'\n'
            else
                clean_desc=$(echo "$msg" | sed -E "s/^(feature|refactor|fix|docs|config|update): *//")

                MODS=$(get_modules_for_commit "$commit_hash")
                for MODULE in $MODS; do
                    CHANGELOG="${CHANGELOG}- ${MATCHED_TYPE}(${MODULE}): ${clean_desc}"$'\n'
                done
            fi
        fi
    done <<< "$normalized_msg"
done < "$TMPFILE"
rm -f "$TMPFILE"

if [ -z "$BUMP" ]; then
    echo "version: no version-bumping commits since $LAST_TAG"
    exit 0
fi

# 4. Calculate the new version number
VERSION=$(echo "$LAST_TAG" | sed 's/^v//')
MAJOR=$(echo "$VERSION" | cut -d. -f1)
MINOR=$(echo "$VERSION" | cut -d. -f2)
PATCH=$(echo "$VERSION" | cut -d. -f3)

if [ "$BUMP" = "minor" ]; then
    MINOR=$((MINOR + 1))
    PATCH=0
else
    PATCH=$((PATCH + 1))
fi

NEW_TAG="v$MAJOR.$MINOR.$PATCH"

# 5. Extract package versions using pkg-config if available
SLIMCOMMON_VER=$(pkg-config --modversion slimcommon 2>/dev/null || echo "unknown")
LIBTSGO_VER=$(pkg-config --modversion libtsgo 2>/dev/null || echo "unknown")
BORINGSSL_VER=$(pkg-config --modversion boringssl 2>/dev/null || echo "unknown")

TAG_BODY="${NEW_TAG} Changes:"$'\n'"${CHANGELOG}"$'\n'
TAG_BODY="${TAG_BODY}Package Versions:"$'\n'
TAG_BODY="${TAG_BODY}- slimcommon: ${SLIMCOMMON_VER}"$'\n'
TAG_BODY="${TAG_BODY}- libtsgo: ${LIBTSGO_VER}"$'\n'
TAG_BODY="${TAG_BODY}- boringssl: ${BORINGSSL_VER}"

# 6. Create the local Git tag
git tag -a "$NEW_TAG" -m "$TAG_BODY"
echo "version: tagged $NEW_TAG"
printf '%s\n' "$TAG_BODY"
