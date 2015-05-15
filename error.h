#ifndef _ERROR_H
#define _ERROR_H

#include <stdarg.h>
#include "ast.h"

void print_error(int error_type, struct ast_node *node, ...);

#endif
