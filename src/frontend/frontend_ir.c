#include "lemma/frontend_ir_builder.h"

#include <stdlib.h>
#include <string.h>

struct lemma_circuit_builder {
    lemma_circuit storage;
    size_t port_capacity;
    size_t signal_capacity;
    size_t expression_capacity;
    size_t driver_capacity;
};

//-------------------------------------------------------------------
// Release all storage owned by a circuit without freeing its struct.
//
// @circuit: Non-NULL circuit whose owned allocations are released.
// @returns: None.
//-------------------------------------------------------------------
static void lemma_circuit_release_storage(lemma_circuit *circuit)
{
    free(circuit->module_name);
    for (size_t index = 0U; index < circuit->signal_count; ++index) {
        free(circuit->signals[index].name);
    }
    free(circuit->ports);
    free(circuit->signals);
    free(circuit->expressions);
    free(circuit->drivers);
    free(circuit->ffds);
}

//---------------------------------------------------------------------
// Create an empty caller-owned circuit builder.
//
// @returns: Caller-owned empty builder, or NULL when allocation fails.
//---------------------------------------------------------------------
lemma_circuit_builder *lemma_circuit_builder_create(void)
{
    return calloc(1U, sizeof(lemma_circuit_builder));
}

//----------------------------------------------------------------------
// Destroy a builder and every partially constructed circuit allocation.
//
// @builder: Builder to destroy, or NULL.
// @returns: None.
//----------------------------------------------------------------------
void lemma_circuit_builder_destroy(lemma_circuit_builder *builder)
{
    if (builder == NULL) {
        return;
    }
    lemma_circuit_release_storage(&builder->storage);
    free(builder);
}

//-----------------------------------------------
// Destroy a circuit and all storage owned by it.
//
// @circuit: Circuit to destroy, or NULL.
// @returns: None.
//-----------------------------------------------
void lemma_circuit_destroy(lemma_circuit *circuit)
{
    if (circuit == NULL) {
        return;
    }
    lemma_circuit_release_storage(circuit);
    free(circuit);
}

//------------------------------------------------------------------------------------
// Grow an array using the builder's initial and doubling capacity policy.
//
// @array: Existing allocation, left owned by the caller on failure.
// @capacity: Current element capacity.
// @element_size: Nonzero size of one array element.
// @next_capacity: Valid output pointer, written only when reallocation succeeds.
// @returns: Reallocated array on success; NULL on overflow or allocation failure.
//------------------------------------------------------------------------------------
static void *lemma_circuit_grow_array(void *array, size_t capacity,
                                      size_t element_size,
                                      size_t *next_capacity)
{
    if (element_size == 0U) {
        return NULL;
    }

    size_t grown_capacity = capacity;
    if (grown_capacity == 0U) {
        grown_capacity = 4U;
    } else {
        if (grown_capacity > SIZE_MAX / 2U) {
            return NULL;
        }
        grown_capacity *= 2U;
    }
    if (grown_capacity > SIZE_MAX / element_size) {
        return NULL;
    }

    void *grown_array = realloc(array, grown_capacity * element_size);
    if (grown_array == NULL) {
        return NULL;
    }
    *next_capacity = grown_capacity;
    return grown_array;
}

//--------------------------------------------------------------------------------------------------------------------
// Copy a validated byte slice into a fresh owned, NUL-terminated string.
//
// @name: Non-NULL readable bytes borrowed for the call.
// @name_length: Number of readable bytes, not SIZE_MAX.
// @returns: Owned string copy, or NULL when the length overflows or allocation fails.
//--------------------------------------------------------------------------------------------------------------------
static char *lemma_circuit_copy_name(const char *name, size_t name_length)
{
    if (name_length == SIZE_MAX) {
        return NULL;
    }
    char *copy = malloc(name_length + 1U);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, name, name_length);
    copy[name_length] = '\0';
    return copy;
}

//--------------------------------------------------------------------------------
// Copy a validated nonempty module-name slice into a fresh builder.
//
// @builder: Fresh mutable builder.
// @name: Non-NULL readable module-name bytes borrowed for the call.
// @name_length: Positive number of readable bytes, not SIZE_MAX.
// @returns: Nonzero on success; zero if the length overflows or allocation fails.
//--------------------------------------------------------------------------------
int lemma_circuit_builder_set_module_name(lemma_circuit_builder *builder,
                                          const char *name, size_t name_length)
{
    char *copy = lemma_circuit_copy_name(name, name_length);
    if (copy == NULL) {
        return 0;
    }
    builder->storage.module_name = copy;
    return 1;
}

