# Formal Execution and Assertion Semantics

## 1. Scope and status

This document defines the proposed execution and assertion semantics for **Lemma**, the formal engine for a synthesizable RTL subset with SVA assertions. It describes the abstract proof model rather than simulation timing or implementation details.

The model is intentionally conservative: behavior is considered over all executions allowed by the RTL and by explicit verification assumptions. Items identified as **open** are not silently resolved by this specification.

## 2. Semantic foundations

### 2.1 Stable states

Lemma models RTL behavior as a sequence of stabilized states. It does not model combinational propagation delays or transient values during settling.

A state `S` contains the values of RTL registers and the values of combinational signals after combinational logic has settled for the current register state and current input valuation. State and inputs are distinct:

- **State** is the persistent information carried between RTL events, principally register values.
- **Inputs** are environment-provided values associated with an execution step. They may change from one step to the next and do not become persistent state unless the RTL stores them.

For a property proof, the conceptual transition is scoped to the property's
selected clock domain. For an active edge in that domain:

1. Begin in a stabilized state `S_t` with the event's input valuation.
2. Sample the state and relevant signals for assertion purposes.
3. Evaluate sequential RTL from the pre-event state and inputs.
4. Update all affected registers simultaneously to form the post-event register state.
5. Reevaluate combinational logic until it stabilizes, forming `S_{t+1}`.

Transient states between register update and combinational settling are excluded from the execution semantics.

### 2.2 Executions and transitions

For a property proof, an **execution** is a finite or infinite sequence of
stabilized states in the property's selected clock domain, connected by
transitions recognized by the supported RTL semantics, including:

- an active edge of the selected clock; or
- assertion of a supported asynchronous reset.

A clock event may update state even when no register changes value; in that case the transition still represents evaluation of the sequential logic and assertions at that event. An asynchronous reset assertion may cause a transition outside any clock event. Reset deassertion alone does not update registers and is not, by itself, a state transition; the affected registers retain their values until the RTL causes an update.

Only active edges of the selected clock trigger the synchronous logic included
in that proof. Lemma does not infer an implicit clock from signal changes.

## 3. Clock domains and property scope

RTL may contain any number of clock domains. Clocks are modeled as abstract
events, not as waveforms in physical time. Each property selects one clock
domain; Lemma builds that property's proof model from the selected domain and
the transitive signal-dependency cone of the property.

The cone may include constants and unconstrained module inputs, but every
state element in it must be updated in the selected domain. Combinational
signals in the cone must in turn depend only on constants, inputs, and state
from that domain. If the cone depends on a register updated by another clock
domain, Lemma must reject the property rather than treat that register as a
free value. This rule applies to assertions, assumptions, covers, and
`disable iff` expressions used in the proof context.

Clock domains outside the dependency cone are allowed in the RTL and do not
affect the property proof. Their state transitions and event interleavings are
not modeled. Consequently, Lemma does not verify cross-domain properties or
perform clock-domain-crossing analysis. Clock-domain identification and
dependency analysis must be sufficiently conservative that no dependency on
another domain is silently omitted.

## 4. Initial state and initialization

At proof start, registers with a supported RTL initialization take that value.
Every other register is initialized to `0` by Lemma's default-zero policy and
retains that value until updated by the RTL. This is a modeling default, not an
RTL-defined power-up value or an assumption that reset has occurred.

During RTL analysis, Lemma must warn for each register assigned the default
zero, identifying its name and source location. Proof reports must disclose
the policy and affected registers. Results therefore apply to the model with
these proof-start values; they do not establish behavior from arbitrary
physical power-up values.

This rule applies to register state, not to inputs. Inputs remain environment
choices on each applicable execution step and do not become persistent state
unless the RTL stores them.

Lemma does **not** assume that reset occurred at startup. A register is
initialized by an asynchronous reset only when that event is actually
observed. A reset-named condition in a clocked process body is ordinary RTL
control; when true, it affects state only through the process assignments at
the next applicable active clock edge. A supported asynchronous reset
assertion may update state immediately, including outside a clock event.

An RTL initialization is honored only when it is in the supported synthesizable RTL subset. The default-zero policy applies only when no supported RTL initialization is present. Any additional startup behavior must be introduced through an explicit verification assumption or protocol constraint.

## 5. Reset semantics

### 5.1 Reset type and triggering

Only asynchronous reset event controls have special reset-event semantics:
assertion can cause a state transition independently of a clock edge.

A condition in a clocked process body, including a synchronous-reset-style
condition, is evaluated as ordinary RTL at the process's active clock edge.
Its effect follows the written assignments and branch order; it does not
create an independent event.

