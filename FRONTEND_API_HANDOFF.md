# Frontend API and Implementation Handoff

> **Status: ACTIVE.** This root-level handoff records API choices and scopes the
> frontend rebuild. It is not a normative RTL specification. Current specs and
> later user alignment take precedence.

**Date:** 2026-11-08

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
- Keep Tree-sitter types in `include/lemma/frontend_ts_internal.h`, which is an
  internal header despite its location. Keep them out of syntax-view types.
- Use Tree-sitter's direct-child traversal. Basic reusable helpers provide
  explicit all/named/named-non-extra child filtering, ordered child visitation,
  unique direct-child lookup, field lookup, checked node text, and source spans.
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
  per-name declarator events linked by source span. The group carries shared raw
  syntax; each declarator carries its name, suffix, and initializer/default.
  Preserve ANSI shorthand ports with omitted attributes as written. Extraction
  does not copy or infer attributes; elaboration applies the accepted inheritance
  rule in `doc/rtl-support.md` and rejects partial omissions.
- `main` handles CLI arguments and file loading, then calls
  `lemma_frontend_build`. That entry point runs parsing through circuit-IR
  construction and selects the sole module automatically; no `--top` option or
  syntax-only CLI mode is needed for this profile.
- Process and statement views are syntax-only. Keep source order and nesting
  spans/depth; preserve unclassified constructs as opaque source slices. Do not
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

The current implementation still combines low-level Tree-sitter helpers and
grammar-specific visitors in `frontend_ts.c`. Elaboration also classifies types
and packed widths by scanning raw syntax text.

The statement `header` currently carries syntax-only control/label text. Its
exact per-kind ranges are an implementation detail and can be refined with
focused tests if consumers need a stronger guarantee. Parent span and depth
preserve nesting; add branch-role metadata only if a consumer needs then/else
roles without inspecting source.

## Next implementation scope

The documented first profile is implemented; syntax extraction also describes
processes, declarations, and statements that the first profile rejects. Do not
extend RTL acceptance without updating `doc/rtl-support.md` and adding focused
tests. `lemma_run_job`, SVA/proof modeling, and solver execution remain out of
scope.

Before expanding the RTL subset, review these two refactors with the user:

1. Separate atomic Tree-sitter helpers from grammar-specific module/port/body/
   assignment/process visitors. Keep the nested call structure and expose only
   the reusable helpers and required syntax interfaces in headers.
2. Have extraction report typed declaration facts—built-in type category,
   signedness syntax, and packed bound slices—so elaboration does not search raw
   type text or parse a whole range slice. Keep these facts syntax-only and keep
   current acceptance rules in `rtl-support.md`.

The new session should review the proposed header changes before implementing
these refactors. Preserve the current packed-range comment fix and tests.

## Verification

`./flow.sh test` passes all seven CTest targets after the rebuild. Unit coverage
checks parser failures, scalar/vector alias IR, inherited ANSI shorthand ports,
syntax-view grouping, assignment destination/RHS/ternary-condition slices,
process statements, and first-profile RTL rejections.
Keep the build warning-clean and run the full test command after C changes.

## Deferred review findings (triaged 2026-11-08)

The review found two gaps. The escaped-type case is expected to be addressed by
the typed syntax-view refactor above; the user considers it rare, so it is not a
separate urgent fix. Drive strength is outside MVP interest and remains deferred:

- An escaped user type spelled `\logic` is mistaken for the built-in `logic`
  keyword by raw text matching and is accepted as a one-bit port.
- A continuous assignment with `drive_strength`, for example
  `assign (strong1, highz0) y = a;`, is accepted even though strength is not
  represented in the IR.
The packed-range comment issue was fixed on 2026-11-08: the width scanner skips
whitespace, line comments, and block comments, with acceptance and
unterminated-comment tests.

Stack-depth robustness is an accepted priority. The parser's recursive error
search and the syntax visitor's recursive statement walks must not exhaust the C
stack on deeply nested input. Replace them with iterative traversal or add a
checked depth limit and diagnostic before broadening the accepted frontend
profile; cover the limit with a deep-input regression test.
