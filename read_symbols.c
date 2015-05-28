#include "read_symbols.h"
#include "syntax.tab.h"
#include "symbol_table.h"
#include "error.h"

#define child(node, n) \
    (n == 0 ? node->child : get_nth_child_ast_node(node, n))
inline int child_num(struct ast_node *node) {
    int n = 0;
    while (child(node, n)) n++;
    return n;
}
#define CODE(...) add_ir_code(new(struct ir_code, __VA_ARGS__))

struct var_type *func_ret_type;

#define VAR_TYPE_STR_SIZE 1024

struct ir_operand imme_zero = {OP_IMMEDIATE, .val_int=0};
struct ir_operand imme_one  = {OP_IMMEDIATE, .val_int=1};
struct ir_operand imme_four  = {OP_IMMEDIATE, .val_int=4};

#define IMME_OP(value) new(struct ir_operand, OP_IMMEDIATE, \
        OP_MDF_NONE, .val_int=value)

struct ir_operand *modify_operator(struct ir_operand *op, enum modifier_type mod) {
    assert(op->kind == OP_VARIABLE || op->kind == OP_TEMP_VAR);
    struct ir_operand *op_mod = new(struct ir_operand);
    *op_mod = *op;
    switch (op->modifier) {
    case OP_MDF_NONE:
        op_mod->modifier = mod;
        break;
    case OP_MDF_AND:
        assert(mod == OP_MDF_STAR);
        op_mod->modifier = OP_MDF_NONE;
        break;
    case OP_MDF_STAR:
        assert(mod == OP_MDF_AND);
        op_mod->modifier = OP_MDF_NONE;
        break;
    }
    return op_mod;
}

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

#pragma GCC diagnostic ignored "-Wswitch"
int get_var_type_size(struct var_type *vt) {
    assert(vt->kind != FUNCTION);
    int size;
    switch (vt->kind) {
    case BASIC:
        return 4;
    case ARRAY:
        size = get_var_type_size(vt->array.elem);
        struct array_size_list *p = vt->array.size_list;
        for (; p; p = p->next)
            size *= p->size;
        return size;
    case STRUCTURE:
        size = 0;
        struct field_list *field = vt->struct_type->fields;
        for (; field; field = field->tail) size += get_var_type_size(field->type);
        return size;
    }
    return 0;
}

char* get_var_type_str(struct var_type *vt) {
    char buf[VAR_TYPE_STR_SIZE];
    char vt_str[VAR_TYPE_STR_SIZE * 2];
    if (vt == NULL) {
        static char* unknown_str = "[UNKNOWN]";
        char* ret = (char*) malloc(sizeof(char) * (strlen(unknown_str)+1));
        strcpy(ret, unknown_str);
        return ret;
    }
    switch (vt->kind) {
    case BASIC:
        strcpy(vt_str, vt->basic == INT ? "int" : "float");
        break;
    case ARRAY:
        sprintf(vt_str, "%s", to_free = get_var_type_str(vt->array.elem));
        free(to_free);
        struct array_size_list *sl = vt->array.size_list;
        while (sl != NULL) {
            sprintf(buf, "[%d]", sl->size);
            strcat(vt_str, buf);
            sl = sl->next;
        }
        break;
    case STRUCTURE:
        strcpy(vt_str, "struct { ");
        struct field_list *fl = vt->struct_type->fields;
        while (fl != NULL) {
            sprintf(buf, "%s %s; ", to_free = get_var_type_str(fl->type), fl->name);
            free(to_free);
            strcat(vt_str, buf);
            fl = fl->tail;
        }
        strcat(vt_str, "}");
        break;
    case FUNCTION:
        strcpy(vt_str, to_free = get_var_type_str(vt->func.ret));
        free(to_free);
        strcat(vt_str, " <function>(");
        struct func_param_list *p = vt->func.params;
        while (p) {
            strcat(vt_str, to_free = get_var_type_str(p->type));
            free(to_free);
            if (p->tail) strcat(vt_str, ", ");
            p = p->tail;
        }
        strcat(vt_str, ")");
        break;
    }
    char* ret = (char*) malloc(sizeof(char) * (strlen(vt_str)+1));
    strcpy(ret, vt_str);
    return ret;
}

