# Frontend API and Implementation Handoff

> **Status: ACTIVE.** This root-level handoff records API choices and scopes the
> frontend rebuild. It is not a normative RTL specification. Current specs and
> later user alignment take precedence.

**Date:** 2026-11-17

## Purpose and implementation boundary

Rebuild the frontend as separate parsing, syntax extraction, RTL
validation/elaboration, and circuit-IR construction stages. Source loading is
performed by the CLI or caller; the analysis code receives source buffers. Do
not implement `lemma_run_job`, SVA modeling, proof modeling, or solver execution
as part of this frontend work.

Use `doc/rtl-support.md` as the authority for accepted RTL. Tree-sitter may parse
more syntax than Lemma accepts. Extraction records syntax; elaboration decides
whether that syntax is supported and resolves its meaning. Do not silently drop
unknown module items or statements.

## Agreed API and code-structure choices

- A parsed-source handle borrows its source buffer until the handle is destroyed
  and owns its Tree-sitter tree. Do not copy the source or build a second syntax
  tree. Source slices are borrowed and valid only for their documented lifetime.
- Use the shared `lemma_source_range` from `include/lemma/source_range.h` for
  source coordinates in frontend and verification diagnostics. Byte offsets are
  half-open; rows and byte columns are zero-based. Diagnostic fields are named
  `source_range` and `has_source_range`.
- Keep Tree-sitter types in `include/lemma/frontend_ts_internal.h`, which is an
  internal header despite its location. Keep them out of syntax-view types.
- Use Tree-sitter's direct-child traversal. Basic reusable helpers provide
  explicit all/named/named-non-extra child filtering, ordered child visitation,
  unique direct-child lookup, field lookup, checked node text, and source ranges.
  Avoid allocated child arrays and generic recursive searches for grammar items.
- Implement grammar extraction as nested functions that follow the source tree.
  The intended call structure is:

  ```text
  visit_source
    visit_module
      visit_parameters
      visit_ports
        visit_port
      visit_body
        visit_signal_declaration
          visit_declarators
        visit_continuous_assignments
          get_destination_signal
          get_right_part_of_line
            get_condition_if_present
        visit_process
          extract_event_control
          visit_statements
            visit_statement recursively
  ```

  These grammar-specific helpers should be private C functions unless another
  module needs to call them. The syntax visitor callback is the way extracted
  records are delivered; it does not require a flat traversal implementation.
- Keep syntax records borrowed and callback-driven. Do not create an owning
  syntax-design copy or an owning elaborated-design object that duplicates the
  circuit. The circuit builder owns the final IR storage while it is populated;
  finalization returns the circuit. Internal builder operations add the module
  name, signals, ordered ports, read expressions, and drivers, and provide
  read-only access for validation. FFD operations are deferred.
- Represent shared declarations with one declaration-group event followed by
  per-name declarator events linked by source range. The group carries shared raw
  syntax; each declarator carries its name, suffix, and initializer/default.
  Preserve ANSI shorthand ports with omitted attributes as written. Extraction
  does not copy or infer attributes; elaboration applies the accepted inheritance
  rule in `doc/rtl-support.md` and rejects partial omissions.
- `main` handles CLI arguments and file loading, then calls
  `lemma_frontend_build`. That entry point runs parsing through circuit-IR
  construction and selects the sole module automatically; no `--top` option or
  syntax-only CLI mode is needed for this profile.
- Process and statement views are syntax-only. Keep source order and nesting
  ranges/depth; preserve unclassified constructs as opaque source slices. Do not
  infer combinational/sequential behavior or implement process semantics unless
  `rtl-support.md` specifies them.
- Function comments in C/H files must follow the exact separator-delimited
  format in `AGENTS.md`.
- Check documented preconditions before calling a function, including a
  function declared in a public header. The callee may rely on them and should
  not repeat the same argument or semantic checks. Keep checks for failures
  caused by operations performed inside the function, such as allocation and
  checked size arithmetic. See
  [`api-boundaries.md`](doc/engineering-notes/api-boundaries.md).

## Implementation status

The frontend implementation is present in `src/frontend/`: parsing, nested
Tree-sitter extraction, first-profile validation/elaboration, IR builder, and the
one-shot entry point. `lemma_frontend_build` returns a caller-owned result with
an optional circuit and a read-only diagnostic array; parse, RTL, and IR
failures return a result with an error diagnostic, while NULL is reserved for
result allocation failure. The result owns its circuit and diagnostics. The CLI
prints diagnostics and reports circuit construction only; it does not claim
proof success.

User-aligned next target: all 54 modules in `tests/fixtures/rtl/` must pass
validation/elaboration and produce circuit IR, without proof execution. The exact
constructs in those fixtures define the support boundary; `doc/rtl-support.md`
and `doc/internal-representation.md` now describe that boundary. The one inline
assertion is outside circuit IR and is not modeled or proved by this milestone.

Source-controlled recursive parser and syntax walks now stop at a checked
128-level limit. A parser error tree that exceeds the bound returns a
source-located diagnostic; syntax visitation returns its failure result. Tests
exercise 160 nested constructs and parse all 54 fixtures.

