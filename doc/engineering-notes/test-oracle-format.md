# RTL Test Oracle Format

- **Status:** accepted (oracle layers); open (machine-readable file format)
- **Date:** 2026-10-31
- **Context:** The RTL fixture corpus now includes combinational and
  resettable sequential examples. A first inline SVA case records an intended
  FFD structure and future proof outcome, but Lemma currently only parses
  syntax.
- **Decision:** Verify the flow with distinct test oracles for (1) parsing,
  (2) RTL validation/extraction, (3) SVA property interpretation, and (4) proof
  outcomes. Circuit expectations describe semantic signals, equations, and FFD
  Q/D/clock/reset facts; property expectations describe the clock, disable
  condition, and temporal expression without depending on solver-specific
  monitor state. Proof expectations record the result class and, where
  applicable, counterexample or witness properties. Not every fixture needs
  to exercise every stage.
- **Boundary:** These layers are accepted, but no machine-readable manifest or
  serialization format is selected. Expected IR is not the production IR
  serialization, and the earlier JSON-manifest suggestion is not accepted.
  The current README expectation for `seq_continuous_assign_alias.sv` is
  descriptive only; its `proven` outcome is not semantically checked.
- **Revisit when:** A minimal circuit IR, property model, and proof test
  interface exist. Then choose a machine-readable test representation that
  avoids coupling expectations to unstable node IDs or solver internals; decide
  separately whether a human-readable catalog should be generated from it.
- **References:** [RTL fixture catalog](../../tests/fixtures/rtl/README.md),
  [fixture preparation checkpoint](../checkpoint/rtl_fixture_preparation.md),
  and [internal representation](../internal-representation.md).
