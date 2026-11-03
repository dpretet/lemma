#include "frontend_internal.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Copy a source byte slice into newly allocated NUL-terminated storage.
 * The caller owns the copy; NULL indicates size overflow or allocation failure.
 */
static char *copy_source_text(const lemma_parsed_design *parsed,
                              uint32_t start, uint32_t end)
{
    size_t length = (size_t)(end - start);
    if (length == SIZE_MAX) {
        return NULL;
    }
    char *copy = malloc(length + 1U);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, parsed->source + start, length);
    copy[length] = '\0';
    return copy;
}

/* Build an owned circuit from live parsed and elaborated designs; elaborated
 * must come from parsed (this is not checked). Inputs are borrowed; diagnostic
 * may be NULL and is cleared on entry. Return NULL for invalid/oversized input
 * or allocation failure.
 */
lemma_circuit *lemma_frontend_build_ir(
    const lemma_parsed_design *parsed,
    const lemma_elaborated_design *elaborated,
    lemma_frontend_diagnostic *diagnostic)
{
    frontend_clear_diagnostic(diagnostic);
    // Ensure resolved counts and allocation sizes fit their representable limits.
    if (parsed == NULL || elaborated == NULL ||
        elaborated->signal_count > UINT32_MAX ||
        elaborated->driver_count > UINT32_MAX ||
        elaborated->driver_count > SIZE_MAX / sizeof(lemma_ir_expression) ||
        elaborated->driver_count > SIZE_MAX / sizeof(lemma_ir_driver) ||
        elaborated->signal_count > SIZE_MAX / sizeof(lemma_ir_signal) ||
        elaborated->signal_count > SIZE_MAX / sizeof(lemma_ir_port)) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
                       "ir.invalid_input", "invalid or oversized resolved design", (TSNode){0});
        return NULL;
    }

    // Allocate the circuit and all fixed-size arrays before lowering entries.
    lemma_circuit *circuit = calloc(1U, sizeof(*circuit));
    if (circuit == NULL) {
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
                       "ir.out_of_memory", "unable to allocate circuit", (TSNode){0});
        return NULL;
    }
    circuit->module_name = copy_source_text(parsed, elaborated->module_name_start,
                                            elaborated->module_name_end);
    circuit->ports = calloc(elaborated->signal_count, sizeof(*circuit->ports));
    circuit->signals = calloc(elaborated->signal_count, sizeof(*circuit->signals));
    circuit->expressions = calloc(elaborated->driver_count, sizeof(*circuit->expressions));
    circuit->drivers = calloc(elaborated->driver_count, sizeof(*circuit->drivers));
    circuit->ffds = NULL;
    if (circuit->module_name == NULL || circuit->ports == NULL ||
        circuit->signals == NULL ||
        (elaborated->driver_count != 0U &&
         (circuit->expressions == NULL || circuit->drivers == NULL))) {
        lemma_circuit_destroy(circuit);
        frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
                       "ir.out_of_memory", "unable to allocate circuit arrays", (TSNode){0});
        return NULL;
    }
    circuit->signal_count = elaborated->signal_count;
    circuit->port_count = elaborated->signal_count;
    circuit->expression_count = elaborated->driver_count;
    circuit->driver_count = elaborated->driver_count;
    circuit->ffd_count = 0U;

    // Lower resolved ports into circuit signals and port records.
    for (size_t index = 0U; index < elaborated->signal_count; ++index) {
        const lemma_resolved_signal *resolved = &elaborated->signals[index];
        lemma_ir_signal *signal = &circuit->signals[index];
        signal->name = copy_source_text(parsed, resolved->name_start, resolved->name_end);
        if (signal->name == NULL) {
            lemma_circuit_destroy(circuit);
            frontend_set_diagnostic(diagnostic, LEMMA_FRONTEND_STAGE_CIRCUIT_IR,
                           "ir.out_of_memory", "unable to copy signal name", (TSNode){0});
            return NULL;
        }
        signal->width = resolved->width;
        signal->is_signed = resolved->is_signed != 0;
        signal->is_port = true;
        signal->source = resolved->source;
        circuit->ports[index].signal_id = (uint32_t)index;
        circuit->ports[index].direction = resolved->direction;
    }

    // Lower each direct alias to a read expression and combinational driver.
    for (size_t index = 0U; index < elaborated->driver_count; ++index) {
        const lemma_resolved_driver *resolved = &elaborated->drivers[index];
        const lemma_resolved_signal *source = &elaborated->signals[resolved->source_signal_id];
        circuit->expressions[index].kind = LEMMA_IR_EXPRESSION_READ_SIGNAL;
        circuit->expressions[index].signal_id = resolved->source_signal_id;
        circuit->expressions[index].width = source->width;
        circuit->expressions[index].is_signed = source->is_signed != 0;
        circuit->expressions[index].source = resolved->expression_source;
        circuit->drivers[index].target_signal_id = resolved->target_signal_id;
        circuit->drivers[index].expression_id = (uint32_t)index;
        circuit->drivers[index].source = resolved->source;
    }
    return circuit;
}

// Destroy the circuit and its owned arrays and strings; NULL is accepted.
void lemma_circuit_destroy(lemma_circuit *circuit)
{
    if (circuit != NULL) {
        if (circuit->signals != NULL) {
            for (size_t index = 0U; index < circuit->signal_count; ++index) {
                free(circuit->signals[index].name);
            }
        }
        free(circuit->module_name);
        free(circuit->ports);
        free(circuit->signals);
        free(circuit->expressions);
        free(circuit->drivers);
        free(circuit->ffds);
        free(circuit);
    }
}
