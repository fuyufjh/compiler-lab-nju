%{
#define YYSTYPE Node*

#include "grammer_tree.h"
#include "syntax.tab.h"

int yycolumn = 1;
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

Value empty_value;

#define MAKE_NODE(name) \
    yylval = make_node_terminal(name, NO_VALUE, empty_value); \
    return name;

%}

D   [0-9]
L   [a-zA-Z_]
H   [a-fA-F0-9]
S   [+-]

%%
{S}?0[0-7]+ {
    /* oct integer */
    //yylval.int_value = strtol(yytext, NULL, 0);
    Value value;
    value.int_value = strtol(yytext, NULL, 0);
    yylval = make_node_terminal(INT, INT_VALUE, value);
    return INT;
}
{S}?0[xX]{H}+ {
    /* hex integer */
    //yylval.int_value = strtol(yytext, NULL, 0);
    Value value;
    value.int_value = strtol(yytext, NULL, 0);
    yylval = make_node_terminal(INT, INT_VALUE, value);
    return INT;
}
{S}?{D}+ {
    /* dec integer */
    //yylval.int_value = atoi(yytext);
    Value value;
    value.int_value = atoi(yytext);
    yylval = make_node_terminal(INT, INT_VALUE, value);
    return INT;
}
{S}?({D}*\.?{D}+|{D}+\.) {
    //yylval.float_value = atof(yytext);
    Value value;
    value.float_value = atof(yytext);
    yylval = make_node_terminal(FLOAT, FLOAT_VALUE, value);
    return FLOAT;
}
";" { MAKE_NODE(SEMI) }
"," { MAKE_NODE(COMMA) }
"=" { MAKE_NODE(ASSIGNOP) }
(>=)|(<=)|(==)|(!=)|<|> {
    Value value;
    value.str_value = strdup(yytext);
    yylval = make_node_terminal(RELOP, STR_VALUE, value);
    return RELOP;
}
"+" { MAKE_NODE(PLUS) }
"-" { MAKE_NODE(MINUS) }
"*" { MAKE_NODE(STAR) }
"/" { MAKE_NODE(DIV) }
"&&" { MAKE_NODE(AND) }
"||" { MAKE_NODE(OR) }
"." { MAKE_NODE(DOT) }
"!" { MAKE_NODE(NOT) }
(int)|(float) {
    Value value;
    value.str_value = strdup(yytext);
    yylval = make_node_terminal(TYPE, STR_VALUE, value);
    return TYPE;
}
"(" { MAKE_NODE(LP) }
")" { MAKE_NODE(RP) }
"[" { MAKE_NODE(LB) }
"]" { MAKE_NODE(RB) }
"{" { MAKE_NODE(LC) }
"}" { MAKE_NODE(RC) }
struct { MAKE_NODE(STRUCT) }
return { MAKE_NODE(RETURN) }
if { MAKE_NODE(IF) }
else { MAKE_NODE(ELSE) }
while { MAKE_NODE(WHILE) }
{L}({D}|{L})* {
    Value value;
    value.str_value = strdup(yytext);
    yylval = make_node_terminal(ID, STR_VALUE, value);
    return ID;
}

\n { yycolumn = 1; }
[ \t] { }
. {
    static char msg_buf[1024];
    sprintf(msg_buf, "Mysterious character \"%s\"", yytext);
    yyerror(msg_buf);
}
%%
extern int no_error;
yyerror(char* msg) {
    no_error = 0;
    fprintf(stderr, "Error type A at Line %d: %s\n", \
        yylineno, msg);
}
