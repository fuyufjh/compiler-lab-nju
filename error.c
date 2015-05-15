#include "error.h"
#include "common.h"

const char *error_format_str[];

void print_error(int error_type, struct ast_node *node, ...) {
    printf("Error type %d at Line %d: ", error_type, node->lineno);
    va_list ap;
    va_start(ap, node);
    vprintf(error_format_str[error_type], ap);
    printf("\n");
}

const char *error_format_str[] = {
    NULL,
    "Undefined variable \"%s\".",
    "Undefined function \"%s\".",
    "Redefined variable \"%s\".",
    "Redefined function \"%s\".",
    "Type mismatched for assignment.",
    "The left-hand side of an assignment must be a variable.",
    "Type mismatched for operands.",
    "Type mismatched for return.",
    "Function \"%s(%s)\" is not applicable for arguments \"(%s)\".",
    "\"%s\" is not an array.",
    "\"%s\" is not a function.",
    "\"%s\" is not an integer.",
    "Illegal use of \".\".",
    "Non-existent field \"%s\".",
    "Redefined field \"%s\".",
    "Duplicated name \"%s\".",
    "Undefined structure \"%s\".",
    "Undefined function \"%s\"",
    "Inconsistent declaration of function \"%s\""
};

