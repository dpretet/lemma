# Fixture-Driven Circuit IR Scope

- **Status:** accepted
- **Date:** 2026-11-17
- **Context:** User aligned the next frontend milestone after review of the
  current first-profile implementation.
- **Decision:** All 54 modules in [`tests/fixtures/rtl`](../../tests/fixtures/rtl)
  are intended to pass RTL validation/elaboration and produce circuit IR. Proof
  execution is not part of this milestone.
- **Decision:** Treat the exact constructs exercised by these fixtures as the
  supported boundary. Update the authoritative RTL and IR specifications with
  the needed SystemVerilog semantics for those forms; do not broaden support to
  nearby constructs that are not represented by the corpus.
- **Decision:** The assertion in `seq_continuous_assign_alias.sv` is outside
  circuit IR and proof execution. Its presence does not prevent the fixture's
  hardware from producing circuit IR; this milestone does not model or prove
  the assertion.
- **Decision:** The frontend build returns a result object containing the
  optional circuit and a read-only diagnostic array, so source-located
  truncation warnings can accompany successful IR construction. This follows
  the result/diagnostic-array shape already selected for verification jobs.
- **Verified implementation detail (2026-11-17):** The result allocation also
  contains inline storage for the current single error diagnostic. Therefore
  parse, RTL, and IR failures can return a non-NULL result without a second
  diagnostic-allocation failure path; successful current alias builds leave the
  array empty. When expression lowering begins emitting multiple warnings, the
  backing storage must be extended while preserving the NULL-only-on-result-
  allocation-failure contract. Evidence: `src/frontend/frontend.c`,
  `include/lemma/frontend.h`, and the result API tests in
  `tests/unit/test_frontend.c`.
- **Verified implementation detail (2026-11-17):** Expression syntax events
  arrive depth-first with operands before their operator and use parent ranges
  plus source-order child indexes; the elaborator can therefore build operator
  nodes after consuming at most two child IDs from its bounded depth frames.
  Every new IR operator must also be added to the dependency walk's arity map,
  or cycle validation will fail despite successful lowering. Evidence:
  `src/frontend/frontend_ts.c`, `src/frontend/frontend_elaboration.c`, and
  `test_frontend_lowers_vector_operator_fixtures` /
  `test_frontend_rejects_unsupported_operator_boundaries` in
  `tests/unit/test_frontend.c`.
- **Rationale:** The fixture set is the agreed incremental conformance target,
  not merely a parser corpus. Support must preserve source behavior in the IR;
  syntactic acceptance alone is insufficient.
- **References:** [`RTL support`](../rtl-support.md),
  [`internal representation`](../internal-representation.md),
  [`fixture catalog`](../../tests/fixtures/rtl/README.md), and the active
  [`historical frontend handoff`](../checkpoint/2026-11-19_FRONTEND_API_HANDOFF.md).