//---------------------------------------------------------------------------
// Append one signal, copying its name and returning its array-position ID.
//
// @builder: Valid mutable builder.
// @name: Non-NULL readable bytes of a validated signal identifier.
// @name_length: Positive number of readable bytes, not SIZE_MAX.
// @width: Positive signal width.
// @is_signed: Declared signedness.
// @is_port: Whether the signal is a module port.
// @source: Signal declaration range.
// @signal_id: Valid output pointer for the new ID.
// @returns: Nonzero on success; zero if the name length overflows or
//     reservation/name allocation fails.
//---------------------------------------------------------------------------
int lemma_circuit_builder_add_signal(lemma_circuit_builder *builder,
                                      const char *name, size_t name_length,
                                      uint32_t width, bool is_signed, bool is_port,
                                      lemma_source_range source,
                                      size_t *signal_id)
{
    if (name_length == SIZE_MAX) {
        return 0;
    }
    if (builder->storage.signal_count == builder->signal_capacity) {
        size_t next_capacity;
        lemma_ir_signal *grown_signals = lemma_circuit_grow_array(
            builder->storage.signals,
            builder->signal_capacity,
            sizeof(*builder->storage.signals),
            &next_capacity
        );
        if (grown_signals == NULL) {
            return 0;
        }
        builder->storage.signals = grown_signals;
        builder->signal_capacity = next_capacity;
    }
    char *copy = lemma_circuit_copy_name(name, name_length);
    if (copy == NULL) {
        return 0;
    }
    size_t index = builder->storage.signal_count++;
    builder->storage.signals[index] = (lemma_ir_signal){
        copy, width, is_signed, is_port, source
    };
    *signal_id = index;
    return 1;
}

//-------------------------------------------------------------------------
// Append an ordered port record referring to a known signal.
//
// @builder: Valid mutable builder.
// @signal_id: ID returned by a successful add_signal call on this builder.
// @direction: Validated input or output direction.
// @returns: Nonzero on success; zero if reservation fails.
//-------------------------------------------------------------------------
int lemma_circuit_builder_add_port(lemma_circuit_builder *builder,
                                   size_t signal_id,
                                   lemma_ir_port_direction direction)
{
    if (builder->storage.port_count == builder->port_capacity) {
        size_t next_capacity;
        lemma_ir_port *grown_ports = lemma_circuit_grow_array(
            builder->storage.ports,
            builder->port_capacity,
            sizeof(*builder->storage.ports),
            &next_capacity
        );
        if (grown_ports == NULL) {
            return 0;
        }
        builder->storage.ports = grown_ports;
        builder->port_capacity = next_capacity;
    }
    builder->storage.ports[builder->storage.port_count++] = (lemma_ir_port){
        signal_id, direction
    };
    return 1;
}

//--------------------------------------------------------------------------------
// Append a read-signal expression with type copied from its signal.
//
// @builder: Valid mutable builder.
// @signal_id: Existing signal ID returned by a successful lookup on this builder.
// @source: Expression source range.
// @expression_id: Valid output pointer for the new ID.
// @returns: Nonzero on success; zero if reservation fails.
//--------------------------------------------------------------------------------
int lemma_circuit_builder_add_read_signal(lemma_circuit_builder *builder,
                                          size_t signal_id,
                                          lemma_source_range source,
                                          size_t *expression_id)
{
    if (builder->storage.expression_count == builder->expression_capacity) {
        size_t next_capacity;
        lemma_ir_expression *grown_expressions = lemma_circuit_grow_array(
            builder->storage.expressions,
            builder->expression_capacity,
            sizeof(*builder->storage.expressions),
            &next_capacity
        );
        if (grown_expressions == NULL) {
            return 0;
        }
        builder->storage.expressions = grown_expressions;
        builder->expression_capacity = next_capacity;
    }
    lemma_ir_signal *signal = &builder->storage.signals[signal_id];
    size_t index = builder->storage.expression_count++;
    builder->storage.expressions[index] = (lemma_ir_expression){
        .kind = LEMMA_IR_EXPRESSION_READ_SIGNAL,
        .signal_id = signal_id,
        .width = signal->width,
        .is_signed = signal->is_signed,
        .source = source
    };
    *expression_id = index;
    return 1;
}

