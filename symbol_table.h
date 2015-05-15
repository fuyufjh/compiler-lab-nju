#ifndef _SYMBOL_TABLE_H
#define _SYMBOL_TABLE_H

#include "symbol_type.h"
#include "common.h"

struct st_node {
    union {
        struct symbol *symbol;
        struct struct_symbol *struct_symbol;
    };
    struct st_node *prev, *next;
    struct st_node *s_prev, *s_next; // Work for ver symbol only
};

#define GLOBAL 0
#define DECLARED (MAX_SCOPE_NUM)
#define MAX_SCOPE_NUM 50

struct symbol *find_symbol(char *name);
struct struct_symbol *find_struct_symbol(char* name);
bool insert_symbol(char* name, struct var_type *vt);
bool insert_func_symbol(char* name, struct var_type *vt, struct ast_node*);
bool insert_struct_symbol(char* name, struct struct_type *st);
void push_scope();
void pop_scope();
void set_declared_scope();
void unset_declared_scope();
struct st_node *delete_node_hashtable(struct st_node *node);
struct st_node *delete_node_scope_list(struct st_node* node);
void delete_symbol(char *name);
void check_declared_fun();

#endif