bool var_type_equal(struct var_type *a, struct var_type *b) {
    if (a == NULL || b == NULL) return false;
    if (a == b) return true;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
    case BASIC:
        return a->basic == b->basic;
    case ARRAY:
        if (!var_type_equal(a->array.elem, b->array.elem)) return false;
        struct array_size_list *sa = a->array.size_list;
        struct array_size_list *sb = b->array.size_list;
        // only 1st dim can be different
        sa = sa->next;
        sb = sb->next;
        while (sa && sb) {
            if (sa->size != sb->size) return false;
            sa = sa->next;
            sb = sb->next;
        }
        if (sa || sb) return false;
        return true;
    case STRUCTURE:
        return a->struct_type == b->struct_type;
    case FUNCTION:
        return false;
    }
    return false;
}

bool params_equal(struct func_param_list *pl_a, struct func_param_list *pl_b) {
    struct func_param_list *p_a = pl_a, *p_b = pl_b;
    while (p_a && p_b) {
        if (!var_type_equal(p_a->type, p_b->type)) return false;
        p_a = p_a->tail;
        p_b = p_b->tail;
    }
    return p_a == NULL && p_b == NULL;
}

int get_field_offset(struct struct_type *st, char* name) {
    struct field_list *head = st->fields;
    int offset = 0;
    while (head != NULL) {
        if (strcmp(head->name, name) == 0) return offset;
        head = head->tail;
        offset += 4;
    }
    exit(4);
}

struct field_list *get_field_list(struct field_list *head, char* name) {
    while (head != NULL) {
        if (strcmp(head->name, name) == 0) return head;
        head = head->tail;
    }
    return NULL;
}

struct var_type *get_field_type(struct struct_type *st, char *name) {
    struct field_list *field = get_field_list(st->fields, name);
    if (field == NULL) return NULL;
    return field->type;
}

struct var_type int_type = { BASIC, {INT} };
struct var_type float_type = { BASIC, {FLOAT} };
struct var_type *get_basic_var_type(char *type_name) {
    if (strcmp(type_name, "int") == 0) {
        return &int_type;
    } else if (strcmp(type_name, "float") == 0) {
        return &float_type;
    } else {
        return NULL;
    }
}

void read_symbols() {
    dfs(ast_root);
}

void dfs(struct ast_node *root) {
    assert(root != NULL); // root can be any type of node
    struct ast_node *child;
    switch (root->symbol) {
    case ExtDef:
        dfs_ext_def(root);
        break;
    case CompSt:
        dfs_comp_st(root);
        break;
    case Exp:
        dfs_exp(root, NULL);
        break;
    case Stmt:
        dfs_stmt(root);
        break;
    default:
        for (child = root->child; child; child = child->peer) {
            dfs(child);
        }
    }
}

void dfs_ext_def(struct ast_node *root) {
    assert(root->symbol == ExtDef);
    struct var_type *var_type = dfs_specifier(child(root, 0));
    if (var_type == NULL) return;
    switch (child(root, 1)->symbol) {
    // ExtDef: Specifier ExtDecList SEMI
    case ExtDecList:
        dfs_ext_dec_list(child(root, 1), var_type);
        break;
    // ExtDef: Specifier SEMI
    case SEMI:
        break;
    // ExtDef: Specifier FunDec CompSt
    // ExtDef: Specifier FunDec SEMI
    case FunDec:
        func_ret_type = var_type;
        push_scope();
        if (child(root, 2)->symbol == CompSt) {
            dfs_fun_dec(child(root, 1), var_type, false);
            dfs_comp_st(child(root, 2));
        } else {
            dfs_fun_dec(child(root, 1), var_type, true);
        }
        pop_scope();
        func_ret_type = NULL;
        break;
    }
}

