%{
#define YYSTYPE Node*

#include <stdarg.h>
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
    yylval = make_node_terminal(name, empty_value); \
    return name;

#define MAKE_STR_NODE(name) Value value; \
    value.str_value = strdup(yytext); \
    yylval = make_node_terminal(name, value); \
    return name;

#define MAKE_INT_NODE(name) Value value; \
    value.int_value = strtol(yytext, NULL, 0); \
    yylval = make_node_terminal(name, value); \
    return name;

#define MAKE_FLOAT_NODE(name) Value value; \
    value.float_value = atof(yytext); \
    yylval = make_node_terminal(name, value); \
    return name;

%}

D   [0-9]
L   [a-zA-Z_]
H   [a-fA-F0-9]
E   [Ee][+-]?{D}+
S   [+-]

%%
"//" {
    char c;
    //while (c != '\n') c = input();
    while ((c = input()) != '\n' && c);
    unput(c);
}
"/*" {
    char c1, c2 = input();
    do {
        if (c2 == '\n') yylineno++;
        c1 = c2;
        c2 = input();
    } while (c1 != '*' && c1 || c2 != '/' && c2);
}

struct { MAKE_NODE(STRUCT) }
return { MAKE_NODE(RETURN) }
if { MAKE_NODE(IF) }
else { MAKE_NODE(ELSE) }
while { MAKE_NODE(WHILE) }
(int)|(float) { MAKE_STR_NODE(TYPE) }

{L}({D}|{L})* { MAKE_STR_NODE(ID) }

{S}?0[0-7]+ { MAKE_INT_NODE(INT) }
{S}?0[xX]{H}+ { MAKE_INT_NODE(INT) }
{S}?([1-9]{D}*)|0 { MAKE_INT_NODE(INT) }

{D}+{E} { MAKE_FLOAT_NODE(FLOAT) }
{D}*"."{D}+({E})? { MAKE_FLOAT_NODE(FLOAT) }
{D}+"."{D}*({E})? { MAKE_FLOAT_NODE(FLOAT) }

";" { MAKE_NODE(SEMI) }
"," { MAKE_NODE(COMMA) }
"=" { MAKE_NODE(ASSIGNOP) }
(>=)|(<=)|(==)|(!=)|<|> { MAKE_STR_NODE(RELOP) }
"+" { MAKE_NODE(PLUS) }
"-" { MAKE_NODE(MINUS) }
"*" { MAKE_NODE(STAR) }
"/" { MAKE_NODE(DIV) }
"&&" { MAKE_NODE(AND) }
"||" { MAKE_NODE(OR) }
"." { MAKE_NODE(DOT) }
"!" { MAKE_NODE(NOT) }
"(" { MAKE_NODE(LP) }
")" { MAKE_NODE(RP) }
"[" { MAKE_NODE(LB) }
"]" { MAKE_NODE(RB) }
"{" { MAKE_NODE(LC) }
"}" { MAKE_NODE(RC) }

\n { yycolumn = 1; yylineno++; }
[ \t\v\f] { }
. {
    extern void lex_error(char*);
    static char msg_buf[1024];
    sprintf(msg_buf, "mysterious character \"%s\"", yytext);
    lex_error(msg_buf);
}
%%
extern int no_error;
yyerror(char* s) {
    no_error = 0;
    printf("Error type B at Line %d: %s\n", yylloc.first_line, s);
}

void lex_error(char* s) {
    no_error = 0;
    printf("Error type A at Line %d: %s\n", yylloc.first_line, s);
}
