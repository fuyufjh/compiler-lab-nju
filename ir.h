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
    enum modifier_type {
        OP_MDF_NONE,
        OP_MDF_AND,
        OP_MDF_STAR
    } modifier;
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
        struct {
            struct ir_operand *dst;
            union {
                struct {
                    struct ir_operand *src1, *src2;
                };
                struct ir_operand *src;
            };
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
    struct ir_code *prev, *next;
} *ir_list, *ir_list_tail;

struct ir_code *remove_ir_code(struct ir_code *code);

inline struct ir_operand *new_temp_var();
inline struct ir_operand *new_variable();
inline struct ir_operand *new_label();

char *get_operand_str(struct ir_operand *op);
void add_ir_code(struct ir_code *code);
void print_ir_code(FILE*, struct ir_code *code);
void print_ir_list(FILE *fp);
struct ir_operand *ir_clean_temp_var(struct ir_operand *op_temp);
void ir_clean_assign();

#endif
