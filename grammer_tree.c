#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include "grammer_tree.h"
#include "syntax.tab.h"

#define NODE_BUFFER_SIZE 2048

extern int yylineno;

void add_children(Node* parent, int n, ...) {
    va_list ap;
    va_start(ap, n);
    int i;
    for (i=0; i<n; i++) {
        Node* child = va_arg(ap, Node*);
        // TODO: efficiency...
        add_child(parent, child);
        if (i == 0) parent->lineno = child->lineno;
    }
}

void add_child(struct Node* parent, Node* node) {
    if (node == NULL || parent == NULL) return;
    if (parent->child == NULL) {
        parent->child = node;
    } else {
        struct Node* p = parent->child;
        while (p->peer != NULL) p = p->peer;
        p->peer = node;
    }
}

/*void add_peer(Node* node, struct Node* peer) {*/
    /*if (node == NULL || peer == NULL) return;*/
    /*struct Node* p = peer;*/
    /*while (p->peer != NULL) p=p->peer;*/
    /*p->peer = node;*/
/*}*/

void print_tree(Node* root, int space) {
    if (root == NULL) return;
    int n;
    for (n=0; n<space; n++) {
        printf("  ");
    }
    printf("%s", get_name(root->symbol));
    switch (root->type) {
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
        struct Node* p = root->child;
        while (p != NULL) {
            print_tree(p, space+1);
            p = p->peer;
        }
    } else {
        printf("\n");
    }
}

static Node node_buffer[NODE_BUFFER_SIZE];
static int node_used = 0;

Node* make_node_terminal(int st, int vt, Value v) {
    assert(node_used < NODE_BUFFER_SIZE);

    node_buffer[node_used].symbol = st;
    node_buffer[node_used].type = vt;
    node_buffer[node_used].value = v;
    node_buffer[node_used].lineno = yylineno;
    //add_child(&node_buffer[node_used], p);

    return &node_buffer[node_used++];
}

Node* make_node_nonterminal(int st) {
    assert(node_used < NODE_BUFFER_SIZE);

    node_buffer[node_used].symbol = st;
    //node_buffer[node_used].lineno = yylineno;
    //add_child(&node_buffer[node_used], p);

    return &node_buffer[node_used++];
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

/*int main() {*/
    /*root = make_node_nonterminal(Program, 1);*/
    /*Node* node1 = make_node_nonterminal(ExtDecList, 1);*/
    /*Node* node2 = make_node_nonterminal(ExtDecList, 2);*/
    /*add_children(root, 2, node1, node2);*/
    /*Value _v;*/
    /*_v.str_value = "int";*/
    /*Node* node3 = make_node_terminal(TYPE, STR_VALUE, _v);*/
    /*_v.str_value = "float";*/
    /*Node* node4 = make_node_terminal(TYPE, STR_VALUE, _v);*/
    /*Node* node5 = make_node_terminal(TYPE, STR_VALUE, _v);*/
    /*add_children(node1, 3, node3, node4, node5);*/
    /*print_tree(root, 0);*/
/*}*/