void dfs_fun_dec(struct ast_node *root, struct var_type *ret, bool dec_only) {
    assert(root->symbol == FunDec);
    struct symbol *symbol;
    struct var_type *vt = new(struct var_type);
    vt->kind = FUNCTION;
    vt->func.params = NULL;
    vt->func.ret = ret;
    char* name = dfs_id(root->child);
    if (child(root, 2)->symbol == VarList) {
        vt->func.params = dfs_var_list(child(root, 2));
    }
    if (dec_only) {
        set_declared_scope();
        if ((symbol = find_symbol(name)) != NULL) {
            if (symbol->type->kind != FUNCTION) {
                print_error(4, root->child, name);
            } else {
                struct func_param_list *pl_dec = symbol->type->func.params;
                struct func_param_list *pl_new = vt->func.params;
                if (!params_equal(pl_dec, pl_new)) {
                    print_error(19, root, name);
                }
                // Well, such boring code
            }
        } else {
            insert_func_symbol(name, vt, root);
        }
        unset_declared_scope();
    } else {
        struct ir_operand *func = new(struct ir_operand, OP_FUNCTION, .name=name);
        CODE(IR_FUNCTION, .op=func);
        if (child(root, 2)->symbol == VarList) {
            struct func_param_list *p = vt->func.params;
            while (p) {
                insert_symbol(p->name, p->type);
                symbol = find_symbol(p->name);
                struct ir_operand *op_var = symbol->operand;
                CODE(IR_PARAM, .op=op_var);
                symbol->operand = modify_operator(op_var, OP_MDF_STAR);
                p = p->tail;
            }
        }
        // find declaration and remove it, if existed
        if ((symbol = find_symbol(name)) && symbol->scope == DECLARED) {
            struct func_param_list *pl_dec = symbol->type->func.params;
            struct func_param_list *pl_new = vt->func.params;
            if (!params_equal(pl_dec, pl_new)) {
                print_error(19, root, name);
            } else {
                delete_symbol(name);
            }
        }
        if (!insert_func_symbol(name, vt, root)) {
            print_error(4, root->child, name);
        }
    }
}

struct func_param_list *dfs_var_list(struct ast_node *root) {
    assert(root->symbol == VarList);
    struct func_param_list *p = new(struct func_param_list);
    p->type = dfs_param_dec(root->child, &p->name);
    if (child_num(root) == 3) {
        p->tail = dfs_var_list(child(root, 2));
    } else {
        p->tail = NULL;
    }
    if (p->type == NULL) {
        free(p);
        return p->tail;
    }
    return p;
}

struct var_type *dfs_param_dec(struct ast_node *root, char **name) {
    assert(root->symbol == ParamDec);
    struct var_type *spec_t = dfs_specifier(root->child);
    if (spec_t == NULL) return NULL;
    if (child(root, 1)) {
        spec_t = dfs_var_dec(child(root, 1), name, spec_t); // array
    }
    return spec_t;
}

void dfs_ext_dec_list(struct ast_node *root, struct var_type *spec_t) {
    assert(root->symbol == ExtDecList);
    char* name;
    struct var_type *t = dfs_var_dec(root->child, &name, spec_t);
    if (!insert_symbol(name, t)) {
        print_error(3, root->child, name);
    }
    if (child(root, 1) != NULL) {
        dfs_ext_dec_list(child(root, 2), spec_t);
    }
}

struct var_type *dfs_var_dec(struct ast_node *root, char **name, struct var_type *vt) {
    assert(root->symbol == VarDec);
    if (root->child->symbol == ID) {
        *name = dfs_id(root->child);
        return vt;
    }
    struct ast_node *node = root;
    struct array_size_list *head = new(struct array_size_list);
    head->size = child(node, 2)->value.int_value;
    head->next = NULL;
    node = node->child;
    while (node->child->symbol == VarDec) {
        struct array_size_list *sl = new(struct array_size_list);
        sl->size = child(node, 2)->value.int_value;
        sl->next = head;
        head = sl;
        node = node->child;
    }
    *name = dfs_id(node->child);
    struct var_type* array_vt = new(struct var_type);
    array_vt->kind = ARRAY;

    array_vt->array.size_list = head;
    array_vt->array.elem = vt;
    return array_vt;
}

struct var_type *dfs_specifier(struct ast_node *root) {
    assert(root->symbol == Specifier);
    struct var_type *p;
    char *name;
    switch (child(root, 0)->symbol) {
    // Specifier: TYPE
    case TYPE:
        name = child(root, 0)->value.str_value;
        if (strcmp(name, "int") == 0) {
            p = &int_type;
        }
        else if (strcmp(name, "float") == 0) {
            p = &float_type;
        }
        else p = NULL;
        break;
    // Specifier: StructSpecifier
    case StructSpecifier:
        p = new(struct var_type);
        p->kind = STRUCTURE;
        p->struct_type = dfs_struct_specifier(root->child);
        if (p->struct_type == NULL) return NULL;
        break;
    }
    return p;
}

