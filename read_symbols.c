#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "common.h"
#include "syntax.tab.h"
#include "read_symbols.h"

#define child(node, n) \
    (n == 0 ? node->child : get_nth_child_ast_node(node, n))

/*static*/ struct var_type int_type = { BASIC, {INT} };
static struct var_type float_type = { BASIC, {FLOAT} };
static struct var_type *get_basic_var_type(char *type_name) {
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
    struct ast_node *child;
    switch (root->symbol) {
    case ExtDef:
        dfs_ext_def(root);
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
        dfs_fun_dec(child(root, 1), var_type);
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
    }
    insert_symbol(name, vt);
}

struct func_param_list *dfs_var_list(struct ast_node *root) {
    assert(root->symbol == VarList);
    struct func_param_list *p = new(struct func_param_list);
    p->type = dfs_param_dec(root->child);
    if (child(root, 2)) {
        p->tail = dfs_var_list(child(root, 2));
    } else {
        p->tail = NULL;
    }
    return p;
}

struct var_type *dfs_param_dec(struct ast_node *root) {
    assert(root->symbol == ParamDec);
    struct var_type *spec_t = dfs_specifier(root->child);
    char* name; // Defined but not saved
    return dfs_var_dec(child(root, 1), &name, spec_t);
}

void dfs_ext_dec_list(struct ast_node *root, struct var_type *spec_t) {
    assert(root->symbol == ExtDecList);
    char* name;
    struct var_type *t = dfs_var_dec(root->child, &name, spec_t);
    insert_symbol(name, t);
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

