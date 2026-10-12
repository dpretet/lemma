# Lemma SVA Support Profile — v1

## 1. Purpose

Lemma v1 supports a restricted subset of SystemVerilog Assertions (SVA) for describing properties of
synchronous RTL.

This document defines the supported constructs. Any SVA construct not listed here is outside the v1
scope and must be rejected with a diagnostic.

## 2. Property statements

Lemma supports the following concurrent property statements:

- `assert property` — specify a property to prove.
- `assume property` — constrain the environment.
- `cover property` — check whether a property is reachable.

One proof run selects exactly one target `assert property` or `cover property`.
It may also include zero or more `assume property` constraints. Assumptions
constrain the executions considered for the target; they are not additional
assertions to prove.

Every target and assumption must specify the same single clock using
`@(posedge clock)`. Its full signal-dependency cone must belong to that clock
domain, apart from constants and unconstrained module inputs. A dependency on
state updated by another clock domain is unsupported and must be rejected.
Module inputs are environment values and are not assigned to a clock domain by
Lemma.

`disable iff (expression)` is supported. Its condition is asynchronous: while
it is true, new property attempts are disabled and active attempts are aborted.
When it becomes false, aborted attempts do not resume; new attempts may start
at later clock samples. It does not imply that reset has occurred at proof
startup.

A property is attempted at every sample of its selected clock unless disabled
or in its history-function warm-up period.

## 3. Sequences and temporal operators

Supported sequence constructs:

- Boolean expressions.
- Fixed-delay concatenation: `##N`
- Bounded-delay concatenation: `##[N:M]`
- Overlapped implication: `|->`
- Non-overlapped implication: `|=>`

`N` and `M` must be non-negative elaboration-time integer constants, with
`M >= N`. A fixed delay `##N` matches the following sequence exactly `N`
sampled clock events after the preceding sequence completes; `##0` uses the
same sample. A bounded delay `##[N:M]` permits a match at any one offset from
`N` through `M`, inclusive.

For an implication, `|->` starts its consequent at the sample where the
antecedent sequence completes. `|=>` starts the consequent one sampled clock
event later. Each match of an antecedent starts an independent consequent
obligation. Thus, if `req` is true on several consecutive samples, each sample
starts its own attempt.

Example:

```systemverilog
assert property (@(posedge clk) req |-> ##[1:3] gnt);
```

This property requires `gnt` to be true one, two, or three sampled clock edges after `req` is true.

For a simple Boolean antecedent, `req |-> gnt` checks `gnt` at the same sample
as `req`, while `req |=> gnt` checks it at the next sample. An implication
passes vacuously on traces where its antecedent never matches; use a companion
`cover property` when reachability of the antecedent matters.


## 4. Expressions
Property expressions may use:

- RTL signals and constants.
- Parentheses.
- Boolean operators: `!`, `&&`, `||`.
- Equality and relational operators supported by Lemma’s RTL expression subset.

Expressions and their transitive RTL dependencies must be compatible with the
clock associated with their property. This restriction also applies to
`assume property`, `cover property`, and `disable iff` expressions used in the
proof context. The operator widths and signedness are those of the elaborated
RTL expression subset.

## 5. Sampled-value functions

Lemma supports:

- `$past(expr)` — value of expr at the previous sampled clock edge.
- `$rose(expr)` — expression changed from low to high.
- `$fell(expr)` — expression changed from high to low.
- `$stable(expr)` — expression did not change from the previous sampled clock edge.

These functions use the property's clock. `$past(expr)` uses the value sampled
at the preceding clock event. `$rose` and `$fell` are limited to single-bit
expressions; `$stable` compares the complete expression value.

Lemma automatically suppresses evaluation of a property that uses one of
these history functions on the first sample of its clock domain, when no prior
sample exists. Evaluation starts once the required history is available. The
history-validity tracking is internal to Lemma and is not an environmental
assumption or RTL state. It advances on every selected clock sample and is not
cleared by `disable iff`. An explicit user `past_valid` guard remains supported
and may be used for portability to other tools; it is not required by Lemma.
When such a guard becomes true after a prior sample, Lemma must not add another
warm-up cycle beyond the guard's own RTL behavior.

Reports must disclose when history warm-up samples were skipped and how many
samples were skipped.

History functions are not supported inside `disable iff` in v1 and must be
rejected there.

## 6. Assumptions, coverage, and vacuity

Module inputs are unconstrained unless restricted by explicit assumptions.
Assumptions define the proof environment and must be listed in reports. An
assumption that refers to DUT state or outputs can exclude real RTL behaviors;
such use must be explicit and visible, not confused with a guarantee proved by
Lemma. Contradictory assumptions must be diagnosed so an empty behavior set is
not reported as an ordinary successful proof.

`cover property` asks Lemma to find an execution matching the cover sequence.
A found trace has outcome `covered`. Failure to find a witness within an
incomplete search has outcome `inconclusive`; outcome `unreachable` is allowed
only when the analysis establishes that no matching execution exists.

An assertion implication may be proven even when its antecedent never matches.
Reports should disclose known vacuity; users should pair important implication
assertions with covers for the antecedent or complete antecedent sequence.

## 7. Common verification patterns

These examples illustrate the intended v1 use. Each target is run separately;
environment assumptions may accompany it.

```systemverilog
// A request must complete within four sampled cycles.
assert property (@(posedge clk) disable iff (!aresetn)
    req |-> ##[1:4] ack);

// A valid transfer must remain stable while the receiver applies backpressure.
assert property (@(posedge clk) disable iff (!aresetn)
    valid && !ready |=> valid && $stable(payload));

// The environment must not request two mutually exclusive operations together.
assume property (@(posedge clk) !(read_req && write_req));

// Check that a request/completion sequence can occur.
cover property (@(posedge clk) disable iff (!aresetn)
    req ##[1:4] ack);
```

For portability, a user may explicitly track whether a preceding clock sample
exists. The following helper is ordinary resettable RTL state, not an
assumption; it is optional in Lemma because Lemma performs history warm-up
automatically:

```systemverilog
logic past_valid;
always @(posedge clk or negedge aresetn) begin
    if (!aresetn) past_valid <= 1'b0;
    else          past_valid <= 1'b1;
end

assert property (@(posedge clk) disable iff (!aresetn)
    past_valid |-> $stable(payload));
```

## 8. Out of scope for v1

The following constructs are not supported:

- Unbounded delays or unbounded sequences.
- Sequence repetition operators.
- `until`, `throughout`, `within`, and other advanced temporal operators.
- Local variables in sequences or properties.
- Multiclock properties or clocking events other than `@(posedge clock)`.
- Advanced SVA constructs such as `first_match`, `intersect`, and sequence `or`/`and`.
- Procedural immediate assertions.

## 9. Unsupported syntax

Lemma must report unsupported SVA syntax as an error. It must not silently ignore, approximate, or
reinterpret a property.

## 10. Clock Domain Scope

RTL may contain any number of clock domains. Each property proof uses one
selected clock domain and only the state and combinational logic in that
property's dependency cone. A property is rejected if that cone depends on a
register updated by another clock domain. Clock domains outside the cone do
not prevent the proof and are not modeled for it.

Lemma does not model cross-domain event interleavings or perform
clock-domain-crossing analysis. Properties that synchronize or otherwise
depend on behavior across domains are outside the v1 scope.
