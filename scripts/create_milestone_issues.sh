#!/usr/bin/env bash
# create_milestone_issues.sh
# Reads a markdown checklist file, creates a Codeberg milestone,
# then recursively creates issues (leaves first, parents after).
# Usage: ./create_milestone_issues.sh [--dry-run] [--debug] [--resume] <path-to-markdown-file>

set -euo pipefail

# ---------------------------------------------------------------------------
# FLAGS
# ---------------------------------------------------------------------------
DRY_RUN=0
DEBUG=0

ARGS=()
for arg in "$@"; do
    case "$arg" in
        --dry-run) DRY_RUN=1 ;;
        --debug)    DEBUG=1 ;;
        *)          ARGS+=("$arg") ;;
    esac
done
set -- "${ARGS[@]:-}"

# ---------------------------------------------------------------------------
# CONFIG — edit these
# ---------------------------------------------------------------------------
BASE_URL="https://codeberg.org/api/v1"
CODEBERG_USER="greergan"
CODEBERG_REPO="SlimTS"
CODEBERG_TOKEN="9c319d4778149e6dc2b9f96c38620c404e631dfc"

# ---------------------------------------------------------------------------

# Rate limit guard — seconds between API calls (Codeberg: 4 req/min = 1 per 15s)
API_SLEEP=10

MARKDOWN_FILE="${1:-}"
if [[ -z "$MARKDOWN_FILE" ]]; then
    echo "Usage: $0 <markdown-file>" >&2
    exit 1
fi
if [[ ! -f "$MARKDOWN_FILE" ]]; then
    echo "File not found: $MARKDOWN_FILE" >&2
    exit 1
fi

# State file: tracks node_id:issue_number for resume support
STATE_FILE="${MARKDOWN_FILE%.md}.state"

# ---------------------------------------------------------------------------
# Globals populated by parse_markdown
# ---------------------------------------------------------------------------
NODE_TITLE=()       # title string
NODE_BODY=()        # story/detail/note block (may be empty)
NODE_PARENT=()      # parent node ID, 0 = root
NODE_DEPTH=()       # indent depth (0-based)
NODE_ISSUE=()       # Codeberg issue number once created
NODE_COUNT=0

MILESTONE_TITLE=""
MILESTONE_ID=""

# ---------------------------------------------------------------------------
# parse_markdown
# ---------------------------------------------------------------------------
parse_markdown() {
    local file="$1"

    awk '
    function ltrim(s) { sub(/^[[:space:]]+/, "", s); return s }
    function rtrim(s) { sub(/[[:space:]]+$/, "", s); return s }
    function trim(s)  { return rtrim(ltrim(s)) }

    function rank_of(spaces,    r) {
        if (!(spaces in indent_levels)) {
            indent_levels[spaces] = next_rank++
        }
        return indent_levels[spaces]
    }

    BEGIN { next_rank = 0 }

    /^# Milestone:/ {
        title = trim(substr($0, index($0, "Milestone:") + 10))
        print "MILESTONE\t" title
        next
    }

    /^[[:space:]]*- \[ \]/ {
        line = $0
        spaces = 0
        while (substr(line, spaces+1, 1) == " ") spaces++
        depth = rank_of(spaces)

        rest = substr(line, spaces + 7)
        rest = trim(rest)

        if (rest ~ /^Note:/) {
            print "NOTE\t" rest
        } else {
            print "NODE\t" depth "\t" rest
        }
        next
    }

    # Body lines: indented content under a node that is NOT a checkbox
    /^[[:space:]]/ {
        line = trim($0)
        if (line == "") next
        sub(/^[-[:space:]]*Story:[[:space:]]*/, "", line)
        if (line == "") next
        print "BODY\t" line
        next
    }
    ' "$file"
}

