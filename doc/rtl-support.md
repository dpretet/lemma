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

The initial parser implementation is C using the Tree-sitter runtime and the
`tree-sitter-systemverilog` grammar. These are syntax-front-end choices only:
the grammar may recognize constructs outside Lemma's supported subset, and
such constructs must still be rejected by Lemma's validation. The exact
language-standard conformance, accepted syntax, preprocessing behavior, and
Verilog grammar requirements remain **TBD**.

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

The exact supported expression operators, general expression-sizing rules,
unsized literal rules, and override forms are **TBD**, except for the
same-width unsigned bitwise operators, logical operators, basic arithmetic and
comparisons, packed concatenation, constant packed bit/part-selects, signedness
conversions, and direct assignment width conversions specified here.

### Same-width unsigned bitwise operators

For unsigned packed operands with the same concrete width, Lemma supports
bitwise AND (`&`), OR (`|`), XOR (`^`), and XNOR (`~^` or `^~`). These binary
operators produce an unsigned result of the operand width and operate
bit-by-bit. Unary bitwise complement (`~`) is also supported; it inverts each
bit and preserves the operand width and unsignedness. The two XNOR spellings
have identical semantics.

Mixed-width or signed operands remain TBD; these rules do not define implicit
sizing or signedness conversion for bitwise operators.

For an unsigned packed-vector operand, Lemma also supports reduction AND
(`&`), NAND (`~&`), OR (`|`), NOR (`~|`), XOR (`^`), and XNOR (`~^` or `^~`).
Each reduction operates across all operand bits and produces a one-bit unsigned
result. NAND, NOR, and XNOR complement the corresponding reduction result; the
two XNOR spellings have identical semantics.

### Same-width unsigned basic arithmetic

For two unsigned packed operands with the same concrete width `N`, Lemma
supports addition (`+`), subtraction (`-`), and multiplication (`*`). Each
result is an unsigned `N`-bit vector containing the mathematical result modulo
`2^N`; bits beyond bit `N-1` are discarded. Mixed-width or signed operands,
division, remainder, and other arithmetic operators remain TBD.

### Same-width unsigned comparisons

For two unsigned packed operands with the same concrete width, Lemma supports
equality (`==`), inequality (`!=`), and relational comparisons (`<`, `<=`, `>`,
`>=`). Each comparison produces a one-bit unsigned result. Relational
comparisons use unsigned ordering. Mixed-width or signed operands and other
comparison operators remain TBD.

### Logical operators

Lemma supports logical negation (`!`), logical AND (`&&`), and logical OR
(`||`) on packed operands of known concrete widths. An operand is true when
its value is nonzero; `!` negates that truth value, `&&` is true when both
operands are true, and `||` is true when either operand is true. Each operator
produces a one-bit unsigned result, regardless of operand width or signedness.
These are logical operators, not bitwise operators; their evaluation follows
Lemma's two-state interpretation of indeterminate bits.

### Packed concatenation

Lemma supports concatenation of packed signal operands using `{a, b, ...}`.
The result is an unsigned packed vector whose width is the sum of the operand
widths. Operands retain their declared widths and are ordered from most
significant (leftmost) to least significant (rightmost); concatenation does not
extend or truncate its operands. Replication and unsized operands remain TBD.

### Constant packed bit-selects and part-selects

For a descending packed vector, Lemma supports in-range constant bit-selects
(`value[index]`) and fixed part-selects (`value[msb:lsb]`) with constant bounds.
A bit-select produces one bit; a part-select produces `msb - lsb + 1` bits,
preserving the selected bits' order. Select results are unsigned. Variable
bit-selects (`value[index]`) and indexed part-selects using `+:` are also
supported when their indices are unsigned expressions with concrete widths.
The width of an indexed part-select must be a positive elaboration-time
constant; its result width is that constant. For a descending packed vector,
`value[base +: width]` selects `value[base + width - 1 : base]`, preserving
that bit order in the result. The frontend must prove every possible selected
index is within the packed vector's bounds. If it cannot, the design is invalid
and must be rejected before proof. Indexed part-selects using `-:` remain TBD.

### Signedness conversions and assignment width conversion

Lemma supports the SystemVerilog signing casts `signed'(expr)` and
`unsigned'(expr)`, and the system functions `$signed(expr)` and
`$unsigned(expr)`. Each pair has the same effect: preserve the operand's bit
width and bit pattern while setting the result's signedness to signed or
unsigned, respectively.

