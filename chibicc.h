#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdarg.h>
#include<stdbool.h>
#include<string.h>


//
//tokenize.c
//

typedef enum{
    TK_RESERVED,
    TK_IDENT,
    TK_NUM,
    TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token{
    TokenKind kind;
    Token*next;
    int val;
    char*str;
    int len;
};

void error(char*fmt,...);
void error_at(char*loc,char*fmt,...);
char*strn_dup(char*p,int len);
bool consume(char*op);
Token*consume_ident();
void expect(char*op);
int expect_number();
bool at_eof();
Token*new_token(TokenKind kind,Token*cur,char*str,int len);
Token*tokenize();

extern char*user_input;
extern Token*token;

//
//parse.c
//

typedef struct Var Var;
struct Var{
    Var*next;
    char*name;
    int offset;
};

typedef enum{
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_EQ,
    ND_NE,//!=
    ND_LT,//<
    ND_LE,//<=
    ND_ASSIGN,
    ND_RETURN,
    ND_IF,
    ND_EXPR_STMT,
    ND_VAR,//local variable
    ND_NUM,
}NodeKind;

typedef struct Node Node;
struct Node{
    NodeKind kind;
    Node*next;

    Node*lhs;
    Node*rhs;

    Node*cond;//condition
    Node*then;
    Node*els;

    Var*var;
    int val;
};

typedef struct{
    Node*node;
    Var*locals;
    int stack_size;
}Program;

Program*program();

//
//codegen.c
//

void codegen(Program*prog);
