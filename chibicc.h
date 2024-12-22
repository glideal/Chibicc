#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdarg.h>
#include<stdbool.h>
#include<string.h>
#include<errno.h>


typedef struct Type Type;
typedef struct Member Member;


//
//tokenize.c
//

typedef enum{
    TK_RESERVED,
    TK_IDENT,
    TK_STR,
    TK_NUM,
    TK_EOF,
} TokenKind;

typedef struct Token Token;
struct Token{
    TokenKind kind;
    Token*next;
    Type*ty;
    int val;
    char*str;
    int len;

    char*contents;//string literal contents including terminating '\0'
    char cont_len;
};

void error(char*fmt,...);
void error_at(char*loc,char*fmt,...);
char*strn_dup(char*p,int len);
void error_tok(Token*tok,char*fmt,...);
Token*peek(char*s);
Token*consume(char*s);
Token*consume_ident();
void expect(char*op);
int expect_number();
char*expect_ident();
bool at_eof();
Token*new_token(TokenKind kind,Token*cur,char*str,int len);
Token*tokenize();

extern char*filename;
extern char*user_input;
extern Token*token;

//
//parse.c
//

typedef struct Var Var;
struct Var{
    char*name;
    Type*ty;
    bool is_local;
    int offset;//local variable

    //global variables
    char*contents;
    int cont_len;
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
    ND_MEMBER,//.(struct member access)
    ND_ADDR,//&
    ND_DEREF,//*
    ND_RETURN,
    ND_IF,
    ND_WHILE,
    ND_FOR,
    ND_SIZEOF,//sizeof演算子.byte数を変えす
    ND_BLOCK,//{...}
    ND_FUNCALL,
    ND_EXPR_STMT,
    ND_STMT_EXPR,
    ND_VAR,//local variable
    ND_NUM,
    ND_NULL
}NodeKind;

typedef struct Node Node;
struct Node{
    NodeKind kind;
    Node*next;
    Type*ty;
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
    //block or statement expression
    Node*body;

    //struct member access
    char*member_name;
    Member*member;

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

typedef struct{
    VarList*globals;
    Function*fns;
}Program;

Program*program();

//
// typing.c
//

/*
配列[]について
Cにおいては配列アクセスのための[]演算子というものはありません。
Cの[]は、ポインタ経由で配列の要素にアクセスするための簡便な記法にすぎないのです。
*/
/*
配列に対して定義されている演算子は、配列のサイズを返すsizeof演算子と、
配列の先頭の要素のポインタを返す&演算子だけ
a[3]がコンパイルできるのは
 Cでは、a[3]は*(a+3)と等価であるものとして定義されているから
 Cにおいては配列アクセスのための[]演算子というものはありません。
 Cの[]は、ポインタ経由で配列の要素にアクセスするための簡便な記法にすぎないのです
*/
/*
というわけで、コンパイラは、ほとんどの演算子の実装において、
配列をポインタに型変換するということを行わなければなりません
*/
typedef enum{
    TY_CHAR,
    TY_INT,

    TY_PTR,
    TY_ARRAY,
    /*
    TY_ARRAYは変数領域を格納するときの掛け算に使うだけで
    代入、返り値として使う時は使わない
    */
    TY_STRUCT
}TypeKind;

struct Type{
    TypeKind kind;
    Type*base;
    int array_size;
    Member*members;
};

struct Member{
    Member*next;
    Type*ty;
    char*name;
    int offset;
};

Type*char_type();
Type*int_type();
Type*pointer_to(Type*base);
Type*array_of(Type*base,int size);
int size_of(Type*ty);

void add_type(Program*prog);

//
//codegen.c
//

void codegen(Program*prog);
