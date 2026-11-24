#ifndef LEMMA_CIRCUIT_IR_BUILDER_INTERNAL_H
#define LEMMA_CIRCUIT_IR_BUILDER_INTERNAL_H

#include "circuit_ir.h"

//-------------------------------------------------------------------
// Create an empty caller-owned builder.
//
// @returns: Caller-owned empty builder, or NULL if allocation fails.
//-------------------------------------------------------------------
lemma_circuit_builder *lemma_circuit_builder_create(void);

//-------------------------------------------------------
// Destroy a builder and its in-progress circuit storage.
//
// @builder: Builder to destroy, or NULL.
// @returns: None.
//-------------------------------------------------------
void lemma_circuit_builder_destroy(lemma_circuit_builder *builder);

//--------------------------------------------------------------------------------
// Copy a validated nonempty module-name slice into a fresh builder.
//
// @builder: Fresh mutable builder; ownership remains with the caller.
// @name: Non-NULL readable module-name bytes borrowed for the call.
// @name_length: Positive number of readable bytes, not SIZE_MAX.
// @returns: Nonzero on success; zero if the length overflows or allocation fails.
//--------------------------------------------------------------------------------
int lemma_circuit_builder_set_module_name(lemma_circuit_builder *builder,
                                          const char *name, size_t name_length);

//-------------------------------------------------------------------------------
// Append a signal with a copied name and return its array-position ID.
//
// @builder: Valid mutable builder receiving the signal.
// @name: Non-NULL readable bytes of a validated signal identifier.
// @name_length: Positive number of readable bytes, not SIZE_MAX.
// @width: Positive concrete bit width.
// @is_signed: Declared signedness.
// @is_port: Whether this signal is a module port.
// @source: Source range attached to the signal.
// @signal_id: Valid output pointer receiving the new ID only on success.
// @returns: Nonzero on success; zero if the name length overflows or
//     reservation/name allocation fails.
//-------------------------------------------------------------------------------
int lemma_circuit_builder_add_signal(lemma_circuit_builder *builder,
                                      const char *name, size_t name_length,
                                      uint32_t width, bool is_signed, bool is_port,
                                      lemma_source_range source,
                                      size_t *signal_id);

//-------------------------------------------------------------------------
// Append an ordered port referring to an existing signal.
//
// @builder: Valid mutable builder receiving the port.
// @signal_id: ID returned by a successful add_signal call on this builder.
// @direction: Validated input or output direction.
// @returns: Nonzero on success; zero if reservation fails.
//-------------------------------------------------------------------------
int lemma_circuit_builder_add_port(lemma_circuit_builder *builder,
                                   size_t signal_id,
                                   lemma_ir_port_direction direction);

//--------------------------------------------------------------------------------
// Append a read-signal expression for an existing signal.
//
// @builder: Valid mutable builder receiving the expression.
// @signal_id: Existing signal ID returned by a successful lookup on this builder.
// @source: Expression source range.
// @expression_id: Valid output pointer receiving the new ID only on success.
// @returns: Nonzero on success; zero if reservation fails.
//--------------------------------------------------------------------------------
int lemma_circuit_builder_add_read_signal(lemma_circuit_builder *builder,
                                          size_t signal_id,
                                          lemma_source_range source,
                                          size_t *expression_id);

//----------------------------------------------------------------------------
// Append a one-bit unsigned logical-negation expression.
//
// @builder: Valid mutable builder receiving the expression.
// @operand_id: Valid existing operand expression ID.
// @source: Expression source range.
// @expression_id: Valid output pointer receiving the new ID only on success.
// @returns: Nonzero on success; zero if reservation fails.
//----------------------------------------------------------------------------
int lemma_circuit_builder_add_logical_not(lemma_circuit_builder *builder,
                                          size_t operand_id,
                                          lemma_source_range source,
                                          size_t *expression_id);

//----------------------------------------------------------------------------
// Append a one-bit unsigned logical AND or OR expression.
//
// @builder: Valid mutable builder receiving the expression.
// @kind: Validated logical AND or logical OR expression kind.
// @left_operand_id: Valid existing left operand expression ID.
// @right_operand_id: Valid existing right operand expression ID.
// @source: Expression source range.
// @expression_id: Valid output pointer receiving the new ID only on success.
// @returns: Nonzero on success; zero if reservation fails.
//----------------------------------------------------------------------------
int lemma_circuit_builder_add_logical_binary(
    lemma_circuit_builder *builder, lemma_ir_expression_kind kind,
    size_t left_operand_id, size_t right_operand_id,
    lemma_source_range source, size_t *expression_id);

//--------------------------------------------------------------------------------
// Append a typed bitwise, reduction, arithmetic, or comparison operator.
//
// @builder: Non-NULL mutable builder receiving the expression.
// @kind: Supported non-logical bit-vector operator kind, validated by the caller.
// @operand_ids: Non-NULL array of existing expression IDs in source order,
//     validated by the caller.
// @operand_count: Validated arity for kind, either one or two.
// @source: Expression source range.
// @expression_id: Non-NULL output receiving the new ID only on success.
// @returns: Nonzero on success; zero for allocation failure. The caller must
//     provide non-NULL pointers, valid IDs, unsigned
//     operands, and matching widths for binary operands.
//--------------------------------------------------------------------------------
int lemma_circuit_builder_add_operator(
    lemma_circuit_builder *builder, lemma_ir_expression_kind kind,
    const size_t operand_ids[2], size_t operand_count,
    lemma_source_range source, size_t *expression_id);

