# Contributing to ace

Thanks for your interest in the project. ace is still in an early stage, so
the guidelines below may change over time.

## How to propose changes

1. Open an issue describing the problem or idea before writing code — this
   helps avoid duplicated work.
2. Fork the repository and create a separate branch for your change.
3. Keep commits small and logically self-contained.
4. Open a pull request describing what changed and why.

## Code style

- The project is written in C++17.
- Comments and identifiers must be in English.
- File and function names should be descriptive, without prefixes like
  `proj_`.
- Header files go in `include/`, source files go in `src/`.
- Before opening a PR, make sure `make` builds without errors and warnings (the
  Makefile enables `-Wall -Wextra`).
- Formatting follows the Google C++ Style Guide: 4-space indentation,
  opening braces on the same line, `snake_case` for variables, `PascalCase`
  for classes and methods.
- It can be very effective to ask a coding assistant to confirm and explain a
  bug, but then verify correctness of the generated explanation by implementing
  and testing your own solution. This process saves you time for the mechanical
  steps, but ensures you understood the problem and solution.
- Contributions must disclose AI assistance, e.g. "Created with help by Claude Opus 4.5".

## Testing changes

Since ace is a window manager, breaking your main X session can be
disruptive. Test changes in Xephyr (see README.md) before using a build as
your primary window manager.

## Areas open for contribution

Current priorities:

- Basic stability of the floating mode;
- Multi-monitor support;

Before sending a pull request for new features, please check with us that the feature is something we want to see in ace by opening an issue which has "feature request" or "enhancement" in its title.

If you want to pick up one of these, please open an issue first so we can
coordinate.
