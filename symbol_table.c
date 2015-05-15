#include "common.h"
#include "symbol_type.h"
#include "symbol_table.h"

#define HASH_MASK 0x3fff

struct st_node *st_hashtable[HASH_MASK + 1];
struct st_node *st_struct_hashtable[HASH_MASK + 1];

struct st_node *st_scope_stack[MAX_SCOPE_NUM + 1]; // "+1" is for DECLARED
static int scope_level = 0;

static unsigned int hash_pjw(char* name) {
    unsigned int val = 0, i;
    for (; *name; ++name) {
        val = (val << 2) + *name;
        if ((i = val & ~HASH_MASK)) val = (val ^ (i >> 12)) & HASH_MASK;
    }
    return val;
}

static void insert_head(struct st_node **head, struct st_node *node) {
    if (*head == NULL) {
        *head = node;
        node->next = node->prev = NULL;
    } else {
        struct st_node *temp = *head;
        *head = node;
        node->next = temp;
        node->prev = NULL;
        temp->prev = node;
    }
}

static struct st_node *delete_head(struct st_node **head) {
    if (*head == NULL) {
        return NULL;
    } else {
        struct st_node* p = *head;
        *head = p->next;
        p->prev = NULL;
        if (p->next != NULL) {
            p->next->prev = NULL;
            p->next = NULL;
        }
        return p;
    }
}

struct st_node *delete_node_hashtable(struct st_node *node) {
    struct st_node **head = &st_hashtable[hash_pjw(node->symbol->name)];
    if (*head == node) {
        assert(delete_head(head) == node);
        return node;
    } else {
        struct st_node *p = *head;
        while (p != node) p = p->next;
        p->prev->next = p->next;
        p->next->prev = p->prev;
        p->next = p->prev = NULL;
        return p;
    }
}

struct st_node *delete_node_scope_list(struct st_node* node) {
    struct st_node **head = &st_scope_stack[node->symbol->scope];
    if (node->s_prev == NULL) {
        *head = node->s_next;
        if (node->s_next) node->s_next->s_prev = NULL;
        node->s_prev = node->s_next = NULL;
    } else {
        if (node->s_next) node->s_prev->s_prev = node->s_prev;
        node->s_prev->s_next = node->s_next;
        node->s_prev = node->s_next = NULL;
    }
    return node;
}

void delete_symbol(char *name) {
    printf("DELETE SYMBOL: %s\n", name);

    struct st_node *p = st_hashtable[hash_pjw(name)];
    while (strcmp(p->symbol->name, name) != 0) {
        p = p->next;
    }
    p = delete_node_hashtable(p);
    p = delete_node_scope_list(p);
}

void push_scope() {
    scope_level++;
    assert(scope_level < MAX_SCOPE_NUM);
    //st_scope_stack[scope_level] = NULL;
};

void pop_scope() {
    struct st_node **head = &st_scope_stack[scope_level];
    while (*head != NULL) {
        struct st_node *p = *head, *temp, *first = p;
        while (p) {
            temp = p->s_next;
            if (p->prev == NULL) { // removable
                struct st_node **ht_head = &st_hashtable[hash_pjw(p->symbol->name)];
                struct st_node* node = delete_head(ht_head);
                free(delete_node_scope_list(node));
                if (first == p) first = temp;
            }
            p = temp;
        }
        *head = first;
    }
    scope_level--;
    assert(scope_level >= 0);
};

static int scope_backup;

void set_declared_scope() {
    scope_backup = scope_level;
    scope_level = DECLARED;
}

void unset_declared_scope() {
    scope_level = scope_backup;
}

void insert_to_stack(struct st_node *node, int scope) {
    //node->s_level = scope_level;
    struct st_node **head = &st_scope_stack[scope];
    if (*head == NULL) {
        *head = node;
        node->s_prev = node->s_next = NULL;
    } else {
        struct st_node *old_head = *head;
        *head = node;
        node->s_next = old_head;
        old_head->s_prev = node;
        node->s_prev = NULL;
    }
}

