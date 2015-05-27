#ifndef _TRANSLATE_H
#define _TRANSLATE_H

#include "common.h"
#include "ir.h"
#include "ast.h"

struct ir_list *translate_exp(struct ast_node *root, struct ir_operand *op);
struct ir_list *translate_exp_cond(struct ast_node *root, \
        struct ir_operand *label_true, struct ir_operand *label_false);

#endif
