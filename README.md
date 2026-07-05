# Stock Exchange

## Local Setup

### Dependencies

- [CMake](https://cmake.org/download/) >= 4.2
- [Python](https://www.python.org/downloads/) + [pip](https://pip.pypa.io/) (used to install Conan)
- [Conan](https://conan.io/) 2.x (`pip install conan`)
- A C++20 compiler

#### Windows

On Windows, I recommend **MSVC**, to avoid issues with Conan. Install it via the [Visual Studio Installer](https://visualstudio.microsoft.com/downloads/) with the "Desktop development with C++" workload.

When selecting individual components, install the **MSVC v143 (VS 2022) build tools** specifically, rather than the newer v144 toolset. Most prebuilt binary packages on [ConanCenter](https://conan.io/center) are currently built against v143, so using it avoids Conan falling back to `--build=missing` and rebuilding dependencies from source.

#### Optional

Only needed if you want to run formatting, linting, or docs generation, or use the `just` aliases below:

- [just](https://github.com/casey/just) (command runner providing short aliases for the commands below)
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html) (code formatting)
- [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) (linting)
- [Doxygen](https://www.doxygen.nl/) (+ [Graphviz](https://graphviz.org/) for call graphs) — documentation generation

### Building

1. Detect your Conan profile (first time only):

   ```sh
   conan profile detect --force
   ```

2. Install dependencies and generate the CMake toolchain/presets:

   ```sh
   conan install . --build=missing --output-folder=cmake-build-release
   ```

3. Configure the project:

   ```sh
   cmake --preset conan-release
   ```

4. Build:

   ```sh
   cmake --build --preset conan-release
   ```

### Testing

```sh
ctest --test-dir cmake-build-release/build/Release --output-on-failure
```

### Optional steps

#### Formatting

```sh
find client common server -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -print0 | xargs -0 --no-run-if-empty clang-format -i
```

#### Linting

Requires the project to have been built first (see [Setup](#setup)), since clang-tidy reads the compile commands from the build output.

```sh
find client common server -type f \( -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \) -not -path '*/test/*' -print0 | xargs -0 --no-run-if-empty clang-tidy -p cmake-build-release/build/Release
```

#### Docs generation

```sh
doxygen
```

Output is written to `docs/html`.

### Using `just`

There is a `Justfile` with short aliases for the commands above (and a few extras like dependency auditing and simulated CI dry-runs via `act`). Once `just` is installed, run commands from the repo root:

```sh
just install   # conan install
just build     # cmake configure + build
just test      # ctest
just format    # clang-format -i
just lint      # clang-tidy
just docs      # doxygen
just check     # format check, audit, install, build, lint, test
just fix       # format, install, build, lint-fix
```

Run `just --list` to see all available aliases.