#include <stdio.h>
#include "read_symbols.h"
/*#include <assert.h>
extern struct var_type int_type;
void print_var_type(struct var_type *vt);
void print_struct_type(struct struct_type *st);

void print_var_type(struct var_type *vt) {
    switch (vt->kind) {
    case BASIC:
        printf("  basic: %s\n", vt->basic == 273 ? "int" : "float");
        break;
    case ARRAY:
        printf(" array: ");
        print_var_type(vt->array.elem);
        struct array_size_list *sl = vt->array.size_list;
        while (sl != NULL) {
            printf("%d ", sl->size);
            sl = sl->next;
        }
        printf("\n");
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
        f = f->tail;
    }
    printf("  }\n");
}*/

struct st_node *find_node_in_scope(char *name, int scope) {
    struct st_node *p = st_hashtable[hash_pjw(name)];
    while (p && p->symbol->scope == scope) {
        if (strcmp(p->symbol->name, name) == 0) return p;
        p = p->next;
    }
    return NULL;
}

struct st_node *find_struct_symbol_node(char* name) {
    struct st_node *p = st_struct_hashtable[hash_pjw(name)];
    while (p) {
        if (strcmp(p->struct_symbol->name, name) == 0) return p;
        p = p->next;
    }
    return NULL;
}

bool insert_st_node(struct symbol *s) {
    if (find_node_in_scope(s->name, s->scope)) return false;
    printf("INSERT SYMBOL: %s\n", s->name);
    //print_var_type(s->type);
    printf("  %s\n", to_free = get_var_type_str(s->type));
    free(to_free);

    unsigned int hash_val = hash_pjw(s->name);
    struct st_node* new_node = new(struct st_node);
    new_node->symbol = s;
    //new_node->next = NULL;
    //new_node->prev = NULL;
    insert_head(&st_hashtable[hash_val], new_node);
    insert_to_stack(new_node, s->scope);
    return true;
}

bool insert_st_struct_node(struct struct_symbol *s) {
    if (find_struct_symbol_node(s->name)) return false;
    // debug
    printf("INSERT STRUCT SYMBOL: %s\n", s->name);
    static struct var_type svt = { STRUCTURE , {} };
    svt.struct_type = s->type;
    printf("  %s\n", to_free = get_var_type_str(&svt));
    free(to_free);

    unsigned int hash_val = hash_pjw(s->name);
    struct st_node* new_node = new(struct st_node);
    new_node->struct_symbol = s;
    new_node->next = NULL;
    new_node->prev = NULL;
    insert_head(&st_struct_hashtable[hash_val], new_node);
    return true;
}

bool insert_symbol(char* name, struct var_type *vt) {
    assert(vt->kind != FUNCTION);
    struct symbol *s = new(struct symbol);
    s->type = vt;
    s->name = name;
    s->scope = scope_level;
    return insert_st_node(s);
}

bool insert_func_symbol(char* name, struct var_type *vt, struct ast_node *node) {
    assert(vt->kind == FUNCTION);
    struct symbol *s = new(struct symbol);
    s->type = vt;
    s->name = name;
    s->ast_node = node;
    if (scope_level != DECLARED) s->scope = GLOBAL;
    else s->scope = DECLARED; // Only 2 scopes for function
    return insert_st_node(s);
}

bool insert_struct_symbol(char* name, struct struct_type *st) {
    struct struct_symbol *s = new(struct struct_symbol);
    s->type = st;
    s->name = name;
    return insert_st_struct_node(s);
}

struct symbol *find_symbol(char *name) {
    unsigned int hash_val = hash_pjw(name);
    struct st_node *pnode = st_hashtable[hash_val];
    while (pnode && strcmp(pnode->symbol->name, name) != 0) {
        pnode = pnode->next;
    }
    if (pnode == NULL) return NULL;
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

void check_declared_fun() {
    struct st_node *p = st_scope_stack[DECLARED];
    if (p != NULL) {
        print_error(18, p->symbol->ast_node, p->symbol->name);
        p = p->s_next;
    }
}
