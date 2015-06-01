#include <stdio.h>
#include "ir.h"
#include "common.h"

static int count_temp_var = 1;
static int count_variable = 1;
static int count_label    = 1;

inline struct ir_operand *new_temp_var() {
    return new(struct ir_operand, OP_TEMP_VAR, OP_MDF_NONE, .no=count_temp_var++);
}
inline struct ir_operand *new_variable() {
    return new(struct ir_operand, OP_VARIABLE, OP_MDF_NONE, .no=count_variable++);
}
inline struct ir_operand *new_label() {
    return new(struct ir_operand, OP_LABEL, .no=count_label++);
}

char *get_operand_str(struct ir_operand *op) {
    static char buf[32];
    char *modifier;
    switch (op->modifier) {
    case OP_MDF_AND:
        modifier = "&"; break;
    case OP_MDF_STAR:
        modifier = "*"; break;
    default:
        modifier = ""; break;
    }
    switch (op->kind) {
    case OP_VARIABLE:
        sprintf(buf, "%sv%d", modifier, op->no);
        break;
    case OP_IMMEDIATE:
        sprintf(buf, "#%d", op->val_int);
        break;
    case OP_TEMP_VAR:
        sprintf(buf, "%st%d", modifier, op->no);
        break;
    case OP_LABEL:
        sprintf(buf, "label%d", op->no);
        break;
    case OP_FUNCTION:
        sprintf(buf, "%s", op->name);
        break;
    }
    char *ret = (char*) malloc(sizeof(char)*(strlen(buf) + 1));
    strcpy(ret, buf);
    return ret;
}

char* relop_str[] = {
    "==", "!=", ">", "<", ">=", "<="
};

#pragma GCC diagnostic ignored "-Wsequence-point"
void print_ir_code(FILE* out, struct ir_code *code) {
    char* to_free[3];
    int len = 0;
#define OP(op) to_free[len++]=get_operand_str(code->op)
    switch (code->kind) {
    case IR_LABEL:
        fprintf(out, "LABEL %s :", OP(op));
        break;
    case IR_FUNCTION:
        fprintf(out, "FUNCTION %s :", OP(op));
        break;
    case IR_ASSIGN:
        fprintf(out, "%s := %s", OP(dst), OP(src));
        break;
    case IR_ADD:
        fprintf(out, "%s := %s + %s", OP(dst), OP(src1), OP(src2));
        break;
    case IR_SUB:
        fprintf(out, "%s := %s - %s", OP(dst), OP(src1), OP(src2));
        break;
    case IR_MUL:
        fprintf(out, "%s := %s * %s", OP(dst), OP(src1), OP(src2));
        break;
    case IR_DIV:
        fprintf(out, "%s := %s / %s", OP(dst), OP(src1), OP(src2));
        break;
    case IR_GOTO:
        fprintf(out, "GOTO %s", OP(op));
        break;
    case IR_IF_GOTO:
        fprintf(out, "IF %s %s %s GOTO %s", OP(src1), relop_str[code->relop], OP(src2), OP(dst));
        break;
    case IR_RETURN:
        fprintf(out, "RETURN %s", OP(op));
        break;
    case IR_DEC:
        fprintf(out, "DEC %s %d", OP(op), code->size);
        break;
    case IR_ARG:
        fprintf(out, "ARG %s", OP(op));
        break;
    case IR_CALL:
        fprintf(out, "%s := CALL %s", OP(ret), OP(func));
        break;
    case IR_PARAM:
        fprintf(out, "PARAM %s", OP(op));
        break;
    case IR_READ:
        fprintf(out, "READ %s", OP(op));
        break;
    case IR_WRITE:
        fprintf(out, "WRITE %s", OP(op));
        break;
    default:
        exit(-1);
    }
    while (len) free(to_free[--len]);
    fprintf(out, "\n");
}

void print_ir_list(FILE *fp) {
    if (ir_list == NULL) return;
    struct ir_code *p = ir_list;
    while (p != NULL) {
        print_ir_code(fp, p);
        p = p->next;
    }
}

struct ir_code *remove_ir_code(struct ir_code *code) {
    if (code == ir_list && code == ir_list_tail) {
        ir_list = ir_list_tail = NULL;
    } else if (code == ir_list) {
        ir_list = code->next;
        ir_list->prev = NULL;
    } else if (code == ir_list_tail) {
        ir_list_tail = code->prev;
        ir_list_tail->next = NULL;
    } else {
        if (code->prev) code->prev->next = code->next;
        if (code->next) code->next->prev = code->prev;
    }
    code->prev = code->next = NULL;
    return code;
}

