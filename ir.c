#include <stdio.h>
#include "ir.h"
#include "common.h"

static int count_temp_var = 1;
static int count_variable = 1;
static int count_label    = 1;

struct ir_operand *new_temp_var() {
    struct ir_operand *t = new(struct ir_operand);
    t->kind = OP_TEMP_VAR;
    t->no = count_temp_var++;
    return t;
}

struct ir_operand *new_variable() {
    struct ir_operand *t = new(struct ir_operand);
    t->kind = OP_VARIABLE;
    t->no = count_variable++;
    return t;
}

struct ir_operand *new_label() {
    struct ir_operand *t = new(struct ir_operand);
    t->kind = OP_LABEL;
    t->no = count_label++;
    return t;
}

inline struct ir_list *create_ir_list(struct ir_code *code) {
    struct ir_node *node = new(struct ir_node, code, NULL);
    return new(struct ir_list, node, node);
}

void add_ir_code(struct ir_list *list, struct ir_code *code) {
    struct ir_node *node = new(struct ir_node);
    node->code = code;
    node->next = NULL;
    if (list->head == NULL) {
        list->head = list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
}

struct ir_list *concat_ir_list(struct ir_list *a, struct ir_list *b) {
    if (a == NULL) return b;
    if (b == NULL) return a;
    if (a->head == NULL || a->tail == NULL) {
        free(a);
        return b;
    }
    if (b->head == NULL || b->tail == NULL) {
        free(b);
        return a;
    }
    a->tail->next = b->head;
    a->tail = b->tail;
    free(b);
    return a;
}

static FILE *out;

static char *get_operand_str(struct ir_operand *op) {
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

void print_ir_code(struct ir_code *code) {
#define OP(op) get_operand_str(code->op)
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
    fprintf(out, "\n");
}

void print_ir_list(struct ir_list *list, FILE *fp) {
    if (list == NULL) return;
#ifndef IR_DEBUG
    out = fp;
#else
    out = stdout;
#endif
    struct ir_node *p = list->head;
    while (p != NULL) {
        print_ir_code(p->code);
        p = p->next;
    }
}
