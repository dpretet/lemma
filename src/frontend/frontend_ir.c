#include "lemma/circuit_ir_builder_internal.h"

#include <stdlib.h>
#include <string.h>

struct lemma_circuit_builder {
    lemma_circuit storage;
    size_t port_capacity;
    size_t signal_capacity;
    size_t expression_capacity;
    size_t driver_capacity;
};

//--------------------------------------------------------
// Grow an owned array to include one additional element.
//
// @array: Address of the array pointer.
// @count: Current array length.
// @capacity: Address of the current capacity.
// @element_size: Size of one element.
// @returns: Nonzero when capacity is available, zero on overflow or allocation failure.
//--------------------------------------------------------
static int reserve_one(void **array, size_t count, size_t *capacity,
                       size_t element_size)
{
    if (count < *capacity) {
        return 1;
    }
    size_t next = *capacity == 0U ? 4U : *capacity * 2U;
    if (next < *capacity || next > SIZE_MAX / element_size) {
        return 0;
    }
    void *grown = realloc(*array, next * element_size);
    if (grown == NULL) {
        return 0;
    }
    *array = grown;
    *capacity = next;
    return 1;
}

//--------------------------------------------------------
// Create an empty caller-owned circuit builder.
//
// @returns: Caller-owned empty builder, or NULL when allocation fails.
//--------------------------------------------------------
lemma_circuit_builder *lemma_circuit_builder_create(void)
{
    return calloc(1U, sizeof(lemma_circuit_builder));
}

//--------------------------------------------------------
// Destroy a builder and every partially constructed circuit allocation.
//
// @builder: Builder to destroy, or NULL.
// @returns: None.
//--------------------------------------------------------
void lemma_circuit_builder_destroy(lemma_circuit_builder *builder)
{
    if (builder == NULL) {
        return;
    }
    lemma_circuit *circuit = &builder->storage;
    free(circuit->module_name);
    for (size_t index = 0U; index < circuit->signal_count; ++index) {
        free(circuit->signals[index].name);
    }
    free(circuit->ports);
    free(circuit->signals);
    free(circuit->expressions);
    free(circuit->drivers);
    free(circuit->ffds);
    free(builder);
}

//--------------------------------------------------------
// Copy a borrowed non-NUL-terminated module-name slice into builder storage.
//
// @builder: Mutable builder.
// @name: Non-NULL module-name bytes.
// @name_length: Number of readable module-name bytes.
// @returns: Nonzero on success, zero for invalid input or allocation failure.
//--------------------------------------------------------
int lemma_circuit_builder_set_module_name(lemma_circuit_builder *builder,
                                          const char *name, size_t name_length)
{
    if (builder == NULL || name == NULL || name_length == 0U ||
        name_length == SIZE_MAX || builder->storage.module_name != NULL) {
        return 0;
    }
    char *copy = malloc(name_length + 1U);
    if (copy == NULL) {
        return 0;
    }
    memcpy(copy, name, name_length);
    copy[name_length] = '\0';
    builder->storage.module_name = copy;
    return 1;
}

//--------------------------------------------------------
// Append one signal, copying its name and returning its array-position ID.
//
// @builder: Mutable builder.
// @name: Non-NULL signal-name bytes borrowed for the call.
// @name_length: Number of readable name bytes.
// @width: Positive signal width.
// @is_signed: Declared signedness.
// @is_port: Whether the signal is a module port.
// @source: Signal declaration span.
// @signal_id: Required output for the new ID.
// @returns: Nonzero on success, zero for invalid input, ID limit, or allocation failure.
//--------------------------------------------------------
int lemma_circuit_builder_add_signal(lemma_circuit_builder *builder,
                                     const char *name, size_t name_length,
                                     uint32_t width, bool is_signed, bool is_port,
                                     lemma_ir_source_span source,
                                     uint32_t *signal_id)
{
    if (builder == NULL || name == NULL || name_length == 0U ||
        name_length == SIZE_MAX || width == 0U || signal_id == NULL ||
        builder->storage.signal_count >= UINT32_MAX ||
        !reserve_one((void **)&builder->storage.signals, builder->storage.signal_count,
                     &builder->signal_capacity, sizeof(*builder->storage.signals))) {
        return 0;
    }
    char *copy = malloc(name_length + 1U);
    if (copy == NULL) {
        return 0;
    }
    memcpy(copy, name, name_length);
    copy[name_length] = '\0';
    size_t index = builder->storage.signal_count++;
    builder->storage.signals[index] = (lemma_ir_signal){
        copy, width, is_signed, is_port, source
    };
    *signal_id = (uint32_t)index;
    return 1;
}

