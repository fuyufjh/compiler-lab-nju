#ifndef _SYMBOL_TABLE_H
#define _SYMBOL_TABLE_H

#include "symbol_type.h"

struct st_node {
    struct symbol *symbol;
    struct st_node *prev, *next;
};

void insert_symbol(struct symbol *s);

#endif
