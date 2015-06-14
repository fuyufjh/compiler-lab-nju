#include "asm_mips.h"
#include "frame.h"

static int n_callee_args;

static struct mips_operand op_imm_minus4 = {MIPS_OP_IMM, .value=-4};

static char buf[128];

struct mips_inst *add_mips_inst(struct mips_inst *inst) {
    if (asm_list == NULL) {
        asm_list = asm_list_tail = inst;
        inst->next = inst->prev = NULL;
    } else {
        asm_list_tail->next = inst;
        inst->prev = asm_list_tail;
        asm_list_tail = inst;
    }
    ///////debug
    print_mips_inst(stdout, inst);

    return inst;
}

char *common_mips_header;
void asm_mips_translate(FILE *f) {
    fputs(common_mips_header, f);
    // first generate the 0~31 register operand
    for (int i = 0; i < 32; i++) {
        reg_op[i] = new_mips_op(MIPS_OP_REG, .reg=i);
    }
    asm_cut_function_block(ir_list, ir_list_tail);
    for (struct mips_inst *p = asm_list; p != asm_list_tail->next; p = p->next) {
        print_mips_inst(f, p);
    }
}

void asm_cut_function_block(struct ir_code *head, struct ir_code *tail) {
    assert(head && tail && head != tail);
    struct ir_code *p = head->next;
    struct ir_code *block_head = head;
    while (p != tail) {
        if (p->kind == IR_FUNCTION) {
            asm_proc_function_block(block_head, p->prev);
            block_head = p;
        }
        p = p->next;
    }
    asm_proc_function_block(block_head, tail);
}

void asm_proc_function_block(struct ir_code *head, struct ir_code *tail) {
    //debug
    printf(">>>>> ");
    print_ir_code(stdout, head);

    assert(head && tail && head != tail);
    assert(head->kind == IR_FUNCTION);
    assert(frame == NULL);
    new_frame();
    add_mips_inst(new_mips_inst(MIPS_LABEL, .label=head->op->name));
    // Generate prologue:
    // sw $fp, 0($sp)
    // sw $ra, -4($sp)
    // move $fp, $sp
    // addi $sp, $sp, -frame_size
    struct mips_inst *inst;
    add_mips_inst(new_mips_inst(MIPS_SW, .op1=reg_op[REG_FP], \
                .op2 = new_mips_op(MIPS_OP_ADDR, .reg=REG_SP, .offset=0)));
    add_mips_inst(new_mips_inst(MIPS_SW, .op1=reg_op[REG_RA], \
                .op2 = new_mips_op(MIPS_OP_ADDR, .reg=REG_SP, .offset=-4)));
    add_mips_inst(new_mips_inst(MIPS_MOVE, .op1=reg_op[REG_FP], .op2=reg_op[REG_SP]));
    add_mips_inst(inst = new_mips_inst(MIPS_ADDI, .op1=reg_op[REG_SP], \
                .op2=reg_op[REG_SP], .op3=new_mips_op(MIPS_OP_IMM)));
    int *p_frame_size = {&inst->op3->value}; // wait to be filled
    // put arguments in the frame variables table
    struct ir_code *p = head->next;
    while (p->kind == IR_PARAM) {
        add_arg_var(p->op->kind, p->op->no);
        p = p->next;
    }
    // Process other code (cut them into basic blocks)
    while (p != tail->next) {
        switch (p->kind) {
        case IR_FUNCTION:
        case IR_PARAM:
            assert(false);
        default:
            asm_proc_ir_code(p);
        }
        p = p->next;
    }
    // back fill
    *p_frame_size = frame->var_offset;
    release_frame();
}

