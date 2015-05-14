#ifndef _SYMBOL_TABLE_H
#define _SYMBOL_TABLE_H

#include "symbol_type.h"

struct st_node {
    union {
        struct symbol *symbol;
        struct struct_symbol *struct_symbol;
    };
    struct st_node *prev, *next;
};

struct symbol *find_symbol(char *name);
struct struct_symbol *find_struct_symbol(char* name);
void insert_symbol(char* name, struct var_type *vt);
void insert_struct_symbol(char* name, struct struct_type *st);

#endif