struct struct_type *dfs_struct_specifier(struct ast_node *root) {
    assert(root->symbol == StructSpecifier);
    struct struct_type *st;
    char* name;
    switch (child(root, 1)->symbol) {
    // StructSpecifier: STRUCT OptTag LC DefList RC
    case OptTag:
        st = new(struct struct_type);
        if (child(root, 3) && child(root, 3)->symbol == DefList) {
            push_scope();
            st->fields = dfs_def_list(child(root, 3));
            pop_scope();
        } else {
            st->fields = NULL;
        }
        name = dfs_opt_tag(child(root, 1));
        if (!insert_struct_symbol(name, st)) {
            print_error(16, child(root, 1), name);
        }
        break;
    // StructSpecifier: STRUCT LC DefList RC
    case LC:
        st = new(struct struct_type);
        if (child(root, 2) && child(root, 2)->symbol == DefList) {
            push_scope();
            st->fields = dfs_def_list(child(root, 2));
            pop_scope();
        } else {
            st->fields = NULL;
        }
        break;
    // StructSpecifier: STRUCT Tag
    case Tag:
        name = dfs_tag(child(root, 1));
        struct struct_symbol *symbol = find_struct_symbol(name);
        if (symbol != NULL) st = symbol->type;
        else {
            print_error(17, child(root, 1), name);
            return NULL;
        }
        break;
    }
    return st;
}

char* dfs_tag(struct ast_node* root) {
    assert(root->symbol == Tag);
    return dfs_id(root->child);
}

char* dfs_opt_tag(struct ast_node* root) {
    assert(root->symbol == OptTag);
    return dfs_id(root->child);
}

char* dfs_id(struct ast_node* root) {
    assert(root->symbol == ID);
    return root->value.str_value;
}

int dfs_int(struct ast_node* root) {
    assert(root->symbol == INT);
    return root->value.int_value;
}

float dfs_float(struct ast_node* root) {
    assert(root->symbol == FLOAT);
    return root->value.float_value;
}

struct field_list *dfs_def_list(struct ast_node *root) {
    assert(root->symbol == DefList);
    struct field_list *p = dfs_def(root->child);
    if (child(root, 1)) {
        if (p == NULL) {
            p = dfs_def_list(child(root, 1));
        } else {
            struct field_list *tail = p, *prev;
            while (tail) {
                prev = tail;
                tail = tail->tail;
            }
            prev->tail = dfs_def_list(child(root, 1));
        }
    }
    return p;
}

struct field_list *dfs_def(struct ast_node *root) {
    assert(root->symbol == Def);
    struct var_type *vt = dfs_specifier(root->child);
    if (vt == NULL) return NULL;
    return dfs_dec_list(child(root, 1), vt);
}

struct field_list *dfs_dec_list(struct ast_node *root, struct var_type *vt) {
    assert(root->symbol == DecList);
    struct field_list *fl = dfs_dec(root->child, vt);
    if (child(root, 2)) {
        if (fl != NULL) {
            fl->tail = dfs_dec_list(child(root, 2), vt);
        } else {
            fl = dfs_dec_list(child(root, 2), vt);
        }
    }
    return fl;
}

struct field_list *dfs_dec(struct ast_node *root, struct var_type *vt) {
    assert(root->symbol == Dec);
    struct field_list *fl = new(struct field_list);
    fl->type = dfs_var_dec(root->child, &fl->name, vt);
    fl->tail = NULL;
    if (!insert_symbol(fl->name, fl->type)) {
        if (func_ret_type == NULL) { // now in struct
            print_error(15, root->child, fl->name);
        } else { // now in function
            print_error(3, root->child, fl->name);
        }
        free(fl);
        return NULL;
    }
    if (func_ret_type == NULL) { // in struct
        if (child(root, 2)) { // initializing is forbidden
            print_error(20, root);
        }
    } else { // in function
        if (child_num(root) == 1) { // VarDec
            if (fl->type->kind == STRUCTURE || fl->type->kind == ARRAY) {
                struct ir_operand *op = find_symbol(fl->name)->operand;
                CODE(IR_DEC, .op=op, .size=get_var_type_size(fl->type));
            }
        } else { // VarDec ASSIGNOP Exp
            assert(child(root, 0)->child->symbol == ID); // Only ID can be assigned when declared
            struct ir_operand *op = find_symbol(fl->name)->operand;
            struct var_type *exp_type = dfs_exp(child(root, 2), op);
            if (!var_type_equal(exp_type, fl->type)) {
                print_error(5, root);
            }
        }
    }
    return fl;
}

void dfs_comp_st(struct ast_node *root) {
    assert(root->symbol == CompSt);
    if (child(root, 1)->symbol == DefList) {
        dfs_def_list(child(root, 1));
    }
    if (child(root, 2) && child(root,2)->symbol == StmtList) {
        dfs(child(root, 2));
    } else if (child(root, 1) && child(root,1)->symbol == StmtList) {
        dfs(child(root, 1));
    }
}

