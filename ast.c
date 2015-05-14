#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "ast.h"
#include "syntax.tab.h"

extern int yylineno;

void add_children_ast_node(struct ast_node *parent, int n, ...) {
    va_list ap;
    va_start(ap, n);
    int is_first_child = 1;
    struct ast_node *prev;
    while(n--) {
        struct ast_node *child = va_arg(ap, struct ast_node*);
        if (child == NULL) continue;
        if (is_first_child) {
            parent->child = child;
            parent->lineno = child->lineno;
            parent->column = child->column;
            parent->length = child->length;
            is_first_child = 0;
        } else {
            parent->length += child->length;
            prev->peer = child;
        }
        child->parent = parent;
        prev = child;
    }
}

struct ast_node *get_nth_child_ast_node(struct ast_node *root, int n) {
    struct ast_node *p = root->child;
    while (n-- && p != NULL) {
        p = p->peer;
    }
    return p;
}

void print_ast(struct ast_node *root, int space) {
    if (root == NULL) return;
    int n;
    for (n=0; n<space; n++) {
        printf("  ");
    }
    printf("%s", get_symbol_name(root->symbol));
    switch (get_value_type(root->symbol)) {
    case NO_VALUE:
        break;
    case INT_VALUE:
        printf(": %d", root->value.int_value);
        break;
    case FLOAT_VALUE:
        printf(": %f", root->value.float_value);
        break;
    case DOUBLE_VALUE:
        printf(": %f", root->value.double_value);
        break;
    case STR_VALUE:
        printf(": %s", root->value.str_value);
        break;
    default:
        printf(": Unknown Value Type");
    }
    if (root->child) {
        printf(" (%d)\n", root->lineno);
        struct ast_node *p = root->child;
        while (p != NULL) {
            print_ast(p, space+1);
            p = p->peer;
        }
    } else {
        printf("\n");
    }
}

extern int yyleng;

struct ast_node *make_ast_node_terminal(int st, union ast_value value) {
    struct ast_node *node = (struct ast_node *) malloc(sizeof(struct ast_node));
    node->symbol = st;
    node->value = value;
    node->lineno = yylineno;
    node->column = yylloc.first_column;
    node->length = yyleng;
    return node;
}

struct ast_node *make_ast_node_nonterminal(int st) {
    struct ast_node *node = (struct ast_node *) malloc(sizeof(struct ast_node));
    node->symbol = st;
    node->lineno = yylineno;
    node->column = yylloc.first_column;
    node->length = yyleng;
    return node;
}

const char* nt_symbol_name[] = {
    "Program", "ExtDefList", "ExtDef", "ExtDecList", "Specifier",
    "StructSpecifier", "OptTag", "Tag", "VarDec", "FunDec",
    "VarList", "ParamDec", "CompSt", "StmtList", "Stmt",
    "DefList", "Def", "DecList", "Dec", "Exp", "Args"
};

const char* t_symbol_name[] = {
    "ASSIGNOP", "OR", "AND", "RELOP", "MINUS", "PLUS", "STAR",
    "DIV", "NOT", "LP", "RP", "LB", "RB", "DOT",
    NULL, // LOWER_THAN_ELSE
    "INT", "FLOAT", "TYPE", "ID", "SEMI", "COMMA", "STRUCT",
    "RETURN", "IF", "ELSE", "WHILE", "LC", "RC",
};

int get_value_type(int node_type) {
    switch (node_type) {
        case INT:
            return INT_VALUE;
        case FLOAT:
            return FLOAT_VALUE;
        case TYPE: case ID: case RELOP:
            return STR_VALUE;
        default:
            return NO_VALUE;
    }
}

char* get_str_src(int line, int column) {
    char *p = source_code;
    int l = 1, c = 1;
    while (*p) {
        if (l == line && c == column) break;
        if (*p == '\n') {
            l++;
            c = 1;
        } else {
            c++;
        }
        p++;
    }
    return p;
}

char *get_ast_node_code(struct ast_node *node) {
    char *text = (char*) malloc(node->length + 1);
    strncpy(text, get_str_src(node->lineno, node->column), node->length);
    text[node->length] = '\0';
    return text;
}

