#include "block_opt.h"

static void insert_dag_node(struct dag_node_list **phead, struct dag_node* node) {
    if (*phead == NULL) {
        *phead = new(struct dag_node_list, node, NULL);
    } else {
        struct dag_node_list *t = new(struct dag_node_list, node, *phead);
        *phead = t;
    }
}

#define new_node(_op, _tp, ...) ({ \
    struct dag_node *__n=(struct dag_node*)malloc(sizeof(struct dag_node));\
    memset((void*)__n, 0, sizeof(struct dag_node));\
    *__n = (struct dag_node){.op=_op, .type=_tp, __VA_ARGS__};\
    __n; })

#define LEAF_NODE -1

static struct map_operand_node *map_head;

bool operand_equal(struct ir_operand *a, struct ir_operand *b) {
    return a->kind == b->kind && \
           a->modifier == b->modifier && \
           a->no == b->no;
}

static struct dag_node *find_dag_node(struct ir_operand *op) {
    struct map_operand_node *p;
    for (p = map_head; p; p = p->next) {
        if (operand_equal(op, p->op)) return p->node;
    }
    return NULL;
}

static void map(struct ir_operand *op, struct dag_node *node) {
    struct map_operand_node *p;
    for (p = map_head; p; p = p->next) {
        if (operand_equal(op, p->op)) {
            p->node = node;
            return;
        }
    }
    struct map_operand_node *m = new(struct map_operand_node, op, node, NULL);
    if (map_head == NULL) {
        map_head = m;
    } else {
        m->next = map_head;
        map_head = m;
    }
}

static void clean_map() {
    while (map_head) {
        struct map_operand_node *p = map_head;
        map_head = map_head->next;
        free(p);
    }
}

