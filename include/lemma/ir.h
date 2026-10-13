#ifndef LEMMA_IR_H
#define LEMMA_IR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* IDs are indexes into the corresponding circuit table. */
typedef uint32_t LemmaIRSignalId;
typedef uint32_t LemmaIRExprId;

#define LEMMA_IR_INVALID_ID UINT32_MAX
#define LEMMA_IR_INVALID_OPERATOR UINT32_C(0)

typedef struct {
    /* NULL path means source provenance is unavailable. Lines/columns are 1-based. */
    const char *path;
    uint32_t start_byte;
    uint32_t end_byte;
    uint32_t start_line;
    uint32_t start_column;
    uint32_t end_line;
    uint32_t end_column;
} LemmaIRSourceSpan;

/* Bits are stored least-significant word first. A zero in known_mask denotes
 * an unconstrained two-state choice for that bit; bits at those positions are
 * ignored. The last word is truncated to the width supplied by its owner. */
typedef struct {
    const uint64_t *words;
    const uint64_t *known_mask;
    size_t word_count;
} LemmaIRBitVector;

typedef struct {
    const char *name;
    uint32_t width;
    bool is_signed;
    LemmaIRSourceSpan source;
} LemmaIRSignal;

typedef enum {
    LEMMA_IR_PORT_INPUT,
    LEMMA_IR_PORT_OUTPUT
} LemmaIRPortDirection;

typedef struct {
    LemmaIRSignalId signal;
    LemmaIRPortDirection direction;
} LemmaIRPort;

typedef enum {
    LEMMA_IR_EXPR_BIT_VECTOR,
    LEMMA_IR_EXPR_SIGNAL,
    LEMMA_IR_EXPR_OPERATOR
} LemmaIRExprKind;

/* Operator IDs are reserved for the separately specified bit-vector operator
 * catalog. Zero is invalid; source lowering must not invent operator meanings. */
typedef uint32_t LemmaIROperator;

typedef struct {
    uint32_t width;
    bool is_signed;
    LemmaIRExprKind kind;
    LemmaIRSourceSpan source;
    union {
        LemmaIRBitVector bit_vector;
        LemmaIRSignalId signal;
        struct {
            LemmaIROperator opcode;
            const LemmaIRExprId *operands;
            size_t operand_count;
        } operation;
    } value;
} LemmaIRExpr;

typedef struct {
    LemmaIRSignalId target;
    LemmaIRExprId value;
    LemmaIRSourceSpan source;
} LemmaIRCombDriver;

typedef enum {
    LEMMA_IR_EDGE_POSEDGE,
    LEMMA_IR_EDGE_NEGEDGE
} LemmaIREdge;

typedef struct {
    bool present;
    LemmaIRSignalId signal;
    LemmaIREdge asserted_edge;
    LemmaIRBitVector value;
} LemmaIRAsyncReset;

typedef struct {
    LemmaIRSignalId q;
    LemmaIRExprId d;
    LemmaIRSignalId clock;
    LemmaIREdge active_edge;
    LemmaIRAsyncReset async_reset;
    bool has_initial_value;
    LemmaIRBitVector initial_value;
    LemmaIRSourceSpan source;
} LemmaIRFFD;

/* All arrays and strings referenced by the circuit, including operand and
 * bit-vector storage, are borrowed and must outlive the circuit. IDs are table
 * indexes, so appending entries does not change existing IDs. */
typedef struct {
    const char *module_name;
    const LemmaIRPort *ports;
    size_t port_count;
    const LemmaIRSignal *signals;
    size_t signal_count;
    const LemmaIRExpr *expressions;
    size_t expression_count;
    const LemmaIRCombDriver *comb_drivers;
    size_t comb_driver_count;
    const LemmaIRFFD *ffds;
    size_t ffd_count;
} LemmaIRCircuit;

typedef enum {
    LEMMA_IR_OBJECT_CIRCUIT,
    LEMMA_IR_OBJECT_PORT,
    LEMMA_IR_OBJECT_SIGNAL,
    LEMMA_IR_OBJECT_EXPRESSION,
    LEMMA_IR_OBJECT_DRIVER,
    LEMMA_IR_OBJECT_FFD
} LemmaIRObjectKind;

typedef enum {
    LEMMA_IR_STATUS_OK,
    LEMMA_IR_STATUS_INVALID_ARGUMENT,
    LEMMA_IR_STATUS_INVALID_NAME,
    LEMMA_IR_STATUS_INVALID_ARRAY,
    LEMMA_IR_STATUS_INVALID_ID,
    LEMMA_IR_STATUS_INVALID_WIDTH,
    LEMMA_IR_STATUS_INVALID_KIND,
    LEMMA_IR_STATUS_INVALID_BIT_VECTOR,
    LEMMA_IR_STATUS_INVALID_OPERATOR,
    LEMMA_IR_STATUS_INVALID_PORT,
    LEMMA_IR_STATUS_WIDTH_MISMATCH,
    LEMMA_IR_STATUS_INVALID_CLOCK,
    LEMMA_IR_STATUS_INVALID_RESET,
    LEMMA_IR_STATUS_INPUT_HAS_DRIVER,
    LEMMA_IR_STATUS_MULTIPLE_DRIVERS,
    LEMMA_IR_STATUS_COMBINATIONAL_CYCLE,
    LEMMA_IR_STATUS_OUT_OF_MEMORY
} LemmaIRStatus;

typedef struct {
    LemmaIRStatus status;
    LemmaIRObjectKind object_kind;
    size_t object_index;
} LemmaIRDiagnostic;

/* Checks table references, concrete widths, port rules, and unique drivers.
 * It does not check operator semantics or source-language elaboration rules. */
LemmaIRStatus lemma_ir_validate(const LemmaIRCircuit *circuit,
                                LemmaIRDiagnostic *diagnostic);

#endif
