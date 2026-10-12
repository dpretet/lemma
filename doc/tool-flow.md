# Tool Flow

## 1. Purpose

This document defines the high-level flow used by Lemma to process an RTL design and its assertions,
then report formal verification results.

It specifies the responsibilities and expected outputs of each stage. It does not prescribe a
parser, an internal representation, or a proof-engine implementation.

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

Lemma submits the design model and its properties to the proof engine.

For each property, the proof engine may determine that:

- The property holds for all executions allowed by the model.
- A counterexample exists.
- The result is inconclusive, for example because the proof did not complete.

The proof engine’s algorithm and implementation are outside the scope of this document.

## 8. Results and Diagnostics

Lemma reports a result for each property and an overall job status.

A result must distinguish at least:

- **Proven** — the property holds for all executions represented by the model.
- **Failed** — a violating execution was found.
- **Inconclusive** — the tool could not determine whether the property holds.

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
- The parser technology.
- The internal representation format.
- The proof algorithm or backend.
- The exact output format for logs and counterexamples.
- The detailed semantics of RTL or SVA constructs.

Those topics are defined in separate specifications.