struct ir_code *insert_ir_code_before(struct ir_code *place, struct ir_code *code) {
    if (place == ir_list) {
        ir_list = code;
        code->prev = NULL;
    } else {
        if (place->prev) place->prev->next = code;
        code->prev = place->prev;
    }
    code->next = place;
    place->prev = code;
    return code;
}

struct ir_code *insert_ir_code_after(struct ir_code *place, struct ir_code *code) {
    if (place == ir_list_tail) {
        ir_list_tail = code;
        code->next = NULL;
    } else {
        if (place->next) place->next->prev = code;
        code->next = place->next;
    }
    code->prev = place;
    place->next = code;
    return code;
}

void add_ir_code(struct ir_code *code) {
    // optimizing begin
    struct ir_operand *src = NULL;
    switch (code->kind) {
    case IR_ADD:
        if (code->src1->kind == OP_IMMEDIATE && code->src2->kind == OP_IMMEDIATE) {
            int val = code->src1->val_int + code->src2->val_int;
            src = new(struct ir_operand, OP_IMMEDIATE, OP_MDF_NONE, .val_int=val);
        } else if (code->src1->kind == OP_IMMEDIATE && code->src1->val_int == 0)
            src = code->src2;
        else if (code->src2->kind == OP_IMMEDIATE && code->src2->val_int == 0)
            src = code->src1;
        break;
    case IR_SUB:
        if (code->src1->kind == OP_IMMEDIATE && code->src2->kind == OP_IMMEDIATE) {
            int val = code->src1->val_int - code->src2->val_int;
            src = new(struct ir_operand, OP_IMMEDIATE, OP_MDF_NONE, .val_int=val);
        } else if (code->src2->kind == OP_IMMEDIATE && code->src2->val_int == 0)
            src = code->src1;
        break;
    case IR_MUL:
        if (code->src1->kind == OP_IMMEDIATE && code->src2->kind == OP_IMMEDIATE) {
            int val = code->src1->val_int * code->src2->val_int;
            src = new(struct ir_operand, OP_IMMEDIATE, OP_MDF_NONE, .val_int=val);
        } else if (code->src1->kind == OP_IMMEDIATE && code->src1->val_int == 1)
            src = code->src2;
        else if (code->src2->kind == OP_IMMEDIATE && code->src2->val_int == 1)
            src = code->src1;
        break;
    case IR_DIV:
        if (code->src1->kind == OP_IMMEDIATE && code->src2->kind == OP_IMMEDIATE) {
            int val = code->src1->val_int / code->src2->val_int;
            src = new(struct ir_operand, OP_IMMEDIATE, OP_MDF_NONE, .val_int=val);
        } else if (code->src2->kind == OP_IMMEDIATE && code->src2->val_int == 1)
            src = code->src1;
        break;
    default:
        break;
    }
    if (src) {
        struct ir_code *t = code;
        code = new(struct ir_code, IR_ASSIGN, .dst=t->dst, .src=src);
        free(t);
    }
    // optimizing end
    if (ir_list == NULL) {
        ir_list = ir_list_tail = code;
        code->prev = code->next = NULL;
    } else {
        ir_list_tail->next = code;
        code->prev = ir_list_tail;
        ir_list_tail = code;
    }
}

struct ir_operand *ir_clean_temp_var(struct ir_operand *op_temp) {
    assert(op_temp->kind == OP_TEMP_VAR);
    if (ir_list_tail->kind == IR_ASSIGN && ir_list_tail->dst == op_temp) {
        free(op_temp);
        struct ir_code *p = ir_list_tail;
        struct ir_operand *ret = p->src;
        ir_list_tail = ir_list_tail->prev;
        ir_list_tail->next = NULL;
        free(p);
        //count_temp_var--;
        return ret;
    }
    return op_temp;
}

void ir_clean_assign() {
    assert(ir_list_tail->kind == IR_ASSIGN && ir_list_tail->prev);
    struct ir_code *prev = ir_list_tail->prev, *this = ir_list_tail;
    switch (prev->kind) {
    case IR_ASSIGN:
    case IR_ADD:
    case IR_SUB:
    case IR_MUL:
    case IR_DIV:
    case IR_CALL:
    case IR_READ:
        if (prev->op != this->src) return;
        // this code: I don't know why, but the IRSIM does not support it...
        if (this->op->modifier == OP_MDF_STAR) return;
        prev->op = this->op;
        ir_list_tail = prev;
        prev->next = NULL;
        free(this);
        //count_temp_var--;
        return;
    default:
        return;
    }
}
