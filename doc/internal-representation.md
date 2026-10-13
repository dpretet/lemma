# Circuit Internal Representation

## 1. Purpose and boundary

This document defines the structural contract for Lemma's elaborated circuit
IR. The IR is independent of SystemVerilog syntax and is the boundary between
RTL parsing/elaboration and proof-model construction. It is not a source AST and
does not prescribe a solver encoding.

The IR describes one selected, fully elaborated module. Before a circuit is
lowered to this IR, parameters and generate constructs must be resolved,
hierarchy must be reduced to the selected circuit, and every signal must have a
concrete width. The initial MVP is a single flat top module.

This specification defines the representation shape, not the complete RTL
subset. A source construct may be lowered only after its accepted syntax and
semantics are specified in [RTL Support](rtl-support.md). Unsupported or
ambiguous source constructs must not be omitted while building the IR.

## 2. Circuit and signal model

A circuit contains:

- one module name;
- an ordered list of ports;
- a signal table;
- a graph of combinational expressions;
- combinational drivers; and
- sequential state elements.

Each signal has a stable circuit-local ID, a source-level name, a positive
concrete bit width, signedness, and source provenance when available. Signal
values are two-state bit-vectors. Ports refer to signals by ID and have a
direction (`input` or `output`); port order is preserved. An input signal is an
environment value, an output signal is an externally observable value, and an
internal signal is neither. The IR does not retain source distinctions such as
`wire` versus `logic` unless a later semantic need is specified.

`inout` ports, memories, unpacked arrays, and unresolved parameter-sized
signals are deferred from this initial circuit IR, not ruled out permanently.
A later IR revision may add them once their representation and semantics are
specified. Until then, the frontend must reject them before IR construction.

## 3. Combinational expression graph

Combinational logic is represented as a directed graph of typed expression
nodes. Every node has a stable ID, a result width and signedness, an operation
kind, operand IDs, and source provenance when available. Nodes may be shared;
their meaning must not depend on source traversal order.

The initial structural node kinds are:

- a known bit-vector constant;
- an indeterminate bit-vector value, for source `X`/`Z` bits under the
  two-state semantics (each indeterminate bit is an unconstrained choice);
- a read of a signal ID; and
- an operator node with an explicitly identified operator and operands.

The operator vocabulary and bit-vector meaning are governed by the RTL subset
and must be explicit for each supported operator. Before an expression reaches
the IR, implicit source-language sizing, signedness, and extension behavior
must be resolved into concrete widths and explicit conversions as needed. The
IR must not rely on a later stage to reinterpret SystemVerilog sizing rules.

A combinational driver associates a target signal with an expression ID. The
driver describes the full target signal; partial writes, procedural control
flow, and assignment priority must first be lowered to an equivalent full-width
expression. Expression dependencies describe the logic paths from input and
state signals to driven signals.

Combinational cycles, conflicting/multiple drivers, and incomplete assignments
must not be silently collapsed into this graph. Their acceptance and semantics
are still subject to the RTL subset specification; until specified, a frontend
must diagnose them rather than claim a valid circuit IR.

## 4. Sequential state elements (D-type flip-flops, FFDs)

Each edge-triggered register is represented by one FFD state element. It has:

- a Q signal ID, which is the state value visible to combinational logic;
- a D expression ID, which computes the value sampled at its active clock
  event;
- a clock signal ID and active edge (`posedge` or `negedge`);
- optional asynchronous-reset metadata: reset signal, asserted edge/polarity,
  and reset value;
- optional supported RTL initialization metadata; and
- source provenance when available.

The Q signal width and D expression width must match. A clock and asynchronous
reset signal are one bit wide, and an asynchronous reset value has the same
width as Q.

The FFD is an explicit state boundary in the logic graph: its Q signal is read
by expressions, while its D expression describes the next value. All normal
clocked control, including synchronous-reset-style conditions, enables, and
retention, is represented in the D expression. In particular, retention is
expressed as feedback from Q where required; it is not an implicit property of
an ordinary combinational driver. An asynchronous reset is separate metadata,
not a clocked condition folded into D.

This representation follows the register semantics: state updates are
simultaneous, inputs are not persistent state, and Lemma's default-zero proof
start policy applies to registers without supported RTL initialization. The IR
records RTL initialization if supported; whether the proof model applies the
default-zero policy is a later modeling step and must not be misreported as RTL
behavior.

Latches and other level-sensitive storage are not FFDs and are outside this
initial representation. Multiple asynchronous reset events per register and
the exact treatment of coincident clock/reset events remain governed by the
register-semantics specifications.

## 5. Invariants at the IR boundary

A circuit IR is valid only when:

1. every referenced signal and expression ID resolves;
2. all signal and expression widths are concrete and positive;
3. every expression has an explicit operator and concrete result type;
4. every sequential element has a defined clock edge and a D expression whose
   width matches its Q signal;
5. all accepted source behavior is represented, including state boundaries,
   assignment priority, and reset behavior; and
6. no unsupported or ambiguous construct has been discarded.

IR validation failures are distinct from parse failures and elaboration failures.
Source locations should be retained on signals, expressions, drivers, and FFDs
so diagnostics can identify the responsible RTL.

## 6. Separation from proof modeling

The circuit IR represents hardware structure and RTL-derived initialization.
It does not contain assertion monitors, proof assumptions, selected proof
clocks, default-zero policy decisions, or solver terms. Those are added by
subsequent property-model and proof-model stages. An IR consumer must preserve
the circuit's input/state distinction and FFD transition behavior when building
the proof model.

## 7. Schematic visualization

A schematic view of an elaborated circuit should be based on this IR and show
its ports, signals, combinational operations, and sequential state boundaries
at the IR level. It is not a gate-level netlist: operators should not be
decomposed into gates, and the view must not imply synthesis or optimization
that the IR does not represent. The rendering format and interface are not
specified here.

## 8. Open details

The following are intentionally not decided here and must be specified before
the corresponding source constructs can be lowered:

- the supported operator set and exact bit-vector semantics;
- signedness, expression sizing, casts, and unsized literal rules;
- the accepted procedural statements and their lowering to expressions;
- multiple-driver, partial-assignment, latch, and combinational-cycle rules;
- supported RTL initialization forms;
- the precise IR serialization/API; and
- future representations for `inout` ports, memories, arrays, and other
  deferred constructs.

These open details do not prevent implementing the structural data model, but
they do prevent accepting source constructs whose semantics are not yet fixed.
