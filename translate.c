#include "translate.h"
#include "symbol_table.h"
#include "syntax.tab.h"

#define child(node, n) \
    (n == 0 ? node->child : get_nth_child_ast_node(node, n))
inline int child_num(struct ast_node *node) {
    int n = 0;
    while (child(node, n)) n++;
    return n;
}
#define CAT(a,b) concat_ir_list(a,b)
#define NEW_CODE(...) create_ir_list(new(struct ir_code, __VA_ARGS__))

struct ir_operand imme_zero = {OP_IMMEDIATE, .val_int=0};
struct ir_operand imme_one  = {OP_IMMEDIATE, .val_int=1};

extern bool no_error;

enum relop_type get_relop(struct ast_node *root) {
    assert(root->symbol == RELOP);
    if (strcmp(root->value.str_value, "==") == 0) return RELOP_EQ;
    if (strcmp(root->value.str_value, "!=") == 0) return RELOP_NEQ;
    if (strcmp(root->value.str_value, ">" ) == 0) return RELOP_G;
    if (strcmp(root->value.str_value, "<" ) == 0) return RELOP_L;
    if (strcmp(root->value.str_value, ">=") == 0) return RELOP_GE;
    if (strcmp(root->value.str_value, "<=") == 0) return RELOP_LE;
    perror("Invalid rel-op string");
    exit(1);
}

enum relop_type get_not_relop(struct ast_node *root) {
    enum relop_type r = get_relop(root);
    switch (r) {
    case RELOP_EQ:  return RELOP_NEQ;
    case RELOP_NEQ: return RELOP_EQ;
    case RELOP_L:   return RELOP_GE;
    case RELOP_GE:  return RELOP_L;
    case RELOP_G:   return RELOP_LE;
    case RELOP_LE:  return RELOP_G;
    }
    exit(2);
}

struct ir_list *translate(struct ast_node *root) {
    assert(root != NULL);
    if (!no_error) return NULL;
    switch (root->symbol) {
    case Exp:
        return translate_exp(root, NULL);
    case Stmt:
        return translate_stmt(root);
    case FunDec:
        return translate_fun_dec(root);
    case Dec:
        return translate_dec(root);
    default:
        return NULL;
    }
}

struct ir_list *translate_args(struct ast_node *root, struct ir_list **ir_list_args) {
    struct ir_operand *t = new_temp_var();
    struct ir_list *code, *code2;
    if (child_num(root) == 1) { // Exp
        code = translate_exp(child(root, 0), t);
        *ir_list_args = NEW_CODE(IR_ARG, .op=t);
        return code;
    } else { // Exp COMMA Args
        code = translate_exp(child(root, 0), t);
        struct ir_list *other_args;
        code2 = translate_args(child(root, 2), &other_args);
        *ir_list_args = CAT(NEW_CODE(IR_ARG, .op=t), other_args);
        return CAT(code, code2);
    }
};

struct ir_operand *translate_exp_lval(struct ast_node *root) {
    assert(root->symbol == Exp);
    assert(child(root, 0)->symbol == ID && child(root, 1) == NULL);
    struct ast_node* id_node = child(root ,0);
    return find_symbol(id_node->value.str_value)->operand;
}