void gen_mips_load(struct ir_operand *op, enum mips_reg reg) {
    if (op->kind == OP_VARIABLE || op->kind == OP_TEMP_VAR) {
        int offset = get_offset(op->kind, op->no);
        if (op->modifier == OP_MDF_NONE) {
            add_mips_inst(new_mips_inst(MIPS_LW, .op1=reg_op[reg], \
                        .op2=new_mips_op(MIPS_OP_ADDR, .offset=offset, .reg=REG_FP)));
        } else if (op->modifier == OP_MDF_AND) {
            add_mips_inst(new_mips_inst(MIPS_LA, .op1=reg_op[reg], \
                        .op2=new_mips_op(MIPS_OP_ADDR, .offset=offset, .reg=REG_FP)));
        } else if (op->modifier == OP_MDF_STAR) {
            add_mips_inst(new_mips_inst(MIPS_LW, .op1=reg_op[reg], \
                        .op2=new_mips_op(MIPS_OP_ADDR, .offset=offset, .reg=REG_FP)));
            add_mips_inst(new_mips_inst(MIPS_LW, .op1=reg_op[reg], \
                        .op2=new_mips_op(MIPS_OP_ADDR, .offset=0, .reg=reg)));
        }
    } else if (op->kind == OP_IMMEDIATE) {
        add_mips_inst(new_mips_inst(MIPS_LI, .op1=reg_op[reg], \
                    .op2=new_mips_op(MIPS_OP_IMM, .value=op->val_int)));
    } else assert(false);
}