//-----------------------------------------------------------------------------
// Append a one-bit unsigned logical-negation expression.
//
// @builder: Valid mutable builder receiving the expression.
// @operand_id: Valid existing operand expression ID.
// @source: Expression source range.
// @expression_id: Valid output pointer for the new ID.
// @returns: Nonzero on success; zero if reservation fails.
//-----------------------------------------------------------------------------
int lemma_circuit_builder_add_logical_not(lemma_circuit_builder *builder,
                                          size_t operand_id,
                                          lemma_source_range source,
                                          size_t *expression_id)
{
    if (builder->storage.expression_count == builder->expression_capacity) {
        size_t next_capacity;
        lemma_ir_expression *grown_expressions = lemma_circuit_grow_array(
            builder->storage.expressions,
            builder->expression_capacity,
            sizeof(*builder->storage.expressions),
            &next_capacity
        );
        if (grown_expressions == NULL) {
            return 0;
        }
        builder->storage.expressions = grown_expressions;
        builder->expression_capacity = next_capacity;
    }
    size_t index = builder->storage.expression_count++;
    builder->storage.expressions[index] = (lemma_ir_expression){
        .kind = LEMMA_IR_EXPRESSION_LOGICAL_NOT,
        .operand_ids = {operand_id, 0U},
        .width = 1U,
        .is_signed = false,
        .source = source
    };
    *expression_id = index;
    return 1;
}

//-----------------------------------------------------------------------------
// Append a one-bit unsigned logical AND or OR expression.
//
// @builder: Valid mutable builder receiving the expression.
// @kind: Validated logical AND or logical OR expression kind.
// @left_operand_id: Valid existing left operand expression ID.
// @right_operand_id: Valid existing right operand expression ID.
// @source: Expression source range.
// @expression_id: Valid output pointer for the new ID.
// @returns: Nonzero on success; zero if reservation fails.
//-----------------------------------------------------------------------------
int lemma_circuit_builder_add_logical_binary(
    lemma_circuit_builder *builder, lemma_ir_expression_kind kind,
    size_t left_operand_id, size_t right_operand_id,
    lemma_source_range source, size_t *expression_id)
{
    if (builder->storage.expression_count == builder->expression_capacity) {
        size_t next_capacity;
        lemma_ir_expression *grown_expressions = lemma_circuit_grow_array(
            builder->storage.expressions,
            builder->expression_capacity,
            sizeof(*builder->storage.expressions),
            &next_capacity
        );
        if (grown_expressions == NULL) {
            return 0;
        }
        builder->storage.expressions = grown_expressions;
        builder->expression_capacity = next_capacity;
    }
    size_t index = builder->storage.expression_count++;
    builder->storage.expressions[index] = (lemma_ir_expression){
        .kind = kind,
        .operand_ids = {left_operand_id, right_operand_id},
        .width = 1U,
        .is_signed = false,
        .source = source
    };
    *expression_id = index;
    return 1;
}

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
    lemma_source_range source, size_t *expression_id)
{
    const lemma_ir_expression *left =
        &builder->storage.expressions[operand_ids[0]];

    uint32_t result_width = left->width;
    if (kind >= LEMMA_IR_EXPRESSION_REDUCTION_AND &&
        kind <= LEMMA_IR_EXPRESSION_REDUCTION_XNOR) {
        result_width = 1U;
    } else if (kind >= LEMMA_IR_EXPRESSION_EQUAL &&
               kind <= LEMMA_IR_EXPRESSION_GREATER_EQUAL) {
        result_width = 1U;
    }
    if (builder->storage.expression_count == builder->expression_capacity) {
        size_t next_capacity;
        lemma_ir_expression *grown_expressions = lemma_circuit_grow_array(
            builder->storage.expressions,
            builder->expression_capacity,
            sizeof(*builder->storage.expressions),
            &next_capacity
        );
        if (grown_expressions == NULL) {
            return 0;
        }
        builder->storage.expressions = grown_expressions;
        builder->expression_capacity = next_capacity;
    }
    size_t index = builder->storage.expression_count++;
    builder->storage.expressions[index] = (lemma_ir_expression){
        .kind = kind,
        .operand_ids = {operand_ids[0],
                        operand_count == 2U ? operand_ids[1] : 0U},
        .width = result_width,
        .is_signed = false,
        .source = source
    };
    *expression_id = index;
    return 1;
}

