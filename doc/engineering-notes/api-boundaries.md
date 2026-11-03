# Public API and Frontend Boundaries

- **Status:** accepted (stage, source, diagnostic, IR-visibility, outcome,
  public result layout, diagnostic read-only array and field layout,
  diagnostic span representation, result ownership and return shape,
  diagnostic-stage granularity, job-status, public function declarations,
  clock-selection, parameter-value, and trace-content boundaries, and shared
  arbitrary-width bit-vector type);
  provisional (including asynchronous-reset transitions in traces); open
  (target-label encoding, result serialization, and NULL-argument behavior)
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
- **Decision:** Diagnostics are exposed as a contiguous read-only array in the
  public result structure, with a `diagnostic_count` field. The records and
  their strings are owned by the result and remain valid until it is destroyed.
  An empty diagnostic list has count zero and a NULL array pointer.
- **Decision:** The public diagnostic record carries stage, severity, stable
  code, message, optional source name, and optional inline source span. Source
  names and diagnostic strings are result-owned. A missing source is represented
  by a NULL name; `has_span` indicates whether the inline span is available.
- **Decision:** Public source spans use Lemma-owned `size_t` fields for byte
  offsets and Tree-sitter-compatible zero-based row and byte-column positions.
  The end byte/position is exclusive. Tree-sitter types do not appear in the
  public verification header.
- **Decision:** Public diagnostic stages distinguish job input, parsing, RTL
  validation/elaboration, circuit IR construction, SVA property modeling, proof
  modeling, and proof execution. The circuit IR, property model, and proof model
  remain separate stages; diagnostics identify the stage that produced them.
- **Decision:** Job completion and proof outcome are distinct. Parse,
  elaboration, or validation failures return diagnostics without a proof
  classification; `failed` is reserved for a counterexample to a successfully
  built proof problem. The job result also carries an explicit overall status,
  independent of any proof outcome: `completed`, `rejected`, or
  `internal_error`. `completed` can include an `inconclusive` proof result; a
  proof result is absent for `rejected` or `internal_error` jobs. The structured
  result is authoritative; the CLI exit code summarizes job execution, not
  proof success (`completed` exits successfully, other job statuses do not).
- **Decision:** A `completed` job always has a proof result for its selected
  target. A `rejected` or `internal_error` job has no proof result.
- **Decision:** A job result contains structured diagnostics and an optional
  proof result. The proof result identifies the selected target label/kind and
  outcome, with a full trace when the outcome provides a counterexample or
  witness. Frontend errors leave the proof result absent.
- **Decision:** The API returns a result object owned by the caller and released
  through a destroy function. Diagnostic text, source names/provenance, and any
  returned counterexample/witness data are owned by that result, not borrowed
  from the input source buffers.
- **Decision:** The job API uses a return-pointer shape (`lemma_run_job`) and a
  destroy function (`lemma_result_destroy`). Ordinary job outcomes, including
  rejection and internal error, are represented in a non-NULL result. NULL is
  reserved for failure to create a result object, such as allocation failure.
  This keeps job execution status distinct from inability to produce the result.
- **Decision:** `lemma_result` is a public structure with `status`,
  `diagnostic_count`, a read-only `diagnostics` array pointer, and an optional
  read-only `proof` pointer. The proof pointer is NULL when no proof result is
  available. Callers inspect the structure and release it with
  `lemma_result_destroy`.
- **Decision:** `lemma_proof_result` is a public structure with the selected
  `target_label`, `target_kind`, `outcome`, and an optional read-only `trace`
  pointer. The trace pointer is NULL when the outcome has no counterexample or
  witness. The label and trace are owned by the containing result.
- **Decision:** The public job structures provide named source buffers and
  optional named parameter overrides. An override reuses `lemma_bit_vector`;
  the source and parameter input storage is borrowed only during the job call.
- **Decision:** `property_label` is supplied as a NUL-terminated C string and
  matches the parsed target label exactly and case-sensitively within the
  selected top. The selected target label is copied into the result.
- **Decision:** The public header declares `lemma_run_job`, result destruction,
  and status/diagnostic/proof accessors. This API declaration does not imply
  that the parser-stage executable implements verification jobs.
- **Decision:** The public proof-outcome enum has `proven`, `failed`,
  `inconclusive`, `covered`, and `unreachable` values. Assertions use the first
  three; covers use the last three. This follows the outcome contract in
  [`tool-flow.md`](../tool-flow.md).
- **Decision:** A public `lemma_trace` structure contains a transition count
  and a read-only array of transitions. The transitions and their values are
  owned by the containing result.
- **Provisional:** Keep both clock-edge and asynchronous-reset transitions in
  the trace for now; the user is not yet convinced reset transitions are
  useful. This follows the existing trace contract in [`tool-flow.md`](../tool-flow.md),
  and is not a specification change.
- **Decision:** Each trace transition's event record identifies its kind
  (clock edge or asynchronous reset), signal name, and edge (`posedge` or
  `negedge`). The event signal name is owned by the containing result.
- **Decision:** Each trace value record carries its own `signal_name` and a
  `lemma_bit_vector` value. Names are repeated in each valuation rather than
  maintained in a separate trace-level signal table, so each value identifies
  its signal directly.
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
- **Decision:** The public API will use one reusable arbitrary-width bit-vector
  type (`lemma_bit_vector`) for values such as parameter overrides and trace
  valuations, rather than a native fixed-width integer. It has `width_bits`,
  `is_signed`, and a byte pointer; the byte length is derived from the width,
  with little-endian packing. Job inputs borrow parameter bytes for the call;
  trace bytes are owned by the result.
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
- **Still open:** Target-label encoding, behavior of API functions for NULL
  arguments, and whether a stable read-only IR serialization is useful. JSON is
  one possible serialization, not a selected format; it is distinct from the
  test-oracle format.
- **References:** [Tool flow](../tool-flow.md), [internal representation](../internal-representation.md),
  and [`include/lemma/parser.h`](../../include/lemma/parser.h).