void asm_proc_ir_code(struct ir_code *ir) {
    //debug
    printf(">>>>> ");
    print_ir_code(stdout, ir);

    // special treatment for "*a = b"
    if (ir->kind == IR_ASSIGN && ir->dst->modifier == OP_MDF_STAR) {
        int offset = get_offset(ir->dst->kind, ir->dst->no);
        add_mips_inst(new_mips_inst(MIPS_LW, .op1=reg_op[REG_T1], \
                    .op2=new_mips_op(MIPS_OP_ADDR, .offset=offset, .reg=REG_FP)));
        gen_mips_load(ir->src, REG_T2);
        add_mips_inst(new_mips_inst(MIPS_SW, .op1=reg_op[REG_T2], \
                    .op2=new_mips_op(MIPS_OP_ADDR, .offset=0, .reg=REG_T1)));
        return;
    }

    if (!find_variable(ir->op->kind, ir->op->no)) {
        // special treatment for "DEC v"
        if (ir->kind == IR_DEC)
            dec_local_var(ir->op->kind, ir->op->no, ir->size);
        else
            add_local_var(ir->op->kind, ir->op->no);
    }
    // load dst/op to t1 if needed
    if (ir->kind == IR_RETURN || ir->kind == IR_ARG || ir->kind == IR_WRITE) {
        gen_mips_load(ir->op, REG_T1);
    }
    // load src1 to t2, src2 to t3 if needed
    if (ir->kind != IR_CALL && ir->src1) {
        if (!find_variable(ir->src1->kind, ir->src1->no))
            add_local_var(ir->src1->kind, ir->src1->no);
        gen_mips_load(ir->src1, REG_T2);
    }
    if (ir->src2) {
        if (!find_variable(ir->src2->kind, ir->src2->no))
            add_local_var(ir->src2->kind, ir->src2->no);
        gen_mips_load(ir->src2, REG_T3);
    }

    switch (ir->kind) {
    case IR_LABEL:
        sprintf(buf, "label%d", ir->op->no);
        add_mips_inst(new_mips_inst(MIPS_LABEL, .label=strdup(buf)));
        break;
    case IR_CALL:
        add_mips_inst(new_mips_inst(MIPS_JAL, \
                    .op=new_mips_op(MIPS_OP_LABEL, .label=ir->func->name)));
        add_mips_inst(new_mips_inst(MIPS_ADDI, .op1=reg_op[REG_SP], \
                    .op2=reg_op[REG_SP],
                    .op3=new_mips_op(MIPS_OP_IMM, .value=n_callee_args * 4)));
        add_mips_inst(new_mips_inst(MIPS_MOVE, .op1=reg_op[REG_T1], .op2=reg_op[REG_V0]));
        break;
    case IR_ASSIGN:
        add_mips_inst(new_mips_inst(MIPS_MOVE, .op1=reg_op[REG_T1], .op2=reg_op[REG_T2]));
        break;
    case IR_ADD:
        add_mips_inst(new_mips_inst(MIPS_ADD, .op1=reg_op[REG_T1], \
                    .op2=reg_op[REG_T2], .op3=reg_op[REG_T3]));
        break;
    case IR_SUB:
        add_mips_inst(new_mips_inst(MIPS_SUB, .op1=reg_op[REG_T1], \
                    .op2=reg_op[REG_T2], .op3=reg_op[REG_T3]));
        break;
    case IR_MUL:
        add_mips_inst(new_mips_inst(MIPS_MUL, .op1=reg_op[REG_T1], \
                    .op2=reg_op[REG_T2], .op3=reg_op[REG_T3]));
        break;
    case IR_DIV:
        add_mips_inst(new_mips_inst(MIPS_DIV, .op1=reg_op[REG_T2], .op2=reg_op[REG_T3]));
        add_mips_inst(new_mips_inst(MIPS_MFLO, .op1=reg_op[REG_T1]));
        break;
    case IR_GOTO:
        sprintf(buf, "label%d", ir->op->no);
        add_mips_inst(new_mips_inst(MIPS_J, .op=new_mips_op(MIPS_OP_LABEL, .label=strdup(buf))));
        break;
        enum mips_inst_type branch_inst;
    case IR_IF_GOTO:
        switch (ir->relop) {
        case RELOP_EQ:  branch_inst = MIPS_BEQ; break;
        case RELOP_NEQ: branch_inst = MIPS_BNE; break;
        case RELOP_G:   branch_inst = MIPS_BGT; break;
        case RELOP_L:   branch_inst = MIPS_BLT; break;
        case RELOP_GE:  branch_inst = MIPS_BGE; break;
        case RELOP_LE:  branch_inst = MIPS_BLE; break;
        }
        sprintf(buf, "label%d", ir->op->no);
        add_mips_inst(new_mips_inst(branch_inst, .op1=reg_op[REG_T2], \
                    .op2=reg_op[REG_T3], .op3=new_mips_op(MIPS_OP_LABEL, .label=strdup(buf))));
        break;
    case IR_RETURN:
        add_mips_inst(new_mips_inst(MIPS_MOVE, .op1=reg_op[REG_V0], .op2=reg_op[REG_T1]));
        // Generate epilogue:
        // move $sp, $fp
        // lw $ra, -4($sp)
        // lw $fp, 0($sp)
        // jr $ra
        add_mips_inst(new_mips_inst(MIPS_MOVE, .op1=reg_op[REG_SP], .op2=reg_op[REG_FP]));
        add_mips_inst(new_mips_inst(MIPS_LW, .op1=reg_op[REG_RA],\
                    .op2=new_mips_op(MIPS_OP_ADDR, .reg=REG_SP, .offset=-4)));
        add_mips_inst(new_mips_inst(MIPS_LW, .op1=reg_op[REG_FP],\
                    .op2=new_mips_op(MIPS_OP_ADDR, .reg=REG_SP, .offset=0)));
        add_mips_inst(new_mips_inst(MIPS_JR, .op=reg_op[REG_RA]));
        break;
    case IR_DEC:
        break; // has been processed before
    case IR_ARG:
        add_mips_inst(new_mips_inst(MIPS_SW, .op1=reg_op[REG_T1], \
                    .op2=new_mips_op(MIPS_OP_ADDR, .offset=0, .reg=REG_SP)));
        add_mips_inst(new_mips_inst(MIPS_ADDI, .op1=reg_op[REG_SP], .op2=reg_op[REG_SP], \
                    .op3=&op_imm_minus4));
        n_callee_args++;
        break;
    case IR_READ:
        add_mips_inst(new_mips_inst(MIPS_SW, .op1=reg_op[REG_RA], \
                    .op2=new_mips_op(MIPS_OP_ADDR, .reg=REG_SP, .offset=0)));
        add_mips_inst(new_mips_inst(MIPS_JAL, .op=new_mips_op(MIPS_OP_LABEL, .label="read")));
        add_mips_inst(new_mips_inst(MIPS_MOVE, .op1=reg_op[REG_T1], .op2=reg_op[REG_V0]));
        add_mips_inst(new_mips_inst(MIPS_LW, .op1=reg_op[REG_RA],\
                    .op2=new_mips_op(MIPS_OP_ADDR, .reg=REG_SP, .offset=0)));
        break;
    case IR_WRITE:
        add_mips_inst(new_mips_inst(MIPS_SW, .op1=reg_op[REG_RA], \
                    .op2=new_mips_op(MIPS_OP_ADDR, .reg=REG_SP, .offset=0)));
        add_mips_inst(new_mips_inst(MIPS_MOVE, .op1=reg_op[REG_A0], .op2=reg_op[REG_T1]));
        add_mips_inst(new_mips_inst(MIPS_JAL, .op=new_mips_op(MIPS_OP_LABEL, .label="write")));
        add_mips_inst(new_mips_inst(MIPS_LW, .op1=reg_op[REG_RA],\
                    .op2=new_mips_op(MIPS_OP_ADDR, .reg=REG_SP, .offset=0)));
        break;
    case IR_FUNCTION:
    case IR_PARAM:
        assert(false);
    }

    // store result if needed
    int offset = get_offset(ir->op->kind, ir->op->no);
    if (ir->kind == IR_ASSIGN || ir->kind == IR_ADD || ir->kind == IR_SUB || \
            ir->kind == IR_DIV || ir->kind == IR_MUL || ir->kind == IR_READ || \
            ir->kind == IR_CALL) {
        add_mips_inst(new_mips_inst(MIPS_SW, .op1=reg_op[REG_T1], \
                    .op2=new_mips_op(MIPS_OP_ADDR, .reg=REG_FP, .offset=offset)));
    }
}

