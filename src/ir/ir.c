#include "lemma/ir.h"

#include <stdlib.h>

static LemmaIRStatus fail(LemmaIRDiagnostic *diagnostic,
                          LemmaIRStatus status,
                          LemmaIRObjectKind object_kind,
                          size_t object_index)
{
    if (diagnostic != NULL) {
        diagnostic->status = status;
        diagnostic->object_kind = object_kind;
        diagnostic->object_index = object_index;
    }
    return status;
}

static bool has_input_port(const LemmaIRCircuit *circuit, LemmaIRSignalId signal)
{
    for (size_t index = 0; index < circuit->port_count; ++index) {
        if (circuit->ports[index].signal == signal &&
            circuit->ports[index].direction == LEMMA_IR_PORT_INPUT) {
            return true;
        }
    }
    return false;
}

static LemmaIRStatus validate_bit_vector(const LemmaIRBitVector *value,
                                         uint32_t width,
                                         LemmaIRDiagnostic *diagnostic,
                                         LemmaIRObjectKind object_kind,
                                         size_t object_index)
{
    size_t expected_words = ((size_t)width + 63U) / 64U;
    if (value->word_count != expected_words || value->words == NULL ||
        value->known_mask == NULL) {
        return fail(diagnostic, LEMMA_IR_STATUS_INVALID_BIT_VECTOR,
                    object_kind, object_index);
    }
    return LEMMA_IR_STATUS_OK;
}

static LemmaIRStatus claim_driver(const LemmaIRCircuit *circuit,
                                  LemmaIRSignalId signal,
                                  LemmaIRObjectKind object_kind,
                                  size_t object_index,
                                  LemmaIRDiagnostic *diagnostic)
{
    if (has_input_port(circuit, signal)) {
        return fail(diagnostic, LEMMA_IR_STATUS_INPUT_HAS_DRIVER,
                    object_kind, object_index);
    }

    for (size_t index = 0; index < circuit->comb_driver_count; ++index) {
        if (circuit->comb_drivers[index].target == signal &&
            !(object_kind == LEMMA_IR_OBJECT_DRIVER && index == object_index)) {
            return fail(diagnostic, LEMMA_IR_STATUS_MULTIPLE_DRIVERS,
                        object_kind, object_index);
        }
    }
    for (size_t index = 0; index < circuit->ffd_count; ++index) {
        if (circuit->ffds[index].q == signal &&
            !(object_kind == LEMMA_IR_OBJECT_FFD && index == object_index)) {
            return fail(diagnostic, LEMMA_IR_STATUS_MULTIPLE_DRIVERS,
                        object_kind, object_index);
        }
    }
    return LEMMA_IR_STATUS_OK;
}

typedef struct {
    const LemmaIRCircuit *circuit;
    unsigned char *colors;
    size_t expression_offset;
    LemmaIRDiagnostic *diagnostic;
} LemmaIRCycleCheck;

static LemmaIRStatus visit_expression(LemmaIRCycleCheck *check,
                                     LemmaIRExprId expression_id);

static LemmaIRStatus visit_signal(LemmaIRCycleCheck *check,
                                  LemmaIRSignalId signal_id)
{
    size_t node_id = (size_t)signal_id;
    if (check->colors[node_id] == 1U) {
        return fail(check->diagnostic, LEMMA_IR_STATUS_COMBINATIONAL_CYCLE,
                    LEMMA_IR_OBJECT_SIGNAL, signal_id);
    }
    if (check->colors[node_id] == 2U) {
        return LEMMA_IR_STATUS_OK;
    }

    check->colors[node_id] = 1U;
    for (size_t index = 0; index < check->circuit->comb_driver_count; ++index) {
        const LemmaIRCombDriver *driver = &check->circuit->comb_drivers[index];
        if (driver->target == signal_id) {
            LemmaIRStatus status = visit_expression(check, driver->value);
            if (status != LEMMA_IR_STATUS_OK) {
                return status;
            }
            break;
        }
    }
    check->colors[node_id] = 2U;
    return LEMMA_IR_STATUS_OK;
}

