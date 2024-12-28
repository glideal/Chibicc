#include"chibicc.h"

typedef struct TagScope TagScope;
struct TagScope{
    TagScope*next;
    char*name;
    Type*ty;
};

VarList*locals;
VarList*globals;
VarList*scope;
TagScope*tag_scope;

//find variable by name
Var*find_var(Token*tok){
    for(VarList*vl=scope;vl;vl=vl->next){
        Var*var=vl->var;
        if(strlen(var->name)==tok->len
        &&!memcmp(tok->str,var->name,tok->len)){
            return var;
        }
    }
    return NULL;
}

TagScope*find_tag(Token*tok){
    for(TagScope*sc=tag_scope;sc;sc=sc->next){
        if(strlen(sc->name)==tok->len
        &&!memcmp(tok->str,sc->name,tok->len)){
            return sc;
        }
        return NULL;
    }
}

Node*new_node(NodeKind kind,Token*tok){
    Node*node=calloc(1,sizeof(Node));
    node->kind=kind;
    node->tok=tok;
    return node;
}

Node*new_binary(TokenKind kind,Node*lhs,Node*rhs,Token*tok){
    Node*node=new_node(kind,tok);
    node->lhs=lhs;
    node->rhs=rhs;
    return node;
}

Node*new_unary(NodeKind kind,Node*expr,Token*tok){
    Node*node=new_node(kind,tok);
    node->lhs=expr;
    return node;
}

Node*new_num(int val,Token*tok){
    Node*node=new_node(ND_NUM,tok);
    node->val=val;
    return node;
}

Node*new_var(Var*var,Token*tok){
    Node*node=new_node(ND_VAR,tok);
    node->var=var;
    return node;
}

Var*push_var(char*name,Type*ty,bool is_local){
    Var*var=calloc(1,sizeof(Var));
    VarList*vl=calloc(1,sizeof(VarList));
    var->name=name;
    var->ty=ty;
    var->is_local=is_local;
    vl->var=var;
    if(is_local){
        vl->next=locals;
        locals=vl;
    }else{
        vl->next=globals;
        globals=vl;
    }

    VarList*sc=calloc(1,sizeof(VarList));
    sc->var=var;
    sc->next=scope;
    scope=sc;

    return var;
}

char*new_label(){
    static int cnt=0;
    char buf[20];
    sprintf(buf,".L.date.%d",cnt);
    cnt++;
    return strn_dup(buf,20);
}

Program*program();
Function*function();
Type*basetype();
Type*struct_decl();//declaration
Member*struct_member();
void global_var();
Node*declaration();
bool is_typename();
Node*stmt();
Node*expr();
Node*assign();
Node*equality();
Node*relational();
Node*add();
Node*mul();
Node*unary();
Node*postfix();
Node*primary();

bool is_function(){
    Token*tok=token;
    basetype();
    bool isfunc=consume_ident()&&consume("(");
    token=tok;
    return isfunc;
}

//program=(global_var|function)*
Program*program(){
    Function head;
    head.next=NULL;
    Function*cur=&head;
    globals=NULL;

    while(!at_eof()){
        if(is_function()){
            cur->next=function();
            cur=cur->next;
        }else{
            global_var();
        }
    }

    Program*prog=calloc(1,sizeof(Program));
    prog->globals=globals;
    prog->fns=head.next;

    return prog;
}

//basetype=("char"|"int"|struct_decl)"*"*
Type*basetype(){
    if(!is_typename(token)){
        error_tok(token,"typename expected");
    }
    Type*ty;
    if(consume("char")){
        ty=char_type();
    }else if(consume("int")){
        ty=int_type();
    }else{
        ty=struct_decl();
    }

    while(consume("*")){
        ty=pointer_to(ty);
    }
    return ty;
}

Type*read_type_suffix(Type*base){
    if(!consume("[")){
        return base;
    }
    int sz=expect_number();
    expect("]");
    base=read_type_suffix(base);
    return array_of(base,sz);
}

void push_tag_scope(Token*tok,Type*ty){
    TagScope*sc=calloc(1,sizeof(TagScope));
    sc->next=tag_scope;
    sc->name=strn_dup(tok->str,tok->len);
    sc->ty=ty;
    tag_scope=sc;
}