//--------------------------------------------------------
// Append an ordered port record referring to a known signal.
//
// @builder: Mutable builder.
// @signal_id: Existing signal array-position ID.
// @direction: Input or output direction.
// @returns: Nonzero on success, zero for invalid ID/direction or allocation failure.
//--------------------------------------------------------
int lemma_circuit_builder_add_port(lemma_circuit_builder *builder,
                                   uint32_t signal_id,
                                   lemma_ir_port_direction direction)
{
    if (builder == NULL || signal_id >= builder->storage.signal_count ||
        (direction != LEMMA_IR_PORT_INPUT && direction != LEMMA_IR_PORT_OUTPUT) ||
        builder->storage.port_count >= UINT32_MAX ||
        !reserve_one((void **)&builder->storage.ports, builder->storage.port_count,
                     &builder->port_capacity, sizeof(*builder->storage.ports))) {
        return 0;
    }
    builder->storage.ports[builder->storage.port_count++] = (lemma_ir_port){
        signal_id, direction
    };
    return 1;
}

//--------------------------------------------------------
// Append a read-signal expression with type copied from its signal.
//
// @builder: Mutable builder.
// @signal_id: Existing signal ID.
// @source: Expression source span.
// @expression_id: Required output for the new ID.
// @returns: Nonzero on success, zero for invalid ID, limit, or allocation failure.
//--------------------------------------------------------
int lemma_circuit_builder_add_read_signal(lemma_circuit_builder *builder,
                                          uint32_t signal_id,
                                          lemma_ir_source_span source,
                                          uint32_t *expression_id)
{
    if (builder == NULL || expression_id == NULL ||
        signal_id >= builder->storage.signal_count ||
        builder->storage.expression_count >= UINT32_MAX ||
        !reserve_one((void **)&builder->storage.expressions,
                     builder->storage.expression_count, &builder->expression_capacity,
                     sizeof(*builder->storage.expressions))) {
        return 0;
    }
    lemma_ir_signal *signal = &builder->storage.signals[signal_id];
    size_t index = builder->storage.expression_count++;
    builder->storage.expressions[index] = (lemma_ir_expression){
        LEMMA_IR_EXPRESSION_READ_SIGNAL, signal_id, signal->width,
        signal->is_signed, source
    };
    *expression_id = (uint32_t)index;
    return 1;
}

//--------------------------------------------------------
// Append a combinational driver referring to an existing signal and expression.
//
// @builder: Mutable builder.
// @target_signal_id: Existing target signal ID.
// @expression_id: Existing expression ID.
// @source: Assignment source span.
// @returns: Nonzero on success, zero for invalid IDs, limit, or allocation failure.
//--------------------------------------------------------
int lemma_circuit_builder_add_driver(lemma_circuit_builder *builder,
                                     uint32_t target_signal_id,
                                     uint32_t expression_id,
                                     lemma_ir_source_span source)
{
    if (builder == NULL || target_signal_id >= builder->storage.signal_count ||
        expression_id >= builder->storage.expression_count ||
        builder->storage.driver_count >= UINT32_MAX ||
        !reserve_one((void **)&builder->storage.drivers, builder->storage.driver_count,
                     &builder->driver_capacity, sizeof(*builder->storage.drivers))) {
        return 0;
    }
    builder->storage.drivers[builder->storage.driver_count++] = (lemma_ir_driver){
        target_signal_id, expression_id, source
    };
    return 1;
}

//--------------------------------------------------------
// Find an existing signal by exact source-name bytes without copying them.
//
// @builder: Builder containing the signal table.
// @name: Non-NULL source-name bytes.
// @name_length: Number of name bytes to match.
// @signal_id: Required output set only when a match exists.
// @returns: Nonzero when found; zero for invalid input or no match.
//--------------------------------------------------------
int lemma_circuit_builder_find_signal(const lemma_circuit_builder *builder,
                                      const char *name, size_t name_length,
                                      uint32_t *signal_id)
{
    if (builder == NULL || name == NULL || name_length == 0U || signal_id == NULL) {
        return 0;
    }
    for (size_t index = 0U; index < builder->storage.signal_count; ++index) {
        const char *stored = builder->storage.signals[index].name;
        size_t stored_length = strlen(stored);
        if (stored_length == name_length &&
            memcmp(stored, name, name_length) == 0) {
            *signal_id = (uint32_t)index;
            return 1;
        }
    }
    return 0;
}

