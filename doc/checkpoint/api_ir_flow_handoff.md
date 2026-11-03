# Frontend and Circuit-IR Implementation Handoff

**Checkpoint date:** 2026-11-03

## Purpose and scope

This handoff resumes the agreed design with the first implementation work:
make the parser-driven RTL validation/elaboration and private circuit-IR
construction stages real. It is a checkpoint, not a normative specification.
Keep discussion at API and execution-flow level; surface substantive conflicts
with the specifications, not routine C details.

The initial coding scope is the frontend through circuit IR. SVA semantic
modeling, proof modeling, solver execution, and a successful public verification
job are later stages. Do not describe those planned stages as implemented.

## Current implementation and test corpus

- The executable remains at the parser/build-bootstrap stage. The existing
  Tree-sitter parser is syntax-only, and the public job API has declarations but
  no implementation.
- An internal first slice now implements parse, validation/elaboration, and
  private circuit-IR construction for scalar and explicitly sized packed I/O
  with direct, same-width continuous aliases. It does not implement the public
  `lemma_run_job` path.
- `include/lemma/trace.h` and `include/lemma/verification.h` define the public
  data/API outline. `lemma_run_job` is not implemented; the headers do not imply
  otherwise.
- `tests/fixtures/rtl/` contains 27 combinational and 27 sequential fixtures.
  All 54 pass syntax parsing. The scalar and signed/unsigned four-bit alias
  fixtures now also have checked frontend/IR tests; the other 52 fixtures have
  no semantic IR or proof expectations checked. `seq_continuous_assign_alias.sv`
  still records an expected FFD and intended assertion outcome descriptively,
  not as a checked proof.
- A previous narrow circuit-builder prototype was reverted at the user's
  request. Start from the current IR contract and the scoped work below, not
  that prototype.

See [`tests/fixtures/rtl/README.md`](../../tests/fixtures/rtl/README.md) and
[`rtl_fixture_preparation.md`](rtl_fixture_preparation.md).

## Agreed public API and execution flow

The accepted design is recorded in
[`engineering-notes/api-boundaries.md`](../engineering-notes/api-boundaries.md).

- The caller loads files and supplies named source buffers. One synchronous
  job selects a top, one labeled assertion or cover, and optional named,
  explicitly sized parameter overrides. Inputs are borrowed for the call.
- Internally, keep these stages distinct: parse; RTL validation/elaboration;
  circuit IR construction; SVA property modeling; proof modeling; proof
  execution. Tree-sitter and the circuit IR stay out of the public API.
- Frontend stages return structured diagnostics; they do not print. A result is
  caller-owned. `completed` always has a proof result; `rejected` and
  `internal_error` do not. A completed result may have any applicable proof
  outcome, including `inconclusive`.
- The CLI/job adapter owns file loading, diagnostic rendering, and process exit
  status. The library analyzes supplied buffers and returns structured data.
- The public diagnostic stage enum is being refined to distinguish job input,
  parsing, RTL validation/elaboration, circuit IR, property model, proof model,
  and proof execution. These enum edits and associated tests are currently
  uncommitted; inspect `git status --short` before working.

## Circuit IR baseline and storage choice

[`internal-representation.md`](../internal-representation.md) is the authority
for the structural IR contract:

- One selected, fully elaborated module with ordered ports, a signal table,
  typed combinational expression graph, full-width combinational drivers, and
  explicit FFD state elements.
- All parameter/generate decisions, signal widths, expression sizing, and
  signedness are resolved before the corresponding structures enter the IR.
- The IR must preserve all accepted behavior; unsupported or ambiguous source
  constructs must not be silently discarded.
- The IR excludes SVA monitors, assumptions, proof clocks, proof-start policy,
  and solver terms.

Accepted storage outline: one private owning circuit object contains arrays of
signals, expressions, combinational drivers, and FFDs. Signal and expression
IDs are positions in their respective arrays. See
[`engineering-notes/circuit-ir-storage.md`](../engineering-notes/circuit-ir-storage.md).

Provisional elaboration handoff: do not create a full elaborated-design copy
that duplicates the IR. Keep elaboration as a separate pass; pass only compact
resolved facts to IR construction if needed, then release them. Revisit this
once pass interfaces and memory behavior are visible in code.

## Implemented first profile

The first profile is now documented in [`rtl-support.md`](../rtl-support.md)
and exercised by the scalar and vector alias fixtures. It accepts one selected
flat module with scalar or explicitly sized packed `wire`/`logic` I/O and direct
same-width signal-to-output continuous assignments. Parameters, hierarchy,
generate, procedural logic, internal declarations, and other module items are
outside this incremental slice and are rejected.

The internal stages are separately callable and testable. Validation creates
only resolved port and alias facts; IR construction creates the owning arrays
and array-position IDs. The implementation rejects malformed, out-of-profile,
undriven, multiply-driven, width-mismatched, and cyclic aliases.

## What remains

1. Integrate internal stages with the public named-source job representation
   only when the later SVA/proof path can uphold the result contract. The
   current internal parser handles one source buffer; `lemma_run_job` remains a
   declaration and must not report frontend-only success as `completed`.
2. Expand the RTL slice incrementally: parameter/default and override
   resolution, generate, hierarchy, richer expressions and operators, internal
   signals, procedural combinational logic, and sequential FFD construction all
   require their documented semantics and tests.
3. Keep adding stage-specific success and rejection tests. The current alias
   tests do not validate the other 52 fixtures, SVA property meaning, proof
   outcomes, or traces.

The current RTL-support document contains material TBDs that block broad
acceptance, including exact general expression sizing and unsized-literal
rules, precise procedural statement/expression forms, generate scoping/naming,
module hierarchy details, and incomplete-assignment/latch behavior. Treat these
as specification gaps, not invitations to guess. Before implementing a slice
that depends on a TBD, make the focused specification decision first.

Important API constraint: a parse/elaboration/IR-only success is not a
`completed` verification job, because `completed` guarantees a proof result.
Keep the early stages independently testable internally; do not fake a public
proof result or report frontend-only success as verification. Defer a successful
`lemma_run_job` implementation until its later property/proof stages can honor
the public contract.

## Handoff for the next coding task

The scalar/vector alias slice is implemented. Next work should expand the
frontend only to another bounded, specification-backed slice. Use the project
sequence `task-preparer` → `builder` → `executor-reporter` for C changes; stop
for alignment if the next feature depends on an RTL-support **TBD** rather than
choosing semantics in code. Do not begin SVA/proof implementation as part of
an RTL-only expansion.

## Working-tree note

At checkpoint update, the working tree has uncommitted frontend/IR code, tests,
the RTL-support and fixture documentation updates, engineering-note updates,
and changes under `.opencode/agents/`. Inspect `git status --short` before
editing or committing; preserve agent-configuration and other unrelated changes.
The public API headers and earlier handoff/API decisions are committed. This
checkpoint itself is updated alongside the current implementation work.
