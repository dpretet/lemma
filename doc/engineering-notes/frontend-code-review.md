# First-Profile Frontend C Review

- **Status:** verified (drive-strength acceptance and review baseline); open
  (stack-depth handling, test-oracle coverage, packed-bound syntax)
- **Date:** 2026-11-10
- **Reviewed base:** `b8a13764955fa15bc5f0ae01e1d3b83c08c4b8e0`
- **Scope:** First-party C sources, headers, and tests for the current frontend,
  compared with the README, RTL support, IR, tool-flow, validation, and relevant
  engineering notes. This is a review record, not a change to RTL behavior.

## Findings

### Drive strength is accepted and discarded

- **Status:** verified.
- `visit_continuous_assignments` checks delay controls but does not reject a
  `drive_strength` child. The assignment is emitted as a plain signal alias,
  so the modifier is absent from the resulting IR.
- Reproduction: `assign (strong1, highz0) y = a;` in an otherwise valid alias
  module was accepted by `./build/lemma` on the reviewed base.
- This conflicts with the first-profile boundary and the rule against silently
  omitting unsupported semantics. Reject the modifier with a source-located
  diagnostic and add a regression test.
- Evidence: [`frontend_ts.c`](../../src/frontend/frontend_ts.c),
  [`rtl-support.md`](../rtl-support.md),
  [`internal-representation.md`](../internal-representation.md), and the
  previously recorded open finding in [`api-boundaries.md`](api-boundaries.md).

### Tree-sitter walks have unbounded recursion

- **Status:** open; recursive source-controlled walks are verified by inspection,
  but stack exhaustion was not reproduced in this review.
- `first_syntax_error` recursively descends the parse tree. Syntax extraction
  also recurses through loop bodies, nested statements, and event expressions.
  Deep input can exhaust the C stack instead of returning a structured error.
- Replace these walks with iterative traversal or enforce a checked depth limit
  that returns a diagnostic; add deep-input regression tests.
- Evidence: [`parser.c`](../../src/frontend/parser.c),
  [`frontend_ts.c`](../../src/frontend/frontend_ts.c), and the open item in
  [`api-boundaries.md`](api-boundaries.md).

### Rejection test oracles do not consistently check classification

- **Status:** open; verified by inspection of the current tests.
- Most rows in `test_frontend_rejects_unsupported_constructs` only require a
  rejection, non-NULL code, and source range. They do not assert the expected
  diagnostic stage or code, so stage-classification regressions can pass.
- Record the expected stage and code per case. The fixture catalog also states
  that all 54 RTL fixtures parse, but the CTest suite has no parser sweep for
  that corpus; add one to keep the claim regression-tested.
- Evidence: [`test_frontend.c`](../../tests/unit/test_frontend.c),
  [`CMakeLists.txt`](../../CMakeLists.txt),
  [`RTL fixture README`](../../tests/fixtures/rtl/README.md), and
  [`validation-criteria.md`](../validation-criteria.md).

### Packed-bound syntax needs a specification decision

- **Status:** open; implementation behavior is verified, allowed syntax is not
  fully specified.
- `parse_decimal_bound` accepts only unsigned digit bytes and a `uint32_t` value.
  The first-profile text says “concrete decimal integers” but does not clarify
  negative bounds or digit separators. Decide and document the accepted form
  before treating such inputs as implementation defects or expanding support.
- Evidence: [`frontend_elaboration.c`](../../src/frontend/frontend_elaboration.c)
  and [`rtl-support.md`](../rtl-support.md).

## Scope boundary

The README accurately limits the current executable to parsing, first-profile
RTL validation/elaboration, and alias-only circuit-IR construction. This review
does not treat absent proof, SVA, parameter, or sequential behavior as an
unexpected defect. The current IR and public verification declarations are not
yet an implementation of the full IR or verification-job contracts; the MVP
acceptance baseline remains future work.

## Review evidence

- `./flow.sh test` passed all 7 CTest tests on the reviewed base, with no
  compiler warnings.
- The drive-strength reproduction above was accepted; the embedded-NUL input
  probe was rejected during parsing.
- No source changes were made during the review.
