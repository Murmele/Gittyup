# Working on Gittyup

## Workflow

- Inspect the relevant code and propose a plan before implementing changes.
- Keep changes focused; preserve unrelated worktree changes.
- Validate each significant task, then commit it using Conventional Commits
  (`feat:`, `fix:`, `test:`, `docs:`, etc.). Stage only files belonging to that task.
- Update directly related documentation and tests with behavior changes.

## Architecture and conventions

- This is a C++/Qt desktop application built with CMake. Follow nearby Qt
  signal/slot, ownership, naming, and translation (`tr()`) patterns.
- `src/git` wraps libgit2, `src/ui` contains repository views, `src/dialogs`
  contains settings and operation dialogs, and `test` contains Qt tests.
- Keep presentation preferences separate from Git data and operations.
- Use the existing application configuration for app-specific repository and
  global preferences; do not write those preferences into Git's own settings.
- Avoid changing vendored dependencies in `dep`.

## Build, test, and format

- See [README.md](README.md) for prerequisites, submodules, build options, and
  contribution guidance.
- Configure: `cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release`.
- Build: `cmake --build build/release`.
- Build and run tests: `cmake --build build/release --target check`.
- For focused validation, build the relevant `test_<name>` targets, then run
  `ctest --test-dir build/release -R '<test-name-regex>' --output-on-failure`.
- Follow [cl-fmt.sh](cl-fmt.sh) for formatting (clang-format 13 and
  cmake-format 0.6.13). Prefer formatting touched files rather than unrelated code.
- If an `rtk` CLI proxy is available, prefix shell commands with `rtk`
  (use `rtk proxy` for unsupported commands).
