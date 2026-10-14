# Validation Criteria

This document defines what Lemma must validate. It specifies observable obligations and evidence, not a particular parser, elaborator, solver, trace format, or test framework.

## 1. Validation contract

A validation run should make clear:

- which source, options, assumptions, and supported language features were examined;
- which interpretation of the input was validated;
- which obligations were checked and which were not applicable or unsupported; and
- enough context to reproduce the result without implying guarantees beyond the checked scope.

Validation must distinguish **accepted and validated**, **rejected as invalid**, **unsupported**, **inconclusive**, and **failed by the validation infrastructure**. An implementation must not silently turn an unsupported construct, parse recovery, missing assumption, or internal failure into a proof result.

## 2. Input and parsing

Validate that:

- the input is read according to the documented language and encoding rules;
- lexical structure, delimiters, literals, identifiers, and source locations are handled consistently;
- malformed or incomplete input is rejected or reported according to the documented contract;
- parser recovery, if present, cannot cause omitted or altered constructs to be treated as validated; and
- module boundaries, declarations, dependencies, and referenced files are identified without ambiguity.

Tests should cover valid inputs, boundary forms, malformed input, and failures at multiple source locations. The validation evidence should retain the relevant source span and a stable diagnostic identity where the tool provides one.

For clock-domain handling, tests should demonstrate that a design with multiple
domains can be accepted for a property whose complete dependency cone is local
to one domain, and that a property depending on state from another domain is
rejected.

## 3. Elaboration and supported constructs

Validate that parsing produces the intended elaborated representation for every construct claimed as supported. In particular, check:

- name binding, scopes, shadowing, and references;
- widths, signedness, types, parameters, and constant expressions;
- implicit or explicit conversions and their order;
- procedural and concurrent structure, clock/reset information, and dependencies; and
- unsupported, ambiguous, or partially elaborated constructs.

Elaboration must preserve distinctions that affect proof meaning. If a construct cannot be elaborated with the required confidence, validation must report that limitation rather than inventing a default interpretation. Diagnostics should identify both the user-facing construct and, where useful, the elaborated context.

## 4. Semantic preservation

Validate that each translation or normalization step preserves the semantics required by the proof model. The checks should cover, as applicable:

- state elements, supported RTL initialization, Lemma's default-zero initialization policy, and state updates;
- sampling points, event ordering, and simultaneous updates;
- combinational stabilization and the treatment of transient behavior;
- reset type, polarity, value, assertion/deassertion, and priority as expressed by the input;
- retention when no update is selected;
- unconstrained inputs and explicit environmental assumptions;
- two-state treatment or other handling of unknown/high-impedance values; and
- concurrent assertion sampling and disable behavior when supported.

The validation should compare the source-level meaning with the model actually used for proof, including edge cases where a plausible but different interpretation would change a result. The criteria do not prescribe whether preservation is established by equivalence checking, proof obligations, differential execution, or another method; the chosen evidence must be documented.

## 5. Proof outcomes

Validate that the reported outcome matches the checked obligation and its scope:

- **proved** means the stated property holds under the stated model and assumptions;
- **disproved** means a valid counterexample reaches a violating observation;
- **unknown/inconclusive** means the procedure did not establish either result; and
- **rejected/unsupported** means validation could not be performed for a stated input or feature reason.

A result must not be labeled proved when assumptions, initialization, unsupported semantics, vacuity, bounds, or solver/infrastructure failures weaken or change the obligation without being disclosed. Where applicable, validate both positive properties and properties expected to fail, including vacuous and unreachable cases.

## 6. Counterexamples and witnesses

For a disproved property, validate that the witness:

- is consistent with the elaborated model and all stated assumptions;
- identifies the violated obligation and the relevant time/event position;
- includes sufficient state, input, reset, and clock context to replay or inspect the failure;
- represents supported RTL initial values and Lemma default-zero proof-start values explicitly when relevant; and
- is stable enough to distinguish a semantic failure from a reporting or reconstruction failure.

If a witness cannot be replayed or is incomplete, report that limitation separately; do not present an unverified trace as a definitive counterexample.

## 7. Diagnostics and reporting

Validate diagnostics for correctness, locality, consistency, and actionability. They should:

- point to the relevant source span or model element when one exists;
- explain whether the issue is invalid input, unsupported semantics, an unmet validation obligation, inconclusive reasoning, or infrastructure failure;
- avoid claiming a stronger guarantee than the evidence supports;
- report assumptions, initialization rules, ignored or unsupported constructs, and scope limits that affect interpretation;
- emit a source-located warning for each register initialized to zero by Lemma's default policy, and disclose that policy in the proof report; and
- remain deterministic enough for users and regression tests to compare meaningful changes.