For a direct assignment from a packed signal or one of these cast/function
results to a wider packed signal, Lemma extends the source to the destination
width according to the source expression's signedness: signed sources are
sign-extended; unsigned sources are zero-extended. Equal-width assignments
preserve the bit pattern, while the destination keeps its declared signedness.

For a direct assignment from a packed signal or cast/function result to a
narrower packed signal, Lemma discards the most-significant bits and preserves
the destination-width least-significant bits, regardless of source signedness.
The destination keeps its declared signedness. RTL analysis must emit a
source-located warning for each such narrowing assignment, stating that the
most-significant bits are discarded and the represented value may change. The
warning does not reject the assignment. The IR must represent the cast and any
widening or truncation explicitly.

Unsized literals and sizing or signedness rules for other expressions remain
TBD and must not be inferred from these rules.

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
- combinational procedural blocks using `always @(*)` or `always_comb`.

For Lemma's stable-state circuit model, a supported `always_comb` block is
lowered to combinational equations in the same way as a supported
`always @(*)` block. Lemma does not model simulation scheduling, implicit
sensitivity-list behavior, or time-zero execution as distinct circuit
semantics. Both forms must obey the same supported combinational statement and
driver rules.

Within a combinational procedural block:

- use blocking assignments (`=`);
- do not use nonblocking assignments (`<=`);
- do not describe sequential behavior.

Blocking assignments take effect in source order. If multiple executed
assignments in one process target the same variable, the last executed
assignment determines its value for that process invocation. A conditional
assignment that does not execute leaves the value established by earlier
executed statements unchanged.

For a supported procedural `if`, a packed condition is true when its value is
nonzero and false when all bits are zero; the condition's signedness does not
change this test. Indeterminate bits follow Lemma's two-state interpretation,
so each bit is an unconstrained binary choice when evaluating the condition.

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
- multiple assignments to the same register within one process are evaluated
  in source order; when more than one assignment executes, the last executed
  assignment determines that register's next value;
- a conditional assignment that does not execute leaves any earlier assignment
  to that register in the same process in effect. If no assignment executes,
  the register retains its current value.

Reset semantics are defined in [register-semantics.md](register-semantics.md).
A sequential process may contain at most one asynchronous reset event control.
Conditions in the process body—including a signal conventionally named
`srst`—are ordinary RTL control logic, not additional reset events. Supported
event controls and other assignment semantics remain TBD. This source-order
priority rule applies only to assignments within one process; multiple
procedural drivers for a register are unsupported in the MVP and must be
diagnosed.

## 8. Process restrictions

A single procedural process must be exclusively combinational or sequential.
Mixing the two styles in one process is unsupported and must be diagnosed.

The rules for incomplete combinational assignments, inferred latches, and
conflicting assignments remain **TBD**. Multiple procedural drivers for a
register are unsupported in the MVP and must be diagnosed.

## 9. Internal representation boundary

Parsing and elaboration must produce an internal representation independent
of the source syntax.

At the IR boundary:

- parameters must be resolved;
- generate constructs must be expanded;
- widths and concrete signal declarations must be known;
- unsupported source constructs must not be silently discarded.

The structural IR contract is documented in
[internal-representation.md](internal-representation.md). Operator semantics
and supported source constructs remain governed by this document and must be
specified before those constructs are lowered.

## 10. Initially unsupported or undecided features

Unless explicitly added to this specification, the following are not part
of the initial supported subset:

- VHDL;
- `parameter type`;
- dynamic or runtime-dependent elaboration;
- unsynthesizable testbench constructs;
- mixed combinational/sequential processes;
- source preprocessing directives, including `` `include ``, macros, and
  conditional compilation. These may be added later under an explicit job
  configuration; the long-term target in Section 12 does not include them in
  the MVP.

Other constructs—including user-defined functions, system functions other than
`$signed` / `$unsigned`, tasks, interfaces, assertions, memories, arrays,
structs, and `always_ff`—remain **TBD**
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
  procedural and generate loops, logical operators, arithmetic, comparisons, bitwise and
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
   `always_comb`, source-order priority for repeated assignments to one signal,
   `if` / `case`, and complete assignment behavior. Include rejection tests for
   unsupported statements, mixed process styles, and unintended latches.
3. **Sequential state and resets:** nonblocking updates, source-order priority
   for repeated assignments within one clocked process, simultaneous state
   updates, retention, clock events, default-zero initialization warnings,
   explicit initialization, asynchronous reset events, and ordinary
   reset-named control conditions sampled at clock edges. Include the common
   asynchronous-reset-plus-synchronous-control pattern and rejection tests for
   multiple procedural drivers and unsupported multiple asynchronous reset
   event controls.
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