bool params_args_equal(struct func_param_list *param, struct func_arg_list *arg) {
    while (param && arg && var_type_equal(param->type, arg->var_type)) {
        param = param->tail;
        arg = arg->tail;
    }
    if (param || arg) return false;
    return true;
}

struct var_type *dfs_exp_cond(struct ast_node *root, \
        struct ir_operand *label_true, struct ir_operand *label_false) {
    assert(root->symbol == Exp);
    int n = child_num(root);
    struct ir_operand *t1, *t2, *label1;
    struct var_type *vt, *left, *right;
    if (n == 3 && child(root, 1)->symbol == RELOP) {
        left = dfs_exp(child(root, 0), t1 = new_temp_var());
        t1 = ir_clean_temp_var(t1);
        right = dfs_exp(child(root, 2), t2 = new_temp_var());
        t2 = ir_clean_temp_var(t2);
        if (left == NULL || right == NULL) return NULL;
        if (!var_type_equal(left, right)) {
            print_error(7, root);
            return NULL;
        }
        if (label_true && label_false) {
            CODE(IR_IF_GOTO, .src1=t1, .src2=t2, \
                    .relop=get_relop(child(root, 1)), .dst=label_true);
            CODE(IR_GOTO, .dst=label_false);
        } else if (label_true) {
            CODE(IR_IF_GOTO, .src1=t1, .src2=t2,\
                    .relop=get_relop(child(root, 1)), .dst=label_true);
        } else if (label_false) {
            CODE(IR_IF_GOTO, .src1=t1, .src2=t2,\
                    .relop=get_not_relop(child(root, 1)), .dst=label_false);
        }
        return left;
    } else if (n == 3 && child(root, 1)->symbol == AND) {
        if (label_false) {
            left = dfs_exp_cond(child(root, 0), NULL, label_false);
            right = dfs_exp_cond(child(root, 2), label_true, label_false);
        } else {
            left = dfs_exp_cond(child(root, 0), NULL, label1 = new_label());
            right = dfs_exp_cond(child(root, 2), label_true, label_false);
            CODE(IR_LABEL, .op=label1);
        }
        if (left == NULL || right == NULL) return NULL;
        if (!var_type_equal(left, right)) {
            print_error(7, root);
            return NULL;
        }
        if (left->basic != INT) {
            print_error(7, root);
            return NULL;
        }
        return left;
    } else if (n == 3 && child(root, 1)->symbol == OR) {
        if (label_true) {
            left = dfs_exp_cond(child(root, 0), label_true, NULL);
            right = dfs_exp_cond(child(root, 2), label_true, label_false);
        } else {
            left = dfs_exp_cond(child(root, 0), label1 = new_label(), NULL);
            right = dfs_exp_cond(child(root, 2), label_true, label_false);
            CODE(IR_LABEL, .op=label1);
        }
        if (left == NULL || right == NULL) return NULL;
        if (!var_type_equal(left, right)) {
            print_error(7, root);
            return NULL;
        }
        if (left->basic != INT) {
            print_error(7, root);
            return NULL;
        }
        return left;
    } else if (n == 2 && child(root, 0)->symbol == NOT) {
        vt = dfs_exp_cond(child(root, 1), label_false, label_true);
        if (vt == NULL) return NULL;
        if (vt->kind != BASIC || vt->basic != INT) {
            print_error(7, child(root, 1));
            return NULL;
        }
        return vt;
    } else {
        vt = dfs_exp(root, t1 = new_temp_var());
        t1 = ir_clean_temp_var(t1);
        if (label_true && label_false) {
            CODE(IR_IF_GOTO, .src1=t1, .src2=&imme_zero,\
                        .relop=RELOP_NEQ, .dst=label_true);
            CODE(IR_GOTO, .op=label_false);
        } else if (label_true) {
            CODE(IR_IF_GOTO, .src1=t1, .src2=&imme_zero,\
                        .relop=RELOP_NEQ, .dst=label_true);
        } else if (label_false) {
            CODE(IR_IF_GOTO, .src1=t1, .src2=&imme_zero,\
                        .relop=RELOP_EQ, .dst=label_false);
        }
        return vt;
    }
}

