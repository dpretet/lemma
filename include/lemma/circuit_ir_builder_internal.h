#ifndef LEMMA_CIRCUIT_IR_BUILDER_INTERNAL_H
#define LEMMA_CIRCUIT_IR_BUILDER_INTERNAL_H

#include "circuit_ir.h"

//--------------------------------------------------------
// Create an empty caller-owned builder.
//
// @returns: Caller-owned empty builder, or NULL if allocation fails.
//--------------------------------------------------------
lemma_circuit_builder *lemma_circuit_builder_create(void);

//--------------------------------------------------------
// Destroy a builder and its in-progress circuit storage.
//
// @builder: Builder to destroy, or NULL.
// @returns: None.
//--------------------------------------------------------
void lemma_circuit_builder_destroy(lemma_circuit_builder *builder);

//--------------------------------------------------------
// Set the module name by copying a borrowed non-NUL-terminated source slice.
//
// @builder: Mutable builder that owns the resulting copy.
// @name: Non-NULL module-name bytes borrowed for the call.
// @name_length: Number of readable module-name bytes.
// @returns: Nonzero on success; zero for invalid input or allocation failure.
//--------------------------------------------------------
int lemma_circuit_builder_set_module_name(lemma_circuit_builder *builder,
                                          const char *name, size_t name_length);

//--------------------------------------------------------
// Append a signal with a copied name and return its array-position ID.
//
// @builder: Mutable builder receiving the signal.
// @name: Non-NULL signal-name bytes borrowed for the call.
// @name_length: Number of readable signal-name bytes.
// @width: Positive concrete bit width.
// @is_signed: Declared signedness.
// @is_port: Whether this signal is a module port.
// @source: Source range attached to the signal.
// @signal_id: Receives the new ID only on success.
// @returns: Nonzero on success; zero for invalid input, limit, or allocation failure.
//--------------------------------------------------------
int lemma_circuit_builder_add_signal(lemma_circuit_builder *builder,
                                     const char *name, size_t name_length,
                                     uint32_t width, bool is_signed, bool is_port,
                                     lemma_source_range source,
                                     uint32_t *signal_id);

//--------------------------------------------------------
// Append an ordered port referring to an existing signal.
//
// @builder: Mutable builder receiving the port.
// @signal_id: Existing signal array-position ID.
// @direction: Input or output direction.
// @returns: Nonzero on success; zero for invalid input, limit, or allocation failure.
//--------------------------------------------------------
int lemma_circuit_builder_add_port(lemma_circuit_builder *builder,
                                   uint32_t signal_id,
                                   lemma_ir_port_direction direction);

//--------------------------------------------------------
// Append a read-signal expression for an existing signal.
//
// @builder: Mutable builder receiving the expression.
// @signal_id: Existing signal array-position ID.
// @source: Expression source range.
// @expression_id: Receives the new ID only on success.
// @returns: Nonzero on success; zero for invalid input, limit, or allocation failure.
//--------------------------------------------------------
int lemma_circuit_builder_add_read_signal(lemma_circuit_builder *builder,
                                          uint32_t signal_id,
                                          lemma_source_range source,
                                          uint32_t *expression_id);

//--------------------------------------------------------
// Append a combinational driver for existing signal and expression IDs.
//
// @builder: Mutable builder receiving the driver.
// @target_signal_id: Existing target signal ID.
// @expression_id: Existing expression ID.
// @source: Assignment source range.
// @returns: Nonzero on success; zero for invalid input, limit, or allocation failure.
//--------------------------------------------------------
int lemma_circuit_builder_add_driver(lemma_circuit_builder *builder,
                                     uint32_t target_signal_id,
                                     uint32_t expression_id,
                                     lemma_source_range source);

//--------------------------------------------------------
// Look up an existing signal by exact source-name bytes without copying them.
//
// @builder: Builder containing signals.
// @name: Non-NULL source-name bytes to match.
// @name_length: Number of name bytes to match.
// @signal_id: Receives the matching ID only when found.
// @returns: Nonzero when found; zero for invalid input or no matching signal.
//--------------------------------------------------------
int lemma_circuit_builder_find_signal(const lemma_circuit_builder *builder,
                                      const char *name, size_t name_length,
                                      uint32_t *signal_id);

//--------------------------------------------------------
// Return the number of signals currently stored in the builder.
//
// @builder: Builder whose final IR signal table is inspected.
// @returns: Number of stored signals, or zero for a null builder.
//--------------------------------------------------------
size_t lemma_circuit_builder_signal_count(const lemma_circuit_builder *builder);

//--------------------------------------------------------
// Read a signal from the builder's final IR storage.
//
// @builder: Builder containing the signal.
// @signal_id: Signal array-position ID.
// @returns: Borrowed signal pointer, or NULL for invalid input or ID.
//--------------------------------------------------------
const lemma_ir_signal *lemma_circuit_builder_signal_at(
    const lemma_circuit_builder *builder, uint32_t signal_id);

//--------------------------------------------------------
// Return the number of ordered ports currently stored in the builder.
//
// @builder: Builder whose final IR port table is inspected.
// @returns: Number of stored ports, or zero for a null builder.
//--------------------------------------------------------
size_t lemma_circuit_builder_port_count(const lemma_circuit_builder *builder);

//--------------------------------------------------------
// Read an ordered port from the builder's final IR storage.
//
// @builder: Builder containing the port.
// @port_index: Port array index.
// @returns: Borrowed port pointer, or NULL for invalid input or index.
//--------------------------------------------------------
const lemma_ir_port *lemma_circuit_builder_port_at(
    const lemma_circuit_builder *builder, size_t port_index);

//--------------------------------------------------------
// Return the number of drivers currently stored in the builder.
//
// @builder: Builder whose final IR driver table is inspected.
// @returns: Number of stored drivers, or zero for a null builder.
//--------------------------------------------------------
size_t lemma_circuit_builder_driver_count(const lemma_circuit_builder *builder);

//--------------------------------------------------------
// Read a driver from the builder's final IR storage.
//
// @builder: Builder containing the driver.
// @driver_index: Driver array index.
// @returns: Borrowed driver pointer, or NULL for invalid input or index.
//--------------------------------------------------------
const lemma_ir_driver *lemma_circuit_builder_driver_at(
    const lemma_circuit_builder *builder, size_t driver_index);

//--------------------------------------------------------
// Read an expression from the builder's final IR storage.
//
// @builder: Builder containing the expression.
// @expression_id: Expression array-position ID.
// @returns: Borrowed expression pointer, or NULL for invalid input or ID.
//--------------------------------------------------------
const lemma_ir_expression *lemma_circuit_builder_expression_at(
    const lemma_circuit_builder *builder, uint32_t expression_id);

//--------------------------------------------------------
// Finalize populated builder storage into an owned circuit, consuming on success.
//
// @builder: Caller-owned builder.
// @returns: Caller-owned circuit on success; NULL on allocation failure, leaving
//     the builder caller-owned for destruction.
//--------------------------------------------------------
lemma_circuit *lemma_circuit_builder_finalize(lemma_circuit_builder *builder);

#endif
