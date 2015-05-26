#ifndef _COMMON_H
#define _COMMON_H

// General header files
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

// General macro define
#define bool int
#define true 1
#define false 0

// WOW! I invented this! DIAO BAO LE!
#define new(type, ...) ({\
        type* t = (type*)malloc(sizeof(type));\
        *t = (type){__VA_ARGS__};\
        t; })

// Global flags
bool flag_print_ast;
bool flag_verbose;

// Others
char *to_free;

#endif