struct var_type *dfs_exp_addr(struct ast_node *root, struct ir_operand *op) {
    assert(root->symbol == Exp);
    char* name;
    struct symbol *symbol;
    struct ir_operand *t1, *t2, *t3, *t4;
    struct var_type *vt1, *vt2;
    switch (child(root, 0)->symbol) {
    case ID: // ID
        name = dfs_id(child(root, 0));
        symbol = find_symbol(name);
        if (symbol == NULL) {
            print_error(1, child(root, 0), name);
            return NULL;
        }
        t1 = symbol->operand;
        t2 = modify_operator(t1, OP_MDF_AND);
        if (op) CODE(IR_ASSIGN, .dst=op, .src=t2);
        return symbol->type;
    case LP: // LP Exp RP
        return dfs_exp_addr(child(root, 1), op);
    case Exp:
        switch (child(root, 1)->symbol) {
        case LB: // Exp LB Exp RB
            vt1 = dfs_exp_addr(child(root, 0), t1 = new_temp_var());
            t1 = ir_clean_temp_var(t1);
            vt2 = dfs_exp(child(root, 2), t2 = new_temp_var());
            t2 = ir_clean_temp_var(t2);
            if (vt2 == NULL || vt2->kind != BASIC || vt2->basic != INT) {
                print_error(12, child(root, 2), get_ast_node_code(child(root, 2)));// TODO
            }
            struct var_type *vt = new(struct var_type);
            *vt = *vt1;
            vt->array.size_list = vt->array.size_list->next;
            if (vt->array.size_list == NULL) {
                struct var_type *temp = vt;
                vt = vt->array.elem;
                free(temp);
                if (t2->kind == OP_IMMEDIATE) {
                    t3 = IMME_OP(4*t2->val_int);
                    if (op) CODE(IR_ADD, .dst=op, .src1=t1, .src2=t3);
                } else {
                    CODE(IR_MUL, .dst=(t3 = new_temp_var()), .src1=t2, .src2=&imme_four);
                    t3 = ir_clean_temp_var(t3);
                    if (op) CODE(IR_ADD, .dst=op, .src1=t1, .src2=t3);
                }
            } else {
                int dim_size = 4;
                struct array_size_list *p = vt->array.size_list;
                while (p) {
                    dim_size *= p->size;
                    p = p->next;
                }
                t3 = new_temp_var();
                t4 = IMME_OP(dim_size);
                CODE(IR_MUL, .dst=t3, .src1=t2, .src2=t4);
                t3 = ir_clean_temp_var(t3);
                if (op) CODE(IR_ADD, .dst=op, .src1=t3, .src2=t1);
            }
            return vt;
        case DOT: // Exp DOT ID
            vt = dfs_exp_addr(child(root, 0), t1 = new_temp_var());
            t1 = ir_clean_temp_var(t1);
            if (vt == NULL) return NULL;
            if (vt->kind != STRUCTURE) {
                print_error(13, child(root, 0));
                return NULL;
            }
            char* name = dfs_id(child(root, 2));
            struct var_type *field_vt = get_field_type(vt->struct_type, name);
            if (field_vt == NULL) {
                print_error(14, child(root, 2), name);
                return NULL;
            }
            int offset = get_field_offset(vt->struct_type, name);
            t2 = IMME_OP(offset);
            if (op) CODE(IR_ADD, .dst=op, .src1=t1, .src2=t2);
            return field_vt;
        }
    }
    return NULL;
}