//struct_decl="struct" ident
//           |"struct" ident? "{" struct_member "}"
Type*struct_decl(){
    expect("struct");

    Token*tag=consume_ident();
    if(tag&&!peek("{")){
        TagScope*sc=find_tag(tag);
        if(!sc){
            error_tok(tag,"unknown struct type");
        }
        return sc->ty;
    }
    expect("{");

    Member head;
    head.next=NULL;
    Member*cur=&head;

    while(!consume("}")){
        cur->next=struct_member();
        cur=cur->next;
    }

    Type*ty=calloc(1,sizeof(Type));
    ty->kind=TY_STRUCT;
    ty->members=head.next;

    //assign offset within the struct to members
    /*解説

    struct{
        char p;
        char q;
        int num;
        int val;
        char r;
    }x;
    を例に考える。
    この時点で順番通り、p->q->num->val->rとなっている
    x.pのalignは1。offsetはalign_to(offset,1)=offset=0; offset=1にインクリメント
    x.qのalignは1。offsetはalign_to(offset,1)=offset=1; offset=2にインクリメント
    x.numのalignは8。offsetはalign_to(offset,8)=8  offset=16にインクリメント
    x.valのalignは8。offsetはalign_to(offset,8)=16  offset=24にインクリメント
    x.rのalignは1。offsetはalign_to(offset,1)=offset=24; offset=25にインクリメント
    そしてty->alignは8。これは要素内のalignの最大値である

    32|_|      =align_to(end,ty->align)...size_of
      |_|
      |_|
      |_|
      |_|
      |_|
      |_|
      |_|      =emd...size_of
    24|_|<-x.r =mem->offset()...struct_decl
      |_|
      |_|
      |_|
      |_|
      |_|
      |_|
      |_|
    16|_|<-x.val
      |_|
      |_|
      |_|
      |_|
      |_|
      |_|
      |_|
     8|_|<-x.num
      |_|
      |_|
      |_|
      |_|
      |_|
      |_|
      |_|<-x.q
     0|_|<-x.p <-x

     size_of関数でおいてstruct型は以下のような返り値(最後２行)
     //memは最後の要素。ここではx.r
            int end=mem->offset+size_of(mem->ty);
            return align_to(end,ty->align);
    end=24+1
    align_to(end,ty->align)=align_to(19,8)=32
    */
    int offset=0;
    for(Member*mem=ty->members;mem;mem=mem->next){
        offset=align_to(offset,mem->ty->align);
        mem->offset=offset;
        offset+=size_of(mem->ty);

        if(ty->align < mem->ty->align){//ty->alignはcallocで0に初期化されてる
            ty->align=mem->ty->align;
        }
    }

    //struct tag declaration
    if(tag){
        push_tag_scope(tag,ty);
    }

    return ty;
}

//struct_member=basetype ident ("[" num "]")* ";"
Member*struct_member(){
    Member*mem=calloc(1,sizeof(Member));
    mem->ty=basetype();
    mem->name=expect_ident();
    mem->ty=read_type_suffix(mem->ty);
    expect(";");
    return mem;
}

VarList*read_func_param(){
    Type*ty=basetype();
    char*name=expect_ident();
    ty=read_type_suffix(ty);

    VarList*vl=calloc(1,sizeof(VarList));
    vl->var=push_var(name,ty,true);
    return vl;
}

VarList*read_func_params(){
    if(consume(")")){
        return NULL;
    }

    VarList head;
    head.next=NULL;
    VarList*cur=&head;
    cur->next=read_func_param();
    cur=cur->next;
    while(!consume(")")){
        expect(",");
        cur->next=read_func_param();
        cur=cur->next;
    }
    return head.next;
}

//function=basetype ident "(" params? ")" "{" stmt* "}"
//params  =param("," param)*
//param   =basetype ident
Function*function(){
    //printf("program\n");
    locals=NULL;
    Function*fn=calloc(1,sizeof(Function));
    basetype();
    fn->name=expect_ident();

    expect("(");
    fn->params=read_func_params();
    expect("{");

    Node head;
    head.next=NULL;
    Node*cur=&head;

    while(!consume("}")){
        cur->next=stmt();
        cur=cur->next;
    }
    //return head.next;

    fn->node=head.next;
    fn->locals=locals;
    return fn;
}

void global_var(){
    Type*ty=basetype();
    char*name=expect_ident();
    ty=read_type_suffix(ty);
    expect(";");
    push_var(name,ty,false);
}

