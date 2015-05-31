#ifndef _BLOCK_OPT_H
#define _BLOCK_OPT_H

#include "ir.h"

struct dag_node {
    struct ir_operand *op;
    enum ir_type type;
    struct dag_node_list *parent_left, *parent_right, *parent_assign;
};

struct dag_node_list {
    struct dag_node *node;
    struct dag_node_list *next;
};

struct map_operand_node {
    struct ir_operand *op;
    struct dag_node *node;
    struct map_operand_node *next;
};

void block_optimize();

#endif