//------------------------------------------------------------------------
// Append a combinational driver for existing signal and expression IDs.
//
// @builder: Valid mutable builder receiving the driver.
// @target_signal_id: Valid existing target signal ID.
// @expression_id: Valid existing expression ID.
// @source: Assignment source range.
// @returns: Nonzero on success; zero if reservation fails.
//------------------------------------------------------------------------
int lemma_circuit_builder_add_driver(lemma_circuit_builder *builder,
                                     size_t target_signal_id,
                                     size_t expression_id,
                                     lemma_source_range source);

//----------------------------------------------------------------------------
// Look up an existing signal by exact source-name bytes without copying them.
//
// @builder: Valid builder containing signals.
// @name: Readable bytes of a nonempty, validated identifier.
// @name_length: Positive number of identifier bytes to match.
// @signal_id: Valid output pointer, written only when a match exists.
// @returns: Nonzero when found; zero when no signal matches.
//----------------------------------------------------------------------------
int lemma_circuit_builder_find_signal(const lemma_circuit_builder *builder,
                                      const char *name, size_t name_length,
                                      size_t *signal_id);

//------------------------------------------------------------------
// Return the number of signals currently stored in the builder.
//
// @builder: Valid non-NULL builder whose signal table is inspected.
// @returns: Number of stored signals.
//------------------------------------------------------------------
size_t lemma_circuit_builder_signal_count(const lemma_circuit_builder *builder);

//----------------------------------------------------------------------------
// Read a signal from the builder's final IR storage.
//
// @builder: Valid non-NULL builder; signal_id must be in range.
// @signal_id: In-range signal array-position ID.
// @returns: Borrowed signal pointer; may be invalidated when the signal array
//     grows and is invalid after builder destruction. No NULL-on-out-of-range
//     guarantee.
//----------------------------------------------------------------------------
const lemma_ir_signal *lemma_circuit_builder_signal_at(
    const lemma_circuit_builder *builder, size_t signal_id);

//--------------------------------------------------------------------
// Return the number of ordered ports currently stored in the builder.
//
// @builder: Valid non-NULL builder whose port table is inspected.
// @returns: Number of stored ports.
//--------------------------------------------------------------------
size_t lemma_circuit_builder_port_count(const lemma_circuit_builder *builder);

//------------------------------------------------------------------------------
// Read an ordered port from the builder's final IR storage.
//
// @builder: Valid non-NULL builder; port_index must be in range.
// @port_index: In-range port array index.
// @returns: Borrowed port pointer; may be invalidated when the port array grows
//     and is invalid after builder destruction. No NULL-on-out-of-range
//     guarantee.
//------------------------------------------------------------------------------
const lemma_ir_port *lemma_circuit_builder_port_at(
    const lemma_circuit_builder *builder, size_t port_index);

//------------------------------------------------------------------
// Return the number of drivers currently stored in the builder.
//
// @builder: Valid non-NULL builder whose driver table is inspected.
// @returns: Number of stored drivers.
//------------------------------------------------------------------
size_t lemma_circuit_builder_driver_count(const lemma_circuit_builder *builder);

//----------------------------------------------------------------------
// Return the number of expressions currently stored in the builder.
//
// @builder: Valid non-NULL builder whose expression table is inspected.
// @returns: Number of stored expressions.
//----------------------------------------------------------------------
size_t lemma_circuit_builder_expression_count(
    const lemma_circuit_builder *builder);

//----------------------------------------------------------------------------
// Read a driver from the builder's final IR storage.
//
// @builder: Valid non-NULL builder; driver_index must be in range.
// @driver_index: In-range driver array index.
// @returns: Borrowed driver pointer; may be invalidated when the driver array
//     grows and is invalid after builder destruction. No NULL-on-out-of-range
//     guarantee.
//----------------------------------------------------------------------------
const lemma_ir_driver *lemma_circuit_builder_driver_at(
    const lemma_circuit_builder *builder, size_t driver_index);

//------------------------------------------------------------------------------
// Read an expression from the builder's final IR storage.
//
// @builder: Valid non-NULL builder; expression_id must be in range.
// @expression_id: In-range expression array-position ID.
// @returns: Borrowed expression pointer; may be invalidated when the expression
//     array grows and is invalid after builder destruction. No
//     NULL-on-out-of-range guarantee.
//------------------------------------------------------------------------------
const lemma_ir_expression *lemma_circuit_builder_expression_at(
    const lemma_circuit_builder *builder, size_t expression_id);

//--------------------------------------------------------------------------------
// Finalize populated builder storage into an owned circuit, consuming on success.
//
// @builder: Valid populated builder after successful elaboration; consumed only
//     when finalization succeeds.
// @returns: Caller-owned circuit on success; NULL on allocation failure, leaving
//     the builder caller-owned for destruction.
//--------------------------------------------------------------------------------
lemma_circuit *lemma_circuit_builder_finalize(lemma_circuit_builder *builder);

#endif
