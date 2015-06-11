#ifndef _COMMON_H
#define _COMMON_H

// General header files
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

// WOW! I invented this! DIAO BAO LE!
#define new(type, ...) ({\
        type* __t = (type*)malloc(sizeof(type));\
        *__t = (type){__VA_ARGS__};\
        __t; })

// Global flags
bool flag_print_ast;
bool flag_verbose;
bool flag_disable_block_optimize;

// Others
char *to_free;
typedef long long unsigned SYMBOL_STRING;

#endif