Reusable direct-child and checked range/text Tree-sitter helpers now live in
`frontend_ts_helpers.c`; grammar-specific nested visitors remain private in
`frontend_ts.c`. Declaration-group views retain raw syntax and expose syntax-only
built-in type, `var`, signedness, first packed-bound slices, and packed-dimension
count. Elaboration consumes those typed facts and parses the bounds individually.

The statement `header` currently carries syntax-only control/label text. Its
exact per-kind ranges are an implementation detail and can be refined with
focused tests if consumers need a stronger guarantee. Parent range and depth
preserve nesting; add branch-role metadata only if a consumer needs then/else
roles without inspecting source.

Frontend and verification diagnostics now share `lemma_source_range`; the old
`lemma_ir_source_span`, `lemma_source_span`, and `span` field names were replaced
without changing layout or coordinate semantics.

## Next implementation scope

Circuit-IR lowering covers the scalar and vector alias fixtures,
`comb_continuous_assign_boolean.sv`, and the six vector bitwise, reduction,
unsigned arithmetic, and unsigned comparison fixtures. Syntax extraction
classifies these forms while retaining source provenance and parent/operand
relationships. Lowering preserves source-ordered operand IDs, typed widths and
signedness, and all nested signal dependencies. The other 45 fixture modules
remain pending RTL/IR checks; parser coverage of all 54 fixtures is not full IR
coverage. Mixed-width and signed bit-vector operators, division/remainder,
constants, selections, conversions, procedural processes, FFDs, and warning
support remain outside this slice. Continue with focused expression, sequential,
and other fixture-bounded IR slices and expected-IR and boundary tests. Do not
add constructs beyond the fixture-bounded RTL specification.
`lemma_run_job`, assertion modeling, proof modeling, and solver execution remain
out of scope.

Duplicate-check cleanup is complete across the internal circuit-builder API:
elaboration validates argument and semantic preconditions before calling the
builder, and the builder relies on those contracts. This includes valid builder,
name, width, direction, ID, and output-pointer arguments; `finalize` likewise
requires a populated builder after successful elaboration. Count queries require
a non-NULL builder. The signal, port, driver, and expression accessors require a
non-NULL builder and in-range ID/index from their callers; they return borrowed
pointers valid for the table lifetime and do not repeat bounds checks.

Intentional checks remain for SIZE_MAX name lengths before adding a terminator,
signal/port/expression/driver ID capacity, array reservation and allocation
failures, and final circuit allocation failure. Its destroy functions remain
NULL-safe. In particular, elaboration validates operator kind, arity, operand
IDs, signedness, and widths before calling the IR operator builder, which
continues to rely on those preconditions while checking expression-ID capacity
and allocation failure.

Keep extraction syntax-only and preserve raw provenance; do not infer or inherit
declaration attributes during extraction. The typed facts distinguish
escaped/user-defined types from built-in wire/logic and retain all packed
dimensions so elaboration can reject unsupported extras. Preserve the
packed-range comment behavior and tests.

The remaining frontend caller-precondition cleanup is complete. The syntax
visitor relies on its caller to provide a live parsed handle with non-NULL tree
and source plus a non-NULL callback; it does not re-check these preconditions.
Elaboration's internal entry requires the successfully parsed handle and fresh
builder already provided by `frontend_build`; its accessors and cycle traversal
rely on IDs established by successful lookup/appends and indices bounded by
builder counts, including the circuit-builder accessors.
The Tree-sitter child, unique-child, range, and text helpers likewise rely on
their audited callers to provide required arguments; optional nodes are checked
at the grammar wrappers before helper calls. Intentional checks remain for
public parse inputs, optional diagnostics, unsupported RTL and semantic
validation, callback stop/failure, source offsets and coordinate
representability, allocation and size arithmetic, and temporary traversal-stack
bounds. The field-name child helper remains defensive because it has no
first-party callers to audit.

## Verification

The parser test verifies the complete 54-file corpus. Circuit-IR tests cover
the alias baseline, boolean continuous assignment, and the six vector-operator
fixtures; the remaining 45 fixtures are still pending RTL/IR implementation.
The latest independent run passed all 7 CTest targets and `git diff --check`.
`./flow.sh test` is the required full validation command after C changes.

## Deferred review findings (triaged 2026-11-08)

The review found two gaps. The escaped-type case is resolved by the typed
syntax-view refactor; drive strength is outside MVP interest and remains deferred:

- An escaped user type spelled `\logic` was mistaken for the built-in `logic`
  keyword by raw text matching and accepted as a one-bit port. Extraction now
  classifies it as an unsupported type form; a focused rejection test covers it.
- A continuous assignment with `drive_strength`, for example
  `assign (strong1, highz0) y = a;`, is accepted even though strength is not
  represented in the IR.
Packed-range comment behavior remains covered after the typed-bound refactor:
valid ranges with inter-token comments are accepted and an unterminated block
comment is rejected.

Stack-depth robustness is addressed for the recursive parser error search and
syntax statement, loop, and event walks. The 128-level bound and 160-level
regressions are recorded in
[`doc/engineering-notes/api-boundaries.md`](doc/engineering-notes/api-boundaries.md).
