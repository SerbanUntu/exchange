check-format:
    find apps client common server -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 --no-run-if-empty clang-format --dry-run --Werror

format:
    find apps client common server -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 --no-run-if-empty clang-format -i

audit:
    conan audit provider auth conancenter --token="$CONAN_AUDIT_TOKEN"
    -conan audit scan .

install:
    conan install . --build=missing --output-folder=cmake-build-release

lint:
    find apps client common server -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -not -path '*/test/*' -print0 | xargs -0 --no-run-if-empty clang-tidy -p cmake-build-release/build/Release

lint-fix:
    find apps client common server -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 --no-run-if-empty clang-tidy -fix -p cmake-build-release/build/Release

build:
     cmake --preset conan-release
     cmake --build --preset conan-release

test:
    ctest --test-dir cmake-build-release/build/Release --output-on-failure

check:
    just check-format
    just audit
    just install
    just build
    just lint
    just test

check-pipeline:
    act -j build -P ubuntu-latest=catthehacker/ubuntu:full-latest --secret CONAN_AUDIT_TOKEN="$CONAN_AUDIT_TOKEN"

fix:
    just format
    just install
    just build
    just lint-fix

docs:
    doxygen

docs-pipeline:
    act -j docs -P ubuntu-latest=catthehacker/ubuntu:full-latest --secret GITHUB_TOKEN="$GITHUB_TOKEN" --env GITHUB_REPOSITORY=SerbanUntu/exchange --env GITHUB_SERVER_URL=https://github.com