struct ir_code *do_block_optimize(struct ir_code *head, struct ir_code *tail) {
    if (head == tail) return tail;
    struct ir_code *code = head, *block_next = tail->next;
    struct dag_node *node, *node1, *node2;
    tail->next = NULL;
    for (; code; tail = code, code = code->next) {
        // accept only these kinds of code
        switch (code->kind) {
        case IR_READ:
            if (code->op->modifier == OP_MDF_NONE) {
                node = new_node(code->op, LEAF_NODE);
                map(code->op, node);
            }
        case IR_LABEL:
        case IR_FUNCTION:
        case IR_GOTO:
        case IR_IF_GOTO:
        case IR_RETURN:
        case IR_PARAM:
        case IR_WRITE:
            goto CONTINUE;
        case IR_DEC:
        case IR_ARG:
        case IR_CALL:
            continue;
        case IR_ASSIGN:
        case IR_ADD:
        case IR_SUB:
        case IR_MUL:
        case IR_DIV:
            break;
        }
        // skip any code with address-access operation
        if (code->op->modifier == OP_MDF_STAR || \
                (code->src->modifier == OP_MDF_STAR) || \
                (code->src2 && code->src2->modifier == OP_MDF_STAR))
            goto CONTINUE;

        int reusable = true;
        if ((node1 = find_dag_node(code->src))) {
            code->src = node1->op;
        } else {
            node1 = new_node(code->src, LEAF_NODE);
            map(code->src, node1);
            reusable = false;
        }
        if (code->src2) { // op src1 src2
            if ((node2 = find_dag_node(code->src2))) {
                code->src2 = node2->op;
            } else {
                node2 = new_node(code->src2, LEAF_NODE);
                map(code->src2, node2);
                reusable = false;
            }
            node = NULL;
            if (reusable) {
                // find shared parent. low efficient...
                struct dag_node_list *p, *q;
                for (p = node1->parent_left; p; p = p->next)
                    for (q = node2->parent_right; q; q = q->next)
                        if (p->node == q->node && p->node->type == code->kind) {
                            node = p->node;
                            goto FOUND_SHARED_PARENT;
                        }
            }
FOUND_SHARED_PARENT:
            if (node) {
                struct ir_code *temp = code;
                if (flag_verbose)
                    printf("OPT: %s ---> %s\n", get_operand_str(temp->op), \
                            get_operand_str(node->op));
                map(temp->op, node);
                code = code->prev;
                free(remove_ir_code(temp));
            } else {
                node = new_node(code->op, code->kind);
                map(code->op, node);
                insert_dag_node(&node1->parent_left, node);
                insert_dag_node(&node2->parent_right, node);
            }
        } else { // op src
            node = NULL;
            if (reusable) {
                struct dag_node_list *p;
                for (p = node1->parent_assign; p; p = p->next)
                    if (p->node->type == code->kind) {
                        node = p->node;
                        break;
                    }
            }
            if (node) {
                struct ir_code *temp = code;
                if (flag_verbose)
                    printf("OPT: %s ---> %s\n", get_operand_str(temp->op), \
                            get_operand_str(node->op));
                map(temp->op, node);
                code = code->prev;
                free(remove_ir_code(temp));
            } else {
                node = new_node(code->op, code->kind);
                map(code->op, node);
                insert_dag_node(&node1->parent_assign, node);
            }
        }
CONTINUE:
        // if t1 --> t2, *t1 --> *t2
        if (code->src && code->src->modifier == OP_MDF_STAR) {
            code->src->modifier = OP_MDF_NONE;
            if ((node = find_dag_node(code->src))) {
                code->src = new(struct ir_operand, node->op->kind, .no=node->op->no);
            }
            code->src->modifier = OP_MDF_STAR;
        }
        if (code->src2 && code->src2->modifier == OP_MDF_STAR) {
            code->src2->modifier = OP_MDF_NONE;
            if ((node = find_dag_node(code->src2))) {
                code->src2 = new(struct ir_operand, node->op->kind, .no=node->op->no);
            }
            code->src2->modifier = OP_MDF_STAR;
        }
        if (code->op && code->op->modifier == OP_MDF_STAR) {
            code->op->modifier = OP_MDF_NONE;
            if ((node = find_dag_node(code->op))) {
                code->op = new(struct ir_operand, node->op->kind, .no=node->op->no);
            }
            code->op->modifier = OP_MDF_STAR;
        }
    }
    // if some veriables are mapped, store them at the end
    struct map_operand_node *iter;
    for (iter = map_head; iter; iter = iter->next) {
        if (iter->op->kind == OP_VARIABLE && !operand_equal(iter->op, iter->node->op)) {
            struct ir_code *code = new(struct ir_code, IR_ASSIGN, \
                    .dst=iter->op, .src=iter->node->op, .prev=NULL, .next=NULL);
            tail = insert_ir_code_before(tail, code)->next;
        }
    }
    tail->next = block_next;
    clean_map();
    return tail;
}

void block_optimize() {
    struct ir_code *show;
    struct ir_code *p = ir_list, *head = ir_list;
    int block_size = 0;
    for (; p; p = p->next) {
        switch (p->kind) {
        case IR_FUNCTION:
        case IR_LABEL:
        case IR_GOTO:
        case IR_IF_GOTO:
        case IR_RETURN:
        case IR_CALL:
            block_size++;
            if (block_size >= 2) {
                if (flag_verbose) {
                    printf("==== BEFORE OPTIMIZING ==== (%d)\n", block_size);
                    for (show = head; show != p->next; show = show->next)
                        print_ir_code(stdout, show);
                    printf("==== AFTER  OPTIMIZING ==== \n");
                }
                p = do_block_optimize(head, p);
                if (flag_verbose) {
                    for (show = head; show != p->next; show = show->next)
                        print_ir_code(stdout, show);
                }
            }
            block_size = 0;
            head = p->next;
            break;
        default:
            block_size++;
            break;
        }
    }
    if (flag_verbose)
        printf("=========================== \n");
}

