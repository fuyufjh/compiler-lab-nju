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
    int s_level;
};

struct symbol *find_symbol(char *name);
struct struct_symbol *find_struct_symbol(char* name);
bool insert_symbol(char* name, struct var_type *vt);
bool insert_struct_symbol(char* name, struct struct_type *st);
void push_scope();
void pop_scope();

#endif