static LemmaIRStatus visit_expression(LemmaIRCycleCheck *check,
                                     LemmaIRExprId expression_id)
{
    size_t node_id = check->expression_offset + (size_t)expression_id;
    if (check->colors[node_id] == 1U) {
        return fail(check->diagnostic, LEMMA_IR_STATUS_COMBINATIONAL_CYCLE,
                    LEMMA_IR_OBJECT_EXPRESSION, expression_id);
    }
    if (check->colors[node_id] == 2U) {
        return LEMMA_IR_STATUS_OK;
    }

    check->colors[node_id] = 1U;
    const LemmaIRExpr *expression = &check->circuit->expressions[expression_id];
    LemmaIRStatus status = LEMMA_IR_STATUS_OK;
    if (expression->kind == LEMMA_IR_EXPR_SIGNAL) {
        status = visit_signal(check, expression->value.signal);
    } else if (expression->kind == LEMMA_IR_EXPR_OPERATOR) {
        for (size_t index = 0;
             index < expression->value.operation.operand_count;
             ++index) {
            status = visit_expression(
                check, expression->value.operation.operands[index]);
            if (status != LEMMA_IR_STATUS_OK) {
                break;
            }
        }
    }
    if (status != LEMMA_IR_STATUS_OK) {
        return status;
    }
    check->colors[node_id] = 2U;
    return LEMMA_IR_STATUS_OK;
}

static LemmaIRStatus validate_combinational_cycles(
    const LemmaIRCircuit *circuit,
    LemmaIRDiagnostic *diagnostic)
{
    if (circuit->signal_count > SIZE_MAX - circuit->expression_count) {
        return fail(diagnostic, LEMMA_IR_STATUS_INVALID_ARRAY,
                    LEMMA_IR_OBJECT_CIRCUIT, 0U);
    }
    size_t node_count = circuit->signal_count + circuit->expression_count;
    unsigned char *colors = calloc(node_count, sizeof(*colors));
    if (colors == NULL && node_count != 0U) {
        return fail(diagnostic, LEMMA_IR_STATUS_OUT_OF_MEMORY,
                    LEMMA_IR_OBJECT_CIRCUIT, 0U);
    }

    LemmaIRCycleCheck check = {
        .circuit = circuit,
        .colors = colors,
        .expression_offset = circuit->signal_count,
        .diagnostic = diagnostic,
    };
    LemmaIRStatus status = LEMMA_IR_STATUS_OK;
    for (size_t index = 0; index < circuit->expression_count; ++index) {
        status = visit_expression(&check, (LemmaIRExprId)index);
        if (status != LEMMA_IR_STATUS_OK) {
            break;
        }
    }
    free(colors);
    return status;
}