# ---------------------------------------------------------------------------
# build_tree
# ---------------------------------------------------------------------------
build_tree() {
    local file="$1"

    declare -A depth_stack
    local current_node=0
    local pending_body=""
    local pending_note=""
    local combined="" depth title id parent best_depth key

    while IFS=$'\t' read -r record_type rest; do
        case "$record_type" in

            MILESTONE)
                MILESTONE_TITLE="$rest"
                ;;

            NODE)
                if [[ $current_node -gt 0 ]]; then
                    combined=""
                    [[ -n "$pending_body" ]]  && combined+="$pending_body"
                    [[ -n "$pending_note" ]]  && combined+="${combined:+$'\n\n'}> **Note:** $pending_note"
                    NODE_BODY[$current_node]="$combined"
                fi
                pending_body=""
                pending_note=""

                depth="${rest%%$'\t'*}"
                title="${rest#*$'\t'}"

                NODE_COUNT=$(( NODE_COUNT + 1 ))
                id=$NODE_COUNT
                NODE_TITLE[$id]="$title"
                NODE_BODY[$id]=""
                NODE_DEPTH[$id]="$depth"
                NODE_ISSUE[$id]=""

                parent=0
                if [[ $depth -gt 0 ]]; then
                    local best_depth=-1
                    for key in "${!depth_stack[@]}"; do
                        if [[ $key -lt $depth && $key -gt $best_depth ]]; then
                            best_depth=$key
                        fi
                    done
                    if [[ $best_depth -ge 0 ]]; then
                        parent="${depth_stack[$best_depth]}"
                    fi
                fi
                NODE_PARENT[$id]="$parent"

                depth_stack[$depth]=$id
                local keys_to_unset=()
                for key in "${!depth_stack[@]}"; do
                    if [[ $key -gt $depth ]]; then
                        keys_to_unset+=("$key")
                    fi
                done
                for key in "${keys_to_unset[@]:-}"; do
                    unset "depth_stack[$key]"
                done

                current_node=$id
                ;;

            BODY)
                pending_body+="${pending_body:+$'\n'}$rest"
                ;;

            NOTE)
                pending_note+="${pending_note:+$'\n'}$rest"
                ;;
        esac
    done < <(parse_markdown "$file")

    if [[ $current_node -gt 0 ]]; then
        combined=""
        [[ -n "$pending_body" ]] && combined="$pending_body"
        [[ -n "$pending_note" ]] && combined+="${combined:+$'\n\n'}> **Note:** $pending_note"
        NODE_BODY[$current_node]="$combined"
    fi
}

# ---------------------------------------------------------------------------
# json_escape
# ---------------------------------------------------------------------------
json_escape() {
    local s="$1"
    s="${s//\\/\\\\}"
    s="${s//\"/\\\"}"
    s="${s//$'\n'/\\n}"
    s="${s//$'\r'/\\r}"
    s="${s//$'\t'/\\t}"
    printf '%s' "$s"
}

# ---------------------------------------------------------------------------
# json_extract_number
# ---------------------------------------------------------------------------
json_extract_number() {
    local key="$1"
    local json="$2"
    echo "$json" | grep -o "\"${key}\":[[:space:]]*[0-9]*" | grep -o '[0-9]*$'
}

# ---------------------------------------------------------------------------
# api_post
# ---------------------------------------------------------------------------
_DRY_RUN_ISSUE=100
_DRY_RUN_MILESTONE=1

