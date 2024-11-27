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
bool consume(char*op);
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

typedef enum{
    ND_ADD,
    ND_SUB,
    ND_MUL,
    ND_DIV,
    ND_EQ,
    ND_NE,//!=
    ND_LT,//<
    ND_LE,//<=
    ND_NUM,
}NodeKind;

typedef struct Node Node;
struct Node{
    NodeKind kind;
    Node*next;
    Node*lhs;
    Node*rhs;
    int val;
};

Node*program();

//
//codegen.c
//

void codegen(Node*node);
