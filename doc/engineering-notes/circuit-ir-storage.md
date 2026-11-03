# Circuit IR Storage

- **Status:** accepted (owning circuit object and array-position IDs); provisional
  (elaboration-to-IR handoff)
- **Date:** 2026-11-03
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
- **Provisional:** Do not create a full materialized elaborated-design object
  that duplicates the circuit IR. Keep elaboration as a separate validation and
  resolution pass over the parsed design. If needed, pass only a small temporary
  context of resolved facts (such as parameter values and generate choices) to
  IR construction; build the final circuit IR from the syntax tree and that
  context, then release the temporary data. Revisit this choice when the pass
  interfaces and memory usage are clearer.
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
- **Reference:** [Circuit Internal Representation](../internal-representation.md).
