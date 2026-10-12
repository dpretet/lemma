# Proof Model for the Lemma

The proof considers all possible executions of the design. An execution is a sequence of design states over time, together with the inputs presented to the design at each step.

## State and transitions

- The internal state consists primarily of the design's registers.
- Initial states follow the register semantics already defined for the design, including supported RTL initializations and Lemma's default-zero policy for other registers.
- Combinational and sequential logic determine the state transitions.
- Inputs that are not constrained by the proof context may vary freely across executions.

## Proof strategy

Lemma uses bounded model checking (BMC) together with k-induction for assertion
proofs. The elaborated RTL and target property are represented as a state
transition system; supported bounded SVA is lowered into monitor state and
transitions in that model.

- **BMC:** For a selected bound `k`, search for a violating execution of at
  most `k` transitions. A satisfying result is a counterexample. An unsatisfiable
  result means only that no counterexample exists within that bound; it is not
  by itself a proof for all time.
- **k-induction:** Check a base case covering the first `k` transitions, then
  an inductive step showing that any `k` consecutive states satisfying the
  property are followed by another state satisfying it. If both checks
  complete successfully, the property is proven for all executions of the
  model.

An induction step that fails does not by itself refute the property; it may
use a state sequence that is not reachable from the initial state. Lemma may
try larger `k` values within configured limits. If BMC finds no counterexample
and induction does not complete, the result is `Inconclusive`, never
`Proven`.

The initial strategy does not prescribe invariant strengthening, proof
decomposition, or an exact solver. Those remain implementation decisions.

## Meaning of a result

- **Proven:** The property holds for every allowed execution.
- **Failed:** There is an allowed execution in which the property is violated. That violating execution is reported as a counterexample trace.
- **Inconclusive:** The proof engine cannot determine whether the property holds within the available resources.

For a `cover property`, the corresponding question is reachability: report a
`covered` result and witness when found, `unreachable` only when established by
a complete analysis, and otherwise `inconclusive`. An assertion implication may
be proven vacuously when its antecedent never matches; this does not establish
that the antecedent is reachable.

This model describes the proof at the level of allowed executions, states,
inputs, transitions, and the BMC/k-induction strategy. It does not select a
solver or solver interface.
