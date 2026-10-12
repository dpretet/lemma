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

For a single active clock event, the conceptual transition is:

1. Begin in a stabilized state `S_t` with the event's input valuation.
2. Sample the state and relevant signals for assertion purposes.
3. Evaluate sequential RTL from the pre-event state and inputs.
4. Update all affected registers simultaneously to form the post-event register state.
5. Reevaluate combinational logic until it stabilizes, forming `S_{t+1}`.

Transient states between register update and combinational settling are excluded from the execution semantics.

### 2.2 Executions and transitions

An **execution** is a finite or infinite sequence of stabilized states connected by RTL-event transitions. An event transition is caused only by an event recognized by the supported RTL semantics, including:

- an active edge of a declared clock; or
- assertion of a supported asynchronous reset.

A clock event may update state even when no register changes value; in that case the transition still represents evaluation of the sequential logic and assertions at that event. An asynchronous reset assertion may cause a transition outside any clock event. Reset deassertion alone does not update registers and is not, by itself, a state transition; the affected registers retain their values until the RTL causes an update.

Only declared clock transitions trigger synchronous logic. Lemma does not infer an implicit clock from signal changes and does not assume any frequency, period, phase, or fixed relationship between clocks.

## 3. Clock events and interleavings

Clocks are modeled as abstract events, not as waveforms in physical time. Each declared clock domain may produce active-edge events according to the supported RTL clock specification.

When multiple clock domains are present and no explicit relation or verification assumption orders their events, their events may occur in any allowed interleaving. Lemma must consider those unconstrained interleavings rather than inventing a frequency or phase relationship.

### Open semantic point: simultaneous clock edges

The semantics for genuinely simultaneous active edges from different clock domains are **open**. This specification does not decide whether such edges are:

- one atomic multi-clock event,
- an ordered pair of events in either order, or
- another explicitly defined composite transition.

The implementation and multi-clock semantics specification must define this behavior before claims depending on simultaneous edges are treated as fully settled. Until then, a tool must not silently choose one interpretation and present it as the normative model.

## 4. Initial state and initialization

At the start of an execution, register values are arbitrary unless a supported RTL initialization or an explicit verification assumption constrains them. Each unconstrained register bit may initially be `0` or `1`; the selected value is arbitrary but fixed until the register is updated. A formal proof must account for all such initial choices, not just one random or seed-selected state.

This rule applies to register state, not to inputs. Inputs remain environment choices on each applicable execution step. An input may happen to equal a register's initial value, but that does not make it state or give it persistence.

Lemma does **not** assume that reset occurred at startup. A register is initialized by reset only when reset is actually observed and its RTL semantics cause an update. A synchronous reset that is active at the beginning of an execution does not initialize a register until an active edge of its declared clock. A supported asynchronous reset assertion may initialize the register immediately upon assertion, including when that assertion occurs outside a clock event.

An RTL initialization is honored only when it is in the supported synthesizable RTL subset. Any additional startup behavior must be introduced through an explicit verification assumption or protocol constraint, not by default.

## 5. Reset semantics

### 5.1 Reset type and triggering

Lemma distinguishes:

- **Synchronous reset:** evaluated and applied only at the active clock edge specified by the RTL.
- **Asynchronous reset:** applied when the reset is asserted, independently of a clock edge.

Reset polarity is taken from the RTL condition. Lemma imposes no default active-high or active-low polarity. Reset effects, including the value assigned to each register, are taken from the RTL. Where the supported subset requires a restricted reset form, reset values are explicit constants or resolved parameters, and every resettable register receives an explicit reset-branch assignment.

### 5.2 Reset priority and coincident events

When reset is combined with enable or other conditions, priority follows the order and conditions expressed in the RTL; Lemma adds no independent priority rule.

