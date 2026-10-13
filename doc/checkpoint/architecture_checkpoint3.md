# Lemma Architecture Checkpoint 3

**Purpose:** Resume the architecture and specification activity in a future
session. This is a handoff summary, not a replacement for the normative
specifications linked below.

**Checkpoint date:** 2026-10-13

## 1. Project and user context

Lemma is intended to be a free, standalone, open-source formal verification
tool for a documented subset of synthesizable Verilog/SystemVerilog RTL. The
owner is an experienced RTL designer who is learning formal verification and
prefers explanations grounded in small RTL examples. Treat this as a learning
journey: explain formal concepts plainly rather than assuming verification
background.

For a future tutoring session, see
[`formal_verification_tutorial_handoff.txt`](formal_verification_tutorial_handoff.txt).

The implementation language is C, with CMake as the build system. The initial
parser uses the vendored Tree-sitter C runtime and
`tree-sitter-systemverilog` grammar; Unity unit tests run through CTest. The
grammar is only a syntax front end and does not define Lemma's accepted RTL
subset. The `tsoding/arena` library is vendored unchanged for Lemma-owned
allocations, with assertions enabled in all build configurations. The current
parser does not yet use it. The solver dependency and exact implementation
architecture remain unselected. Do not use proprietary tools. Keep parsing,
elaboration, IR construction, and formal solving separate. Do not
silently accept unsupported RTL or SVA.

The repository now includes a small C/CMake parser bootstrap and executable
smoke tests for valid and malformed SystemVerilog syntax. It does not yet
implement RTL-subset validation, elaboration, an IR, or formal solving. The
FRISCV inspection was static source analysis, not a compile or proof run.

## 2. Documentation authority

Use these current documents as the working specifications:

- [`rtl-support.md`](../rtl-support.md) — RTL subset and long-term target;
- [`mvp.md`](../mvp.md) — first milestone boundary and exit criterion;
- [`register-semantics.md`](../register-semantics.md) and
  [`execution-and-assertion-semantics.md`](../execution-and-assertion-semantics.md)
  — state, initialization, clock, and reset behavior;
- [`sva-support.md`](../sva-support.md) — supported assertion syntax and
  semantics;
- [`proof-model.md`](../proof-model.md) and [`tool-flow.md`](../tool-flow.md) —
  proof strategy and observable outcomes;
- [`validation-criteria.md`](../validation-criteria.md) — general validation
  requirements and the MVP acceptance baseline.

This checkpoint summarizes decisions; it does not silently add new supported
constructs. `doc/bkp/` contains temporary historical material and is not a
source of truth. Checkpoints 1 and 2 are historical context; consult the
current specifications when they differ.

## 3. Decisions captured so far

### 3.1 RTL and clock domains

- RTL may contain any number of clock domains.
- Each proof has one target assertion or cover associated with one
  `@(posedge clock)` and may include `assume property` constraints.
- The target and assumptions must have a domain-local dependency cone, apart
  from constants and unconstrained module inputs. A dependency on state updated
  by another clock domain is rejected. Unrelated domains are allowed and are
  excluded from that proof.
- Lemma does not model cross-domain event interleavings, physical clock timing,
  or CDC correctness.
- Properties sample the stabilized pre-edge state. Sequential updates are
  simultaneous; combinational propagation transients are not proof states.

### 3.2 Register initialization and resets

- A register with a supported RTL initialization starts at that value.
- Every other register starts at zero at proof start under Lemma's explicit
  default-zero policy. This is not an RTL reset or a guarantee about physical
  power-up state. Emit a source-located warning for each affected register and
  disclose the policy in proof reports.
- Only an asynchronous reset in a process event control has separate event
  semantics. At most one such reset event control is supported per process;
  its values are constants or resolved parameters, and the asynchronous reset
  branch explicitly assigns every register in the process.
- A condition inside a clocked process body, including a signal named `srst`,
  is ordinary RTL control sampled at the clock edge. Branch order and priority
  follow the source RTL. It is not a separate reset event.
- Inputs are unconstrained unless restricted by explicit assumptions. A reset
  is not assumed to have occurred at startup.
- The proof model is two-state. RTL `X`/`Z` bits are indeterminate and may be
  either `0` or `1` under the documented model.

### 3.3 SVA v1 profile

- Supported target statements: `assert property` and `cover property`; one
  target per run, with zero or more `assume property` constraints.
- Each target and assumption uses the same single `@(posedge clock)` and
  domain-local dependency rules described above.