api_post() {
    local endpoint="$1"
    local payload="$2"

    if [[ $DRY_RUN -eq 1 ]]; then
        echo "  [dry-run] POST ${BASE_URL}${endpoint}" >&2
        echo "  [dry-run] payload: ${payload}" >&2

        if [[ "$endpoint" == */milestones ]]; then
            printf '{"id":%d}' "$_DRY_RUN_MILESTONE"
        else
            _DRY_RUN_ISSUE=$(( _DRY_RUN_ISSUE + 1 ))
            printf '{"number":%d}' "$_DRY_RUN_ISSUE"
        fi
        return
    fi

    local attempt=0
    local max_attempts=1000
    local base_backoff=2
    local raw body headers http_status retry_after wait jitter
    local window_seconds

    while [[ $attempt -le $max_attempts ]]; do
        raw=$(curl -s -D - -X POST \
            -H "Authorization: token ${CODEBERG_TOKEN}" \
            -H "Content-Type: application/json" \
            --data "$payload" \
            "${BASE_URL}${endpoint}")

        headers="${raw%%$'\r\n\r\n'*}"
        body="${raw#*$'\r\n\r\n'}"
        if [[ "$headers" == "$raw" ]]; then
            headers="${raw%%$'\n\n'*}"
            body="${raw#*$'\n\n'}"
        fi

        http_status=$(echo "$headers" | head -1 | grep -o '[0-9][0-9][0-9]' | head -1)

        if [[ "$http_status" == "429" ]]; then
            attempt=0
        fi

        # 429 Too Many Requests
        if [[ "$http_status" == "429" ]]; then
            echo "  [429 Rate Limit Hit] Full Response Headers Received:" >&2
            #echo "----------------------------------------" >&2
            #echo "$headers" >&2
            #echo "----------------------------------------" >&2

            # Extract window remaining time 't=' from the ratelimit header (e.g., ratelimit: "baseline";r=1990;t=600)
            window_seconds=$(echo "$headers" | grep -i '^ratelimit:' | grep -o 't=[0-9]*' | cut -d'=' -f2)

            if [[ -n "$window_seconds" && "$window_seconds" -gt 0 ]]; then
                local sleep_time=$(( window_seconds + 10 ))
                echo "    -> Found window reset time (t=${window_seconds}s). Sleeping for ${sleep_time}s (including 10s buffer)..." >&2
                echo "    -> Sleeping at $(date '+%Y-%m-%d %H:%M:%S')..." >&2
                sleep "$sleep_time"
            else
                echo "    -> No retry headers found. Sleeping for configured API_SLEEP (${API_SLEEP}s)..." >&2
                echo "    -> Sleeping at $(date '+%Y-%m-%d %H:%M:%S')..." >&2
                sleep "$API_SLEEP"
            fi

            attempt=$(( attempt + 1 ))
            continue
        fi

        if [[ "$endpoint" == */milestones ]]; then
            if json_extract_number "id" "$body" | grep -q '[0-9]'; then
                printf '%s' "$body"
                return 0
            fi
        else
            if json_extract_number "number" "$body" | grep -q '[0-9]'; then
                printf '%s' "$body"
                return 0
            fi
        fi

        echo "  [attempt ${attempt}/${max_attempts}] API call failed (HTTP ${http_status:-unknown})." >&2
        echo "  --- Response Headers ---" >&2
        echo "$headers" >&2
        echo "  ------------------------" >&2
        echo "  [raw response] ${body}" >&2

        if [[ $attempt -lt $max_attempts ]]; then
            jitter=$(( RANDOM % (base_backoff + 1) ))
            wait=$(( base_backoff + jitter ))
            echo "  [retry] waiting ${wait}s..." >&2
            sleep "$wait"
            base_backoff=$(( base_backoff * 2 ))
        fi

        attempt=$(( attempt + 1 ))
    done

    printf '%s' "$body"
    return 1
}

# ---------------------------------------------------------------------------
# create_milestone
# ---------------------------------------------------------------------------
create_milestone() {
    echo "Creating milestone: ${MILESTONE_TITLE}"

    local escaped_title
    escaped_title=$(json_escape "$MILESTONE_TITLE")
    local payload="{\"title\":\"${escaped_title}\"}"

    local response
    response=$(api_post "/repos/${CODEBERG_USER}/${CODEBERG_REPO}/milestones" "$payload")

    MILESTONE_ID=$(json_extract_number "id" "$response")

    if [[ -z "$MILESTONE_ID" ]]; then
        echo "Failed to create milestone. Response:" >&2
        echo "$response" >&2
        exit 1
    fi

    echo "  -> Milestone ID: ${MILESTONE_ID}"
    [[ $DRY_RUN -eq 0 ]] && sleep "$API_SLEEP"
}

# ---------------------------------------------------------------------------
# get_children
# ---------------------------------------------------------------------------
get_children() {
    local parent="$1"
    local children=()
    for (( i=1; i<=NODE_COUNT; i++ )); do
        if [[ "${NODE_PARENT[$i]}" == "$parent" ]]; then
            children+=("$i")
        fi
    done
    echo "${children[*]:-}"
}