//declaration=basetype ident ( "[" num "]" )*("=" expr)";"
Node*declaration(){
    Token*tok=token;
    Type*ty=basetype();
    char*name=expect_ident();
    ty=read_type_suffix(ty);
    Var*var=push_var(name,ty,true);

    if(consume(";")){
        return new_node(ND_NULL,tok);
    }

    expect("=");
    Node*lhs=new_var(var,tok);
    Node*rhs=expr();
    expect(";");
    Node*node=new_binary(ND_ASSIGN,lhs,rhs,tok);
    return new_unary(ND_EXPR_STMT,node,tok);
}

Node*read_expr_stmt(){
    Token*tok=token;
    Node*node=new_unary(ND_EXPR_STMT,expr(),tok);
    return node;

}

bool is_typename(){
    return peek("char")||peek("int")||peek("struct");
}

///stmt="return" expr ";" | expr ";"
///    |"if" "(" expr ")" stmt ("else" stmt)?
///    |"while" "(" expr ")" stmt
///    |"for" "(" expr? ";" expr? ";" expr? ")" stmt
///    |"{" stmt* "}"
///    |declaration
///    | expr ";"
Node*stmt(){
    //printf("stmt\n");
    Token*tok;
    if(tok=consume("return")){
        Node*node=new_unary(ND_RETURN,expr(),tok);
        expect(";");
        return node;
    }
    if(tok=consume("if")){
        Node*node=new_node(ND_IF,tok);
        expect("(");
        node->cond=expr();
        expect(")");
        node->then=stmt();
        if(consume("else")){
            node->els=stmt();
        }
        return node;
    }
    if(tok=consume("while")){
        Node*node=new_node(ND_WHILE,tok);
        expect("(");
        node->cond=expr();
        expect(")");
        node->then=stmt();
        return node;
    }
    if(tok=consume("for")){
        Node*node=new_node(ND_FOR,tok);
        expect("(");
        if(!consume(";")){
            node->init=read_expr_stmt();
            expect(";");
        }
        if(!consume(";")){
            node->cond=expr();
            expect(";");
        }
        if(!consume(")")){
            node->inc=read_expr_stmt();
            expect(")");
        }
        node->then=stmt();
        return node;
    }
    if(tok=consume("{")){
        Node head;
        head.next=NULL;
        Node*cur=&head;

        VarList*sc=scope;
        while(!consume("}")){
            cur->next=stmt();
            cur=cur->next;
        }
        scope=sc;

        Node*node=new_node(ND_BLOCK,tok);
        node->body=head.next;
        return node;
    }
    if(is_typename()){
        return declaration();
    }
    Node *node=read_expr_stmt();
    expect(";");
    return node;
}

//expr=assign
Node*expr(){
    return assign();
}

//assign=equality("=" assign)?
Node*assign(){
    //printf("assign\n");
    Node*node=equality();
    Token*tok;
    if(tok=consume("=")){
        node=new_binary(ND_ASSIGN,node,assign(),tok);
    }
    return node;
}

//equality=relational("==" relational | "!=" relational)*
Node*equality(){
    //printf("equality\n");
    Node*node=relational();
    Token*tok;
    
    for(;;){
        if(tok=consume("==")){
            node=new_binary(ND_EQ,node,relational(),tok);
        }else if(tok=consume("!=")){
            node=new_binary(ND_NE,node,relational(),tok);
        }else{
            return node;
        }
    }
}

//relatonal=add("<" add | "<=" add | ">" add | ">=" add)*
Node*relational(){
    //printf("relational\n");
    Node*node=add();
    Token*tok;

    if(tok=consume("<")){
        node=new_binary(ND_LT,node,add(),tok);
    }else if(tok=consume("<=")){
        node=new_binary(ND_LE,node,add(),tok);
    }else if(tok=consume(">")){
        node=new_binary(ND_LT,add(),node,tok);
    }else if(tok=consume(">=")){
        node=new_binary(ND_LE,add(),node,tok);
    }else{
        return node;
    }
}

//add=mul("+" mul |"-" mul)*
Node*add(){
    //printf("add\n");
    Node*node=mul();
    Token*tok;

    for(;;){
        if(tok=consume("+")){
            node=new_binary(ND_ADD,node,mul(),tok);
        }else if(tok=consume("-")){
            node=new_binary(ND_SUB,node,mul(),tok);
        }else{
            return node;
        }
    }
}

