# RTL Fixture Sources

These small, standalone SystemVerilog modules are the starting source corpus
for developing Lemma's combinational RTL frontend and equation-based IR. Each
fixture focuses on a small piece of the documented subset.

## Current coverage

- **Assignments and processes:** `continuous_assign_alias.sv`,
  `continuous_assign_boolean.sv`, `combinational_if_else_mux.sv`,
  `combinational_if_else_boolean.sv`, `combinational_always_comb_mux.sv`,
  `combinational_assignment_order.sv`,
  `combinational_default_then_override.sv`,
  `combinational_vector_if_condition.sv`, and `combinational_case_mux.sv`.
- **Composition and fan-out:** `combinational_composed_netlist.sv`.
- **Widths, signedness, and assignment conversion:**
  `vector_width_signedness_alias.sv`, `signed_to_unsigned_casts.sv`,
  `unsigned_to_signed_casts.sv`, `signed_sign_extend.sv`,
  `unsigned_zero_extend.sv`, `signed_truncate.sv`, and
  `unsigned_truncate.sv`.
- **Vector operators:** `vector_bitwise_operators.sv`,
  `vector_reduction_operators.sv`, `vector_logical_operators.sv`,
  `vector_unsigned_add.sv`, `vector_unsigned_subtract.sv`,
  `vector_unsigned_multiply.sv`, and `vector_unsigned_comparisons.sv`.
- **Packed construction and selection:** `vector_concatenation.sv`,
  `vector_constant_selects.sv`, and `vector_variable_selects.sv`.

See [`doc/rtl-support.md`](../../../doc/rtl-support.md) for the current subset
and its remaining TBD areas. In particular, variable selections must be
provably in range; `-:` indexed part-selects, replication, and general
mixed-width or signed operator sizing are not yet specified.

## Test status

All current fixtures pass the Tree-sitter syntax parser. This only confirms that
the grammar accepts the source; Lemma does not yet elaborate these files or
build an IR. The fixtures are not registered as automated semantic tests and
do not yet have machine-checked expected equations or IR structures.

Incomplete assignments, inferred latches, and other misconception/rejection
cases are planned for a separate suite. Their behavior and diagnostics still
need specification before those cases become normative MVP tests.
