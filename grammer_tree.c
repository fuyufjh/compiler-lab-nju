#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "grammer_tree.h"
#include "syntax.tab.h"

extern int yylineno;

void add_children(Node* parent, int n, ...) {
    va_list ap;
    va_start(ap, n);
    int i, is_first_child = 1;
    Node* prev;
    while(n--) {
        Node* child = va_arg(ap, Node*);
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

/*void add_child(struct Node* parent, Node* node) {*/
    /*if (node == NULL || parent == NULL) return;*/
    /*if (parent->child == NULL) {*/
        /*parent->child = node;*/
    /*} else {*/
        /*struct Node* p = parent->child;*/
        /*while (p->peer != NULL) p = p->peer;*/
        /*p->peer = node;*/
    /*}*/
/*}*/

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
    switch (get_type(root->symbol)) {
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

Node* make_node_terminal(int st, Value v) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->symbol = st;
    node->value = v;
    node->lineno = yylineno;
    return node;
}

Node* make_node_nonterminal(int st) {
    Node* node = (Node*)malloc(sizeof(Node));
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

int get_type(int s) {
    switch (s) {
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
