# Lemma MVP Specification

## Goal

The first milestone demonstrates end-to-end verification of a simple property on small RTL. The complete flow must accept supported RTL and one property, resolve parameters, run the proof, and report a useful result.

## Scope

The MVP supports:

- A single, flat top module.
- RTL may contain any number of clock domains. Each proof checks one property
  on one clock domain, and the property's full signal-dependency cone must be
  confined to that domain (apart from constants and unconstrained inputs).
  Cross-domain-dependent properties are unsupported and must be rejected.
- The limited synthesizable Verilog/SystemVerilog subset defined by the RTL specification.
- Parameter resolution as defined by the RTL specification.
- The previously defined register semantics, including stable-state proof steps, abstract clock events, RTL-defined synchronous/asynchronous reset behavior, simultaneous register updates, retention when no assignment occurs, unconstrained inputs unless explicitly assumed, and Lemma's default-zero proof-start value for registers without a supported RTL initialization. Such registers must be identified by warnings and in the proof report.
- The initial SVA subset defined separately.
- One property at a time.
- Proof outcomes of `proven`, `failed`, or `inconclusive`.
- A readable counterexample when a property fails.
- Diagnostics for unsupported constructs and other errors.

The proof must use the defined register semantics rather than assuming that reset has occurred. A failed result should identify the behavior that violates the property. Reports must disclose when the default-zero policy applies; this default is not evidence that physical hardware or an RTL reset establishes zero.

## End-to-end behavior

For a supported single-flat-top design and one property, Lemma shall:

1. Read and validate the RTL and property inputs.
2. Resolve parameters according to the RTL specification.
3. Build the proof model using the defined register semantics and the separately defined initial SVA subset.
4. Attempt the property proof.
5. Return exactly one of the MVP statuses: `proven`, `failed`, or `inconclusive`.
6. Provide a readable counterexample for `failed`, or a diagnostic explaining the unsupported construct or error when the flow cannot proceed.

## Out of scope

The MVP excludes:

- VHDL.
- Hierarchical designs; only a single flat top is supported.
- Properties whose signal-dependency cones span multiple clock domains.
- SVA beyond the separately defined initial subset.
- Multiple properties in one proof run.
- Performance optimization.

## Exit criterion

The milestone is complete when tests exercise the full flow on small supported RTL and demonstrate both passing and failing properties. Passing tests must produce `proven`; failing-property tests must produce `failed` with a readable counterexample. Tests must also verify default-zero initialization warnings and proof-report disclosure, cover the expected `inconclusive` outcome, and check diagnostics for unsupported constructs or errors so that the supported boundary is explicit.
