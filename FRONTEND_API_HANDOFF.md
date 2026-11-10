# Frontend API and Implementation Handoff

> **Status: ACTIVE.** This root-level handoff records API choices and scopes the
> frontend rebuild. It is not a normative RTL specification. Current specs and
> later user alignment take precedence.

**Date:** 2026-11-10

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

## Implementation status

The frontend implementation is present in `src/frontend/`: parsing, nested
Tree-sitter extraction, first-profile validation/elaboration, IR builder, and the
one-shot entry point. The CLI loads a file and calls that entry point; it does
not claim proof success. The builder owns the final IR arrays and returns a
circuit on successful finalization.

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

The documented first profile is implemented; syntax extraction also describes
processes, declarations, and statements that the first profile rejects. Do not
extend RTL acceptance without updating `doc/rtl-support.md` and adding focused
tests. `lemma_run_job`, SVA/proof modeling, and solver execution remain out of
scope.

The two agreed C refactors are complete. Keep extraction syntax-only and preserve
raw provenance; do not infer or inherit declaration attributes during extraction.
The typed facts distinguish escaped/user-defined types from built-in wire/logic,
and retain all packed dimensions so elaboration can reject unsupported extras.
Preserve the packed-range comment behavior and tests. Do not broaden RTL
acceptance without updating `doc/rtl-support.md` and adding focused tests.

## Verification

`./flow.sh test` passes all seven CTest targets after the rebuild. Unit coverage
checks parser failures, scalar/vector alias IR, inherited ANSI shorthand ports,
syntax-view grouping, assignment destination/RHS/ternary-condition slices,
process statements, typed declaration facts and borrowed bounds, escaped-type
and extra-packed-dimension rejection, and first-profile RTL rejections.
Keep the build warning-clean and run the full test command after C changes.

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

Stack-depth robustness is an accepted priority. The parser's recursive error
search and the syntax visitor's recursive statement walks must not exhaust the C
stack on deeply nested input. Replace them with iterative traversal or add a
checked depth limit and diagnostic before broadening the accepted frontend
profile; cover the limit with a deep-input regression test.
