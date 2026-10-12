# Verilog and SystemVerilog Support

## 1. Purpose

This document defines the RTL language subset accepted by the tool and its
working long-term target. The target is common, synthesizable Verilog and
SystemVerilog RTL suitable for formal analysis, not the full SystemVerilog
language. Support is added incrementally; a feature is not accepted until its
semantics and conformance tests are specified.

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

An RTL design may contain clocked processes from any number of clock domains.
The supported event controls and rules for identifying each process's clock
domain remain subject to the subset defined here; a design is not rejected
solely because it contains multiple domains. Property-specific restrictions on
cross-domain dependencies are defined in the SVA support specification.

Within a sequential block:

- use nonblocking assignments (`<=`);
- do not mix combinational and sequential behavior in the same process.

Reset semantics are defined in [register-semantics.md](register-semantics.md).
A sequential process may contain at most one asynchronous reset event control.
Conditions in the process body—including a signal conventionally named
`srst`—are ordinary RTL control logic, not additional reset events. Supported
event controls and assignment semantics remain TBD.

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

## 12. Practical long-term target

The intended target is guided by common constructs found in the FRISCV RTL
review. The list describes the direction of the project, not features already
implemented or automatically included in the MVP:

- Module hierarchy, parameterized modules, `parameter` / `localparam`, and
  statically elaborated `if` / `case` / `for` generate constructs.
- Common declarations such as `wire` and `logic`, fixed-width packed vectors,
  and the port styles `input wire` / `output logic`.
- Sized constants and the SystemVerilog fill literals `'0` and `'1`, with
  documented width, signedness, and expression-sizing rules.
- Continuous assignments and separate combinational and sequential
  procedural blocks, including `always @(*)`, `always_comb`, and
  edge-triggered processes using nonblocking assignments.
- Common control and expression constructs: `if` / `else`, `case`, bounded
  procedural and generate loops, arithmetic, comparisons, bitwise and
  reduction operators, concatenation, replication, signed casts, variable
  indexing, and indexed part-selects.
- Common synthesizable SystemVerilog helpers, including `typedef enum logic`
  state types and automatic combinational functions with a restricted body.
- Statically sized unpacked arrays and memories with supported indexing and
  access patterns. Memory initialization and read/write behavior require their
  own specification; the default-zero policy for registers does not define
  memory semantics.
- The limited source preprocessing needed by real RTL projects, such as
  includes, macros, and conditional compilation under an explicit job
  configuration. Diagnostics must retain useful source locations after
  preprocessing.

The target does not imply support for full SystemVerilog, testbench features,
interfaces, packages, classes, randomization, DPI, or arbitrary dynamic
elaboration. Simulation-only tracing and logging should be excluded through a
documented preprocessing configuration or rejected explicitly; it must not be
silently dropped.

The FRISCV source commonly combines an asynchronous active-low reset in a
process event control with a reset-named condition in the process body (for
example, `negedge aresetn` plus `else if (srst)`). Lemma treats `aresetn` as
the asynchronous reset event and `srst` as ordinary control sampled at the
clock edge. Branch ordering and priority follow the RTL. This is a target use
case; conformance tests must distinguish the asynchronous event from the
clocked control condition.

## 13. Incremental conformance-test roadmap

The test suite should grow in small layers. Each layer should use minimized,
standalone RTL examples rather than requiring the complete FRISCV project or a
proprietary toolchain. A layer becomes supported only after its accepted cases,
boundary cases, expected model, and rejection cases are documented.

1. **Source and expression foundation:** explicit compile configuration and
   preprocessing; module ports; `wire` / `logic`; packed widths; assignments;
   common operators, signedness, expression sizing, `'0` / `'1`, concatenation,
   indexing, and part-selects.
2. **Combinational processes:** blocking assignments in `always @(*)` and
   `always_comb`, `if` / `case`, and complete assignment behavior. Include
   rejection tests for unsupported statements, mixed process styles, and
   unintended latches.
3. **Sequential state and resets:** nonblocking updates, simultaneous state
   updates, retention, clock events, default-zero initialization warnings,
   explicit initialization, asynchronous reset events, and ordinary
   reset-named control conditions sampled at clock edges. Include the common
   asynchronous-reset-plus-synchronous-control pattern and rejection tests for
   unsupported multiple asynchronous reset event controls.
4. **Parameters, generate, and hierarchy:** constant-expression resolution,
   named parameter overrides, static generate branches/loops, nested module
   instances, parameterized widths, and useful diagnostics for failed
   elaboration.
5. **Common SystemVerilog abstractions:** enum-based FSMs, supported automatic
   combinational functions, and bounded procedural loops.
6. **Arrays and memories:** static array declarations, variable addresses,
   supported read/write ports and byte enables, and separately specified
   memory initialization behavior.
7. **Realistic integration profile:** a small selected FRISCV configuration
   using only features already covered by earlier layers. Treat this as an
   integration regression, not as a substitute for focused unit tests.

The sequence is a proposed dependency order, not a schedule. Exact operators,
reset/event forms, memory patterns, and preprocessing behavior remain
non-normative until their layer is specified and tested. The current MVP
limits continue to apply until they are explicitly revised.
