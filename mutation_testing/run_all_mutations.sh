#!/usr/bin/env bash
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/common.sh"

shopt -s nullglob
patches=("$patches_dir"/*.patch)

if [[ ${#patches[@]} -eq 0 ]]
then
    echo "There are no patches in mutation_testing/patches" >&2
    exit 1
fi

require_toolchain
require_clean_tree
mkdir -p "$logs_dir" "$rows_dir"

for index in "${!layer_names[@]}"
do
    layer="${layer_names[$index]}"
    build_log="$logs_dir/baseline-${layer_dirs[$index]}-build.log"
    test_log="$logs_dir/baseline-${layer_dirs[$index]}-test.log"
    echo "Checking that $layer pass without any mutation"
    if ! build_layer "$index" "$build_log" || ! run_layer_tests "$index" "$test_log"
    then
        echo "$layer fail without any mutation, see $logs_dir/baseline-${layer_dirs[$index]}-*.log" >&2
        exit 1
    fi
done

for patch in "${patches[@]}"
do
    name="$(basename "$patch" .patch)"
    echo
    echo "=== $name ==="
    status=0
    bash "$mutation_dir/test_patch.sh" "$name" || status=$?
    if [[ $status -eq 130 ]]
    then
        exit 130
    fi
done

declare -A counts=()
rows=()

for patch in "${patches[@]}"
do
    name="$(basename "$patch" .patch)"
    row_file="$rows_dir/$name.md"
    if [[ -f "$row_file" ]]
    then
        row="$(cat "$row_file")"
    else
        row="| $name | Error | N/A | N/A | N/A | N/A | N/A | N/A |"
    fi
    rows+=("$row")
    result="${row#"| ${name//|/\\|} | "}"
    result="${result%% |*}"
    counts["$result"]=$(( ${counts["$result"]:-0} + 1 ))
done

caught="${counts["Caught"]:-0}"
survived="${counts["Survived"]:-0}"
score="N/A"
if [[ $(( caught + survived )) -gt 0 ]]
then
    score="$(( caught * 100 / (caught + survived) ))%"
fi

results="$build_root/MUTATION_TESTING_RESULTS.md"

{
    echo "# Mutation testing results"
    echo
    echo "- Commit: $(git rev-parse --short HEAD)"
    echo "- Date: $(date -u '+%Y-%m-%d %H:%M UTC')"
    echo "- Per-test timeout: ${test_timeout}s"
    echo "- Mutations: ${#patches[@]}"
    echo "- Caught: $caught"
    echo "- Survived: $survived"
    echo "- Build failed: ${counts["Build failed"]:-0}"
    echo "- Outdated: ${counts["Outdated"]:-0}"
    echo "- Error: ${counts["Error"]:-0}"
    echo "- Mutation score (caught / (caught + survived)): $score"
    echo
    echo "| Mutation | Result | Caught by layer | Sanitizer | Failing tests | Detection time (s) | Total time (s) | Files changed |"
    echo "| --- | --- | --- | --- | --- | --- | --- | --- |"
    printf '%s\n' "${rows[@]}"
} > "$results"

echo
echo "Wrote ${results#"$root"/}"