struct var_type *dfs_exp(struct ast_node *root, struct ir_operand *op) {
    assert(root->symbol == Exp);
    char* name;
    struct symbol *symbol;
    struct var_type *vt, *left, *right;
    struct ir_operand *t1, *t2, *t3, *label1;
    enum ir_type ir_type;
    switch (child_num(root)) {
    int value;
    case 1:
        switch (child(root, 0)->symbol) {
        case ID:
            name = dfs_id(child(root, 0));
            symbol = find_symbol(name);
            if (symbol == NULL) {
                print_error(1, child(root, 0), name);
                return NULL;
            }
            if (op) CODE(IR_ASSIGN, .dst=op, .src=symbol->operand);
            return symbol->type;
        case INT:
            value = dfs_int(child(root, 0));
            t1 = IMME_OP(value);
            if (op) CODE(IR_ASSIGN, .dst=op, .src=t1);
            return &int_type;
        case FLOAT:
            dfs_float(child(root, 0));
            perror("Float value is not supported.");
            exit(1);
            return &float_type;
        }
    case 2:
        switch (child(root, 0)->symbol) {
        case MINUS:
            vt = dfs_exp(child(root, 1), t1 = new_temp_var());
            t1 = ir_clean_temp_var(t1);
            if (vt == NULL) return NULL;
            if (vt->kind != BASIC) {
                print_error(7, child(root, 1));
                return NULL;
            }
            if (op) CODE(IR_SUB, .dst=op, .src1=&imme_zero, .src2=t1);
            return vt;
        case NOT:
            if (op) CODE(IR_ASSIGN, .dst=op, .src=&imme_zero);
            vt = dfs_exp(child(root, 1), label1 = new_label());
            if (vt == NULL) return NULL;
            if (vt->kind != BASIC || vt->basic != INT) {
                print_error(7, child(root, 1));
                return NULL;
            }
            if (op) CODE(IR_ASSIGN, .dst=op, .src=&imme_one);
            CODE(IR_LABEL, .op=label1);
            return vt;
        }
    case 3:
        switch (child(root, 1)->symbol) {
        case Exp: // LP Exp RP
            return dfs_exp(child(root, 1), op);
        case ASSIGNOP:
            left = dfs_exp_addr(child(root, 0), t1 = new_temp_var());
            t1 = ir_clean_temp_var(t1);
            right = dfs_exp(child(root, 2), t2 = new_temp_var());
            t2 = ir_clean_temp_var(t2);
            if (left == NULL || right == NULL) return NULL;
            if (!var_type_equal(left, right)) {
                print_error(5, root);
                return NULL;
            }
            t3 = modify_operator(t1, OP_MDF_STAR);
            CODE(IR_ASSIGN, .dst=t3, .src=t2);
            // optimizing
            ir_clean_assign();
            if (op) CODE(IR_ASSIGN, .dst=op, .src=t3);
            return left;
        case DOT:
            vt = dfs_exp_addr(root, t1=new_temp_var());
            t1 = ir_clean_temp_var(t1);
            if (vt == NULL) return NULL;
            t2 = modify_operator(t1, OP_MDF_STAR);
            if (op) CODE(IR_ASSIGN, .dst=op, .src=t2);
            return vt;
        case LP:
            goto ID_LP_RP;
        case PLUS:
            ir_type = IR_ADD; goto ADD_SUB_MUL_DIV;
        case MINUS:
            ir_type = IR_SUB; goto ADD_SUB_MUL_DIV;
        case STAR:
            ir_type = IR_MUL; goto ADD_SUB_MUL_DIV;
        case DIV:
            ir_type = IR_DIV; goto ADD_SUB_MUL_DIV;
ADD_SUB_MUL_DIV:
            left = dfs_exp(child(root, 0), t1 = new_temp_var());
            t1 = ir_clean_temp_var(t1);
            right = dfs_exp(child(root, 2), t2 = new_temp_var());
            t2 = ir_clean_temp_var(t2);
            if (op) CODE(ir_type, .dst=op, .src1=t1, .src2=t2);
            if (left == NULL || right == NULL) return NULL;
            if (!var_type_equal(left, right)) {
                print_error(7, root);
                return NULL;
            }
            return left;
        case AND:
        case OR:
        case RELOP:
            if (op) CODE(IR_ASSIGN, .dst=op, .src=&imme_zero);
            vt = dfs_exp_cond(root, NULL, label1 = new_label());
            if (op) CODE(IR_ASSIGN, .dst=op, .src=&imme_one);
            CODE(IR_LABEL, .op=label1);
            return vt;
        }
    }
    if (child(root, 0)->symbol == ID) {
        // ID LP Args RP
ID_LP_RP:
        name = dfs_id(root->child);
        symbol = find_symbol(name);
        if (symbol == NULL) {
            print_error(2, child(root, 0), name);
            return NULL;
        }
        if (symbol->type->kind != FUNCTION) {
            print_error(11, child(root, 0), get_ast_node_code(child(root, 0)));
            return NULL;
        }
        struct func_arg_list *arg_list = NULL;
        if (child(root, 2)->symbol == Args) {
            arg_list = dfs_args(child(root, 2));
        }
        struct func_param_list *param_list = symbol->type->func.params;
        if (!params_args_equal(param_list, arg_list)) {
            // Processing arg_str
            int arg_str_len = 0;
            struct func_arg_list *p_arg = arg_list;
            while (p_arg) {
                char* vt_str = get_var_type_str(p_arg->var_type);
                arg_str_len += strlen(vt_str);
                free(vt_str);
                p_arg = p_arg->tail;
            }
            p_arg = arg_list;
            char* arg_str = (char*) malloc(sizeof(char) * (arg_str_len+1));
            strcpy(arg_str, "");
            while (p_arg) {
                char* vt_str = get_var_type_str(p_arg->var_type);
                strcat(arg_str, vt_str);
                free(vt_str);
                p_arg = p_arg->tail;
                if (p_arg) strcat(arg_str, ", ");
            }
            // Processing param_str
            int param_str_len = 0;
            struct func_param_list *p_param = param_list;
            while (p_param) {
                char* vt_str = get_var_type_str(p_param->type);
                param_str_len += strlen(vt_str);
                free(vt_str);
                p_param = p_param->tail;
            }
            p_param = param_list;
            char* param_str = (char*) malloc(sizeof(char) * (param_str_len+1));
            strcpy(param_str, "");
            while (p_param) {
                char* vt_str = get_var_type_str(p_param->type);
                strcat(param_str, vt_str);
                free(vt_str);
                p_param = p_param->tail;
                if (p_param) strcat(param_str, ", ");
            }
            print_error(9, child(root, 2), name, param_str, arg_str);
            return symbol->type->func.ret;
        }
        // below if for translation
        if (strcmp(symbol->name, "read") == 0) {
            if (op) CODE(IR_READ, .op=op);
        } else if (strcmp(symbol->name, "write") == 0) {
            CODE(IR_WRITE, .op=arg_list->op);
        } else {
            struct func_arg_list *p_arg = arg_list;
            for (; p_arg; p_arg = p_arg->tail) {
                if (p_arg->var_type->kind == BASIC) {
                    CODE(IR_ARG, .op=p_arg->op);
                } else {
                    CODE(IR_ARG, .op=modify_operator(p_arg->op, OP_MDF_AND));
                }
            }
            struct ir_operand *func = new(struct ir_operand, OP_FUNCTION, \
                    .name=symbol->name);
            CODE(IR_CALL, .ret=op ? op:(t1 = new_temp_var()), .func=func);
        }
        return symbol->type->func.ret;
    } else if (child(root, 0)->symbol == Exp) {
        // Exp LB Exp RB
        vt = dfs_exp_addr(root, t1 = new_temp_var());
        t1 = ir_clean_temp_var(t1);
        if (vt == NULL) return NULL;
        t2 = modify_operator(t1, OP_MDF_STAR);
        if (op) CODE(IR_ASSIGN, .dst=op, .src=t2);
        return vt;
    }
    assert(false);
    return NULL;
}

