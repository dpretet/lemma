# Lemma Formal RTL Verification — Project Checkpoint

**Checkpoint status:** Semantics decisions captured from the supplied project discussion; broader product scope remains partly unconfirmed.

## 1. Project purpose

Lemma is intended to provide formal verification of RTL behavior using a stable-state, event-driven proof model. The project should prove properties over all behaviors permitted by the RTL and explicit environmental assumptions, rather than validate a single simulation run or a single seeded initial state.

The supplied context does not state the final product goals, target implementation language, supported tools, or intended users. Those items remain open.

## 2. Supported scope

### Confirmed or explicitly supported by the discussion

- RTL sequential behavior is modeled through stable states around active clock events.
- Clock domains are advanced as abstract events; physical periods and time are not modeled.
- Concurrent assertions sample expressions at the clock event, before sequential register updates.
- `disable iff` is supported as an asynchronous disable: when its condition becomes true, the assertion is disabled and active property attempts are aborted.
- Module inputs are unconstrained by default; only explicit `assume` constraints restrict them.
- The proof model is two-state. RTL `X` and `Z` are not simulation states; each such bit is treated as indeterminate and may be either `0` or `1` for proof purposes.

### Explicitly limited or deferred

- A sequential process supports at most one reset, either synchronous or asynchronous. Multiple resets are deferred.
- Multiple-clock-domain details are deferred.
- Reset values must be constants or resolved parameters. Values depending on other signals are not supported.
- Every register in a resettable sequential process must receive an explicit value in the reset branch.

### Not established in the supplied context

The discussion does not establish the complete RTL grammar, language/version support, combinational-logic constructs, memories, arrays, arithmetic/width rules, latches, generated RTL, interfaces, clocking blocks, sampled-value functions beyond the points above, or the complete SVA operator/property subset. These must not be treated as supported until separately confirmed.

## 3. Register and reset semantics

### 3.1 Stable-state proof model

Let `S_t` be the RTL state stabilized immediately before an active clock edge:

1. Concurrent assertions sample expressions from `S_t`.
2. Sequential processes compute next values from `S_t`.
3. Registers update simultaneously, forming the next state.
4. Combinational logic is reevaluated until it stabilizes, forming `S_{t+1}`.

Transient states caused only by combinational propagation are excluded from proof semantics. Property expressions use the values sampled at the clock event, not values after register updates and combinational settling.

### 3.2 Clock events

Clocks are events, not timed waveforms. Lemma advances each clock domain abstractly and does not model clock periods or physical time. Detailed multi-clock semantics remain deferred.

### 3.3 Initialization and arbitrary initial state

A register without a supported RTL initialization starts with an arbitrary value; each bit may be `0` or `1`. The value is arbitrary but fixed until the register is updated. Formal proof must cover all possible initial values. A seed-based selection would cover only one initial state and is therefore not sufficient for an all-initial-states proof.

Lemma must honor an RTL initialization only when that initialization is supported. It must not assume that reset has occurred unless the RTL or explicit proof constraints establish it. A synchronous reset does not initialize a register merely because it is active at the beginning of the proof; it takes effect at an active clock edge.

### 3.4 Reset behavior

- **Synchronous reset:** sampled and applied only at the active clock edge.
- **Asynchronous reset:** applied when asserted, independently of a clock edge, and therefore capable of causing a state transition outside a clock event.
- **Polarity:** inferred from the RTL condition; no default active polarity is imposed.
- **Reset values:** each register may have its own value, but the value must be a constant or resolved parameter.
- **Reset branch:** every register in a resettable sequential process must be assigned explicitly there.
- **Priority:** when reset is combined with enable or other conditions, priority follows the order and conditions expressed in the RTL; Lemma adds no priority rule.
- **Coincident events:** an active asynchronous reset imposes the reset value, including when assertion coincides with a clock edge. A synchronous reset is evaluated at the clock edge according to the RTL.
- **Deassertion:** deasserting reset does not itself update a register. The register retains its current value and resumes loading data at the next active clock edge, as specified by the RTL.

### 3.5 Retention

If no branch assigns a value to a register at an active clock edge, the register retains its previous value. This includes conditional updates such as an inactive enable. Register updates are simultaneous and are computed from the pre-edge state.

## 4. Execution and proof semantics — conclusions

The proof engine should reason over all legal initial states and all unconstrained input choices, subject only to explicit assumptions. It should not inject an implicit startup reset, choose one random seed as proof coverage, or treat combinational propagation transients as observable proof states.