char *mips_inst_str[] = {
    NULL, "li", "la", "move", "add", "addi", "sub", "mul", "div",
    "mflo", "lw", "sw", "j", "jal", "jr", "beq", "bne", "bgt",
    "blt", "bge", "ble"
};

char *reg_name[] = {
    "$zero",
    "$at",
    "$v0", "$v1",
    "$a0", "$a1", "$a2", "$a3",
    "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
    "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
    "$t8", "$t9",
    "$k0", "$k1",
    "$gp",
    "$sp",
    "$fp",
    "$ra"
};

void print_mips_inst(FILE* f, struct mips_inst *inst) {
    if (inst->kind == MIPS_LABEL) {
        fprintf(f, "%s:\n", inst->label);
        return;
    }
    fprintf(f, "%s", mips_inst_str[inst->kind]);
    if (inst->op1)
        fprintf(f, " %s", get_mips_op_str(inst->op1));
    if (inst->op2)
        fprintf(f, ", %s", get_mips_op_str(inst->op2));
    if (inst->op3)
        fprintf(f, ", %s", get_mips_op_str(inst->op3));
    fprintf(f, "\n");
}

char *get_mips_op_str(struct mips_operand *op) {
    switch (op->kind) {
    case MIPS_OP_LABEL:
        return op->label;
    case MIPS_OP_IMM:
        sprintf(buf, "%d", op->value);
        return strdup(buf);
    case MIPS_OP_REG:
        return reg_name[op->reg];
    case MIPS_OP_ADDR:
        sprintf(buf, "%d(%s)", op->offset, reg_name[op->reg]);
        return strdup(buf);
    }
    assert(false);
}

char *common_mips_header = ".data\n\
_prompt: .asciiz \"Enter an integer:\"\n\
_ret: .asciiz \"\\n\"\n\
.globl main\n\
.text\n\
read:\n\
li $v0, 4\n\
la $a0, _prompt\n\
syscall\n\
li $v0, 5\n\
syscall\n\
jr $ra\n\
\n\
write:\n\
li $v0, 1\n\
syscall\n\
li $v0, 4\n\
la $a0, _ret\n\
syscall\n\
move $v0, $0\n\
jr $ra\n\
\n";