An active synchronous reset is handled at its clock edge. An asserted asynchronous reset imposes its RTL-defined effect independently of the clock. If asynchronous reset assertion coincides with a clock edge, the treatment must preserve the RTL event semantics; any unresolved atomicity or ordering question for such coincident events remains an implementation semantic point and must not be hidden by an arbitrary choice.

### 5.3 Deassertion and supported structure

Reset deassertion does not itself update a register. The register retains its current value and resumes ordinary RTL updates at the next applicable active clock edge.

The proposed supported subset permits at most one reset per sequential process, either synchronous or asynchronous. Multiple-reset structures are deferred and require an explicit extension of this specification.

## 6. Register updates and retention

Sequential processes compute next values from the same pre-event state. Register updates take effect simultaneously at the end of the sequential evaluation phase.

If no executed RTL branch assigns a value to a register at an applicable active clock event, the register retains its previous value. This includes conditional updates such as an inactive enable. Retention is an RTL consequence, not an implicit initialization rule.

## 7. Inputs, assumptions, and protocols

Module inputs are unconstrained by default. For each execution step at which an input is relevant, the proof engine may choose any value permitted by the signal's modeled domain, independently of prior input values.

Only explicit verification assumptions—such as `assume` properties, environment constraints, or a declared protocol model—restrict input choices, event ordering, reset sequence, or startup behavior. Such constraints define the set of allowed executions and must be reported with the proof context. No reset sequence, handshake discipline, clock relationship, or input stability requirement is implied by the absence of a counterexample.

## 8. Unknown and high-impedance values

Lemma uses a two-state proof interpretation for values in the supported proof model. RTL `X` and `Z` bits are not treated as persistent four-state simulation values. Each `X` or `Z` bit is indeterminate, so the engine may choose `0` or `1` for it. The proof must account for both choices wherever the value is unconstrained.

This rule is separate from arbitrary register initialization: initial register bits are likewise chosen from `0` or `1`, while an input or RTL expression containing an `X`/`Z` bit is resolved as an allowed two-state choice at the point where it is modeled.

## 9. Assertion semantics

An assertion passes only if it holds on **all allowed executions**: all permitted initial register states, input valuations, event interleavings, reset behaviors, and choices for indeterminate `X`/`Z` bits, subject to the explicit assumptions.

For an assertion associated with a clock event, the property observes the stabilized pre-event state and the event's sampled signals. It is not evaluated using a post-update transient or by silently substituting the state after register updates. Sequential RTL then computes and applies its updates from that same pre-event state.

The precise sampling regions, implication timing, `$past`/history behavior, overlapped versus non-overlapped implication, repetition, `disable iff`, and other SVA-specific sampling and evaluation details belong in the **SVA Support Specification**. This document establishes the execution-state boundary and the all-executions proof criterion; it does not replace that SVA specification.

## 10. Proof outcomes

Lemma reports one of the following outcomes for a checked assertion:

- **PASS:** the assertion holds on every allowed execution under the modeled RTL subset and explicit assumptions.
- **FAIL:** Lemma found an allowed execution that violates the assertion. The counterexample should identify relevant initial register choices, input choices, event sequence, reset activity, and any interleaving choices.
- **UNKNOWN:** Lemma cannot establish PASS or FAIL within the supported semantics, implementation capabilities, or configured resources. UNKNOWN is not a proof of the assertion and is not a counterexample.

Reports should state whether arbitrary initial register values were included and should make the assumptions that restricted the execution set visible.

## 11. Normative implications and placement guidance

The execution engine specification should reference this document for stabilized-state transitions, arbitrary initial register state, reset triggering, free inputs, and clock-event interleavings. The synthesizable RTL subset specification should define which initialization and reset forms are accepted. The SVA Support Specification should define assertion sampling and evaluation details. A future multi-clock/event specification must resolve simultaneous-edge semantics and any coincident asynchronous-reset/clock-event atomicity questions.

Until those open items are resolved, tools and reports should identify them explicitly rather than treating an implementation choice as a settled formal guarantee.
