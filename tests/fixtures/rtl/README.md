# RTL Fixture Sources

These small, standalone SystemVerilog modules are the starting source corpus
for developing Lemma's RTL frontend and equation-based IR. Each fixture focuses
on a small piece of the documented subset.

The corpus contains 27 combinational `comb_*.sv` fixtures and 27 corresponding
sequential `seq_*.sv` fixtures. Each sequential fixture registers the original
module outputs with asynchronous reset; the three representative fixtures
`seq_continuous_assign_alias.sv`, `seq_if_else_mux.sv`, and
`seq_composed_netlist.sv` use active-high reset, and the other 24 use active-low
reset.

## Current coverage

- **Assignments and processes:** `comb_continuous_assign_alias.sv`,
  `comb_continuous_assign_boolean.sv`, `comb_if_else_mux.sv`,
  `comb_if_else_boolean.sv`, `comb_always_comb_mux.sv`,
  `comb_assignment_order.sv`,
  `comb_default_then_override.sv`,
  `comb_vector_if_condition.sv`, and `comb_case_mux.sv`.
- **Composition and fan-out:** `comb_composed_netlist.sv`.
- **Widths, signedness, and assignment conversion:**
  `comb_vector_width_signedness_alias.sv`, `comb_signed_to_unsigned_casts.sv`,
  `comb_unsigned_to_signed_casts.sv`, `comb_signed_sign_extend.sv`,
  `comb_unsigned_zero_extend.sv`, `comb_signed_truncate.sv`, and
  `comb_unsigned_truncate.sv`.
- **Vector operators:** `comb_vector_bitwise_operators.sv`,
  `comb_vector_reduction_operators.sv`, `comb_vector_logical_operators.sv`,
  `comb_vector_unsigned_add.sv`, `comb_vector_unsigned_subtract.sv`,
  `comb_vector_unsigned_multiply.sv`, and `comb_vector_unsigned_comparisons.sv`.
- **Packed construction and selection:** `comb_vector_concatenation.sv`,
  `comb_vector_constant_selects.sv`, and `comb_vector_variable_selects.sv`.

See [`doc/rtl-support.md`](../../../doc/rtl-support.md) for the current subset
and its remaining TBD areas. In particular, variable selections must be
provably in range; `-:` indexed part-selects, replication, and general
mixed-width or signed operator sizing are not yet specified.

## Test status

All 54 current fixtures pass the Tree-sitter syntax parser. This only confirms
that the grammar accepts the source; Lemma does not yet elaborate these files
or build an IR. The fixtures are not registered as automated semantic tests
and do not yet have machine-checked expected equations or IR structures. The
first expected-result/SVA case is `seq_continuous_assign_alias.sv`:

- Expected sequential structure: FFD with Q=`y`, D=`a`, clocked on posedge
  `clk`, with active-high asynchronous reset `areset` to zero.
- Inline assertion's intended future proof outcome: `proven`. This outcome is
  recorded as an expectation only; Lemma does not semantically check the
  assertion or prove it yet.
- Inline assertion label: `target_continuous_assign_alias`.

Incomplete assignments, inferred latches, and other misconception/rejection
cases are planned for a separate suite. Their behavior and diagnostics still
need specification before those cases become normative MVP tests.
