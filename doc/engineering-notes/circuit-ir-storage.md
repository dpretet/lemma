# Circuit IR Storage

- **Status:** accepted (owning circuit object and array-position IDs); provisional
  (elaboration-to-IR handoff)
- **Date:** 2026-11-03
- **Context:** The circuit IR contract in [`internal-representation.md`](../internal-representation.md)
  defines circuit-local signal and expression IDs, but not their C storage.
- **Decision:** The private circuit IR uses one owning circuit object with
  arrays for signals, expressions, combinational drivers, and FFDs. Each
  signal/expression ID is its position in the corresponding array.
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
- **Reference:** [Circuit Internal Representation](../internal-representation.md).
