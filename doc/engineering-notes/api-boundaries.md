# Public API and Frontend Boundaries

- **Status:** accepted (stage, source, diagnostic, IR-visibility, outcome,
  result-ownership, job-status, clock-selection, parameter-value, and
  trace-content boundaries); open (signatures and result serialization)
- **Date:** 2026-11-01
- **Context:** Lemma is beginning its RTL frontend and equation-based circuit IR;
  the current parser API exposes Tree-sitter's `TSTree` directly.
- **Decision:** Keep Tree-sitter types private to the frontend. The stable public
  API is a top-level verification-job interface. Internally, parsing, RTL
  elaboration/validation, circuit IR construction, SVA property modeling, and
  proof modeling remain separate and independently testable stages.
- **Decision:** File loading is separate from RTL analysis. The CLI/job adapter
  reads source files and supplies named source buffers to the analysis API; the
  RTL analysis library does not open file paths.
- **Decision:** Source buffers are borrowed for the duration of a synchronous
  job call. Frontend stages must not retain caller pointers after return; names
  and source provenance needed in returned results must be copied.
- **Decision:** Frontend stages return structured diagnostics rather than
  printing. The CLI is responsible for rendering diagnostics and choosing its
  process exit status. Diagnostics carry a stage, severity, stable code,
  message, source name, and source range in both byte-offset and line/column
  form.
- **Decision:** Job completion and proof outcome are distinct. Parse,
  elaboration, or validation failures return diagnostics without a proof
  classification; `failed` is reserved for a counterexample to a successfully
  built proof problem. The job result also carries an explicit overall status,
  independent of any proof outcome: `completed`, `rejected`, or
  `internal_error`. `completed` can include an `inconclusive` proof result; a
  proof result is absent for `rejected` or `internal_error` jobs. The structured
  result is authoritative; the CLI exit code summarizes job execution, not
  proof success (`completed` exits successfully, other job statuses do not).
- **Decision:** A job result contains structured diagnostics and an optional
  proof result. The proof result identifies the selected target label/kind and
  outcome, with a full trace when the outcome provides a counterexample or
  witness. Frontend errors leave the proof result absent.
- **Decision:** The API returns a result object owned by the caller and released
  through a destroy function. Diagnostic text, source names/provenance, and any
  returned counterexample/witness data are owned by that result, not borrowed
  from the input source buffers.
- **Decision:** Counterexample and cover traces contain a full per-step
  valuation of the modeled proof cone. Each transition identifies its event
  and pre-event/post-event valuations. At a selected clock edge the pre-event
  values are the SVA sample; an asynchronous reset event is not itself a
  sample. Delta-only traces are insufficient.
- **Decision:** The job accepts named top-level parameter overrides. Parameters
  omitted from the map use their RTL-declared defaults.
- **Decision:** Each supplied parameter override is an explicitly sized
  bit-vector with explicit signedness, represented by a byte array, and is
  fully known (no `X`/`Z` bits) and converted to the resolved parameter type
  using standard SystemVerilog parameter-assignment conversion rules.
- **Decision:** Parameter bytes use little-endian bit packing: byte zero holds
  the least-significant bits, bit zero is the least-significant bit of its byte,
  and unused high bits in the final byte are zero.
- **Decision:** If that conversion discards high-order bits, emit a warning that
  identifies the parameter and notes that its value may change; still apply the
  standard conversion.
- **Decision:** Inline SVA is collected into a property model separate from the
  circuit IR. A job selects one labeled assertion or cover in the selected top;
  supported inline assumptions constrain that target. The proof clock is
  derived from the selected property's event control, not supplied separately
  by the job.
- **Decision:** Keep the semantic circuit IR private to the implementation;
  the public API exposes job inputs, diagnostics, and verification results, not
  mutable IR structures. Tests may inspect IR through internal interfaces.
- **Rationale:** Parser implementation details should not become a public ABI
  contract, while internal stage boundaries allow precise tests and diagnostics.
- **Still open:** Exact C signatures and diagnostic fields, target-label
  encoding, exact proof-result structures, and whether a stable read-only IR
  serialization is useful. JSON is one possible serialization, not a selected
  format; it is distinct from
  the test-oracle format.
- **References:** [Tool flow](../tool-flow.md), [internal representation](../internal-representation.md),
  and [`include/lemma/parser.h`](../../include/lemma/parser.h).
