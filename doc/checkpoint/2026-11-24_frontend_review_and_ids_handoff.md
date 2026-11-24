# Frontend Review and IR-ID Handoff

- **Status:** ACTIVE
- **Date:** 2026-11-24
- **Base commit:** `4b26034` (`change: Widen frontend IR IDs and clarify reset precedence`)
- **Worktree:** Clean at the base commit before this checkpoint update.

## Current implementation boundary

Lemma currently parses and extracts SystemVerilog, validates/elaborates its
first RTL profile, and builds circuit IR. The executable does not yet implement
the verification-job path, semantic assertion checking, or proof execution.
Keep those planned stages distinct from the implemented frontend.

The circuit IR uses `size_t` for signal and expression IDs and their references,
matching array counts and indexing. Tree-sitter byte/child values and RTL
widths/packed bounds remain `uint32_t`. The parser's Tree-sitter input limit is
unchanged. Packed declaration widths work for ascending and descending bounds;
this does not add ascending select semantics. Continuous-assignment drive
strengths are rejected as unsupported.

Accessor contracts require valid indices and state that borrowed pointers may
be invalidated when their array grows. An elaboration failure without a
diagnostic is classified at RTL validation/elaboration.

## Accepted decisions

- When asynchronous reset is active at a coincident clock edge, the reset branch
  takes precedence over the ordinary clock-triggered update. The normative
  register semantics and related RTL/execution/IR documentation now agree.
- Direct `malloc`/`calloc`/`realloc` use in frontend/IR storage is transitional.
  The intended direction is arena-backed storage for large designs and future
  workloads with many SVA properties.
- Prefer `size_t` for internal IR IDs. Keep Tree-sitter and RTL numeric types
  unchanged.
- Defer deterministic allocation-failure injection for the current realloc
  growth path until the allocator redesign; avoid test-only allocator wiring for
  a strategy expected to be replaced.

## Open design work

- Define arena ownership and lifetimes across frontend/IR stages. The bundled
  `tsoding_arena` asserts on underlying allocation failure; establish a
  recoverable out-of-memory policy before using it for production-owned data.
- Continue the user's static review of frontend argument checks, distinguishing
  genuine allocation/representation guards from redundant or arbitrary checks.
  The fixed `UINT32_MAX` IR element caps are already removed. In particular,
  review whether the Tree-sitter `uint32_t`-to-`size_t` representability checks
  in `frontend_ts_helpers.c` are needed on supported targets, and whether the
  cycle-validation workspace size guards are implied by builder invariants.
  Discuss any code cleanup with the user before editing.
- No optimized Release test run has been recorded. `./flow.sh test` runs Debug;
  CMake enables AddressSanitizer and UndefinedBehaviorSanitizer for GNU/Clang
  test builds.

## Verification evidence

- `./flow.sh test` passed all 7 CTest tests after the code and specification
  changes.
- `git diff --check` passed before the base commit.
- Allocation-failure injection tests were intentionally deferred by user
  direction.

This checkpoint update itself is documentation-only and is not part of commit
`4b26034` unless the user requests a separate commit.
