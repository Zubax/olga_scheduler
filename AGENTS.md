# Repository Guidelines

## Project Structure & Module Organization
- `include/olga_scheduler/`: public single-file headers.
- `tests/`: GoogleTest suites (`test_*.cpp`, `test_*.c`) plus demos.
- `lib/cavl/`: vendored CAVL header-only dependency used by the scheduler.
- `CMakeLists.txt`: build, test, formatting, static-analysis, and coverage wiring.

## Build, Test, and Development Commands
Common CMake workflow:
```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```
Format sources (requires `clang-format`):
```sh
cmake --build build --target format
```
Static analysis is enabled by default and requires `clang-tidy`. Disable with:
```sh
cmake -S . -B build -DNO_STATIC_ANALYSIS=1
```
Coverage build (requires `gcovr` and GCC/Clang):
```sh
cmake -S . -B build-coverage -DOLGA_ENABLE_COVERAGE=ON
cmake --build build-coverage --target coverage
```
Build the C demo:
```sh
cmake --build build --target olga_scheduler_c_demo
```

## Coding Style & Naming Conventions
- C99 for C code and C++20 for C++ code.
- Formatting is defined in `.clang-format` (Mozilla base, 4-space indent, 120 column limit). Use `clang-format` via the `format` target.
- `clang-tidy` is enforced (warnings as errors). Keep code warning-free; the project builds with `-Wall -Wextra -Werror -pedantic`.
- Tests are named `test_*.c` / `test_*.cpp`. Headers stay in `include/olga_scheduler/`.

## Testing Guidelines
- Tests use GoogleTest (fetched via CMake `FetchContent`).
- Run with `ctest` from the build directory (see commands above).
- Coverage target enforces 100% line and branch coverage for `include/olga_scheduler/olga_scheduler.h` (C API only), so keep coverage updates in sync with header changes.

## Commit & Pull Request Guidelines
- Commit history on feature branches is irrelevant as we use squash merging only.
- PRs should clearly describe behavioral changes, list tests run (e.g., `ctest --test-dir build`), and link related issues/PRs when applicable.
