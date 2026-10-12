# Lemma Formal RTL Verification — Project Checkpoint

**Checkpoint status:** Semantics decisions captured from the supplied project discussion; broader product scope remains partly unconfirmed.

## 1. Project purpose

Lemma is intended to provide formal verification of RTL behavior using a stable-state, event-driven proof model. The project should prove properties over all behaviors permitted by the RTL, Lemma's documented initialization policy, and explicit environmental assumptions, rather than validate a single simulation run.

The supplied context does not state the final product goals, target implementation language, supported tools, or intended users. Those items remain open.

## 2. Supported scope

### Confirmed or explicitly supported by the discussion

- RTL sequential behavior is modeled through stable states around active clock events.
- RTL may contain multiple clock domains. Each property proof is scoped to one clock domain and its
  domain-local dependency cone; physical periods and time are not modeled.
- Concurrent assertions sample expressions at the clock event, before sequential register updates.
- `disable iff` is supported as an asynchronous disable: when its condition becomes true, the assertion is disabled and active property attempts are aborted.
- Module inputs are unconstrained by default; only explicit `assume` constraints restrict them.
- The proof model is two-state. RTL `X` and `Z` are not simulation states; each such bit is treated as indeterminate and may be either `0` or `1` for proof purposes.

### Explicitly limited or deferred

- A sequential process supports at most one asynchronous reset event control. Reset-named conditions in the process body are ordinary RTL control, not additional reset events. Multiple asynchronous reset event controls are deferred.
- Cross-domain-dependent properties are outside the MVP scope and must be rejected. Unrelated clock
  domains in the RTL are allowed and excluded from a domain-local proof.
- Asynchronous reset values must be constants or resolved parameters. Values depending on other signals are not supported for the asynchronous reset branch.
- Every register in a process with an asynchronous reset event control must receive an explicit value in that asynchronous reset branch.

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

Clocks are events, not timed waveforms. RTL may contain multiple clock domains, but each property
proof is built for one selected domain. Lemma models that domain's active edges and the state and
combinational logic in the property's dependency cone. If the cone depends on state updated by
another clock domain, the property is unsupported and must be rejected. Other, unrelated domains are
excluded from that proof. Lemma does not model clock periods, physical time, or cross-domain event
interleavings.

### 3.3 Initialization and default-zero policy

A register with a supported RTL initialization starts with that value. Every
other register starts at `0` at proof start under Lemma's default-zero policy
and retains that value until updated by the RTL. This policy is a tool-defined
initialization convention; it is not inferred from RTL and does not mean that
reset has occurred.

During RTL analysis, Lemma must emit a source-located warning for each register
assigned the default zero. Proof reports must disclose the policy and list the
affected registers. Results are conditional on this model and do not establish
behavior from arbitrary physical power-up values.

Lemma must honor an RTL initialization only when that initialization is
supported. A reset-named condition in a clocked process body does not
initialize a register merely because it is active at proof start; it is
ordinary control evaluated at an active clock edge.

### 3.4 Reset behavior

- **Clocked reset condition:** a branch condition in a clocked process body (often named `srst`) is ordinary RTL control evaluated at the active clock edge. It has no separate reset-event semantics.
- **Asynchronous reset:** an event control applied when asserted, independently of a clock edge, and therefore capable of causing a state transition outside a clock event.
- **Asynchronous reset polarity:** inferred from the RTL event control and condition; no default active polarity is imposed.
- **Asynchronous reset values:** each register may have its own value, but the value must be a constant or resolved parameter.
- **Asynchronous reset branch:** every register in a process with an asynchronous reset event control must be assigned explicitly there.
- **Priority:** ordinary clocked conditions follow their RTL branch order. When combined with asynchronous reset, event control and branch priority follow the RTL; Lemma adds no priority rule.
- **Coincident events:** an active asynchronous reset imposes the reset value, including when assertion coincides with a clock edge. Other conditions are evaluated according to the process RTL at the clock edge.
- **Deassertion:** deasserting reset does not itself update a register. The register retains its current value and resumes loading data at the next active clock edge, as specified by the RTL.

### 3.5 Retention

If no branch assigns a value to a register at an active clock edge, the register retains its previous value. This includes conditional updates such as an inactive enable. Register updates are simultaneous and are computed from the pre-edge state.

## 4. Execution and proof semantics — conclusions

The proof engine should reason over the initial state defined by supported RTL
initializations and Lemma's default-zero policy, and over all unconstrained
input choices, subject to explicit assumptions. It should not infer that an
RTL reset occurred at startup or treat combinational propagation transients
as observable proof states.

For reporting, Lemma must identify registers assigned default zero and disclose
that results use this initialization policy. Counterexamples should show the
relevant proof-start values. Assertion and cover result categories are defined
in the tool-flow and MVP specifications; the exact trace format and
counterexample presentation remain open.

## 5. Proposed MVP flow

The following flow is consistent with the confirmed semantics, but its tooling and exact interfaces are **not yet confirmed**:

