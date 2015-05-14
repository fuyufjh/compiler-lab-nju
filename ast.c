#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
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
            is_first_child = 0;
        } else {
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

struct ast_node *make_ast_node_terminal(int st, union ast_value value) {
    struct ast_node *node = (struct ast_node *) malloc(sizeof(struct ast_node));
    node->symbol = st;
    node->value = value;
    node->lineno = yylineno;
    return node;
}

struct ast_node *make_ast_node_nonterminal(int st) {
    struct ast_node *node = (struct ast_node *) malloc(sizeof(struct ast_node));
    node->symbol = st;
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

/*int main() {*/
    /*root = make_node_nonterminal(Program, 1);*/
    /*struct ast_node *node1 = make_node_nonterminal(ExtDecList, 1);*/
    /*struct ast_node *node2 = make_node_nonterminal(ExtDecList, 2);*/
    /*add_children(root, 2, node1, node2);*/
    /*Value _v;*/
    /*_v.str_value = "int";*/
    /*struct ast_node *node3 = make_node_terminal(TYPE, STR_VALUE, _v);*/
    /*_v.str_value = "float";*/
    /*struct ast_node *node4 = make_node_terminal(TYPE, STR_VALUE, _v);*/
    /*struct ast_node *node5 = make_node_terminal(TYPE, STR_VALUE, _v);*/
    /*add_children(node1, 3, node3, node4, node5);*/
    /*print_tree(root, 0);*/
/*}*/