//------------------------------------------------------------------------------
// Append a combinational driver referring to an existing signal and expression.
//
// @builder: Valid mutable builder.
// @target_signal_id: Valid existing target signal ID.
// @expression_id: Valid existing expression ID.
// @source: Assignment source range.
// @returns: Nonzero on success; zero if reservation fails.
//------------------------------------------------------------------------------
int lemma_circuit_builder_add_driver(lemma_circuit_builder *builder,
                                     size_t target_signal_id,
                                     size_t expression_id,
                                     lemma_source_range source)
{
    if (builder->storage.driver_count == builder->driver_capacity) {
        size_t next_capacity;
        lemma_ir_driver *grown_drivers = lemma_circuit_grow_array(
            builder->storage.drivers,
            builder->driver_capacity,
            sizeof(*builder->storage.drivers),
            &next_capacity
        );
        if (grown_drivers == NULL) {
            return 0;
        }
        builder->storage.drivers = grown_drivers;
        builder->driver_capacity = next_capacity;
    }
    builder->storage.drivers[builder->storage.driver_count++] = (lemma_ir_driver){
        target_signal_id, expression_id, source
    };
    return 1;
}

//-------------------------------------------------------------------------
// Find an existing signal by exact source-name bytes without copying them.
//
// @builder: Valid builder containing the signal table.
// @name: Readable bytes of a nonempty, validated identifier.
// @name_length: Positive number of identifier bytes to match.
// @signal_id: Valid output pointer, written only when a match exists.
// @returns: Nonzero when found; zero when no signal matches.
//-------------------------------------------------------------------------
int lemma_circuit_builder_find_signal(const lemma_circuit_builder *builder,
                                      const char *name, size_t name_length,
                                      size_t *signal_id)
{
    for (size_t index = 0U; index < builder->storage.signal_count; ++index) {
        const char *stored = builder->storage.signals[index].name;
        size_t stored_length = strlen(stored);
        if (stored_length == name_length &&
            memcmp(stored, name, name_length) == 0) {
            *signal_id = index;
            return 1;
        }
    }
    return 0;
}

//------------------------------------------------------------------
// Return the number of signals currently stored in the builder.
//
// @builder: Valid non-NULL builder whose signal table is inspected.
// @returns: Number of stored signals.
//------------------------------------------------------------------
size_t lemma_circuit_builder_signal_count(const lemma_circuit_builder *builder)
{
    return builder->storage.signal_count;
}

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
    const lemma_circuit_builder *builder, size_t signal_id)
{
    return &builder->storage.signals[signal_id];
}

//--------------------------------------------------------------------
// Return the number of ordered ports currently stored in the builder.
//
// @builder: Valid non-NULL builder whose port table is inspected.
// @returns: Number of stored ports.
//--------------------------------------------------------------------
size_t lemma_circuit_builder_port_count(const lemma_circuit_builder *builder)
{
    return builder->storage.port_count;
}

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
    const lemma_circuit_builder *builder, size_t port_index)
{
    return &builder->storage.ports[port_index];
}

//------------------------------------------------------------------
// Return the number of drivers currently stored in the builder.
//
// @builder: Valid non-NULL builder whose driver table is inspected.
// @returns: Number of stored drivers.
//------------------------------------------------------------------
size_t lemma_circuit_builder_driver_count(const lemma_circuit_builder *builder)
{
    return builder->storage.driver_count;
}

//----------------------------------------------------------------------
// Return the number of expressions currently stored in the builder.
//
// @builder: Valid non-NULL builder whose expression table is inspected.
// @returns: Number of stored expressions.
//----------------------------------------------------------------------
size_t lemma_circuit_builder_expression_count(
    const lemma_circuit_builder *builder)
{
    return builder->storage.expression_count;
}

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
    const lemma_circuit_builder *builder, size_t driver_index)
{
    return &builder->storage.drivers[driver_index];
}

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
    const lemma_circuit_builder *builder, size_t expression_id)
{
    return &builder->storage.expressions[expression_id];
}

//-------------------------------------------------------------------------------
// Transfer the builder's sole circuit storage into a caller-owned circuit.
//
// @builder: Valid populated builder after successful elaboration; consumed only
//     when finalization succeeds.
// @returns: Caller-owned circuit on success; NULL on allocation failure with the
//     builder intact.
//-------------------------------------------------------------------------------
lemma_circuit *lemma_circuit_builder_finalize(lemma_circuit_builder *builder)
{
    lemma_circuit *circuit = malloc(sizeof(*circuit));
    if (circuit == NULL) {
        return NULL;
    }
    *circuit = builder->storage;
    free(builder);
    return circuit;
}
