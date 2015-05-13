#include <stdlib.h>
#include "symbol_type.h"
#include "symbol_table.h"

#define HASH_MASK 0x3fff
#define MAX_BLOCK 50

struct st_node *st_hashtable[HASH_MASK + 1];

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

void insert_symbol(struct symbol *s) {
    if (s == NULL) return;
    unsigned int hash_val = hash_pjw(s->name);
    struct st_node* new_node = (struct st_node *) malloc(sizeof(struct st_node));
    new_node->symbol = s;
    insert_head(&st_hashtable[hash_val], new_node);
}
