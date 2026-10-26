#!/usr/bin/env bash
set -euo pipefail

root="$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)"
patches_dir="$root/mutation_testing/patches"

cd "$root"

if git diff --quiet HEAD -- . ':(exclude)mutation_testing'
then
    echo "There are no changes outside mutation_testing to generate a patch from" >&2
    exit 1
fi

mkdir -p "$patches_dir"
shopt -s nullglob

for i in $(seq 1 99)
do
    number="$(printf '%02d' "$i")"
    existing=("$patches_dir/$number"*.patch)
    if [[ ${#existing[@]} -eq 0 ]]
    then
        patch="$patches_dir/$number.patch"
        git diff --binary HEAD -- . ':(exclude)mutation_testing' > "$patch"
        echo "Created mutation_testing/patches/$number.patch"
        git reset -q -- . ':(exclude)mutation_testing'
        if ! git apply -R "$patch"
        then
            echo "Could not revert the working tree, the changes are still unstaged in it" >&2
            exit 1
        fi
        echo "Reverted the working tree, run git apply mutation_testing/patches/$number.patch to restore it"
        exit 0
    fi
done

echo "All patch numbers from 01 to 99 are already in use" >&2
exit 1
