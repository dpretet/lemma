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

- The executable is at the parser/build-bootstrap stage. Tree-sitter syntax
  parsing exists; the job API has declarations but no implementation. RTL
  subset validation/elaboration and circuit-IR construction are not implemented.
- `include/lemma/trace.h` and `include/lemma/verification.h` define the public
  data/API outline. `lemma_run_job` is not implemented; the headers do not imply
  otherwise.
- `tests/fixtures/rtl/` contains 27 combinational and 27 sequential fixtures.
  All 54 pass syntax parsing only; no broad IR or proof expectations are
  machine-checked. `seq_continuous_assign_alias.sv` records an expected FFD and
  intended assertion outcome descriptively, not as a checked proof.
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

## What remains before and during implementation

1. **Choose a narrow, fully specified first vertical slice.** The grammar
   accepts more than Lemma's subset. Start with constructs whose behavior is
   actually defined in [`rtl-support.md`](../rtl-support.md); reject unsupported
   constructs explicitly. Do not try to machine-check all 54 fixtures at once.
2. **Connect parsing to named job sources.** The existing Tree-sitter parser is
   syntax-only. Add internal source/job parsing and stage diagnostics without
   exposing Tree-sitter objects or taking over file loading.
3. **Implement RTL validation/elaboration for the chosen slice.** Select the
   top, apply parameter overrides/defaults, and resolve only elaboration forms
   whose conversion and semantics are specified.
4. **Build and validate the private circuit IR.** Enforce ID resolution,
   concrete positive widths, explicit expression types, FFD invariants, and
   preservation of all accepted source behavior. Add success and rejection
   tests at the parse, elaboration, and IR layers.

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

Use the project sequence `task-preparer` → `builder` → `executor-reporter` for
C changes. Have the preparer identify a bounded, specification-backed first
slice and relevant engineering notes. The builder should implement that slice,
add accepted and rejection tests, and run `./flow.sh test`; the executor should
independently run the exact validation commands. If the selected slice requires
an unresolved RTL semantic choice, stop for alignment rather than broadening
the subset implicitly.

## Working-tree note

As of this checkpoint, the following work is uncommitted and must be preserved
or deliberately committed by the owner:

- `include/lemma/verification.h` and `tests/unit/test_verification_header.c`
  contain the diagnostic-stage refinement;
- `doc/engineering-notes/api-boundaries.md` and its index contain recent API
  decisions;
- `doc/engineering-notes/circuit-ir-storage.md` is a new provisional design
  note; and
- this checkpoint file is updated to capture the current frontend/IR handoff.

The public API headers and prior API design notes are already committed; inspect
current status rather than relying on this historical list after resuming.