Warnings and non-fatal limitations must not be hidden by a successful exit status or a proof-looking summary.

## 8. Regression testing

Maintain regression coverage for every validated contract and every discovered semantic boundary. The suite should include:

Organize the suite in incremental feature layers following the proposed
[RTL-support roadmap](rtl-support.md#13-incremental-conformance-test-roadmap).
Use small standalone examples for each layer, including accepted cases,
semantic boundary cases, and explicit rejection cases. Add larger IP examples
only as integration regressions after the constructs they use have their own
focused tests.

- representative valid and invalid inputs;
- elaboration cases for names, types, widths, parameters, events, and unsupported forms;
- semantic corner cases for initialization, reset, retention, sampling, assumptions, and unknown/high-impedance values;
- proof-strategy cases showing that bounded unsatisfiability alone is not reported as proven, successful base and induction steps establish `proven`, and induction failure without a counterexample remains inconclusive;
- SVA implication timing, repeated antecedent matches, history-function warm-up, asynchronous `disable iff`, cover witnesses, vacuity, and inconsistent assumptions;
- `covered`, `unreachable` (only when established), and `inconclusive` cover outcomes;
- proved, disproved, inconclusive, rejected, and infrastructure-failure outcomes;
- counterexample generation and replay; and
- diagnostic text or structured fields where their contract is intentional.

Each regression should state its intended obligation and expected classification, not merely compare an incidental output. Changes to the proof model, supported language, or diagnostics should update the relevant criteria and tests together. Tests should also detect accidental nondeterminism, unsound weakening of assumptions, and silent acceptance of newly unsupported behavior.

## 9. Evidence and change control

A validation claim is complete only when its test or proof evidence, model assumptions, limitations, and expected outcome are recorded. New implementation choices may satisfy these criteria in different ways; until such a choice is documented and tested, it must remain an explicit open decision rather than an unstated guarantee.

## 10. MVP acceptance baseline

The MVP is accepted only when the following evidence is present. This section
narrows the general criteria above to the first end-to-end milestone; the
broader FRISCV-inspired RTL roadmap is not an MVP acceptance requirement.

### Required test coverage

1. **Input classification:** A supported flat-top example reaches proof
   execution. Malformed RTL, unsupported active syntax, unresolved required
   parameters, source preprocessing directives (including `` `include ``),
   `bind` directives, and cross-domain-dependent properties are rejected at
   the appropriate stage. Rejection must not become a proof result.
2. **RTL semantics:** Tests establish default-zero proof-start values and
   source-located warnings, supported explicit RTL initialization, simultaneous
   updates, retention, source-order priority for repeated assignments to one
   register within a clocked process, rejection of multiple procedural drivers
   for one register, supported asynchronous reset events, ordinary clocked
   control conditions, and two-state handling of `X`/`Z` where applicable.
   Multi-clock RTL must be accepted for a domain-local property and rejected
   when the property dependency cone crosses domains.
3. **Assertion semantics:** Tests cover an invariant, a failing assertion with
   a counterexample, overlapped and non-overlapped implication timing, fixed
   and bounded delays, repeated antecedent matches, asynchronous `disable iff`,
   history-function warm-up, and the explicit `past_valid` pattern.
4. **Assumptions, vacuity, and covers:** Tests show that assumptions restrict
   the environment and are reported; inconsistent assumptions do not yield an
   unqualified proof; an implication can pass vacuously; and a companion cover
   can demonstrate antecedent reachability. Cover tests include a found
   witness, an inconclusive bounded search, and an unreachable case only when
   the analysis is complete.
5. **Proof strategy:** A BMC-found counterexample produces `failed`; bounded
   search with no counterexample alone never produces `proven`; successful
   k-induction base and step checks produce `proven`; and induction that does
   not complete without a counterexample produces `inconclusive`.
6. **Reports and witnesses:** Reports identify the target, selected clock,
   assumptions, default-zero registers, history warm-up samples, and any
   relevant proof bounds or limitations. Counterexamples must match the model,
   include relevant initial/input/reset/event values, and be checked against
   the model before being presented as valid.

Each required test must state its expected stage and result classification.
Small hand-checkable designs are sufficient; the complete FRISCV IP, broad
performance targets, full IEEE SystemVerilog compliance, and proof
certificates are not required for MVP acceptance. A larger IP regression is
useful only after all constructs enabled in its configuration have their own
conformance tests.
