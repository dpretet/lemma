# Lemma Architecture Checkpoint 1

## Purpose

Lemma is an independent open-source formal verification tool for synthesizable RTL, focused on
designs written in a simple, well-defined subset of Verilog/SystemVerilog.

The tool verifies RTL using a restricted subset of SVA assertions. It is a standalone software
project, not an extension of SVUT.

The project is intended to be developed incrementally by a solo RTL designer. Architectural
decisions should favor a small, explicit scope and clear semantics over broad language coverage.

## User Context

The project owner is an experienced RTL designer with approximately 20 years of experience. They use
Verilator and Icarus Verilog for simulation and maintain SVUT, a SystemVerilog testbench frontend.

They have some C experience but are not primarily a software developer. Implementation language and
parser choices have not yet been finalized.

## Agreed RTL Scope

- Support synthesizable RTL written in a simple Verilog/SystemVerilog subset.
- VHDL is out of scope for the initial project.
- Use an existing parser if practical; Tree-sitter has been considered.
- Elaborate parameters and resolve their values.
- Support generate constructs early, with a deliberately limited scope.
- Support combinational logic using continuous assignments and `always @(*)`.
- Support sequential logic using clocked processes.
- Combinational processes use blocking assignments (`=`).
- Sequential processes use nonblocking assignments (`<=`).
- Do not mix combinational and sequential logic in one process.
- Keep the supported subset explicit and document unsupported constructs.

The exact grammar, IR, and elaboration rules still need to be specified.

## Agreed SVA Scope

- Lemma is an assertion-based formal verification tool.
- Support a restricted but useful subset of SVA.
- The exact supported syntax and semantics are documented separately in the SVA support document.
- RTL designs may contain multiple clock domains. Each v1 property is associated with one clock using
  `@(posedge clock)`, and its full signal-dependency cone must be confined to that domain (apart from
  constants and unconstrained inputs).
- Properties with dependencies spanning multiple clock domains are unsupported and must be rejected.
  Lemma does not check CDC behavior or model cross-domain event interleavings for a proof.

## Project Name

The project name is **Lemma**, referencing mathematical proof.

## Architecture Topics Not Yet Defined

The following architectural decisions remain open:

1. **Frontend and elaboration**
   - Parser selection and integration.
   - Module selection, parameter overrides, and top-level elaboration.
   - Exact supported generate constructs and elaboration behavior.

2. **Internal representations**
   - RTL IR design and invariants.
   - Representation of properties and their clock association.
   - How source locations and names are preserved for diagnostics.

3. **Formal semantics**
   - State-transition model for sequential logic.
   - Supported RTL initialization forms and reset treatment (including the default-zero policy for registers without supported initialization).
   - Cycle and clock semantics.
   - Four-state versus two-state interpretation.
   - Semantics of assumptions, assertions, and unsupported/unknown results.

4. **Proof engine and backend**
   - Bounded versus unbounded proof strategy.
   - SAT/SMT encoding and solver interface.
   - Counterexample representation and replay/export.
   - Resource limits, termination, and result classification.

5. **Tool interface and workflow**
   - Command-line interface and project configuration.
   - Input file organization and build/elaboration flow.
   - Machine-readable and human-readable reports.

6. **Validation and project structure**
   - Unit, parser, elaboration, semantic, and end-to-end tests.
   - Reference examples and regression suite.
   - Documentation and contributor/agent instructions.

## Recommended Next Discussion

Define the **formal semantics** first, beginning with the sequential transition model, initial-state
handling, reset behavior, and single-clock property evaluation. These choices form the contract
between the RTL IR, SVA representation, and proof backend.

## Working Style

- Discuss one architectural point at a time.
- Keep responses factual and concise; avoid long digressions.
- Do not generate code unless explicitly requested.
- Documentation intended for the repository should be written in English.
- Treat the agreed scope above as provisional only where a decision is explicitly marked open.
