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

Each property must specify a single clock using `@(posedge clock)`. Multiclock properties are not supported.

`disable iff (expression)` is supported to disable a property while the expression is true.

## 3. Sequences and temporal operators

Supported sequence constructs:

- Boolean expressions.
- Fixed-delay concatenation: `##N`
- Bounded-delay concatenation: `##[N:M]`
- Overlapped implication: `|->`
- Non-overlapped implication: `|=>`

`N` and `M` must be non-negative integer constants, with `M >= N`.

Example:

```systemverilog
assert property (@(posedge clk) req |-> ##[1:3] gnt);
```

This property requires `gnt` to be true one, two, or three sampled clock edges after `req` is true.


## 4. Expressions
Property expressions may use:

- RTL signals and constants.
- Parentheses.
- Boolean operators: `!`, `&&`, `||`.
- Equality and relational operators supported by Lemma’s RTL expression subset.

Expressions must be compatible with the clock associated with their property.

## 5. Sampled-value functions

Lemma supports:

- `$past(expr)` — value of expr at the previous sampled clock edge.
- `$rose(expr)` — expression changed from low to high.
- `$fell(expr)` — expression changed from high to low.
- `$stable(expr)` — expression did not change from the previous sampled clock edge.

These functions use the property’s clock. Their sampling and initial-history behavior must be
defined consistently with SystemVerilog sampled-value semantics.

## 6. Out of scope for v1

The following constructs are not supported:

- Unbounded delays or unbounded sequences.
- Sequence repetition operators.
- `until`, `throughout`, `within`, and other advanced temporal operators.
- Local variables in sequences or properties.
- Multiclock properties or clocking events other than `@(posedge clock)`.
- Advanced SVA constructs such as `first_match`, `intersect`, and sequence `or`/`and`.
- Procedural immediate assertions.

## 7. Unsupported syntax

Lemma must report unsupported SVA syntax as an error. It must not silently ignore, approximate, or
reinterpret a property.

## 8. Clock Domains

A design may contain multiple clock signals and multiple clock domains. Lemma supports this at the
RTL level.

In v1, each SVA property must be associated with exactly one clock using `@(posedge clock)`. A
property may not reference or synchronize events from multiple clock domains.

Lemma does not require clocks to have a defined frequency or phase relationship. Properties spanning
multiple clock domains, including clock-domain-crossing properties, are outside the v1 scope.

The presence of multiple clocks in a design does not prevent proving properties that are local to a
single clock domain.
