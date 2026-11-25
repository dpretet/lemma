# Circuit IR Storage

- **Status:** accepted (owning circuit object and array-position IDs; direct
  elaboration-to-IR builder handoff; future arena-backed allocation direction);
  open (arena ownership boundaries, allocation-failure behavior, and retained
  capacity at finalization)
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
  built with assertions enabled. The current production frontend/IR use of
  `malloc`, `calloc`, and `realloc` is transitional, not the intended long-term
  allocation design.
- **Accepted user direction (2026-11-24):** Move frontend and IR storage toward
  arena-backed allocation rather than retaining the current per-object heap
  allocation and array-reallocation approach. This direction is motivated by
  scaling to large designs and future workloads with many SVA properties; it
  does not claim SVA modeling is currently implemented. The arena migration is
  future work, not current behavior.
- **Accepted user decision (2026-11-24):** Use `size_t` for internal circuit-IR
  IDs and references, matching array counts and indexing. Keep Tree-sitter's
  `uint32_t` byte/child values and RTL bit widths/bounds unchanged; they encode
  separate external or language limits. Implemented in the circuit IR, builder,
  elaboration, and tests; `./flow.sh test` passed 7/7 CTest tests.
- **Open:** Define arena ownership/lifetimes across frontend and IR stages, and
  make allocation failure recoverable before using the bundled arena for
  production-owned data. Preserve the current fallible heap-allocation behavior
  until that design is settled; do not treat the bundled asserting allocator as
  a drop-in replacement.
- **Open memory concern (2026-11-24):** Builder arrays start at capacity four
  and double when full. Finalization transfers their pointers and counts into
  the circuit but does not shrink the allocations, so unused capacity remains
  for the circuit's lifetime. If a later frontend stage copies the used entries
  before the circuit is released, this slack can increase peak memory. No
  growth-factor or finalization-compaction policy has been selected; evaluate
  this together with arena ownership and allocation-failure behavior. Evidence:
  `lemma_circuit_grow_array` and `lemma_circuit_builder_finalize` in
  [`frontend_ir.c`](../../src/frontend/frontend_ir.c).
- **Accepted user direction (2026-11-24):** Defer deterministic allocation-
  failure injection tests for the current array-growth reallocations until the
  allocation strategy is redesigned. Revisit failure testing alongside the
  arena/allocator design rather than adding test-only wiring for a transitional
  heap-growth path.
- **Frontend implementation status (verified 2026-11-07):** Parsing,
  Tree-sitter extraction, first-profile elaboration, direct IR building, and CLI
  frontend integration are implemented in `src/frontend/` and `src/main.c`.
  `./flow.sh test` passes. Process and broader declaration syntax are extracted
  but remain rejected by the first RTL profile.
- **Reference:** [Circuit Internal Representation](../internal-representation.md).
