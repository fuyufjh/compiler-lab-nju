#ifndef _SYMBOL_TYPE_H
#define _SYMBOL_TYPE_H

struct symbol {
    char* name;
    struct var_type *type;
};

struct var_type {
    enum { BASIC, ARRAY, STRUCTURE, FUNCTION } kind;
    union {
        int basic;
        struct {
            struct var_type *elem;
            int size;
        } array;
        struct struct_field_list *fields;
        struct func_param_list *params;
    };
};

struct struct_field_list {
    char *name;
    struct var_type *type;
    struct struct_field_list *tail;
};

struct func_param_list {
    struct var_type *type;
    struct func_param_list *tail;
};

#endif