struct func_arg_list *dfs_args(struct ast_node *root) {
    assert(root->symbol == Args);
    struct func_arg_list *al = new(struct func_arg_list);
    struct ir_operand *t = new_temp_var();
    al->var_type = dfs_exp(child(root, 0), t);
    t = ir_clean_temp_var(t);
    al->op = t;
    if (child_num(root) == 3) { // Exp COMMA Args
        al->tail = dfs_args(child(root, 2));
    } else { // Exp
        al->tail = NULL;
    }
    return al;
}

void dfs_stmt(struct ast_node *root) {
    assert(root->symbol == Stmt);
    struct ir_operand *t1, *label1, *label2;
    struct var_type *vt;
    switch (root->child->symbol) {
    case Exp:
        dfs_exp(child(root, 0), NULL);
        return;
    case CompSt:
        push_scope();
        dfs_comp_st(child(root, 0));
        pop_scope();
        return;
    case RETURN:
        vt = dfs_exp(child(root, 1), t1 = new_temp_var());
        t1 = ir_clean_temp_var(t1);
        if (!var_type_equal(vt, func_ret_type)) {
            print_error(8, child(root, 1));
            return;
        }
        CODE(IR_RETURN, .op=t1);
        return;
    case IF:
        if (child_num(root) == 5) { // IF LP Exp RP Stmt
            dfs_exp_cond(child(root, 2), NULL, label1 = new_label());
            dfs_stmt(child(root, 4));
            CODE(IR_LABEL, .op=label1);
        } else { // IF LP Exp RP Stmt ELSE Stmt
            dfs_exp_cond(child(root, 2), NULL, label1 = new_label());
            dfs_stmt(child(root, 4));
            label2 = new_label();
            CODE(IR_GOTO, .op=label2);
            CODE(IR_LABEL, .op=label1);
            dfs_stmt(child(root, 6));
            CODE(IR_LABEL, .op=label2);
        }
        return;
    case WHILE: // WHILE LP Exp RP Stmt
        CODE(IR_LABEL, .op=(label1 = new_label()));
        dfs_exp_cond(child(root, 2), NULL, label2 = new_label());
        dfs_stmt(child(root, 4));
        CODE(IR_GOTO, .op=label1);
        CODE(IR_LABEL, .op=label2);
        return;
    default:
        exit(2);
    }
}