//mul=unary("*" unary | "/" unary)*
Node*mul(){
    //printf("mul\n");
    Node*node=unary();
    Token*tok;
    for(;;){
        if(tok=consume("*")){
            node=new_binary(ND_MUL,node,unary(),tok);
        }else if(tok=consume("/")){
            node=new_binary(ND_DIV,node,unary(),tok);
        }else{
            return node;
        }
    }
}

//unary=("+" | "-" | "*" | "&" )?unary  
//     | postfix
Node*unary(){
    //printf("unary\n");
    Token*tok;
    if(consume("+")){
        return unary();
    }
    if(tok=consume("-")){
        return new_binary(ND_SUB,new_num(0,tok),unary(),tok);
    }
    if(tok=consume("&")){
        return new_unary(ND_ADDR,unary(),tok);
    }
    if(tok=consume("*")){
        return new_unary(ND_DEREF,unary(),tok);
    }
    return postfix();
}

//postfix=primary ("[" expr "]" | "." ident)*
Node*postfix(){
    Node*node=primary();
    Token*tok;
    for(;;){
        if(tok=consume("[")){
            Node*exp=new_binary(ND_ADD,node,expr(),tok);
            expect("]");
            node=new_unary(ND_DEREF,exp,tok);
            continue;
        }

        if(tok=consume(".")){
            node=new_unary(ND_MEMBER,node,tok);
            node->member_name=expect_ident();
            continue;
        }
        return node;
    }
}

//stmt-expr="(" "{" stmt stmt* "}" ")"
//
//Statement expression is a GNU C extension
//
//GNU is an abbreviationfor GNU's Not UNIX
//GNU is general term for free software 
//aimed at Unix-compatible system that is not UNIX
Node*stmt_expr(Token*tok){
    VarList*sc=scope;

    Node*node=new_node(ND_STMT_EXPR,tok);
    node->body=stmt();
    Node*cur=node->body;

    while(!consume("}")){
        cur->next=stmt();
        cur=cur->next;
    }
    expect(")");

    scope=sc;

    if(cur->kind!=ND_EXPR_STMT){
        error_tok(cur->tok,"stmt expr returning void is not supported");
    }
    *cur=*cur->lhs;
    /*
    stmt-expr="(" "{" stmt stmt* "}" ")"のstmtは
    すべてnode->ty=ND_EXPR_STMT
    node->lhs=unary()である。
    stmt-expr="(" "{" stmt stmt* "}" ")"のstmtのうち、
    一番最後のstmtだけ,ND_STMT_EXPRを挟まず,直でnode->lhsを持ってきている。
    なんで？？
    */
    return node;
}

//func-args="("(assign (","assign)*)? ")"
Node*func_args(){
    if(consume(")")){
        return NULL;
    }
    Node head;
    head.next=NULL;
    Node*cur=&head;
    cur->next=assign();
    cur=cur->next;
    while(!consume(")")){
        expect(",");
        cur->next=assign();
        cur=cur->next;
    }
    return head.next;   
}
//primary= "(" expr "{" stmt-expr-tail
//         | "(" expr ")" | "sizeof" unary | ident func-args? 
//         | str | num 
Node*primary(){
    //printf("primary\n");
    Token*tok;
    if(tok=consume("(")){
        if(consume("{")){
            return stmt_expr(tok);
        }
        Node*node=expr();
        expect(")");
        return node;
    }
    if(tok=consume("sizeof")){
        return new_unary(ND_SIZEOF,unary(),tok);
    }
    if(tok=consume_ident()){
        if(consume("(")){
            Node*node=new_node(ND_FUNCALL,tok);
            node->funcname=strn_dup(tok->str,tok->len);
            node->args=func_args();
            return node;
        }
        Var*var=find_var(tok);
        if(!var){
            error_tok(tok,"undefined variable");
        }
        return new_var(var,tok);
    }
    tok=token;

    if(tok->kind==TK_STR){
        token=token->next;

        Type*ty=array_of(char_type(),tok->cont_len);
        Var*var=push_var(new_label(),ty,false);
        var->contents=tok->contents;
        var->cont_len=tok->cont_len;
        return new_var(var,tok);
    }
    if(tok->kind!=TK_NUM){
        error_tok(tok,"expected expression");
    }
    return new_num(expect_number(),tok);
}
