#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include "common.h"
#include "syntax.tab.h"
#include "read_symbols.h"

#define child(node, n) \
    (n == 0 ? node->child : get_nth_child_ast_node(node, n))
static struct var_type *func_ret_type;

#include <stdio.h>

const char *error_format_str[];

void print_error(int error_type, struct ast_node *node, ...) {
    printf("Error type %d at Line %d: ", error_type, node->lineno);
    va_list ap;
    va_start(ap, node);
    vprintf(error_format_str[error_type], ap);
    printf("\n");
}

const char *error_format_str[] = {
    NULL,
    "Undefined variable \"%s\".",
    "Undefined function \"%s\".",
    "Redefined variable \"%s\".",
    "Redefined function \"%s\".",
    "Type mismatched for assignment.",
    "The left-hand side of an assignment must be a variable.",
    "Type mismatched for operands.",
    "Type mismatched for return.",
    "Function \"%s(%s)\" is not applicable for arguments \"(%s)\".",
    "\"%s\" is not an array.",
    "\"%s\" is not a function.",
    "\"%s\" is not an integer.",
    "Illegal use of \".\".",
    "Non-existent field \"%s\".",
    "Redefined field \"%s\".",
    "Duplicated name \"%s\".",
    "Undefined structure \"%s\"."
};

#define VAR_TYPE_STR_SIZE 1024