struct ir_list *translate_exp(struct ast_node *root, struct ir_operand *op) {
    assert(root->symbol == Exp);
    struct ir_operand *t1, *t2, *label1;
    struct ir_list *code1, *code2;
    struct symbol *s;
    enum ir_type ir_type = -1;
    switch (child_num(root)) {
    case 3:
        if (child(root, 2)->symbol == Exp) {
            switch (child(root, 1)->symbol) {
            case ASSIGNOP:
                t1 = translate_exp_lval(child(root, 0));
                t2 = new_temp_var();
                code1 = translate_exp(child(root, 2), t2);
                return CAT(NEW_CODE(IR_ASSIGN, .dst=t1, .src=t2),\
                        op ? NEW_CODE(IR_ASSIGN, .dst=op, .src=t1):NULL);
            case PLUS:
                ir_type = ((int)ir_type == -1 ? IR_ADD : -1);
            case MINUS:
                ir_type = ((int)ir_type == -1 ? IR_SUB : -1);
            case STAR:
                ir_type = ((int)ir_type == -1 ? IR_MUL : -1);
            case DIV:
                ir_type = ((int)ir_type == -1 ? IR_DIV : -1);
                code1 = translate_exp(child(root, 0), t1 = new_temp_var());
                code2 = translate_exp(child(root, 2), t2 = new_temp_var());
                return CAT(CAT(code1, code2), \
                        op ? NEW_CODE(ir_type, .dst=op, .src1=t1, .src2=t2):NULL);
            case AND:
            case OR:
            case RELOP:
CALL_TRANSLATE_EXP_COND:
                code1 = translate_exp_cond(root, NULL, label1 = new_label());
                return CAT(CAT(CAT(
                        op ? NEW_CODE(IR_ASSIGN, .dst=op, .src=&imme_zero):NULL,\
                        code1),\
                        op ? NEW_CODE(IR_ASSIGN, .dst=op, .src=&imme_one):NULL),\
                        NEW_CODE(IR_LABEL, .op=label1));
            }
        } else if (child(root, 1)->symbol == Exp) { // LP Exp RP
            return translate_exp(child(root, 1), op);
        } else if (child(root, 2)->symbol == RP) { // ID LP RP
            s = find_symbol(child(root, 0)->value.str_value);
            if (strcmp(s->name, "read") == 0) {
                return NEW_CODE(IR_READ, .op=op ? op:(t1 = new_temp_var()));
            } else {
                struct ir_operand *func = new(struct ir_operand, OP_FUNCTION, .name=s->name);
                return NEW_CODE(IR_CALL, .func=func, .ret=op ? op:(t1 = new_temp_var()));
            }
        } else if (child(root, 2)->symbol == ID) {
            perror("Struct is not supported.");
            exit(1);
        }
    case 4:
        if (child(root, 0)->symbol == ID) { // ID LP Args RP
            struct ir_list *ir_list_args;
            s = find_symbol(child(root, 0)->value.str_value);
            code1 = translate_args(child(root, 2), &ir_list_args);
            if (strcmp(s->name, "write") == 0) {
                struct ir_operand *arg_op = ir_list_args->head->code->op;
                return CAT(code1, NEW_CODE(IR_WRITE, .op=arg_op));
            } else {
                return CAT(CAT(code1, ir_list_args),\
                        NEW_CODE(IR_CALL, .ret=op ? op:(t1 = new_temp_var()), .func=s->operand));
            }
        } else { // Exp LB Exp RB
            perror("Array is not supported.");
            exit(1);
        }
    case 1:
        switch (child(root, 0)->symbol) {
            int value;
        case ID:
            s = find_symbol(child(root, 0)->value.str_value);
            return op ? NEW_CODE(IR_ASSIGN, .dst=op, .src=s->operand):NULL;
        case INT:
            value = child(root, 0)->value.int_value;
            t1 = new(struct ir_operand, OP_IMMEDIATE, .val_int=value);
            return  op ? NEW_CODE(IR_ASSIGN, .dst=op, .src=t1):NULL;
        case FLOAT:
            perror("Float value is not supported.");
            exit(1);
        }
    case 2:
        if (child(root, 0)->symbol == MINUS) { // MINUS Exp
            code1 = translate_exp(child(root, 1), t1 = new_temp_var());
            return CAT(code1, \
                    op ? NEW_CODE(IR_SUB, .dst=op, .src1=&imme_zero, .src2=t1):NULL);
        } else { // NOT Exp
            goto CALL_TRANSLATE_EXP_COND;
        }
    default:
        exit(2);
    }
}

struct ir_list *translate_exp_cond(struct ast_node *root, \
        struct ir_operand *label_true, struct ir_operand *label_false) {
    assert(root->symbol == Exp);
    int n = child_num(root);
    struct ir_operand *t1, *t2, *label1;
    struct ir_list *code1, *code2;
    if (n == 3 && child(root, 1)->symbol == RELOP) {
        code1 = translate_exp(child(root, 0), t1 = new_temp_var());
        code2 = translate_exp(child(root, 2), t2 = new_temp_var());
        if (label_true && label_false) {
            return CAT(NEW_CODE(IR_IF_GOTO, .src1=t1, .src2=t2,\
                            .relop=get_relop(child(root, 1)), .dst=label_true),\
                       NEW_CODE(IR_GOTO, .dst=label_false));
        } else if (label_true) {
            return NEW_CODE(IR_IF_GOTO, .src1=t1, .src2=t2,\
                            .relop=get_relop(child(root, 1)), .dst=label_true);
        } else if (label_false) {
            return NEW_CODE(IR_IF_GOTO, .src1=t1, .src2=t2,\
                            .relop=get_not_relop(child(root, 1)), .dst=label_false);
        } else return NULL;
    } else if (n == 3 && child(root, 1)->symbol == AND) {
        if (label_false) {
            code1 = translate_exp_cond(child(root, 0), NULL, label_false);
            code2 = translate_exp_cond(child(root, 2), label_true, label_false);
            return CAT(code1, code2);
        } else {
            code1 = translate_exp_cond(child(root, 0), NULL, label1 = new_label());
            code2 = translate_exp_cond(child(root, 2), label_true, label_false);
            return CAT(CAT(code1, code2), NEW_CODE(IR_LABEL, .op=label1));
        }
    } else if (n == 3 && child(root, 1)->symbol == OR) {
        if (label_true) {
            code1 = translate_exp_cond(child(root, 0), label_true, NULL);
            code2 = translate_exp_cond(child(root, 2), label_true, label_false);
            return CAT(code1, code2);
        } else {
            code1 = translate_exp_cond(child(root, 0), label1 = new_label(), NULL);
            code2 = translate_exp_cond(child(root, 2), label_true, label_false);
            return CAT(CAT(code1, code2), NEW_CODE(IR_LABEL, .op=label1));
        }
    } else if (n == 2 && child(root, 0)->symbol == NOT) {
        return translate_exp_cond(child(root, 1), label_false, label_true);
    } else {
        code1 = translate_exp(root, t1 = new_temp_var());
        if (label_true && label_false) {
            code2 = NEW_CODE(IR_IF_GOTO, .src1=t1, .src2=&imme_zero,\
                        .relop=RELOP_NEQ, .dst=label_true);
            return CAT(CAT(code1, code2), NEW_CODE(IR_GOTO, .op=label_false));
        } else if (label_true) {
            code2 = NEW_CODE(IR_IF_GOTO, .src1=t1, .src2=&imme_zero,\
                        .relop=RELOP_NEQ, .dst=label_true);
            return CAT(code1, code2);
        } else if (label_false) {
            code2 = NEW_CODE(IR_IF_GOTO, .src1=t1, .src2=&imme_zero,\
                        .relop=RELOP_EQ, .dst=label_false);
            return CAT(code1, code2);
        } else return NULL;
    }
}

