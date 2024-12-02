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
void error_tok(Token*tok,char*fmt,...);
Token*consume(char*op);
Token*consume_ident();
void expect(char*op);
int expect_number();
char*expect_ident();
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
    char*name;
    int offset;
};
typedef struct VarList VarList;
struct VarList{
    VarList*next;
    Var*var;
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
    ND_WHILE,
    ND_FOR,
    ND_BLOCK,//{...}
    ND_FUNCALL,
    ND_EXPR_STMT,
    ND_VAR,//local variable
    ND_NUM,
}NodeKind;

typedef struct Node Node;
struct Node{
    NodeKind kind;
    Node*next;
    Token*tok;

    Node*lhs;
    Node*rhs;
    //"if" or "while" statement
    Node*cond;//condition
    Node*then;
    Node*els;
    //"for" statement
    Node*init;
    Node*inc;
    //block
    Node*body;

    //function call
    char*funcname;
    Node*args;

    Var*var;
    int val;
};

//function definition
typedef struct Function Function;
struct Function{
    Function*next;
    char*name;
    VarList*params;

    Node*node;
    VarList*locals;
    int stack_size;
};

Function*program();

//
//codegen.c
//

void codegen(Function*prog);
