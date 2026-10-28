check-format:
    find apps client common server -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 --no-run-if-empty clang-format --dry-run --Werror

format:
    find apps client common server -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 --no-run-if-empty clang-format -i

audit:
    conan audit provider auth conancenter --token="$CONAN_AUDIT_TOKEN"
    conan audit scan .

install:
    conan install . --build=missing --output-folder=cmake-build-release -c tools.cmake.cmaketoolchain:generator=Ninja

lint:
    find apps client common server -type f -name '*.cpp' -not -path '*/test/*' -print0 | xargs -0 --no-run-if-empty clang-tidy -p cmake-build-release/build/Release

lint-fix:
    find apps client common server -type f -name '*.cpp' -print0 | xargs -0 --no-run-if-empty clang-tidy -fix -p cmake-build-release/build/Release

build:
    cmake --preset conan-release
    cmake --build --preset conan-release

test:
    ctest --test-dir cmake-build-release/build/Release --output-on-failure

test-with-asserts build_dir="cmake-build-asserts":
    cmake --preset conan-release -B {{build_dir}} -DCMAKE_TOOLCHAIN_FILE="{{justfile_directory()}}/cmake-build-release/build/Release/generators/conan_toolchain.cmake" -DASSERTS=ON
    cmake --build {{build_dir}}
    ctest --test-dir {{build_dir}} --output-on-failure

san sanitizers="address" build_dir=("cmake-build-san-" + replace(sanitizers, ",", "-")):
    cmake --preset conan-release -B {{build_dir}} -DCMAKE_TOOLCHAIN_FILE="{{justfile_directory()}}/cmake-build-release/build/Release/generators/conan_toolchain.cmake" -DSANITIZERS={{sanitizers}}
    cmake --build {{build_dir}}
    ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1:suppressions={{justfile_directory()}}/tsan.supp ctest --test-dir {{build_dir}} --output-on-failure

san-with-asserts sanitizers="address" build_dir=("cmake-build-san-asserts-" + replace(sanitizers, ",", "-")):
    cmake --preset conan-release -B {{build_dir}} -DCMAKE_TOOLCHAIN_FILE="{{justfile_directory()}}/cmake-build-release/build/Release/generators/conan_toolchain.cmake" -DSANITIZERS={{sanitizers}} -DASSERTS=ON
    cmake --build {{build_dir}}
    ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1 TSAN_OPTIONS=halt_on_error=1:second_deadlock_stack=1:suppressions={{justfile_directory()}}/tsan.supp ctest --test-dir {{build_dir}} --output-on-failure

patch:
    ./mutation_testing/generate_patch.sh

mutate number:
    ./mutation_testing/test_patch.sh {{number}}

mutations:
    ./mutation_testing/run_all_mutations.sh

check:
    just check-format
    -just audit
    just install
    just build
    just lint
    just test

check-pipeline:
    act push -W .github/workflows/build.yml -P ubuntu-latest=catthehacker/ubuntu:full-latest --secret CONAN_AUDIT_TOKEN="$CONAN_AUDIT_TOKEN"

fix:
    just format
    just install
    just build
    just lint-fix

docs:
    doxygen

docs-pipeline:
    act -j docs -P ubuntu-latest=catthehacker/ubuntu:full-latest --secret GITHUB_TOKEN="$GITHUB_TOKEN" --env GITHUB_REPOSITORY=SerbanUntu/exchange --env GITHUB_SERVER_URL=https://github.com
