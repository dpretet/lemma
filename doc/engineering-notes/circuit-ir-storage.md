# Circuit IR Storage

- **Status:** accepted (owning circuit object and array-position IDs; direct
  elaboration-to-IR builder handoff); open (allocator policy)
- **Date:** 2026-11-07
- **Context:** The circuit IR contract in [`internal-representation.md`](../internal-representation.md)
  defines circuit-local signal and expression IDs, but not their C storage.
- **Decision:** The private circuit IR uses one owning circuit object with
  arrays for signals, expressions, combinational drivers, and FFDs. Each
  signal/expression ID is its position in the corresponding array.
- **Decision:** The first frontend/IR implementation slice must support both
  scalar (one-bit) I/O and explicitly sized multi-bit packed I/O for direct,
  same-width continuous signal aliases. The normative input profile is in
  [`rtl-support.md`](../rtl-support.md); this is an incremental starting slice,
  not a permanent restriction on the supported subset.
- **Decision:** Prioritize making parsing, RTL validation/elaboration, and IR
  construction a reusable end-to-end frontend before expanding the operator or
  procedural subset. The alias slice is a bootstrap example, not completion of
  those stages.
- **Decision:** Keep elaboration as a separate validation and resolution layer,
  but do not create an owning elaborated-design object that duplicates the
  circuit IR. Elaboration populates a builder that owns the final IR storage;
  finalization yields the circuit. Temporary scratch state is allowed where
  required for resolution or validation, but it must not duplicate the circuit
  signal/expression/driver graph. The extraction visitor supplies borrowed
  syntax views rather than an owning syntax-design copy; see
  [`api-boundaries.md`](api-boundaries.md).
- **Rationale:** This matches the specified circuit-local IDs and makes
  references straightforward to resolve and validate while keeping the IR
  private to the implementation.
- **Allocator status:** Verified: `tsoding_arena` is linked into `lemma_core`,
  but the production frontend has no arena calls; its only current use is the
  unit test. The bundled arena asserts on underlying allocation failure
  (`third_party/tsoding-arena/arena.h`, `new_region`), and its implementation is
  built with assertions enabled. The frontend's current `malloc`/`free` use
  permits recoverable allocation errors and cleanup. **Open:** choose whether
  to make arena allocation fallible before using it for frontend/IR ownership,
  or keep ordinary heap allocations for those stages. Do not treat the current
  mix as a settled allocator policy.
- **Frontend implementation status (verified 2026-11-07):** Parsing,
  Tree-sitter extraction, first-profile elaboration, direct IR building, and CLI
  frontend integration are implemented in `src/frontend/` and `src/main.c`.
  `./flow.sh test` passes. Process and broader declaration syntax are extracted
  but remain rejected by the first RTL profile.
- **Reference:** [Circuit Internal Representation](../internal-representation.md).
