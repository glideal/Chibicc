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
    long val;
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
long expect_number();
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
    Token*tok;
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
    ND_BITAND,
    ND_BITOR,
    ND_BITXOR,
    ND_LONGAND,
    ND_LONGOR,
    ND_EQ,
    ND_NE,//!=
    ND_LT,//<
    ND_LE,//<=
    ND_ASSIGN,
    ND_PRE_INC,//++x
    ND_PRE_DEC,//--x
    ND_POST_INC,//x++
    ND_POST_DEC,//x--
    ND_A_ADD,// +=
    ND_A_SUB,// -=
    ND_A_MUL,// *=
    ND_A_DIV,// /=
    ND_COMMA,
    ND_MEMBER,//.(struct member access)
    ND_ADDR,//&
    ND_DEREF,//*
    ND_NOT,
    ND_BITNOT,
    ND_RETURN,
    ND_IF,
    ND_WHILE,
    ND_FOR,
    ND_SWITCH,
    ND_CASE,
    ND_SIZEOF,//sizeof演算子.byte数を変えす
    ND_BLOCK,//{...}
    ND_BREAK,
    ND_CONTINUE,
    ND_GOTO,
    ND_LABEL,
    ND_FUNCALL,
    ND_EXPR_STMT,
    ND_STMT_EXPR,
    ND_VAR,//local variable
    ND_NUM,
    ND_CAST,//type cast
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

    //goto or labeled statement
    char*label_name;

    //switch-case
    Node*case_next;
    Node*default_case;
    int case_label;
    int case_end_label;

    Var*var;
    long val;
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
    TY_VOID,
    TY_BOOL,
    TY_CHAR,
    TY_SHORT,
    TY_INT,
    TY_LONG,
    TY_ENUM,

    TY_PTR,
    TY_ARRAY,
    /*
    TY_ARRAYは変数領域を格納するときの掛け算に使うだけで
    代入、返り値として使う時は使わない
    */
    TY_STRUCT,
    TY_FUNC,
}TypeKind;

/*
new_type()は*_type()でも使われている。
*/
struct Type{
    TypeKind kind;      //new_type()
    bool is_typedef;
    bool is_static;
    bool is_incomplete;
    int align;          //new_type()
    Type*base;
    int array_size;
    Member*members;
    Type*return_ty;
};

struct Member{
    Member*next;
    Type*ty;
    Token*tok;
    char*name;
    int offset;
};

int align_to(int n,int align);
Type*void_type();
Type*bool_type();
Type*char_type();
Type*short_type();
Type*int_type();
Type*long_type();
Type*enum_type();
Type*struct_type();
Type*func_type(Type*return_ty);
Type*pointer_to(Type*base);
Type*array_of(Type*base,int size);
int size_of(Type*ty,Token*tok);

void add_type(Program*prog);

//
//codegen.c
//

void codegen(Program*prog);
