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

A register with a supported RTL initialization starts with that value. Every
register without a supported RTL initialization starts at `0` at proof start.
This is Lemma's default-initialization policy, not an initialization or reset
behavior inferred from the RTL. The value remains `0` until the RTL updates the
register.

During RTL analysis, Lemma must emit a warning for each register assigned this
default. The warning must identify the register and its source location and
state that its proof-start value is set to `0` by Lemma. The proof report must
also disclose that the proof uses the default-zero policy.

The default value is applied at proof start; it does not mean that an RTL reset
has occurred. Resets affect registers only according to their RTL semantics.
A reset-named condition in a clocked process body is ordinary RTL control
evaluated at the active clock edge; it is not a separate reset event. An
asynchronous reset event control takes effect only when asserted. A reset
branch may subsequently assign a value different from the default zero.

## 4. Reset Semantics

### 4.1 Reset Type

Lemma gives special event semantics only to an **asynchronous reset** listed
in a sequential process's event control. Its assertion can cause a state
transition independently of a clock edge.

A condition in a clocked process body—even if named `srst` or described as a
synchronous reset—is ordinary sequential control logic. It is evaluated at
the process's active clock edge like any other condition and follows the
process's normal assignment and branch-priority semantics. It does not create
an independent event or receive special reset treatment from Lemma.

An asynchronous reset can therefore cause a state transition outside a clock event. Lemma must not reinterpret it as synchronous.

### 4.2 Reset Polarity and Values

Asynchronous reset polarity is inferred from its RTL event control and
condition. Lemma imposes no default active polarity. Ordinary clocked
conditions have no separate reset-polarity semantics.

For an asynchronous reset branch, each register may have its own reset value.
Those asynchronous reset values must be constants or resolved parameters;
values depending on other signals are not supported. Values assigned by
ordinary clocked conditions are governed by the supported RTL expression
subset.

Every register in a process with an asynchronous reset event control must be
assigned explicitly in its asynchronous reset branch. Incomplete assignments
in other branches follow the ordinary retention rules.

### 4.3 Reset and Clock Events

At a clock edge, all process conditions—including synchronous-reset-style
conditions—are evaluated according to the RTL. An active asynchronous reset
imposes its reset value, including when assertion coincides with a clock edge.

When an asynchronous reset and ordinary clocked conditions are combined, their
priority follows the event control, conditions, and branch order expressed in
the RTL. Lemma adds no priority rule of its own.

### 4.4 Reset Deassertion

Deasserting an asynchronous reset does not itself update a register. The
register retains its current value and resumes ordinary RTL updates at the
next active clock edge. Deasserting a reset-named clocked condition likewise
does not update state until the next active edge evaluates the process.

### 4.5 Supported Reset Structure

A sequential process may contain at most one asynchronous reset event control.
Conditions in the process body are ordinary RTL control and are not counted as
additional resets. Multiple asynchronous reset event controls in one process
are deferred.

## 5. Register Updates and Retention

Sequential processes compute next values from the current state. Register updates take effect simultaneously.

Within one sequential process, assignments to the same register are evaluated
in source order. If multiple assignments execute during an active clock event,
the last executed assignment determines the register's next value. A
conditional assignment that does not execute does not cancel an earlier
assignment to that register in the process. For example:

```systemverilog
if (a)
  q <= x;
else
  q <= z;

if (c)
  q <= b;
```

has the next-value expression `c ? b : (a ? x : z)`. The process's
source-order and branch behavior is represented in the register's D expression.

If no executed branch assigns a value to a register at an active clock edge,
the register retains its previous value. This includes a conditional update
whose condition is false when no earlier assignment in the same process has
assigned that register. Multiple procedural processes driving the same
register are unsupported in the MVP and must be diagnosed; they are not covered
by this within-process priority rule.

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

Proof reports must disclose the default-zero policy and identify registers to
which it was applied. Counterexamples must reflect those zero proof-start
values; reports must not describe them as values established by an RTL reset.
