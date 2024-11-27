#include<stdio.h>
#include<stdlib.h>
#include<ctype.h>
#include<stdarg.h>
#include<stdbool.h>
#include<string.h>

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

char*user_input;

Token *token;//current token

void error(char*fmt,...){
    va_list ap;
    va_start(ap,fmt);
    vfprintf(stderr,fmt,ap);
    fprintf(stderr,"\n");
    exit(1);
}

void error_at(char*loc,char*fmt,...){
    va_list ap;
    va_start(ap,fmt);

    int pos=loc-user_input;
    fprintf(stderr,"%s\n",user_input);
    fprintf(stderr,"%*s",pos,"");
    fprintf(stderr,"^ ");
    vfprintf(stderr,fmt,ap);
    fprintf(stderr,"\n");

    exit(1);
}

bool consume(char* op){
    if(token->kind!=TK_RESERVED||strlen(op)!=token->len||memcmp(token->str,op,token->len)){
        return false;
    }else{
        token=token->next;
        return true;
    }
}

void expect(char* op){
    if(token->kind!=TK_RESERVED||strlen(op)!=token->len||memcmp(token->str,op,token->len)){
        error_at(token->str,"expected \"%s\"",op);
    }

    token=token->next;
}

int expect_number(){
    if(token->kind!=TK_NUM){
        error_at(token->str,"expected a number");
    }

    int val=token->val;
    token=token->next;
    return val;
}

bool at_eof(){
    return token->kind==TK_EOF;
}

Token*new_token(TokenKind kind,Token*cur,char*str,int len){
    Token*tok=calloc(1,sizeof(Token));
    tok->kind=kind;
    tok->str=str;
    tok->len=len;
    cur->next=tok;
    return tok;
}

bool startswith(char*p,char*q){
    return memcmp(p,q,strlen(q))==0;
}

Token*tokenize(){
    char*p=user_input;
    Token head;
    head.next=NULL;
    Token*cur=&head;

    while(*p){
        if(isspace(*p)){
            p++;
            continue;
        }

        if(startswith(p,"==")||startswith(p,"!=")||startswith(p,"<=")||startswith(p,">=")){
            cur=new_token(TK_RESERVED,cur,p,2);
            p+=2;
            continue;
        }
        /*
        本来は
        strchr(const char*s,int c)
        s=対象の文字列 : c=探す文字 (cをchar型にキャストしたもの)
        であるが、ここでは
        cがsにあるいずれかの文字でないかをチェックしている。
        天才。
        */
        if(strchr("+-*/()<>",*p)){
            cur=new_token(TK_RESERVED,cur,p,1);
            p++;
            continue;
        }
        
        if(isdigit(p[0])){
            cur=new_token(TK_NUM,cur,p,0);
            char*q=p;
            cur->val=strtol(p,&p,10);
            cur->len=p-q;
            continue;
        }
        error_at(p,"expected a number");
    }
    new_token(TK_EOF,cur,p,0);
    return head.next;
}

//
//parser
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
    Node*lhs;
    Node*rhs;
    int val;
};

Node*new_node(NodeKind kind){
    Node*node=calloc(1,sizeof(Node));
    node->kind=kind;
    return node;
}

Node*new_binary(TokenKind kind,Node*lhs,Node*rhs){
    Node*node=new_node(kind);
    node->lhs=lhs;
    node->rhs=rhs;
    return node;
}

Node*new_num(int val){
    Node*node=new_node(ND_NUM);
    node->val=val;
    return node;
}

Node*expr();
Node*equality();
Node*relational();
Node*add();
Node*mul();
Node*unary();
Node*primary();

//expr=equality
Node*expr(){
    return equality();
}

//equality=relational("==" relational | "!=" relational)*
Node*equality(){
    Node*node=relational();
    
    for(;;){
        if(consume("==")){
            node=new_binary(ND_EQ,node,relational());
        }else if(consume("!=")){
            node=new_binary(ND_NE,node,relational());
        }else{
            return node;
        }
    }
}

//relatonal=add("<" add | "<=" add | ">" add | ">=" add)*
Node*relational(){
    Node*node=add();

    if(consume("<")){
        node=new_binary(ND_LT,node,add());
    }else if(consume("<=")){
        node=new_binary(ND_LE,node,add());
    }else if(consume(">")){
        node=new_binary(ND_LT,add(),node);
    }else if(consume(">=")){
        node=new_binary(ND_LE,add(),node);
    }else{
        return node;
    }
}

//add=mul("+" mul |"-" mul)*
Node*add(){
    Node*node=mul();

    for(;;){
        if(consume("+")){
            node=new_binary(ND_ADD,node,mul());
        }else if(consume("-")){
            node=new_binary(ND_SUB,node,mul());
        }else{
            return node;
        }
    }
}

//mul=unary("*" unary | "/" unary)*
Node*mul(){
    Node*node=unary();
    for(;;){
        if(consume("*")){
            node=new_binary(ND_MUL,node,unary());
        }else if(consume("/")){
            node=new_binary(ND_DIV,node,unary());
        }else{
            return node;
        }
    }
}

//unary=("+" | "-")?unary  | primary
Node*unary(){
    if(consume("+")){
        return unary();
    }
    if(consume("-")){
        return new_binary(ND_SUB,new_num(0),unary());
    }
    return primary();
}

//primary="(" expr ")" | num
Node*primary(){
    Node*node=calloc(1,sizeof(Node*));
    if(consume("(")){
        node=expr();
        expect(")");
    }else{
        node=new_num(expect_number());
    }
    return node;
}

void gen(Node*node){
    if(node->kind==ND_NUM){
        printf("  push %d\n",node->val);
        return;
    }

    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch(node->kind){
        case ND_ADD:
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            printf("  sub rax, rdi\n");
            break;
        case ND_MUL:
            printf("  imul rax, rdi\n");
            break;
        case ND_DIV:
            /*
            idiv (arg)
            >>>(rdx+rax)(128bit) / (arg)(64bit) = rax ... rdx
            ^raxに商、rdxに余りはidivのデフフォルト

            cqo 
            >>>rax(64bit)を(rax+rdx)(128bit)に引き延ばす
            */
            printf("  cqo\n");
            printf("  idiv rdi\n");
            break;
        case ND_EQ:
            printf("  cmp rax, rdi\n");
            /*
            seteはcmpした二つの値が同じ値だったら、引数のレジスタに1をセット。違う値なら0をセット。
            alはレジスタ //seteは8bitレジスタにしか引数にとれない
            alはraxレジスタの下位8bit。
            movzb rax, al　はraxの下位8bitにalをセットして、上位56bitをゼロクリアしている。
            */
            printf("  sete al\n");//sete=set equal
            printf("  movzb rax, al\n");
            break;
        case ND_NE:
            printf("  cmp rax, rdi\n");
            printf("  setne al\n");//sete=set not equal
            printf("  movzb rax, al\n");
            break;
        case ND_LT:
            printf("  cmp rax, rdi\n");
            printf("  setl al\n");
            printf("  movzb rax, al\n");
            break;
        case ND_LE:
            printf("  cmp rax, rdi\n");
            printf("  setle al\n");
            printf("  movzb rax, al\n");
            break;
    }

    printf("  push rax\n");
}

int main(int argc, char **argv){
    if(argc!=2){
        fprintf(stderr,"%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    user_input=argv[1];
    token=tokenize();
    Node*node=expr();

    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    gen(node);

    printf("  pop rax\n");
    printf("  ret\n");
    printf(".section .note.GNU-stack,\"\",@progbits\n");

    return 0;
}