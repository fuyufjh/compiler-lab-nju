#ifndef _IR_H
#define _IR_H

#include "common.h"

struct ir_operand {
    enum op_type {
        OP_VARIABLE,
        OP_IMMEDIATE,
        OP_TEMP_VAR,
        OP_LABEL,
        OP_FUNCTION
    } kind;
    union {
        int no;
        int val_int;
        char *name;
    };
};

struct ir_code {
    enum ir_type {
        IR_LABEL,
        IR_FUNCTION,
        IR_ASSIGN,
        IR_ADD,
        IR_SUB,
        IR_MUL,
        IR_DIV,
        IR_GET_ADDRESS,
        IR_GET_VALUE,
        IR_ASSIGN_TO_ADDRESS,
        IR_GOTO,
        IR_IF_GOTO,
        IR_RETURN,
        IR_DEC,
        IR_ARG,
        IR_CALL,
        IR_PARAM,
        IR_READ,
        IR_WRITE,
    } kind;
    union {
        struct { // 3-operand
            struct ir_operand *dst, *src1, *src2;
        };
        struct { // 2-operand
            struct ir_operand *_dst, *src;
        };
        struct { // 1-operand
            struct ir_operand *op;
        };
        struct { // for RETURN only
            struct ir_operand *ret, *func;
        };
    };
    union {
        enum relop_type {
            RELOP_EQ,
            RELOP_NEQ,
            RELOP_G,
            RELOP_L,
            RELOP_GE,
            RELOP_LE,
        } relop;
        int size;
    };
};

struct ir_list {
    struct ir_node {
        struct ir_code *code;
        struct ir_node *next;
    } *head, *tail;
} ir_list_all;

void add_ir_code(struct ir_list *list, struct ir_code *code);
struct ir_list *concat_ir_list(struct ir_list *a, struct ir_list *b);
void print_ir_code(struct ir_code *code);
void print_ir_list(struct ir_list *list, FILE *fp);

#endif
