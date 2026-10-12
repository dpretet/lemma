# Proof Model for the Lemma

The proof considers all possible executions of the design. An execution is a sequence of design states over time, together with the inputs presented to the design at each step.

## State and transitions

- The internal state consists primarily of the design's registers.
- Initial states follow the register semantics already defined for the design, including supported RTL initializations and Lemma's default-zero policy for other registers.
- Combinational and sequential logic determine the state transitions.
- Inputs that are not constrained by the proof context may vary freely across executions.

## Meaning of a result

- **Proven:** The property holds for every allowed execution.
- **Refuted:** There is an allowed execution in which the property is violated. That violating execution is reported as a counterexample trace.
- **Inconclusive:** The proof engine cannot determine whether the property holds within the available resources.

This model describes the proof at the level of allowed executions, states, inputs, and transitions; it does not prescribe more specific assertion-language semantics or solver implementation choices.