char* get_var_type_str(struct var_type *vt) {
    char buf[VAR_TYPE_STR_SIZE];
    char vt_str[VAR_TYPE_STR_SIZE * 2];
    switch (vt->kind) {
    case BASIC:
        strcpy(vt_str, vt->basic == INT ? "int" : "float");
        break;
    case ARRAY:
        sprintf(vt_str, "%s", get_var_type_str(vt->array.elem));
        struct array_size_list *sl = vt->array.size_list;
        while (sl != NULL) {
            sprintf(buf, "[%d]", sl->size);
            strcat(vt_str, buf);
            sl = sl->next;
        }
        break;
    case STRUCTURE:
        strcpy(vt_str, "struct {");
        struct field_list *fl = vt->struct_type->fields;
        while (fl != NULL) {
            sprintf(buf, "%s %s; ", get_var_type_str(fl->type), fl->name);
            strcat(vt_str, buf);
            fl = fl->tail;
        }
        strcat(vt_str, "}");
        break;
    case FUNCTION:
        strcpy(vt_str, get_var_type_str(vt->func.ret));
        strcat(vt_str, " <function>(");
        struct func_param_list *p = vt->func.params;
        while (p) {
            strcat(vt_str, get_var_type_str(p->type));
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
    if (a == b) return true;
    if (a->kind != b->kind) return false;
    switch (a->kind) {
    case BASIC:
        return a->basic == b->basic;
    case ARRAY:
        return false;
    case STRUCTURE:
        return a->struct_type == b->struct_type;
    case FUNCTION:
        return false;
    }
    return false;
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
        dfs_exp(root);
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
    switch (child(root, 1)->symbol) {
    // ExtDef: Specifier ExtDecList SEMI
    case ExtDecList:
        dfs_ext_dec_list(child(root, 1), var_type);
        break;
    // ExtDef: Specifier SEMI
    case SEMI:
        break;
    // ExtDef: Specifier FunDec CompSt
    case FunDec:
        func_ret_type = var_type;
        push_scope();
        dfs_fun_dec(child(root, 1), var_type);
        dfs_comp_st(child(root, 2));
        pop_scope();
        func_ret_type = NULL;
        break;
    }
}

void dfs_fun_dec(struct ast_node *root, struct var_type *ret) {
    assert(root->symbol == FunDec);
    struct var_type *vt = new(struct var_type);
    vt->kind = FUNCTION;
    vt->func.params = NULL;
    vt->func.ret = ret;
    char* name = dfs_id(root->child);
    if (child(root, 2)->symbol == VarList) {
        vt->func.params = dfs_var_list(child(root, 2));
        struct func_param_list *p = vt->func.params;
        while (p) {
            insert_symbol(p->name, p->type);
            p = p->tail;
        }
    }
    if (!insert_symbol(name, vt)) {
        print_error(4, root->child, name);
    }
}

struct func_param_list *dfs_var_list(struct ast_node *root) {
    assert(root->symbol == VarList);
    struct func_param_list *p = new(struct func_param_list);
    p->type = dfs_param_dec(root->child, &p->name);
    if (child(root, 2)) {
        p->tail = dfs_var_list(child(root, 2));
    } else {
        p->tail = NULL;
    }
    return p;
}

struct var_type *dfs_param_dec(struct ast_node *root, char **name) {
    assert(root->symbol == ParamDec);
    struct var_type *spec_t = dfs_specifier(root->child);
    return dfs_var_dec(child(root, 1), name, spec_t);
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
        st->fields = dfs_def_list(child(root, 3));
        name = dfs_opt_tag(child(root, 1));
        insert_struct_symbol(name, st);
        break;
    // StructSpecifier: STRUCT Tag
    case Tag:
        name = dfs_tag(child(root, 1));
        st = find_struct_symbol(name)->type;
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
    return dfs_dec_list(child(root, 1), vt);
}

struct field_list *dfs_dec_list(struct ast_node *root, struct var_type *vt) {
    assert(root->symbol == DecList);
    struct field_list *fl = dfs_dec(root->child, vt);
    if (child(root, 2)) {
        fl->tail = dfs_dec_list(child(root, 2), vt);
    }
    return fl;
}

struct field_list *dfs_dec(struct ast_node *root, struct var_type *vt) {
    // Attention! Initial value (if assigned) was not saved.
    assert(root->symbol == Dec);
    struct field_list *fl = new(struct field_list);
    fl->type = dfs_var_dec(root->child, &fl->name, vt);
    fl->tail = NULL;
    return fl;
}

void dfs_comp_st(struct ast_node *root) {
    assert(root->symbol == CompSt);
    if (child(root, 1)->symbol == DefList) {
        struct field_list *fields = dfs_def_list(child(root, 1)), *p = fields;
        while (p != NULL) {
            if (!insert_symbol(p->name, p->type)) {
                print_error(3, child(root, 1), p->name);
            }
            p = p->tail;
        }
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

struct var_type *dfs_exp(struct ast_node *root) {
    assert(root->symbol == Exp);
    char* name;
    struct symbol *symbol;
    struct var_type *vt, *left, *right;
    if (child(root, 1) == NULL) { // num of children = 1
        switch (child(root, 0)->symbol) {
        case ID:
            name = dfs_id(child(root, 0));
            symbol = find_symbol(name);
            if (symbol == NULL) {
                print_error(1, child(root, 0), name);
                return NULL;
            }
            return symbol->type;
        case INT:
            dfs(child(root, 0));
            return &int_type;
        case FLOAT:
            dfs(child(root, 0));
            return &float_type;
        }
    } else if (child(root, 2) == NULL) { // num of children = 2
        switch (child(root, 0)->symbol) {
        case MINUS:
            vt = dfs_exp(child(root, 1));
            if (vt->kind != BASIC) {
                print_error(7, child(root, 1));
                return NULL;
            } else {
                return vt;
            }
        case NOT:
            vt = dfs_exp(child(root, 1));
            if (vt->kind != BASIC || vt->basic != INT) {
                print_error(7, child(root, 1));
                return NULL;
            } else {
                return vt;
            }
        }
    } else if (child(root, 3) == NULL) {
        switch (child(root, 1)->symbol) {
        case Exp: // LP Exp RP
            return dfs_exp(child(root, 1));
        case ASSIGNOP:
            left = dfs_exp(child(root, 0));
            right = dfs_exp(child(root, 2));
            if (left == NULL || right == NULL) return NULL;
            bool left_value = false;
            struct ast_node *lnode = root->child;
            // Judge left value is hard!!
            if (lnode->child->symbol == ID && !lnode->child->peer) left_value = true;
            if (lnode->child->symbol == Exp && child(lnode, 1)->symbol == LB) left_value = true;
            if (left_value == false) {
                print_error(6, child(root, 0));
                return NULL;
            }
            if (!var_type_equal(left, right)) {
                print_error(5, root);
                return NULL;
            }
            return left;
        case DOT:
            vt = dfs_exp(child(root, 0));
            if (vt->kind != STRUCTURE) {
                print_error(13, child(root, 0));
                return NULL;
            }
            char* name = dfs_id(child(root, 2));
            struct var_type *field_vt = get_field_type(vt->struct_type, name);
            if (field_vt == NULL) {
                print_error(14, child(root, 2), name);
                return NULL;
            } else {
                return field_vt;
            }
        default: // Exp Op Exp  Op = AND, OR, PLUS, MINUS ...
            left = dfs_exp(child(root, 0));
            right = dfs_exp(child(root, 2));
            if (left == NULL || right == NULL) return NULL;
            if (!var_type_equal(left, right)) {
                print_error(7, root);
                return NULL;
            }
            return &int_type;
        }
    } else if (child(root, 0)->symbol == ID) {
        // ID LP Args RP
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
        struct func_arg_list *arg_list = dfs_args(child(root, 2));
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
        }
        return symbol->type->func.ret;
    } else if (child(root, 0)->symbol == Exp) {
        // Exp LB Exp RB
        vt = NULL;
        left = dfs_exp(child(root, 0));
        if (left == NULL || left->kind != ARRAY) {
            print_error(10, child(root, 0), get_ast_node_code(child(root, 0)));
        } else {
            vt = new(struct var_type);
            *vt = *left; // copy and modify
            vt->array.size_list = vt->array.size_list->next;
            if (vt->array.size_list == NULL) {
                struct var_type *temp = vt;
                vt = vt->array.elem;
                free(temp);
            }
        }
        right = dfs_exp(child(root, 2));
        if (right == NULL || right->kind != BASIC || right->basic != INT) {
            print_error(12, child(root, 2), get_ast_node_code(child(root, 2)));// TODO
        }
        return vt;
    }
    assert(false);
    return NULL;
}

struct func_arg_list *dfs_args(struct ast_node *root) {
    assert(root->symbol == Args);
    struct func_arg_list *al = new(struct func_arg_list);
    al->var_type = dfs_exp(child(root, 0));
    if (child(root, 1)) {
        al->tail = dfs_args(child(root, 2));
    } else {
        al->tail = NULL;
    }
    return al;
}

void dfs_stmt(struct ast_node *root) {
    assert(root->symbol == Stmt);
    if (child(root, 0)->symbol == RETURN) {
        // RETURN Exp SEMI
        struct var_type *vt = dfs_exp(child(root, 1));
        if (!var_type_equal(vt, func_ret_type)) {
            print_error(8, child(root, 1));
            return;
        }
    } else if (child(root, 0)->symbol == CompSt) {
        push_scope();
        dfs_comp_st(child(root, 0));
        pop_scope();
    }
    else {
        struct ast_node *p;
        for (p = root->child; p; p = p->peer) {
            dfs(p);
        }
    }
}
