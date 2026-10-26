root="$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)"
mutation_dir="$root/mutation_testing"
patches_dir="$mutation_dir/patches"
build_root="$mutation_dir/build"
logs_dir="$build_root/logs"
rows_dir="$build_root/rows"
toolchain="$root/cmake-build-release/build/Release/generators/conan_toolchain.cmake"
test_timeout="${MUTATION_TEST_TIMEOUT:-300}"

layer_names=("Unit tests" "ASan+UBSan" "TSan")
layer_dirs=("plain" "asan-ubsan" "tsan")
layer_sanitizers=("" "address,undefined" "thread")

sanitizer_patterns=("ERROR: AddressSanitizer" "ERROR: LeakSanitizer" "runtime error:" "WARNING: ThreadSanitizer")
sanitizer_labels=("AddressSanitizer" "LeakSanitizer" "UndefinedBehaviorSanitizer" "ThreadSanitizer")

export ASAN_OPTIONS="detect_leaks=1:abort_on_error=1"
export UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1"
export TSAN_OPTIONS="halt_on_error=1:second_deadlock_stack=1:suppressions=$root/tsan.supp"

cd "$root"

now_ms() {
    echo $(( $(date +%s%N) / 1000000 ))
}

format_ms() {
    printf '%d.%02d' $(( $1 / 1000 )) $(( $1 % 1000 / 10 ))
}

join_lines() {
    paste -sd, - | sed 's/,/, /g'
}

require_toolchain() {
    if [[ ! -f "$toolchain" ]]
    then
        echo "The Conan toolchain was not found at $toolchain, run just install first" >&2
        exit 1
    fi
}

require_clean_tree() {
    if ! git diff --quiet HEAD -- . ':(exclude)mutation_testing'
    then
        echo "The working tree has uncommitted changes outside mutation_testing, commit or stash them first" >&2
        exit 1
    fi
}

build_layer() {
    local dir="$build_root/${layer_dirs[$1]}"
    local log="$2"
    : > "$log"
    if [[ ! -f "$dir/CMakeCache.txt" ]]
    then
        cmake --preset conan-release -B "$dir" -DCMAKE_TOOLCHAIN_FILE="$toolchain" -DSANITIZERS="${layer_sanitizers[$1]}" >> "$log" 2>&1 || return 1
    fi
    cmake --build "$dir" --parallel "$(nproc)" >> "$log" 2>&1
}

run_layer_tests() {
    ctest --test-dir "$build_root/${layer_dirs[$1]}" --output-on-failure --timeout "$test_timeout" > "$2" 2>&1
}

failing_tests() {
    sed -n '/The following tests FAILED:/,$ s/^[[:space:]]*[0-9][0-9]* - //p' "$1" | join_lines
}

sanitizer_reports() {
    local i
    for i in "${!sanitizer_patterns[@]}"
    do
        if grep -qF "${sanitizer_patterns[$i]}" "$1"
        then
            echo "${sanitizer_labels[$i]}"
        fi
    done | join_lines
}
