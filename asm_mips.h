#ifndef _ASM_MIPS_H
#define _ASM_MIPS_H

#include "common.h"
#include "ir.h"

enum mips_reg {
    REG_ZERO,
    REG_AT,
    REG_V0, REG_V1,
    REG_A0, REG_A1, REG_A2, REG_A3,
    REG_T0, REG_T1, REG_T2, REG_T3, REG_T4, REG_T5, REG_T6, REG_T7,
    REG_S0, REG_S1, REG_S2, REG_S3, REG_S4, REG_S5, REG_S6, REG_S7,
    REG_T8, REG_T9,
    REG_K0, REG_K1,
    REG_GP,
    REG_SP,
    REG_FP,
    REG_RA
};

struct mips_operand {
    enum mips_op_type {
        MIPS_OP_LABEL,
        MIPS_OP_REG,
        MIPS_OP_IMM,
        MIPS_OP_ADDR
    } kind;
    union {
        int value;
        char *label;
        struct { // MIPS_OP_ADDR use both
            enum mips_reg reg; // MIPS_OP_REG use this only
            int offset;
        };
    };
};

struct mips_inst {
    enum mips_inst_type {
        MIPS_LABEL,
        MIPS_LI,
        MIPS_LA,
        MIPS_MOVE,
        MIPS_ADD,
        MIPS_ADDI,
        MIPS_SUB,
        MIPS_MUL,
        MIPS_DIV,
        MIPS_MFLO,
        MIPS_LW,
        MIPS_SW,
        MIPS_J,
        MIPS_JAL,
        MIPS_JR,
        MIPS_BEQ,
        MIPS_BNE,
        MIPS_BGT,
        MIPS_BLT,
        MIPS_BGE,
        MIPS_BLE
    } kind;
    union {
        struct {
            struct mips_operand *op1, *op2, *op3;
        };
        struct mips_operand *op;
        char *label;
    };
    struct mips_inst *prev, *next;
} *asm_list, *asm_list_tail;

#define new_mips_inst(__kind, ...) \
    new(struct mips_inst, .kind=__kind, ##__VA_ARGS__)
#define new_mips_op(__kind, ...) \
    new(struct mips_operand, .kind=__kind, ##__VA_ARGS__)

struct mips_operand *reg_op[32];

struct mips_inst *add_mips_inst(struct mips_inst *inst);

void asm_mips_translate(FILE *);
void asm_cut_function_block(struct ir_code *head, struct ir_code *tail);
void asm_proc_function_block(struct ir_code *head, struct ir_code *tail);
void asm_proc_ir_code(struct ir_code *);

void print_mips_inst(FILE* f, struct mips_inst *inst);
char *get_mips_op_str(struct mips_operand *op);

#endif
