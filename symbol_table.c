#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "symbol_type.h"
#include "symbol_table.h"

#define HASH_MASK 0x3fff
#define MAX_BLOCK 50

struct st_node *st_hashtable[HASH_MASK + 1];
struct st_node *st_struct_hashtable[HASH_MASK + 1];

static unsigned int hash_pjw(char* name) {
    unsigned int val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if ((i = val & ~HASH_MASK)) val = (val ^ (i >> 12)) & HASH_MASK;
    }
    return val;
}

void insert_head(struct st_node **head, struct st_node *node) {
    if (*head == NULL) {
        *head = node;
        node->next = NULL;
        node->prev = NULL;
    } else {
        struct st_node *temp = *head;
        *head = node;
        node->next = temp;
        node->prev = NULL;
        temp->prev = node;
    }
}

struct st_node *delete_head(struct st_node **head) {
    if (*head == NULL) {
        return NULL;
    } else {
        struct st_node* p = *head;
        *head = p->next;
        p->next->prev = NULL;
        p->next = NULL;
        p->prev = NULL;
        return p;
    }
}

#include <stdio.h>
#include <assert.h>
extern struct var_type int_type;
void print_var_type(struct var_type *vt);
void print_struct_type(struct struct_type *st);

void print_var_type(struct var_type *vt) {
    switch (vt->kind) {
    case BASIC:
        printf("  basic: %s\n", vt->basic == 273 ? "int" : "float");
        break;
    case ARRAY:
        printf("  array: %d(", vt->array.size);
        struct var_type *t = vt->array.elem;
        /*while (t->kind == ARRAY) {
            printf("%d ", t->array.size);
            t = t->array.elem;
        }
        printf("%s )\n", t->array.elem == &int_type ? "int" : "float");
        */
        print_var_type(t);
        printf(")");
        break;
    case STRUCTURE:
        print_struct_type(vt->struct_type);
        break;
    case FUNCTION:
        printf("  function: \n  return: ");
        print_var_type(vt->func.ret);
        struct func_param_list *p = vt->func.params;
        printf("  [\n");
        while (p) {
            print_var_type(p->type);
            p = p->tail;
        }
        printf("  ]\n");
        break;
    }
}

void print_struct_type(struct struct_type *st) {
    printf("  struct: {\n");
    struct field_list *f = st->fields;
    while (f) {
        printf("%s :", f->name);
        print_var_type(f->type);
        f = f->tail;
    }
    printf("  }\n");
}

void insert_st_node(struct symbol *s) {
    printf("INSERT SYMBOL: %s\n", s->name);
    print_var_type(s->type);

    unsigned int hash_val = hash_pjw(s->name);
    struct st_node* new_node = new(struct st_node);
    new_node->symbol = s;
    new_node->next = NULL;
    new_node->prev = NULL;
    insert_head(&st_hashtable[hash_val], new_node);
}

void insert_st_struct_node(struct struct_symbol *s) {
    // debug
    printf("INSERT STRUCT SYMBOL: %s\n", s->name);
    print_struct_type(s->type);

    unsigned int hash_val = hash_pjw(s->name);
    struct st_node* new_node = new(struct st_node);
    new_node->struct_symbol = s;
    new_node->next = NULL;
    new_node->prev = NULL;
    insert_head(&st_struct_hashtable[hash_val], new_node);
}

void insert_symbol(char* name, struct var_type *vt) {
    struct symbol *s = new(struct symbol);
    s->type = vt;
    s->name = name;
    insert_st_node(s);
}

void insert_struct_symbol(char* name, struct struct_type *st) {
    struct struct_symbol *s = new(struct struct_symbol);
    s->type = st;
    s->name = name;
    insert_st_struct_node(s);
}

struct symbol *find_symbol(char *name) {
    unsigned int hash_val = hash_pjw(name);
    struct st_node *pnode = st_hashtable[hash_val];
    while (strcmp(pnode->symbol->name, name) != 0) {
        pnode = pnode->next;
        if (pnode == NULL) return NULL;
    }
    return pnode->symbol;
}

struct struct_symbol *find_struct_symbol(char* name) {
    unsigned int hash_val = hash_pjw(name);
    struct st_node *pnode = st_struct_hashtable[hash_val];
    while (strcmp(pnode->struct_symbol->name, name) != 0) {
        pnode = pnode->next;
        if (pnode == NULL) return NULL;
    }
    return pnode->struct_symbol;
}