# ---------------------------------------------------------------------------
# create_issue_for_node
# ---------------------------------------------------------------------------
create_issue_for_node() {
    local id="$1"
    local title="${NODE_TITLE[$id]}"
    local body="${NODE_BODY[$id]:-}"

    local children_str
    children_str=$(get_children "$id")

    local child_refs=""
    if [[ -n "$children_str" ]]; then
        for child_id in $children_str; do
            create_issue_for_node "$child_id"
        done

        child_refs=$'\n\n## Sub-tasks\n'
        for child_id in $children_str; do
            local child_issue="${NODE_ISSUE[$child_id]}"
            local child_title="${NODE_TITLE[$child_id]}"
            child_refs+="- [ ] #${child_issue} ${child_title}"$'\n'
        done
    fi

    local full_body="${body}${child_refs}"

    # Guard: skip if issue was already created (e.g. during a resume)
    if [[ -n "${NODE_ISSUE[$id]:-}" ]]; then
        echo "  -> Skipping #${NODE_ISSUE[$id]} (already exists): ${title}"
        return
    fi

    echo "Creating issue: ${title}"

    local escaped_title escaped_body
    escaped_title=$(json_escape "$title")
    escaped_body=$(json_escape "$full_body")
    local payload="{\"title\":\"${escaped_title}\",\"body\":\"${escaped_body}\",\"milestone\":${MILESTONE_ID},\"labels\":[2051762]}"

    local response
    response=$(api_post "/repos/${CODEBERG_USER}/${CODEBERG_REPO}/issues" "$payload")

    local issue_number
    issue_number=$(json_extract_number "number" "$response")

    if [[ -z "$issue_number" ]]; then
        echo "  ERROR creating issue '${title}'. Response:" >&2
        echo "$response" >&2
        exit 1
    fi

    NODE_ISSUE[$id]="$issue_number"
    state_save "$id" "$issue_number"
    echo "  -> #${issue_number}: ${title}"
    [[ $DRY_RUN -eq 0 ]] && sleep "$API_SLEEP"
}

# ---------------------------------------------------------------------------
# state_save
# ---------------------------------------------------------------------------
state_save() {
    local node_id="$1"
    local issue_number="$2"
    if [[ $DRY_RUN -eq 0 ]]; then
        echo "${node_id}:${issue_number}" >> "$STATE_FILE"
    fi
}

# ---------------------------------------------------------------------------
# state_load
# ---------------------------------------------------------------------------
state_load() {
    echo "Loading state from: ${STATE_FILE}"
    while IFS=: read -r key value; do
        if [[ "$key" == "milestone" ]]; then
            MILESTONE_ID="$value"
            echo "  -> Milestone ID: ${MILESTONE_ID} (resumed)"
        elif [[ "$key" =~ ^[0-9]+$ ]]; then
            NODE_ISSUE[$key]="$value"
        fi
    done < "$STATE_FILE"
}

# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
main() {
    [[ $DRY_RUN -eq 1 ]] && echo "[DRY RUN — no API calls will be made]" && echo ""
    echo "Parsing: ${MARKDOWN_FILE}"
    build_tree "$MARKDOWN_FILE"

    if [[ -z "$MILESTONE_TITLE" ]]; then
        echo "No milestone title found (expected '# Milestone: ...' heading)" >&2
        exit 1
    fi

    echo "Parsed ${NODE_COUNT} nodes."

    if [[ $DEBUG -eq 1 ]]; then
        echo ""
        printf '%-6s %-6s %-6s %s\n' "ID" "DEPTH" "PARENT" "TITLE"
        printf '%-6s %-6s %-6s %s\n' "---" "---" "---" "---"
        for (( i=1; i<=NODE_COUNT; i++ )); do
            printf '%-6s %-6s %-6s %s\n' \
                "$i" "${NODE_DEPTH[$i]}" "${NODE_PARENT[$i]}" "${NODE_TITLE[$i]:0:60}"
        done
        exit 0
    fi

    echo ""

    if [[ -f "$STATE_FILE" ]]; then
        echo "State file found — resuming from: ${STATE_FILE}"
        state_load
    else
        create_milestone
        if [[ $DRY_RUN -eq 0 ]]; then
            echo "milestone:${MILESTONE_ID}" >> "$STATE_FILE"
        fi
    fi

    echo ""
    echo "Creating issues (post-order: leaves first)..."
    echo ""

    for (( i=1; i<=NODE_COUNT; i++ )); do
        if [[ "${NODE_PARENT[$i]}" == "0" ]]; then
            create_issue_for_node "$i"
        fi
    done

    echo ""
    echo "Done. Milestone '${MILESTONE_TITLE}' and ${NODE_COUNT} issues created."
}

main
