# Frontend Header Reset Handoff

> **Status: HISTORICAL.** Superseded by
> [`FRONTEND_API_HANDOFF.md`](../../FRONTEND_API_HANDOFF.md). This is a handoff,
> not a normative specification; current specifications and user alignment take
> precedence.

**Checkpoint date:** 2026-11-05

## Current state

The previous frontend implementation was removed at the user's direction:

- Removed `src/frontend/parser.c`, `frontend_ts.c`, `frontend_elaboration.c`,
  and `frontend_ir.c`.
- Removed the corresponding old headers, including
  `include/lemma/parser.h`.
- Draft frontend headers are in `include/lemma/`: `frontend_types.h`,
  `frontend_parse.h`, `frontend_syntax.h`, `frontend_ts_internal.h`,
  `frontend_elaboration.h`, and `circuit_ir.h`. Despite that location,
  `frontend_ts_internal.h` is explicitly internal; Tree-sitter types are
  confined to that header.

These are header drafts only; there is no frontend implementation now. The old
frontend/parser tests and parser-behavior CTest entries have been removed;
unrelated tests and CLI argument checks remain. Production CMake and CLI wiring
still refer to deleted frontend/parser sources and APIs. `./flow.sh test` fails
during CMake configuration because several `src/frontend` C files are missing,
including `parser.c`. The header-only syntax check passes, but this is not
frontend behavior validation.

## Aligned design direction

- Step 1 source loading is owned by the CLI or caller; the analysis API receives
  source buffers.
- A parsed-source handle borrows its buffer until the handle is destroyed.
- Tree-sitter-specific types stay in private frontend headers.
- Extraction uses synchronous callbacks with typed, borrowed syntax views; it
  does not build an owning syntax-design copy.
- RTL elaboration populates a builder that owns the final circuit-IR storage.
  Finalization produces the circuit; there is no separate owning elaborated
  design duplicating the IR.

See [`engineering-notes/api-boundaries.md`](../engineering-notes/api-boundaries.md)
and [`engineering-notes/circuit-ir-storage.md`](../engineering-notes/circuit-ir-storage.md).
The current syntax-view fields and builder interface remain draft-level and
should be reviewed before implementation resumes.

## Next step

Review the draft headers with the user, especially whether the syntax views
carry enough Verilog structure to keep grammar traversal out of elaboration.
Then align the builder operations and only after that reintroduce C
implementations and restore the build/test path. Do not infer frontend behavior
from the headers alone.
