#ifndef _COMMON_H
#define _COMMON_H

// General header files
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

// General macro define
#define new(type) ((type*) malloc(sizeof(type)))
#define bool int
#define true 1
#define false 0

// Global flags
bool flag_print_ast;
bool flag_verbose;

// Others
char *to_free;

#endif