- Supported temporal subset: Boolean expressions, `##N`, `##[N:M]`, `|->`,
  `|=>`, `$past`, `$rose`, `$fell`, `$stable`, and `disable iff`.
- `|->` starts the consequent at the antecedent's ending sample; `|=>` starts
  it one sample later. Each antecedent match starts an independent obligation.
  Delay bounds are non-negative elaboration-time constants.
- Concurrent assertions sample before sequential updates. `disable iff` is an
  asynchronous disable that aborts active attempts; it does not assume startup
  reset.
- Lemma suppresses a history-dependent property on initial clock samples for
  which prior sampled history is unavailable. This is internal warm-up, not an
  assumption or RTL state. An explicit `past_valid` guard remains supported
  for portability to other tools.
- An implication can pass vacuously. Use a companion cover for reachability;
  report known vacuity. Contradictory assumptions must not yield an
  unqualified `proven` result.
- Cover outcomes are `covered`, `unreachable` only when complete analysis
  establishes it, or `inconclusive`. A bounded search without a witness is not
  proof of unreachability.
- Unbounded temporal operators, sequence repetition, multiclock properties,
  and immediate assertions are outside v1.

### 3.4 Proof strategy

- The selected strategy is **bounded model checking (BMC) plus k-induction**.
- BMC searches for counterexamples up to a selected depth. A found valid trace
  is `failed`; no trace within that depth is not a proof.
- `proven` requires successful induction base and step checks. If induction
  does not complete and no counterexample exists, return `inconclusive`.
- An induction-step failure alone is not a counterexample; it may rely on an
  unreachable state sequence. Larger `k` may be tried within configured
  limits.
- The exact solver, solver interface/encoding, bounds/resource policies, and
  invariant-strengthening strategy remain undecided. Do not select or add a
  dependency without proposing it and getting an explicit decision.

One semantic inconsistency remains to resolve before implementation: the
register-semantics document says an asynchronous reset imposes its value when
assertion coincides with a clock edge, while the execution-and-assertion
document leaves the atomicity/ordering of that coincidence open.

## 4. MVP boundary and acceptance

The MVP handles a single flat top and one target assertion or cover plus its
assumptions. Multiple clocks may exist in the RTL, but the target's dependency
cone must remain within one clock domain. The supported RTL/SVA constructs are
only those explicitly specified; the long-term FRISCV-inspired target is not
automatically part of the MVP.

The MVP acceptance tests are specified in
[`validation-criteria.md`](../validation-criteria.md#10-mvp-acceptance-baseline).
In brief, they must cover accepted/rejected inputs, RTL state and reset
semantics, clock-domain restrictions, SVA timing and history warm-up,
assumptions/vacuity/covers, BMC versus k-induction outcomes, diagnostics, and
valid counterexamples. Tests should use small, hand-checkable designs; a full
FRISCV integration is not required for the first milestone.

## 5. FRISCV as a long-term reference

The owner identified `../../hdl/friscv/rtl` as a representative design. A
static review found about 46 SystemVerilog files / 15,000 lines, including
extensive `logic`, `'0`/`'1`, hierarchy, parameters/generate, `always @(*)`,
`always_comb`, enums, functions, arrays/memories, and preprocessing. It is a
reference for future practical scope and incremental tests, not an MVP gate.

The default platform is largely organized around `aclk`; `rtc` is sampled as
data through a synchronizer. Some optional configurations use a falling-edge
process and will need explicit event semantics if included in a proof profile.
The FRISCV memory arrays and their initialization need a separate memory model;
the register default-zero rule does not define memory contents. Simulation
tracing/logger code should be removed by a documented preprocessing
configuration or rejected, never silently discarded.

## 6. Open decisions and suggested continuation

1. Select an open-source bit-vector solver/backend and its interface. A
   solver-independent transition IR is preferred; do not write a solver from
   scratch.
2. Define the exact supported source configuration/preprocessor behavior and
   the first normative RTL grammar slice. The bundled Tree-sitter grammar does
   not imply support for all syntax it recognizes.
3. Define the transition/property-monitor IR and how it encodes BMC and
   k-induction queries.
4. Specify BMC depth growth, induction limits, timeouts, and the exact
   circumstances that produce `inconclusive`.
5. Turn the MVP acceptance baseline into focused tests before expanding the
   long-term RTL target. Add FRISCV-derived examples only as constructs become
   individually specified and tested.

When resuming, read `AGENTS.md` first, then these current specifications. Discuss
one decision at a time, explain formal concepts for an RTL designer new to the
field, and keep unresolved features explicitly marked rather than inferring
support from the FRISCV reference.
