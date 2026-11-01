# API, IR, and Verification-Flow Handoff

**Checkpoint date:** 2026-11-01

## Purpose

Resume the design discussion for Lemma's public API, circuit IR, SVA property
model, and verification flow. This is a handoff, not a normative specification.
The user wants to exchange on design before more implementation; do not resume
the reverted circuit-builder prototype or write code without a scoped, aligned
design.

## Current implementation and fixtures

- The implementation is at the parser/build-bootstrap stage. Tree-sitter
  parses SystemVerilog; RTL elaboration, circuit IR extraction, SVA semantic
  processing, and proof are not implemented.
- A previous narrow circuit-builder prototype was reviewed and reverted at the
  user's request. The clean restart is to define the IR/API first.
- `tests/fixtures/rtl/` contains 27 `comb_*.sv` and 27 `seq_*.sv` sources. They
  have passed syntax parsing, but are not generally machine-checked for IR or
  proof behavior.
- `seq_continuous_assign_alias.sv` contains inline SVA labeled
  `target_continuous_assign_alias`. Its expected FFD and intended `proven`
  result are descriptive only; no SVA model or proof currently checks them.

See [`tests/fixtures/rtl/README.md`](../../tests/fixtures/rtl/README.md) and
[`rtl_fixture_preparation.md`](rtl_fixture_preparation.md) for fixture details.

## Accepted API/flow decisions

The current accepted design boundaries are also recorded in
[`engineering-notes/api-boundaries.md`](../engineering-notes/api-boundaries.md).

- Keep Tree-sitter types private. Use a top-level public verification-job API;
  keep parsing, RTL validation/elaboration, circuit IR, SVA property modeling,
  and proof modeling as distinct, independently testable internal stages.
- File loading is separate from RTL analysis. The CLI/job adapter supplies
  named source buffers; the synchronous analysis call borrows them only for the
  call and copies any retained names/provenance.
- The circuit IR remains private to the public API.
- SVA is inline in the selected top module for the MVP. No Lemma-specific
  sidecar attachment; standalone SVA files and external attachment (`bind` or
  hierarchy) are deferred.
- Inline SVA statements are collected into a property model separate from the
  circuit IR. Each target assertion/cover has a unique label, and the job
  selects one by label. Supported assumptions in the selected top constrain
  that target. The proof clock comes from its SVA event control.
- Job parameter overrides are an optional named map. Omitted entries use RTL
  defaults. Values are fully known bit-vectors with explicit width/signedness,
  byte-array representation, little-endian bit packing, and standard
  SystemVerilog parameter-assignment conversion. Truncation emits a warning.
- Diagnostics are returned as structured records (stage, severity, stable
  code, message, source name, byte span, and line/column span); frontend stages
  do not print.
- A caller-owned job result contains diagnostics and an optional proof result.
  Parse/elaboration/validation failure has diagnostics but no proof outcome;
  `failed` means a counterexample to a successfully built proof problem.
- Job statuses are `completed`, `rejected`, or `internal_error`. A completed
  job may have any proof outcome, including `inconclusive`. CLI exit status
  summarizes job execution: completed jobs exit successfully regardless of
  proof outcome; rejected/internal-error jobs do not.
- Counterexample and cover traces use full per-step valuations of the modeled
  proof cone. Each transition identifies its event and pre-/post-event
  valuations. At a clock edge, pre-event values are the SVA sample; asynchronous
  reset events are not themselves SVA samples. Delta-only traces are
  insufficient.

## Still open

- Exact C function/type signatures and API implementation; ownership of result
  subobjects and diagnostic field types need to be made concrete without
  exposing Tree-sitter or mutable IR internals.
- How the target label and typed parameter map are represented in the C job
  structure.
- Exact proof-result structures and trace encoding. A JSON serialization is
  only a possible future external view; neither an IR serialization nor a
  machine-readable test-manifest format has been chosen.
- The accepted test-oracle layers are parsing, RTL extraction, SVA
  interpretation, and proof outcome. Keep expected circuit behavior, expected
  property meaning, and expected proof status separate. The first SVA
  expectation is not yet machine-checked.
- The circuit IR contract in [`internal-representation.md`](../internal-representation.md)
  remains the design baseline; implementation details need to be drafted
  against it before coding.

## Suggested next discussion

Draft the public C job/result type outline without implementing it. Start with
the job's named-source list, selected top and property label, optional typed
parameter map, and the result's overall status, diagnostics, optional proof
result, and trace. Resolve one API detail at a time, then review the IR's
internal data model and stage boundaries before authorizing implementation.

## Working-tree note

At checkpoint creation, the working tree has uncommitted documentation and
fixture changes listed by `git status --short`, plus a modified
`.opencode/agents/builder.md`. Preserve these; inspect status before editing or
committing. This handoff and the API-boundary engineering note are
new/untracked at this point.
