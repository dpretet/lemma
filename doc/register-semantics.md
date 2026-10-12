# Register Semantics

## 1. Proof-State Model

Lemma models RTL behavior as a sequence of stable states. It does not model combinational propagation delays.

Let \(S_t\) be the RTL state stabilized immediately before an active clock edge:

1. Concurrent assertions sample their expressions from \(S_t\).
2. Sequential processes compute their next values from \(S_t\).
3. Registers update simultaneously, forming the next state.
4. Combinational logic is reevaluated until it stabilizes, forming \(S_{t+1}\).

Transient states caused by combinational propagation are excluded from the proof semantics.

For concurrent assertions, property expressions use the values sampled at the clock event. They are not evaluated using the state after register updates and combinational settling.

## 2. Clock Model

RTL may contain multiple clock domains. For a property proof, Lemma models one
selected domain and the state in that property's dependency cone; unrelated
domains are excluded. A dependency on state updated by another clock domain
must be rejected rather than modeled as an unconstrained value. Clock edges
are abstract events; physical periods and time are not modeled. Cross-domain
event interleavings are outside the initial proof scope.

## 3. Register Initialization

A register without a supported RTL initialization starts with a randomly chosen value. Each bit is assigned `0` or `1` independently.

The chosen initial value remains fixed until the register is updated. Lemma does not consider every possible initial value during a single proof run.

An RTL initialization is honored only if it is supported by Lemma. Lemma must not assume that a reset has occurred unless the RTL or proof constraints establish that it has.

For a register with a reset, the reset affects the register only according to its RTL semantics. In particular, a synchronous reset does not initialize the register merely because it is active at the start of the proof; it is applied at an active clock edge.

## 4. Reset Semantics

### 4.1 Reset Type

Lemma distinguishes:

- **Synchronous reset:** sampled and applied only at the active clock edge.
- **Asynchronous reset:** applied as soon as it is asserted, independently of a clock edge.

An asynchronous reset can therefore cause a state transition outside a clock event. Lemma must not reinterpret it as synchronous.

### 4.2 Reset Polarity and Values

Reset polarity is inferred from the RTL condition. Lemma imposes no default active polarity.

Each register may have its own reset value. A reset value must be a constant or a resolved parameter; values that depend on other signals are not supported.

Every register in a resettable sequential process must receive an explicit value in the reset branch.

### 4.3 Reset and Clock Events

At a clock edge, a synchronous reset is evaluated according to the RTL. An active asynchronous reset imposes the reset value, including when reset assertion coincides with a clock edge.

When reset and enable, or other conditions, are combined, their priority follows the order and conditions expressed in the RTL. Lemma adds no priority rule of its own.

### 4.4 Reset Deassertion

Deasserting reset does not itself update a register. The register retains its current value and resumes loading data at the next active clock edge, as specified by the RTL.

### 4.5 Supported Reset Structure

A sequential process may contain at most one reset, either synchronous or asynchronous. Support for multiple resets is deferred.

## 5. Register Updates and Retention

Sequential processes compute next values from the current state. Register updates take effect simultaneously.

If no branch assigns a value to a register at an active clock edge, the register retains its previous value. This includes conditional updates, such as an enable that is not asserted.

## 6. Inputs and Assumptions

Module inputs are unconstrained by default: the proof engine may assign them any value at each cycle.

Only explicit `assume` constraints restrict input behavior.

## 7. Unknown and High-Impedance Values

Lemma uses a two-state proof model. RTL `X` and `Z` values are not represented as simulation states.

Each bit that is `X` or `Z` is treated as indeterminate: the proof engine may assign it either `0` or `1`. The proof must account for both possible values.

## 8. Concurrent Assertions

Concurrent assertion expressions are sampled at their clock event, before sequential register updates. Property evaluation uses those sampled values, even if the state changes and combinational logic settles afterward.

The `disable iff` construct is supported. Its condition acts as an asynchronous disable: when it becomes true, the assertion is disabled and any active property attempts are aborted. The disable condition is not treated as a clock-sampled property expression.

## 9. Reporting

Proof reports must state that registers without an explicit supported initialization receive randomly chosen initial values.

If such values contribute to a counterexample, the trace should show the initial values used.