LemmaIRStatus lemma_ir_validate(const LemmaIRCircuit *circuit,
                                LemmaIRDiagnostic *diagnostic)
{
    if (diagnostic != NULL) {
        diagnostic->status = LEMMA_IR_STATUS_OK;
        diagnostic->object_kind = LEMMA_IR_OBJECT_CIRCUIT;
        diagnostic->object_index = 0U;
    }
    if (circuit == NULL) {
        return fail(diagnostic, LEMMA_IR_STATUS_INVALID_ARGUMENT,
                    LEMMA_IR_OBJECT_CIRCUIT, 0U);
    }
    if (circuit->module_name == NULL || circuit->module_name[0] == '\0') {
        return fail(diagnostic, LEMMA_IR_STATUS_INVALID_NAME,
                    LEMMA_IR_OBJECT_CIRCUIT, 0U);
    }
    if ((circuit->port_count > 0U && circuit->ports == NULL) ||
        (circuit->signal_count > 0U && circuit->signals == NULL) ||
        (circuit->expression_count > 0U && circuit->expressions == NULL) ||
        (circuit->comb_driver_count > 0U && circuit->comb_drivers == NULL) ||
        (circuit->ffd_count > 0U && circuit->ffds == NULL) ||
        circuit->signal_count > UINT32_MAX ||
        circuit->expression_count > UINT32_MAX) {
        return fail(diagnostic, LEMMA_IR_STATUS_INVALID_ARRAY,
                    LEMMA_IR_OBJECT_CIRCUIT, 0U);
    }

    for (size_t index = 0; index < circuit->signal_count; ++index) {
        const LemmaIRSignal *signal = &circuit->signals[index];
        if (signal->name == NULL || signal->name[0] == '\0') {
            return fail(diagnostic, LEMMA_IR_STATUS_INVALID_NAME,
                        LEMMA_IR_OBJECT_SIGNAL, index);
        }
        if (signal->width == 0U) {
            return fail(diagnostic, LEMMA_IR_STATUS_INVALID_WIDTH,
                        LEMMA_IR_OBJECT_SIGNAL, index);
        }
    }

    for (size_t index = 0; index < circuit->port_count; ++index) {
        const LemmaIRPort *port = &circuit->ports[index];
        if (port->signal >= circuit->signal_count ||
            (port->direction != LEMMA_IR_PORT_INPUT &&
             port->direction != LEMMA_IR_PORT_OUTPUT)) {
            return fail(diagnostic, LEMMA_IR_STATUS_INVALID_PORT,
                        LEMMA_IR_OBJECT_PORT, index);
        }
        for (size_t earlier = 0; earlier < index; ++earlier) {
            if (circuit->ports[earlier].signal == port->signal) {
                return fail(diagnostic, LEMMA_IR_STATUS_INVALID_PORT,
                            LEMMA_IR_OBJECT_PORT, index);
            }
        }
    }

    for (size_t index = 0; index < circuit->expression_count; ++index) {
        const LemmaIRExpr *expression = &circuit->expressions[index];
        if (expression->width == 0U) {
            return fail(diagnostic, LEMMA_IR_STATUS_INVALID_WIDTH,
                        LEMMA_IR_OBJECT_EXPRESSION, index);
        }
        switch (expression->kind) {
        case LEMMA_IR_EXPR_BIT_VECTOR:
            if (validate_bit_vector(&expression->value.bit_vector,
                                    expression->width, diagnostic,
                                    LEMMA_IR_OBJECT_EXPRESSION, index) !=
                LEMMA_IR_STATUS_OK) {
                return diagnostic != NULL ? diagnostic->status :
                       LEMMA_IR_STATUS_INVALID_BIT_VECTOR;
            }
            break;
        case LEMMA_IR_EXPR_SIGNAL: {
            LemmaIRSignalId signal_id = expression->value.signal;
            if (signal_id >= circuit->signal_count) {
                return fail(diagnostic, LEMMA_IR_STATUS_INVALID_ID,
                            LEMMA_IR_OBJECT_EXPRESSION, index);
            }
            if (expression->width != circuit->signals[signal_id].width ||
                expression->is_signed != circuit->signals[signal_id].is_signed) {
                return fail(diagnostic, LEMMA_IR_STATUS_WIDTH_MISMATCH,
                            LEMMA_IR_OBJECT_EXPRESSION, index);
            }
            break;
        }
        case LEMMA_IR_EXPR_OPERATOR:
            if (expression->value.operation.opcode == LEMMA_IR_INVALID_OPERATOR) {
                return fail(diagnostic, LEMMA_IR_STATUS_INVALID_OPERATOR,
                            LEMMA_IR_OBJECT_EXPRESSION, index);
            }
            if (expression->value.operation.operand_count == 0U ||
                expression->value.operation.operands == NULL) {
                return fail(diagnostic, LEMMA_IR_STATUS_INVALID_ARRAY,
                            LEMMA_IR_OBJECT_EXPRESSION, index);
            }
            for (size_t operand = 0;
                 operand < expression->value.operation.operand_count;
                 ++operand) {
                if (expression->value.operation.operands[operand] >=
                    circuit->expression_count) {
                    return fail(diagnostic, LEMMA_IR_STATUS_INVALID_ID,
                                LEMMA_IR_OBJECT_EXPRESSION, index);
                }
            }
            break;
        default:
            return fail(diagnostic, LEMMA_IR_STATUS_INVALID_KIND,
                        LEMMA_IR_OBJECT_EXPRESSION, index);
        }
    }

    for (size_t index = 0; index < circuit->comb_driver_count; ++index) {
        const LemmaIRCombDriver *driver = &circuit->comb_drivers[index];
        if (driver->target >= circuit->signal_count ||
            driver->value >= circuit->expression_count) {
            return fail(diagnostic, LEMMA_IR_STATUS_INVALID_ID,
                        LEMMA_IR_OBJECT_DRIVER, index);
        }
        if (circuit->signals[driver->target].width !=
            circuit->expressions[driver->value].width) {
            return fail(diagnostic, LEMMA_IR_STATUS_WIDTH_MISMATCH,
                        LEMMA_IR_OBJECT_DRIVER, index);
        }
        LemmaIRStatus status = claim_driver(circuit, driver->target,
                                            LEMMA_IR_OBJECT_DRIVER, index,
                                            diagnostic);
        if (status != LEMMA_IR_STATUS_OK) {
            return status;
        }
    }

    for (size_t index = 0; index < circuit->ffd_count; ++index) {
        const LemmaIRFFD *ffd = &circuit->ffds[index];
        if (ffd->q >= circuit->signal_count ||
            ffd->d >= circuit->expression_count) {
            return fail(diagnostic, LEMMA_IR_STATUS_INVALID_ID,
                        LEMMA_IR_OBJECT_FFD, index);
        }
        if (ffd->active_edge != LEMMA_IR_EDGE_POSEDGE &&
            ffd->active_edge != LEMMA_IR_EDGE_NEGEDGE) {
            return fail(diagnostic, LEMMA_IR_STATUS_INVALID_CLOCK,
                        LEMMA_IR_OBJECT_FFD, index);
        }
        if (ffd->clock >= circuit->signal_count ||
            circuit->signals[ffd->clock].width != 1U) {
            return fail(diagnostic, LEMMA_IR_STATUS_INVALID_CLOCK,
                        LEMMA_IR_OBJECT_FFD, index);
        }
        const LemmaIRSignal *q = &circuit->signals[ffd->q];
        if (q->width != circuit->expressions[ffd->d].width) {
            return fail(diagnostic, LEMMA_IR_STATUS_WIDTH_MISMATCH,
                        LEMMA_IR_OBJECT_FFD, index);
        }
        if (ffd->async_reset.present) {
            if (ffd->async_reset.signal >= circuit->signal_count ||
                circuit->signals[ffd->async_reset.signal].width != 1U ||
                (ffd->async_reset.asserted_edge != LEMMA_IR_EDGE_POSEDGE &&
                 ffd->async_reset.asserted_edge != LEMMA_IR_EDGE_NEGEDGE)) {
                return fail(diagnostic, LEMMA_IR_STATUS_INVALID_RESET,
                            LEMMA_IR_OBJECT_FFD, index);
            }
            if (validate_bit_vector(&ffd->async_reset.value, q->width,
                                    diagnostic, LEMMA_IR_OBJECT_FFD,
                                    index) != LEMMA_IR_STATUS_OK) {
                return diagnostic != NULL ? diagnostic->status :
                       LEMMA_IR_STATUS_INVALID_BIT_VECTOR;
            }
        }
        if (ffd->has_initial_value &&
            validate_bit_vector(&ffd->initial_value, q->width, diagnostic,
                                LEMMA_IR_OBJECT_FFD, index) !=
            LEMMA_IR_STATUS_OK) {
            return diagnostic != NULL ? diagnostic->status :
                   LEMMA_IR_STATUS_INVALID_BIT_VECTOR;
        }
        LemmaIRStatus status = claim_driver(circuit, ffd->q,
                                            LEMMA_IR_OBJECT_FFD, index,
                                            diagnostic);
        if (status != LEMMA_IR_STATUS_OK) {
            return status;
        }
    }

    return validate_combinational_cycles(circuit, diagnostic);
}
