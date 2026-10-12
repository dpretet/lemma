# Verilog and SystemVerilog Support

## 1. Purpose

This document defines the initial RTL language subset accepted by the tool.
The target is simple, synthesizable RTL suitable for formal analysis.

The implementation must reject unsupported constructs explicitly. It must
not silently ignore them or guess their intended meaning.

This document is a working specification. **TBD** items are not yet normative.

## 2. Source languages

The initial target is Verilog/SystemVerilog RTL.

- SystemVerilog is the primary intended language.
- Verilog support is desirable where it does not complicate the initial
  implementation.
- VHDL is out of scope for the initial version.

The exact language standard and parser requirements are **TBD**.

## 3. Design units and hierarchy

The design is composed of modules, with one selected top-level module.
Module instantiation and hierarchy are required to support practical RTL,
but their detailed rules are **TBD**.

The elaborated design must have resolved parameter values and concrete signal
widths before conversion to the internal representation.

## 4. Parameters and elaboration

Initial parameter support shall include:

- `parameter` and `localparam`;
- integer and bit-vector values with statically known widths;
- constant expressions involving previously declared parameters;
- parameter overrides at module instantiation;
- resolution of parameter values before final signal widths are determined.

Parameter types (`parameter type`) are initially unsupported.

The exact supported expression operators, signedness rules, unsized literal
rules, and override forms are **TBD**.

## 5. Generate constructs

Generate constructs are part of the initial subset. They are elaborated
before producing the final internal representation.

Supported forms:

- `generate if / else`;
- `generate case`;
- `generate for` using `genvar`;
- nested combinations of these forms.

Generate conditions and loop bounds must be constant after parameter
resolution. A generate loop must have statically determinable termination.
Invalid or non-terminating loops must produce an error.

The precise scoping, naming, and loop-validity rules are **TBD**.

## 6. Combinational logic

Combinational logic may be described using:

- continuous `assign` statements;
- combinational procedural blocks, such as `always @(*)`.

Within a combinational procedural block:

- use blocking assignments (`=`);
- do not use nonblocking assignments (`<=`);
- do not describe sequential behavior.

The exact supported statement and expression forms are **TBD**.

## 7. Sequential logic

Sequential logic may be described using clocked procedural blocks.

Within a sequential block:

- use nonblocking assignments (`<=`);
- do not mix combinational and sequential behavior in the same process.

Reset semantics are defined in [register-semantics.md](register-semantics.md). A sequential
process may contain at most one reset, either synchronous or asynchronous. Supported event controls
and assignment semantics remain TBD.

## 8. Process restrictions

A single procedural process must be exclusively combinational or sequential.
Mixing the two styles in one process is unsupported and must be diagnosed.

The rules for incomplete assignments, inferred latches, multiple drivers,
and conflicting assignments are **TBD**.

## 9. Internal representation boundary

Parsing and elaboration must produce an internal representation independent
of the source syntax.

At the IR boundary:

- parameters must be resolved;
- generate constructs must be expanded;
- widths and concrete signal declarations must be known;
- unsupported source constructs must not be silently discarded.

The IR format and its exact semantics are documented separately.

## 10. Initially unsupported or undecided features

Unless explicitly added to this specification, the following are not part
of the initial supported subset:

- VHDL;
- `parameter type`;
- dynamic or runtime-dependent elaboration;
- unsynthesizable testbench constructs;
- mixed combinational/sequential processes.

Other constructs—including functions, tasks, interfaces, assertions,
memories, arrays, structs, and `always_comb` / `always_ff`—remain **TBD**
until explicitly specified.

## 11. Conformance tests

Every supported construct must have:

- at least one accepted example;
- at least one test for relevant boundary or error cases;
- documented expected elaboration or diagnostic behavior.

Tests must distinguish parsing, elaboration, and semantic validation failures.
