# RTL Fixture Preparation Checkpoint

**Checkpoint date:** 2026-10-15

## Purpose

This checkpoint records the initial positive RTL source corpus prepared before
implementing the RTL semantic frontend, netlist construction, or equation-based
IR. It is a handoff note, not a replacement for the normative specifications.

## What is in place

- Added 26 small standalone fixtures under `tests/fixtures/rtl/`, catalogued in
  [`tests/fixtures/rtl/README.md`](../../tests/fixtures/rtl/README.md).
- The fixtures cover continuous assignments; combinational `if` / `else`,
  `case`, `always @(*)`, and `always_comb`; procedural assignment ordering;
  vector conditions; composed drivers and fan-out; width and signedness
  conversions; selected unsigned bitwise, reduction, arithmetic, and comparison
  operators; concatenation; and constant/variable packed selects.
- The current source specification records selected rules for these examples,
  including `always_comb` equation lowering, signedness casts and assignment
  width conversion, operator result widths, and rejection of selects that may
  be out of range.
- All fixture files were run through `./build/lemma` and passed Tree-sitter
  syntax parsing.

## Current limitations

- The executable currently checks syntax only. The frontend does not yet
  validate the supported subset, elaborate these examples, construct a netlist
  or IR, or compare the result to an expected equation.
- These fixtures are not yet registered in CTest as per-file tests; there are
  no automated semantic expectations.
- The examples are positive cases. A separate misconceptions/rejection suite
  is still needed for MVP coverage, including incomplete combinational
  assignments, inferred latches, and invalid or unsupported constructs.
- Some combinational details remain TBD in `doc/rtl-support.md`, including
  general expression sizing, mixed-width/signed operators, division/remainder,
  replication, and `-:` indexed part-selects. Do not infer support from a
  fixture where the specification remains open.

## Suggested next work

1. Implement the smallest parse-to-IR vertical slice and give it a machine-
   checked expected result.
2. Register fixture expectations with the automated test suite as corresponding
   semantic stages become available; syntax acceptance alone is insufficient.
3. Specify and add the MVP misconception/rejection cases, especially the
   incomplete-assignment and latch behavior already identified in the
   validation roadmap.