//--------------------------------------------------------
// Return the number of signals currently stored in the builder.
//
// @builder: Builder whose final IR signal table is inspected.
// @returns: Number of stored signals, or zero for a null builder.
//--------------------------------------------------------
size_t lemma_circuit_builder_signal_count(const lemma_circuit_builder *builder)
{
    return builder == NULL ? 0U : builder->storage.signal_count;
}

//--------------------------------------------------------
// Read a signal from the builder's final IR storage.
//
// @builder: Builder containing the signal.
// @signal_id: Signal array-position ID.
// @returns: Borrowed signal pointer, or NULL for invalid input or ID.
//--------------------------------------------------------
const lemma_ir_signal *lemma_circuit_builder_signal_at(
    const lemma_circuit_builder *builder, uint32_t signal_id)
{
    if (builder == NULL || signal_id >= builder->storage.signal_count) {
        return NULL;
    }
    return &builder->storage.signals[signal_id];
}

//--------------------------------------------------------
// Return the number of ordered ports currently stored in the builder.
//
// @builder: Builder whose final IR port table is inspected.
// @returns: Number of stored ports, or zero for a null builder.
//--------------------------------------------------------
size_t lemma_circuit_builder_port_count(const lemma_circuit_builder *builder)
{
    return builder == NULL ? 0U : builder->storage.port_count;
}

//--------------------------------------------------------
// Read an ordered port from the builder's final IR storage.
//
// @builder: Builder containing the port.
// @port_index: Port array index.
// @returns: Borrowed port pointer, or NULL for invalid input or index.
//--------------------------------------------------------
const lemma_ir_port *lemma_circuit_builder_port_at(
    const lemma_circuit_builder *builder, size_t port_index)
{
    if (builder == NULL || port_index >= builder->storage.port_count) {
        return NULL;
    }
    return &builder->storage.ports[port_index];
}

//--------------------------------------------------------
// Return the number of drivers currently stored in the builder.
//
// @builder: Builder whose final IR driver table is inspected.
// @returns: Number of stored drivers, or zero for a null builder.
//--------------------------------------------------------
size_t lemma_circuit_builder_driver_count(const lemma_circuit_builder *builder)
{
    return builder == NULL ? 0U : builder->storage.driver_count;
}

//--------------------------------------------------------
// Read a driver from the builder's final IR storage.
//
// @builder: Builder containing the driver.
// @driver_index: Driver array index.
// @returns: Borrowed driver pointer, or NULL for invalid input or index.
//--------------------------------------------------------
const lemma_ir_driver *lemma_circuit_builder_driver_at(
    const lemma_circuit_builder *builder, size_t driver_index)
{
    if (builder == NULL || driver_index >= builder->storage.driver_count) {
        return NULL;
    }
    return &builder->storage.drivers[driver_index];
}

//--------------------------------------------------------
// Read an expression from the builder's final IR storage.
//
// @builder: Builder containing the expression.
// @expression_id: Expression array-position ID.
// @returns: Borrowed expression pointer, or NULL for invalid input or ID.
//--------------------------------------------------------
const lemma_ir_expression *lemma_circuit_builder_expression_at(
    const lemma_circuit_builder *builder, uint32_t expression_id)
{
    if (builder == NULL || expression_id >= builder->storage.expression_count) {
        return NULL;
    }
    return &builder->storage.expressions[expression_id];
}

//--------------------------------------------------------
// Transfer the builder's sole circuit storage into a caller-owned circuit.
//
// @builder: Builder consumed only when finalization succeeds.
// @returns: Owned circuit, or NULL on allocation failure with builder intact.
//--------------------------------------------------------
lemma_circuit *lemma_circuit_builder_finalize(lemma_circuit_builder *builder)
{
    if (builder == NULL || builder->storage.module_name == NULL) {
        return NULL;
    }
    lemma_circuit *circuit = malloc(sizeof(*circuit));
    if (circuit == NULL) {
        return NULL;
    }
    *circuit = builder->storage;
    free(builder);
    return circuit;
}

//--------------------------------------------------------
// Destroy a circuit and all storage owned by it.
//
// @circuit: Circuit to destroy, or NULL.
// @returns: None.
//--------------------------------------------------------
void lemma_circuit_destroy(lemma_circuit *circuit)
{
    if (circuit == NULL) {
        return;
    }
    free(circuit->module_name);
    for (size_t index = 0U; index < circuit->signal_count; ++index) {
        free(circuit->signals[index].name);
    }
    free(circuit->ports);
    free(circuit->signals);
    free(circuit->expressions);
    free(circuit->drivers);
    free(circuit->ffds);
    free(circuit);
}
