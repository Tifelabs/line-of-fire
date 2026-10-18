# Line of Fire

Side-view, up-to-4-player combat game where everything is technical drawing.
Pencil on A4 sheets. See `docs/` for the design doc.

## Setup (macOS, Apple Silicon)

    xcode-select --install
    brew install cmake ninja ccache llvm
    export CPM_SOURCE_CACHE=$HOME/.cache/CPM     # optional, shares downloads

## Build and run

    cmake --preset debug
    cmake --build --preset debug
    ctest --preset debug
    ./build/debug/line_of_fire

Controls (dev): P1 = A/D + W or Space, P2 = arrow keys. Esc quits.

Headless (no SDL), good for CI:

    cmake --preset tests-only && cmake --build --preset tests-only && ctest --preset tests-only

## Rules that protect the netcode

- `src/geometry` and `src/sim` never include SDL or rendering code, and use no floats (Q20.12 fixed point).
- Never build with `-ffast-math`. Floats live only in `src/render` and `src/app`.
- World units: 1 unit = 1 cm of paper, an A4 sheet is 29.7 x 21.0 units, a player is about 2 units tall.
- Squares of fixed-point values overflow past ~724 units, so keep distances below that.
- Same inputs must give the same state: `tests/test_sim_determinism.cpp` guards this.
