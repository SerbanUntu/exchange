#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

if [[ $# -ne 1 ]]
then
    echo "Usage: test_patch.sh <patch number>" >&2
    exit 1
fi

prefix="$1"
if [[ "$prefix" =~ ^[0-9]$ ]]
then
    prefix="0$prefix"
fi

shopt -s nullglob
matches=("$patches_dir/$prefix"*.patch)

if [[ ${#matches[@]} -eq 0 ]]
then
    echo "No patch in mutation_testing/patches starts with $prefix" >&2
    exit 1
fi

if [[ ${#matches[@]} -gt 1 ]]
then
    echo "More than one patch starts with $prefix:" >&2
    printf '    %s\n' "${matches[@]##*/}" >&2
    exit 1
fi

patch="${matches[0]}"
name="$(basename "$patch" .patch)"

require_toolchain
require_clean_tree
mkdir -p "$logs_dir" "$rows_dir"
rm -f "$rows_dir/$name.md" "$logs_dir/$name"-*.log

write_row() {
    local line="|"
    local cell
    for cell in "$name" "$@"
    do
        line+=" ${cell//|/\\|} |"
    done
    echo "$line" > "$rows_dir/$name.md"
}

files="$(git apply --numstat "$patch" 2> /dev/null | cut -f3 | sed 's#.*/##' | join_lines || true)"
files="${files:-N/A}"

if ! git apply --check "$patch" > /dev/null 2>&1
then
    echo "Patch $prefix is outdated and results in a merge conflict. Please remove it"
    write_row "Outdated" "N/A" "N/A" "N/A" "N/A" "N/A" "$files"
    exit 1
fi

revert() {
    if ! git apply -R "$patch" || ! git diff --quiet HEAD -- . ':(exclude)mutation_testing'
    then
        echo "Could not restore a clean working tree after $name, check git status" >&2
    fi
}

git apply "$patch"
trap revert EXIT
trap 'exit 130' INT TERM
echo "Applied $name"

result="Survived"
caught_layer="N/A"
sanitizer="N/A"
failing="N/A"
detection_time="N/A"
start="$(now_ms)"

for index in "${!layer_names[@]}"
do
    layer="${layer_names[$index]}"
    build_log="$logs_dir/$name-${layer_dirs[$index]}-build.log"
    test_log="$logs_dir/$name-${layer_dirs[$index]}-test.log"

    echo "Building $layer"
    if ! build_layer "$index" "$build_log"
    then
        result="Build failed"
        caught_layer="$layer"
        break
    fi

    echo "Running $layer"
    test_start="$(now_ms)"
    if run_layer_tests "$index" "$test_log"
    then
        echo "$layer passed in $(format_ms $(( $(now_ms) - test_start )))s"
        continue
    fi

    detection_time="$(format_ms $(( $(now_ms) - test_start )))"
    result="Caught"
    caught_layer="$layer"
    failing="$(failing_tests "$test_log")"
    failing="${failing:-N/A}"
    if [[ -n "${layer_sanitizers[$index]}" ]]
    then
        sanitizer="$(sanitizer_reports "$test_log")"
        sanitizer="${sanitizer:-N/A}"
    fi
    break
done

total_time="$(format_ms $(( $(now_ms) - start )))"
write_row "$result" "$caught_layer" "$sanitizer" "$failing" "$detection_time" "$total_time" "$files"

echo
printf '%-20s %s\n' "Mutation:" "$name"
printf '%-20s %s\n' "Result:" "$result"
printf '%-20s %s\n' "Caught by layer:" "$caught_layer"
printf '%-20s %s\n' "Sanitizer:" "$sanitizer"
printf '%-20s %s\n' "Failing tests:" "$failing"
printf '%-20s %s\n' "Detection time (s):" "$detection_time"
printf '%-20s %s\n' "Total time (s):" "$total_time"
printf '%-20s %s\n' "Files changed:" "$files"
printf '%-20s %s\n' "Logs:" "$logs_dir"