struct ir_list *translate_stmt(struct ast_node *root) {
    assert(root->symbol == Stmt);
    struct ir_operand *t1, *label1, *label2;
    struct ir_list *code1, *code2, *code3;
    switch (root->child->symbol) {
    case Exp:
        return translate_exp(child(root, 0), NULL);
    case CompSt:
        return translate(child(root, 0));
    case RETURN:
        code1 = translate_exp(child(root, 1), t1 = new_temp_var());
        return CAT(code1, NEW_CODE(IR_RETURN, .op=t1));
    case IF:
        if (child_num(root) == 5) { // IF LP Exp RP Stmt
            code1 = translate_exp_cond(child(root, 2), NULL, label1 = new_label());
            code2 = translate_stmt(child(root, 4));
            return CAT(CAT(code1, code2), NEW_CODE(IR_LABEL, .op=label1));
        } else { // IF LP Exp RP Stmt ELSE Stmt
            code1 = translate_exp_cond(child(root, 2), NULL, label1 = new_label());
            code2 = translate_stmt(child(root, 4));
            code3 = translate_stmt(child(root, 6));
            label2 = new_label();
            return CAT(CAT(CAT(CAT(CAT(code1, code2),\
                    NEW_CODE(IR_GOTO, .op=label2)),\
                    NEW_CODE(IR_LABEL, .op=label1)),\
                    code3),\
                    NEW_CODE(IR_LABEL, .op=label2));
        }
    case WHILE: // WHILE LP Exp RP Stmt
        code1 = translate_exp_cond(child(root, 2), NULL, label2 = new_label());
        code2 = translate_stmt(child(root, 4));
        label1 = new_label();
        return CAT(CAT(CAT(CAT(NEW_CODE(IR_LABEL, .op=label1),\
                code1), code2),\
                NEW_CODE(IR_GOTO, .op=label1)),\
                NEW_CODE(IR_LABEL, .op=label2));
    default:
        exit(2);
    }
}

struct ir_list *translate_var_list(struct ast_node *root) {
    assert(root->symbol == VarList);
    char *id = child(root->child, 1)->child->value.str_value;
    struct ir_operand *op_var = find_symbol(id)->operand;
    struct ir_list *code = NEW_CODE(IR_PARAM, .op=op_var);
    if (child_num(root) == 3) {
        code = CAT(code, translate_var_list(child(root, 2)));
    }
    return code;
}

struct ir_list *translate_fun_dec(struct ast_node *root) {
    assert(root->symbol == FunDec);
    char *name = child(root, 0)->value.str_value;
    struct ir_operand *func = new(struct ir_operand, OP_FUNCTION, .name=name);
    struct ir_list *code = NEW_CODE(IR_FUNCTION, .op=func);
    if (child_num(root) == 4) {
        code = CAT(code, translate_var_list(child(root, 2)));
    }
    return code;
}

struct ir_list *translate_dec(struct ast_node *root) {
    assert(root->symbol == Dec);
    if (child_num(root) == 1) return NULL;
    // VarDec ASSIGNOP Exp
    assert(child(root, 0)->child->symbol == ID); // Only ID can be assigned
    char *name = child(root, 0)->child->value.str_value;
    struct ir_operand *op = find_symbol(name)->operand;
    return translate_exp(child(root, 2), op);
}