1. Parse the selected RTL and identify supported sequential processes, registers, clocks, resets, assignments, and supported assertion constructs.
2. Validate the input against the supported subset, including the one-asynchronous-reset-event-control-per-process and asynchronous reset-assignment rules.
3. Build a stable-state transition model with simultaneous register updates and post-update combinational stabilization.
4. Represent unsupported RTL/SVA constructs, unresolved reset values, and unsupported initialization forms as explicit diagnostics rather than silently approximating them.
5. Add unconstrained module inputs and explicit assumptions.
6. Initialize registers without supported RTL initialization to zero and emit a source-located warning for each.
7. Encode concurrent assertion sampling at the event boundary and implement `disable iff` as an asynchronous abort/disable condition.
8. Use BMC to search for counterexamples and k-induction to establish unbounded safety properties over the initialized state and permitted input behaviors. Bounded search alone must not produce `proven`.
9. Report proof status, assumptions, unsupported constructs, default-zero initialization warnings, and counterexample traces including relevant proof-start values.

The exact solver, solver interface/encoding, intermediate representation,
command-line/API flow, induction limits, and expected performance targets
remain undecided.

## 6. Key architecture decisions

- Use a stable-state transition model rather than a propagation-delay simulation model.
- Separate pre-edge sampled assertion values from post-edge register updates and combinational settling.
- Allow multiple clock domains in RTL, while constructing each property proof from one clock domain
  and rejecting dependency cones that cross domains. Treat modeled edges as abstract events and avoid
  physical-time semantics in the initial architecture.
- Use BMC for counterexample search and k-induction for unbounded safety proofs; a bounded search with
  no counterexample is not a proof.
- Model asynchronous reset assertion as a distinct event; treat reset-named conditions in clocked process bodies as ordinary RTL control.
- Infer reset polarity and priority from RTL rather than imposing defaults.
- Initialize registers without supported RTL initialization to zero as an explicit Lemma policy, and disclose the affected registers; do not present this as an RTL reset or guarantee about physical power-up state.
- Treat inputs as unconstrained unless constrained by explicit assumptions.
- Use a two-state proof model while conservatively treating each RTL `X`/`Z` bit as either `0` or `1`.
- Keep the first asynchronous reset model simple: one reset event control per process and constant/resolved-parameter reset values.
- Make unsupported constructs visible through validation and reporting.

## 7. Unresolved questions

1. What exact RTL language/version and parser front end will Lemma support?
2. Which SVA extensions beyond the defined v1 profile are worth considering later?
3. What exact clocked-process and assertion clock forms are accepted, and how are clock domains
   identified in the elaborated design?
4. What are the precise semantics for asynchronous reset assertion/deassertion relative to each
   modeled clock domain?
5. Which RTL initialization forms are supported, including declaration assignments, initial blocks, parameters, and tool-specific constructs?
6. How should unsupported `X`/`Z` expressions be diagnosed when they occur in control conditions rather than as data bits?
7. What combinational constructs, latches, memories, arrays, parameterization, and generated RTL are in scope?
8. What assumption syntax and proof-constraint format will be accepted?
9. Which open-source solver/backend and SAT/SMT encoding will be used, and what intermediate representation will feed it?
10. What exact trace format, reporting schema, and reproducibility controls are required? Assertion and cover result categories are defined in the MVP and tool-flow specifications.
11. How will the MVP bound BMC depth and induction effort, and will it add invariant strengthening or automatic vacuity checks beyond explicit covers?
12. What command-line/API and build/test workflow should users receive?

## 8. High-level next steps

1. Preserve the decided default-zero initialization policy and specify its conformance tests alongside assertion sampling, reset event behavior, and two-state treatment of `X`/`Z`.
2. Define and document the normative RTL and SVA MVP subset, including accepted and rejected examples.
3. Specify the intermediate transition-model representation and event ordering precisely enough for implementation and tests.
4. Decide the parser, solver/backend, assumptions interface, and exact trace/report formats.
5. Create focused semantic tests for retention, simultaneous updates, clocked reset-style conditions, asynchronous reset events, reset deassertion, reset priority, default-zero and supported RTL initialization, warning diagnostics, sampled assertions, and `disable iff` abort behavior.
6. Add negative tests for multiple asynchronous reset event controls, missing assignments in asynchronous reset branches, unsupported asynchronous reset values, unsupported initialization, and unsupported RTL/SVA constructs.
7. Build the smallest end-to-end MVP: parse, validate, model, prove, and report one-clock-domain examples before expanding scope.
8. Revisit cross-domain properties and event semantics, richer SVA, memories, and additional RTL
   features only after the MVP semantics and diagnostics are stable.

## 9. Source and confidence note

This checkpoint is derived from the supplied recent project-context files, chiefly the English register-semantics drafts and the earlier French discussion. Items stated as confirmed are limited to decisions explicitly present there. Where the supplied context is silent, the item is labeled unconfirmed, deferred, or an open question rather than inferred as a project decision.
