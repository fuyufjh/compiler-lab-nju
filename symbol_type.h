#ifndef _SYMBOL_TYPE_H
#define _SYMBOL_TYPE_H

struct symbol {
    char* name;
    struct var_type *type;
    int scope;
    // Below is for function symbol only
    struct ast_node *ast_node;
};

struct var_type {
    enum { BASIC, ARRAY, STRUCTURE, FUNCTION } kind;
    union {
        int basic;
        struct {
            struct var_type *elem;
            struct array_size_list *size_list;
        } array;
        struct struct_type* struct_type;
        struct {
            struct var_type* ret;
            struct func_param_list *params;
        } func;
    };
};

struct array_size_list {
    int size;
    struct array_size_list *next;
};

struct struct_symbol {
    char *name;
    struct struct_type *type;
};

struct struct_type {
    struct field_list *fields;
};

struct field_list {
    char *name;
    struct var_type *type;
    struct field_list *tail;
};

struct func_param_list {
    struct var_type *type;
    char *name; // Optional
    struct func_param_list *tail;
};

struct func_arg_list {
    struct var_type *var_type;
    struct func_arg_list *tail;
};

#endif