For reporting, Lemma must state that registers without an explicit supported initialization have arbitrary initial values. If such values contribute to a counterexample, the trace should identify the initial values used. The supplied discussion does not decide the exact trace format, counterexample presentation, or proof-result taxonomy.

## 5. Proposed MVP flow

The following flow is consistent with the confirmed semantics, but its tooling and exact interfaces are **not yet confirmed**:

1. Parse the selected RTL and identify supported sequential processes, registers, clocks, resets, assignments, and supported assertion constructs.
2. Validate the input against the supported subset, including the one-reset-per-sequential-process and reset-assignment rules.
3. Build a stable-state transition model with simultaneous register updates and post-update combinational stabilization.
4. Represent unsupported RTL/SVA constructs, unresolved reset values, and unsupported initialization forms as explicit diagnostics rather than silently approximating them.
5. Add unconstrained module inputs and explicit assumptions.
6. Apply arbitrary-but-fixed initial values to registers without supported initialization.
7. Encode concurrent assertion sampling at the event boundary and implement `disable iff` as an asynchronous abort/disable condition.
8. Run the proof over all permitted initial states and input behaviors.
9. Report proof status, assumptions, unsupported constructs, arbitrary initial-state information, and counterexample traces including relevant initial values.

The solver, intermediate representation, command-line/API flow, incremental proof strategy, and expected performance targets are not established in the supplied context.

## 6. Key architecture decisions

- Use a stable-state transition model rather than a propagation-delay simulation model.
- Separate pre-edge sampled assertion values from post-edge register updates and combinational settling.
- Treat clocks as abstract events and avoid physical-time semantics in the initial architecture.
- Model synchronous and asynchronous reset as distinct mechanisms.
- Infer reset polarity and priority from RTL rather than imposing defaults.
- Preserve arbitrary initial state symbolically across the proof; do not replace it with a single random seed.
- Treat inputs as unconstrained unless constrained by explicit assumptions.
- Use a two-state proof model while conservatively treating each RTL `X`/`Z` bit as either `0` or `1`.
- Keep the first reset model simple: one reset per sequential process and constant/resolved-parameter reset values.
- Make unsupported constructs visible through validation and reporting.

## 7. Unresolved questions

1. What exact RTL language/version and parser front end will Lemma support?
2. What is the complete supported SVA subset beyond concurrent assertions and `disable iff`?
3. How are properties associated with clocks, especially when a design has multiple clock domains?
4. What are the precise semantics for simultaneous events across clock domains and for asynchronous reset assertion/deassertion relative to clocks?
5. Which RTL initialization forms are supported, including declaration assignments, initial blocks, parameters, and tool-specific constructs?
6. How should unsupported `X`/`Z` expressions be diagnosed when they occur in control conditions rather than as data bits?
7. What combinational constructs, latches, memories, arrays, parameterization, and generated RTL are in scope?
8. What assumption syntax and proof-constraint format will be accepted?
9. Which solver/backend, intermediate representation, and proof algorithms will be used?
10. What exact trace format, reporting schema, reproducibility controls, and proof-result categories are required?
11. Are vacuity checks, coverage, bounded proofs, induction, and proof decomposition part of the MVP?
12. What command-line/API and build/test workflow should users receive?

## 8. High-level next steps

1. Ratify this semantic baseline, especially arbitrary initial state, assertion sampling, reset event behavior, and two-state treatment of `X`/`Z`.
2. Define and document the normative RTL and SVA MVP subset, including accepted and rejected examples.
3. Specify the intermediate transition-model representation and event ordering precisely enough for implementation and tests.
4. Decide the parser, solver/backend, assumptions interface, and proof-result/trace formats.
5. Create focused semantic tests for retention, simultaneous updates, synchronous versus asynchronous reset, reset deassertion, reset priority, arbitrary initialization, sampled assertions, and `disable iff` abort behavior.
6. Add negative tests for multiple resets, missing reset assignments, unsupported reset values, unsupported initialization, and unsupported RTL/SVA constructs.
7. Build the smallest end-to-end MVP: parse, validate, model, prove, and report one-clock-domain examples before expanding scope.
8. Revisit multiple clocks, richer SVA, memories, and additional RTL features only after the MVP semantics and diagnostics are stable.

## 9. Source and confidence note

This checkpoint is derived from the supplied recent project-context files, chiefly the English register-semantics drafts and the earlier French discussion. Items stated as confirmed are limited to decisions explicitly present there. Where the supplied context is silent, the item is labeled unconfirmed, deferred, or an open question rather than inferred as a project decision.
