#ifndef _TRANSLATE_H
#define _TRANSLATE_H

#include "common.h"
#include "ir.h"
#include "ast.h"

struct ir_list *translate(struct ast_node *root);
struct ir_list *translate_args(struct ast_node *root, struct ir_list **ir_list_args);
struct ir_operand *translate_exp_lval(struct ast_node *root);
struct ir_list *translate_exp(struct ast_node *root, struct ir_operand *op);
struct ir_list *translate_exp_cond(struct ast_node *root, \
        struct ir_operand *label_true, struct ir_operand *label_false);
struct ir_list *translate_stmt(struct ast_node *root);
struct ir_list *translate_var_list(struct ast_node *root);
struct ir_list *translate_fun_dec(struct ast_node *root);
struct ir_list *translate_dec(struct ast_node *root);

#endif
