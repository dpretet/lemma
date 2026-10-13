# Tool Flow

## 1. Purpose

This document defines the high-level flow used by Lemma to process an RTL design and its assertions,
then report formal verification results.

It specifies the responsibilities and expected outputs of each stage. The
current parser implementation uses Tree-sitter; this does not make every
construct recognized by its grammar part of Lemma's supported RTL subset. The
document does not prescribe an internal representation or proof-engine
implementation.

## 2. Overview

Lemma processes a verification job through the following stages:

1. Read the job configuration and source files.
2. Parse the supported RTL and assertion syntax.
3. Elaborate the design.
4. Build an internal model of the design and its properties.
5. Run the proof engine.
6. Report the result, including a counterexample when available.

Each stage must either produce its expected output or report an error that prevents later stages
from running.

## 3. Job Inputs

A verification job consists of:

- RTL source files in the supported language subset.
- Assertion source files, or assertions included in the RTL sources.
- The name of the top-level design unit.
- The clock associated with the property being checked.
- Any configuration required to resolve parameters and select the proof target.

The exact command-line interface and configuration-file format are outside the scope of this
document.

## 4. Parsing

Lemma parses the supplied sources and identifies:

- Design units and their ports.
- Signals, parameters, and supported RTL constructs.
- Sequential and combinational processes.
- Assertions within the supported SVA subset.

During RTL analysis, Lemma must identify registers without a supported RTL
initialization and emit a source-located warning that each receives the
default proof-start value `0`.

Unsupported syntax must be reported as an error. Lemma must not silently ignore unsupported
constructs if doing so could change the meaning of the design or its assertions.

Parsing does not establish that the design is fully elaborated or suitable for proof.

## 5. Elaboration

Lemma elaborates the selected top-level design by resolving the information required to construct a
concrete design model, including:

- Parameter values.
- Signal widths and types.
- The hierarchy needed for the selected design.

If required information cannot be resolved, elaboration fails with a diagnostic identifying the
cause.

RTL may contain multiple clock domains. Before proof, Lemma must identify the
selected property's complete signal-dependency cone. If the cone depends on
state updated by a different clock domain, the property must be rejected. A
domain that is outside the cone does not prevent a proof and need not be
included in that property's proof model.

The initial supported scope may restrict which forms of hierarchy or parameterization are accepted.

## 6. Design and Property Modeling

After elaboration, Lemma constructs an internal model containing:

- The design’s state elements.
- The combinational logic that determines outputs and next-state values.
- Supported RTL initialization values and assumptions about inputs defined by the project’s semantics.
- Lemma's default-zero proof-start values for registers without supported RTL initialization.
- The properties extracted from the supported assertions.

This model must preserve the semantics defined in the project documentation. It must not introduce
assumptions that are not part of those semantics.

## 7. Proof Execution

Lemma submits one target assertion or cover and its assumption properties to
the proof engine. Assumptions restrict the target's allowed executions.

For an assertion, Lemma uses BMC to search for counterexamples up to a selected
depth. A found violating trace produces `failed`. If BMC finds no violation
within that depth, Lemma must not report `proven` on that basis alone.

Lemma also uses k-induction: it checks the initial base cases and an inductive
step for a selected `k`. A property is `proven` only when both complete
successfully. If induction does not complete, including when its step fails
because it cannot establish the needed invariant, the result is
`inconclusive` unless BMC has already found a valid counterexample.

For an assertion, the outcomes are therefore:

- The property holds for all executions allowed by the model.
- A counterexample exists.
- The result is inconclusive, for example because the proof did not complete.

For a cover, the proof engine searches for a matching execution. Failure to
find a witness is not proof of unreachability unless the analysis is complete.

The exact solver, solver interface, bound-selection policy, resource limits,
and any future invariant-strengthening method remain unspecified.

## 8. Results and Diagnostics

Lemma reports a result for each property and an overall job status.

For an assertion, report `proven`, `failed`, or `inconclusive`. For a cover,
report `covered` with a witness, `unreachable` only when established by a
complete analysis, or `inconclusive`. Do not report a bounded search with no
witness as proof of unreachability.

All assumptions used for the target must be reported. Contradictory assumptions
or an empty set of allowed executions must be diagnosed; they must not produce
an unqualified `proven` result. If an implication is known to pass vacuously
because its antecedent is unreachable, disclose that fact.

When a property fails and a counterexample is available, Lemma should report enough information to
identify the violating execution, including the relevant cycle and signal values.

Errors encountered during parsing, elaboration, or model construction must be reported separately
from proof results.

## 9. Stage Ordering and Failure Handling

The stages run in order. A failure in a stage prevents dependent later stages from running.

Lemma should report:

- The stage in which the failure occurred.
- The source location, when available.
- A concise explanation of the problem.

A job with one or more unsupported or invalid properties must not be reported as fully proven.

## 10. Out of Scope

This document does not define:

- The command-line interface or configuration-file syntax.
- The internal representation format.
- The exact solver, solver interface, and encoding implementation. The selected BMC plus k-induction strategy is defined in the proof-model specification.
- The exact output format for logs and counterexamples.
- The detailed semantics of RTL or SVA constructs.

Those topics are defined in separate specifications.