Asynchronous reset polarity is taken from the event control and RTL condition;
Lemma imposes no default polarity. Its reset values follow the RTL and, in the
supported subset, are constants or resolved parameters, with an explicit
assignment for every register in the process. Ordinary clocked conditions
follow the supported RTL expression and assignment semantics.

### 5.2 Reset priority and coincident events

When an asynchronous reset is combined with ordinary clocked conditions,
priority follows the event control, conditions, and branch order expressed in
the RTL; Lemma adds no independent priority rule. A reset-named synchronous
condition is simply one of those ordinary conditions.

An asserted asynchronous reset imposes its RTL-defined effect independently
of the clock. If its assertion coincides with a clock edge, the treatment must
preserve the RTL event semantics; any unresolved atomicity or ordering question
for that coincidence remains an implementation semantic point and must not be
hidden by an arbitrary choice.

### 5.3 Deassertion and supported structure

Reset deassertion does not itself update a register. The register retains its current value and resumes ordinary RTL updates at the next applicable active clock edge.

The proposed supported subset permits at most one asynchronous reset event
control per sequential process. Reset-named conditions in the process body
are ordinary RTL control and are not counted as additional reset events.
Multiple asynchronous reset event controls are deferred and require an
explicit extension of this specification.

## 6. Register updates and retention

Sequential processes compute next values from the same pre-event state. Register updates take effect simultaneously at the end of the sequential evaluation phase.

If no executed RTL branch assigns a value to a register at an applicable active clock event, the register retains its previous value. This includes conditional updates such as an inactive enable. Retention is an RTL consequence, not an implicit initialization rule.

## 7. Inputs, assumptions, and protocols

Module inputs are unconstrained by default. For each execution step at which an input is relevant, the proof engine may choose any value permitted by the signal's modeled domain, independently of prior input values.

Only explicit verification assumptions—such as `assume` properties, environment constraints, or a declared protocol model—restrict input choices, event ordering, reset sequence, or startup behavior. Such constraints define the set of allowed executions and must be reported with the proof context. No reset sequence, handshake discipline, clock relationship, or input stability requirement is implied by the absence of a counterexample.

## 8. Unknown and high-impedance values

Lemma uses a two-state proof interpretation for values in the supported proof model. RTL `X` and `Z` bits are not treated as persistent four-state simulation values. Each `X` or `Z` bit is indeterminate, so the engine may choose `0` or `1` for it. The proof must account for both choices wherever the value is unconstrained.

This rule is separate from register initialization: registers without supported
RTL initialization start at zero by policy, while an input or RTL expression
containing an `X`/`Z` bit is resolved as an allowed two-state choice at the
point where it is modeled.

## 9. Assertion semantics

An assertion passes only if it holds on **all allowed executions** of its
modeled domain-local cone: the proof-start register values defined by supported
RTL initializations and Lemma's default-zero policy, all input valuations,
selected-clock events, asynchronous reset events, and choices for indeterminate
`X`/`Z` bits, subject to the explicit assumptions.

For an assertion associated with a clock event, the property observes the stabilized pre-event state and the event's sampled signals. It is not evaluated using a post-update transient or by silently substituting the state after register updates. Sequential RTL then computes and applies its updates from that same pre-event state.

The precise sampling regions, implication timing, `$past`/history behavior, overlapped versus non-overlapped implication, repetition, `disable iff`, and other SVA-specific sampling and evaluation details belong in the **SVA Support Specification**. This document establishes the execution-state boundary and the all-executions proof criterion; it does not replace that SVA specification.

## 10. Proof outcomes

Lemma reports one of the following outcomes for a checked assertion:

- **PASS:** the assertion holds on every allowed execution under the modeled RTL subset and explicit assumptions.
- **FAIL:** Lemma found an allowed execution that violates the assertion. The counterexample should identify relevant proof-start register values, input choices, selected-clock event sequence, and reset activity.
- **UNKNOWN:** Lemma cannot establish PASS or FAIL within the supported semantics, implementation capabilities, or configured resources. UNKNOWN is not a proof of the assertion and is not a counterexample.

Reports should state whether the default-zero policy was applied, identify the
affected registers, and make the assumptions that restrict the execution set
visible.

## 11. Normative implications and placement guidance

The execution engine specification should reference this document for stabilized-state transitions, property-scoped clock domains, dependency-cone restrictions, default-zero register initialization, reset triggering, and free inputs. The synthesizable RTL subset specification should define which initialization, clock, and reset forms are accepted. The SVA Support Specification should define assertion sampling and evaluation details. A future cross-domain proof extension would require a separately specified multi-clock event model.

Until those open items are resolved, tools and reports should identify them explicitly rather than treating an implementation choice as a settled formal guarantee.
