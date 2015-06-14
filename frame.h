#ifndef __FRAME_H
#define __FRAME_H

#include "asm_mips.h"

static struct frame_info {
    struct local_var {
        enum op_type kind; // OP_VARIABLE or OP_TEMP_VAR
        int no;
        int offset; // offset in stack relative to FP
        struct local_var *next;
    } *variables;
    int var_offset;
    int arg_offset;
} *frame;

static struct local_var *add_local_var(enum op_type kind, int no) {
    assert(frame != NULL);
    struct local_var *var = new(struct local_var, .kind=kind, .no=no);
    var->offset = frame->var_offset; // -8, -12, -16...
    var->next = frame->variables;
    frame->variables = var;
    frame->var_offset -= 4;
    return var;
}

static struct local_var *add_arg_var(enum op_type kind, int no) {
    assert(frame != NULL);
    struct local_var *var = new(struct local_var, .kind=kind, .no=no);
    var->offset = frame->arg_offset; // 4, 8, 12...
    var->next = frame->variables;
    frame->variables = var;
    frame->arg_offset += 4;
    return var;
}

static struct local_var *dec_local_var(enum op_type kind, int no, int size) {
    assert(frame != NULL);
    struct local_var *var = new(struct local_var, .kind=kind, .no=no);
    frame->arg_offset += size;
    var->offset = frame->arg_offset - 4;
    var->next = frame->variables;
    frame->variables = var;
    return var;
}

static struct local_var *find_variable(enum op_type kind, int no) {
    assert(frame != NULL);
    for (struct local_var *p = frame->variables; p; p = p->next) {
        if (p->kind == kind && p->no == no) return p;
    }
    return NULL;
}

static int get_offset(enum op_type kind, int no) {
    struct local_var *var = find_variable(kind, no);
    if (var) return var->offset;
    else return -1; // -1 means variable not found
}

static void release_frame() {
    assert(frame != NULL);
    while (frame->variables) {
        struct local_var *t = frame->variables;
        frame->variables = t->next;
        free(t);
    }
    free(frame);
    frame = NULL;
}

static void new_frame() {
    frame = new(struct frame_info, NULL, -8, 4);
}

#endif
