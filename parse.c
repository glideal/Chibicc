#include"chibicc.h"

//Scope for local variables,global variables,or typedefs
typedef struct VarScope VarScope;
struct VarScope{
    VarScope*next;
    char*name;
    int depth;
    Var*var;
    Type*type_def;
    Type*enum_ty;
    int enum_val;
};

typedef struct TagScope TagScope;
struct TagScope{
    TagScope*next;
    char*name;
    int depth;
    Type*ty;
};

typedef struct{
    VarScope*var_scope;
    TagScope*tag_scope;
}Scope;

VarList*locals;
VarList*globals;
VarScope*var_scope;
TagScope*tag_scope;
int scope_depth;

Node*current_switch;

Scope*enter_scope(){
    Scope*sc=calloc(1,sizeof(Scope));
    sc->var_scope=var_scope;
    sc->tag_scope=tag_scope;
    ++scope_depth;
    return sc;
}

void leave_scope(Scope*sc){
    var_scope=sc->var_scope;
    tag_scope=sc->tag_scope;
    --scope_depth;
}



//find variable or a typedef by name
VarScope*find_var(Token*tok){
    for(VarScope*sc=var_scope;sc;sc=sc->next){
        if(strlen(sc->name)==tok->len
        &&!memcmp(tok->str,sc->name,tok->len)){
            return sc;
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

Node*new_num(long val,Token*tok){
    Node*node=new_node(ND_NUM,tok);
    node->val=val;
    return node;
}

Node*new_var(Var*var,Token*tok){
    Node*node=new_node(ND_VAR,tok);
    node->var=var;
    return node;
}

VarScope*push_scope(char*name){
    VarScope*sc=calloc(1,sizeof(VarScope));
    sc->name=name;
    sc->next=var_scope;
    sc->depth=scope_depth;
    var_scope=sc;
    return sc;
}

Var*push_var(char*name,Type*ty,bool is_local,Token*tok){
    Var*var=calloc(1,sizeof(Var));
    VarList*vl=calloc(1,sizeof(VarList));
    var->name=name;
    var->ty=ty;
    var->is_local=is_local;
    var->tok=tok;
    vl->var=var;
    if(is_local){
        vl->next=locals;
        locals=vl;
    }else if(ty->kind!=TY_FUNC){
        vl->next=globals;
        globals=vl;
    }

    return var;
}

Type*find_typedef(Token*tok){
    if(tok->kind==TK_IDENT){
        VarScope*sc=find_var(token);
        if(sc){
            return sc->type_def;
        }
    }
    return NULL;
}

char*new_label(){
    static int cnt=0;
    char buf[20];
    sprintf(buf,".L.date.%d",cnt);
    cnt++;
    return strn_dup(buf,20);
}

Program*program();//
Function*function();//
Type*type_specifier();
Type*declarator(Type*ty,char**name);
Type*abstract_declarator(Type*ty);
Type*type_suffix(Type*ty);
Type*type_name();
Type*struct_decl();//declaration
Type*enum_specifier();
Member*struct_member();
void global_var();
Node*declaration();//
bool is_typename();
Node*stmt();//
Node*expr();
long eval(Node*node);
long const_expr();
Node*assign();
Node*conditional();
Node*longor();
Node*longand();
Node*bitand();
Node*bitor();
Node*bitxor();
Node*equality();
Node*relational();
Node*shift();
Node*add();
Node*mul();
Node*cast();
Node*unary();
Node*postfix();
Node*primary();

bool is_function(){
    Token*tok=token;
    Type*ty=type_specifier();
    char*name=NULL;
    declarator(ty,&name);
    bool isfunc=name&&consume("(");
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
            Function*fn=function();
            if(!fn){
                continue;
            }
            cur->next=fn;
            cur=cur->next;
            
            continue;
        }
        global_var();
    }

    Program*prog=calloc(1,sizeof(Program));
    prog->globals=globals;
    prog->fns=head.next;

    return prog;
}

//type_specifier=builtin-type|struct-dec1|typedef-name|enum-specifier
//builtin-type="void"
//            |"_Bool" 
//            |"char"
//            |"short"|"short" "int"|"int" "short"
//            |"int"
//            |"long"|"long" "int"|"int" "long"
//
//note that "typedef" and "static" can appear anywhere in a type-specifier
Type*type_specifier(){
    if(!is_typename(token)){
        error_tok(token,"typename expected");
    }
    Type*ty=NULL;

    enum{
        VOID=1<<1,
        BOOL=1<<3,
        CHAR=1<<5,
        SHORT=1<<7,
        INT=1<<9,
        LONG=1<<11,
    };

    int base_type=0;
    Type*user_type=NULL;

    bool is_typedef=false;
    bool is_static=false;

    Token*tok=token;
    for(;;){
        //read one token at a time
        if(consume("typedef")){
            is_typedef=true;
        }else if(consume("static")){
            is_static=true;
        }else if(consume("void")){
            base_type+=VOID;
        }else if(consume("_Bool")){
            base_type+=BOOL;
        }else if(consume("char")){
            base_type+=CHAR;
        }else if(consume("short")){
            base_type+=SHORT;
        }else if(consume("int")){
            base_type+=INT;
        }else if(consume("long")){
            base_type+=LONG;
        }else if(peek("struct")){
            if(base_type||user_type){
                break;
            }
            user_type=struct_decl();
        }else if(peek("enum")){
            if(base_type||user_type){
                break;
            }
            user_type=enum_specifier();
        }else{
            if(base_type||user_type){
                break;
            }
            Type*ty=find_typedef(token);
            if(!ty){
                break;
            }
            token=token->next;
            user_type=ty;
        }
    }

    switch(base_type){
        case VOID:
            ty=void_type();
            break;
        case BOOL:
            ty=bool_type();
            break;
        case CHAR:
            ty=char_type();
            break;
        case SHORT:
        case SHORT+INT:
            ty=short_type();
            break;
        case INT:
            ty=int_type();
            break;
        case LONG:
        case LONG+INT:
            ty=long_type();
            break;
        case 0:
            //if there's no type specifier, it becomes int.
            //for example, 'typedef x' defines x as an alias for int
            ty=user_type?user_type:int_type();
            break;
        default:
            error_tok(tok,"invalid type");
    }


    ty->is_typedef=is_typedef;
    ty->is_static=is_static;
    assert(ty);
    
    return ty;
}

//declarator="*"* ("(" declarator ")" |ident) type-suffix
Type*declarator(Type*ty,char**name){
    while(consume("*")){
        ty=pointer_to(ty);
    }
    if(consume("(")){
        Type*placeholder=calloc(1,sizeof(Type));
        Type*new_ty=declarator(placeholder,name);
        expect(")");
        *placeholder=*type_suffix(ty);
        return new_ty;
    }
    *name=expect_ident();
    return type_suffix(ty);
}

//abstract-declarator="*"* ("(" abstract-declarator ")" |ident) type-suffix
Type*abstract_declarator(Type*ty){
    while(consume("*")){
        ty=pointer_to(ty);
    }
    if(consume("(")){
        Type*placeholder=calloc(1,sizeof(Type));
        Type*new_ty=abstract_declarator(placeholder);
        expect(")");
        *placeholder=*type_suffix(ty);
        return new_ty;
    }
    return type_suffix(ty);
}

//type-suffix=("[" const_expr? "]" type-suffix)?
Type*type_suffix(Type*ty){
    /*
    ex)int*num[3]
    token->str..."["
    (Type*)ty{
        TypeKind kind;      //TY_PTR
        bool is_typedef;    //false
        bool is_static;     //false
        bool is_incomplete; //undefined
        int align;          //8
        Type*base;          //TY_INT
        int array_size;     //undefined
        Member*members;     //undefined
        Type*return_ty;     //undefined
    };
    */
    if(!consume("[")){
        return ty;
    }

    int sz=0;
    bool is_incomplete=true;
        if(!consume("]")){
            sz=const_expr();
            is_incomplete=false;
            expect("]");
        }
    ty=type_suffix(ty);
    ty=array_of(ty,sz);
    ty->is_incomplete=is_incomplete;
    return ty;
    /*
    ex)int*num[3]の続き
    (Type*)ty{
        TypeKind kind;      //TY_ARRAY
        bool is_typedef;    //undefined
        bool is_static;     //undefined
        bool is_incomplete; //false
        int align;          //==base->align==8
        Type*base;          //TY_PTR
        int array_size;     //==sz==3
        Member*members;     //undefined
        Type*return_ty;     //undefined
    };
    */
}

//type-name=type-specifier abstract-declarator type-suffix
Type*type_name(){
    Type*ty=type_specifier();
    ty=abstract_declarator(ty);
    return type_suffix(ty);
}

void push_tag_scope(Token*tok,Type*ty){
    TagScope*sc=calloc(1,sizeof(TagScope));
    sc->next=tag_scope;
    sc->name=strn_dup(tok->str,tok->len);
    sc->depth=scope_depth;
    sc->ty=ty;
    tag_scope=sc;
}

//struct_decl="struct" ident
//           |"struct" ident? "{" struct_member "}"
Type*struct_decl(){
    //read a struct tag
    //struct_decl="struct" ident
    expect("struct");
    Token*tag=consume_ident();
    if(tag&&!peek("{")){
        TagScope*sc=find_tag(tag);//宣言済みかどうか
        if(!sc){//宣言してない場合　//struct fooだけ
            Type*ty=struct_type();
            push_tag_scope(tag,ty);
            return ty;
        }
        if(sc->ty->kind!=TY_STRUCT){
            error_tok(tag,"not a struct tag");
        }
        return sc->ty;//like "struct foo"
    }
    
    //"struct" ident? "{" struct_member "}"

    //Although it looks weird,
    //"struct*foo" is legal C that defines
    //foo as a pointer to an unnamed incomplete struct type
    if(!consume("{"))return struct_type();//"*"だった場合

    TagScope*sc=find_tag(tag);
    Type*ty;
    if(sc&&sc->depth==scope_depth){
        //if there is an existing struct type having the same tag name in the same block scope,
        //this is a redefinition //再定義
        if(sc->ty->kind!=TY_STRUCT)error_tok(tag,"not a struct tag");
        ty=sc->ty;
    }else{
        //register(登録する) a struct type as an incomplete type early, 
        //so that you can write recursive(再帰的な) structs 
        //such as "struct T { struct T*next } ".
        ty=struct_type();
        if(tag)push_tag_scope(tag,ty);
    }

    //read struct members
    Member head;
    head.next=NULL;
    Member*cur=&head;

    while(!consume("}")){
        cur->next=struct_member();
        cur=cur->next;
    }

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
    x.numのalignは4。offsetはalign_to(offset,4)=4  offset=8にインクリメント
    x.valのalignは4。offsetはalign_to(offset,4)=8  offset=12にインクリメント
    x.rのalignは1。offsetはalign_to(offset,1)=offset=12; offset=13にインクリメント
    そしてty->alignは4。これは要素内のalignの最大値である

      |_|...1 byte

    16|_|      =align_to(end,ty->align)...size_of
      |_|
      |_|
      |_|      =end...size_of
    12|_|<-x.r =mem->offset()...struct_decl
      |_|
      |_|
      |_|
     8|_|<-x.val
      |_|
      |_|
      |_|
     4|_|<-x.num
      |_|
      |_|
     1|_|<-x.q
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
        offset+=size_of(mem->ty,mem->tok);

        //ty->align に型のサイズのうち最大のものを格納する
        if(ty->align < mem->ty->align){//ty->alignはcallocで0に初期化されてる
            //mem->ty->alignはnew_type()でセットされる。 mem->ty->alignは型のサイズ
            ty->align=mem->ty->align;
        }
    }

    ty->is_incomplete=false;

    return ty;
}

//enum-specifier="enum" ident
//              |"enum" ident? "{" enum-list? "}"
//
//enum-list=enum-elem ("," enum-elem )* ","?
//enum-elem=ident ("=" const-expr)?
Type*enum_specifier(){
    expect("enum");
    Type*ty=enum_type();

    Token*tag=consume_ident();
    if(tag&&!peek("{")){
        TagScope*sc=find_tag(tag);
        if(!sc){
            error_tok(tag,"unknown enum type");
        }
        if(sc->ty->kind!=TY_ENUM){
            error_tok(tag,"not an enum tag");
        }
        return sc->ty;
    }

    expect("{");
    Member*mem=calloc(1,sizeof(Member));

    int cnt=0;
    for(;;){
        char*name=expect_ident();
        if(consume("=")){
            cnt=const_expr();
        }
        VarScope*sc=push_scope(name);
        sc->enum_ty=ty;
        sc->enum_val=cnt;
        cnt++;

        if(consume(",")){
            if(consume("}")){
                break;
            }
            continue;
        }
        expect("}");
        break;
    }

    if(tag){
        push_tag_scope(tag,ty);
    }
    return ty;
}

//struct_member=type-specifier declarator type-suffix ";"
Member*struct_member(){
    Type*ty=type_specifier();
    Token*tok=token;
    char*name=NULL;
    ty=declarator(ty,&name);
    ty=type_suffix(ty);
    expect(";");
    Member*mem=calloc(1,sizeof(Member));
    mem->name=name;
    mem->ty=ty;
    mem->tok=tok;
    return mem;
}

VarList*read_func_param(){
    Type*ty=type_specifier();
    char*name=NULL;
    Token*tok=token;
    ty=declarator(ty,&name);
    ty=type_suffix(ty);

    //"array of T" is converted to "pointer to T" only in the parameter
    //context. for example, *argv[] is converted to **argv by this.
    if(ty->kind==TY_ARRAY) ty=pointer_to(ty->base);

    Var*var=push_var(name,ty,true,tok);
    push_scope(name)->var=var;
    VarList*vl=calloc(1,sizeof(VarList));
    vl->var=var;
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

//function=type-specifier declarator "(" params? ")" ( "{" stmt* "}" | ";" )
//params  =param("," param)*
//param   =type-specifier declarator type-suffix
Function*function(){
    //printf("program\n");
    locals=NULL;
    Type*ty=type_specifier();
    char*name=NULL;
    Token*tok=token;
    ty=declarator(ty,&name);

    //add a function type to the scope
    Var*var=push_var(name,func_type(ty),false,tok);
    push_scope(name)->var=var;

    //construct a function object
    Function*fn=calloc(1,sizeof(Function));
    fn->name=name;
    expect("(");
    fn->params=read_func_params();
    if(consume(";")){
        return NULL;
    }

    expect("{");

    //read function body
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

//initializer list can and either with "}" or "," followed by "}" // "}" "}," //following...preposition, noun, adjective
//to allow a trailing comma(...末尾のカンマ). this function returns true 
//if it looks like we are at the end of an intializer list
bool peek_end(){
    Token*tok=token;
    bool ret=consume("}")||consume(",")&&consume("}");
    token=tok;
    return ret;
}

bool consume_end(){
    Token*tok=token;
    if(consume("}")||consume(",")&&consume("}")) return true;
    token=tok;
    return false;
}

//global initializer
/*
gvar_init_string は primary() で使う

Node*primary(){
    ...
    if(tok->kind==TK_STR){
        ...
        var->initializer=gvar_init_string(tok->contents,tok->cont_len);
        return new_var(var,tok);
    }
    ...
}
*/
Initializer*new_init_val(Initializer*cur,int sz,int val);

Initializer*gvar_init_string(char*p,int len){
    Initializer head;
    head.next=NULL;
    Initializer*cur=&head;

    for(int i=0;i<len;i++){
        cur=new_init_val(cur,1,p[i]);   
    }
    return head.next;
}


Initializer*new_init_val(Initializer*cur,int sz,int val){
    Initializer*init=calloc(1,sizeof(Initializer));
    init->sz=sz;
    init->val=val;
    cur->next=init;
    return init;
}

Initializer*new_init_zero(Initializer*cur,int nbytes){
    for(int i=0;i<nbytes;i++){
        cur=new_init_val(cur,1,0);
    }
    return cur;
}

Initializer*new_init_label(Initializer*cur, char*label){
    Initializer*init=calloc(1,sizeof(Initializer));
    init->label=label;
    cur->next=init;
    return init;
}

//構造体の初期化
//構造体のメンバーの同士の間を0で埋める
// struct{
//     char p;
//     char q;
//     int num;
//     int val;
//     char r;
// }x;
//
//  |_|...1 byte
//
//16|_|      =align_to(end,ty->align)...size_of
//  |_|
//  |_|
//  |_|      =end...size_of
//12|_|<-x.r =mem->offset()...struct_decl
//  |_|
//  |_|
//  |_|
// 8|_|<-x.val (略)
//  |_|<-x.num(4th byte)
//  |_|<-x.num(3rd byte)
//  |_|<-x.num(2nd byte)
// 4|_|<-x.num(1st byte)
//  |_|...0
//  |_|...0
// 1|_|<-x.q
// 0|_|<-x.p <-x
Initializer*emit_struct_padding(Initializer*cur,Type*parent,Member*mem){
    int end=mem->offset+size_of(mem->ty,token);
    int padding; //綿, 詰め物

    if(mem->next)padding=mem->next->offset-end;
    else padding=size_of(parent,token)-end;

    if(padding>0)cur=new_init_zero(cur,padding);

    return cur;
}

void skip_excess_elements2(){
    while(1){
        if(consume("{")) skip_excess_elements2();
        else assign();

        if(consume_end()) return;
        expect(",");
    }
}

void skip_excess_elements(){
    expect(",");
    warn_tok(token,"excess elements in initializer");
    skip_excess_elements2();
}

//global変数の右辺に変数は来ないらしい。。。 変数のアドレスは来るのに。。。
//これは変数の中身は動的に決定するが、その変数のアドレスはコンパイル時に設定されるからだろうか
Initializer*gvar_initializer(Initializer*cur,Type*ty){
    Token*tok=token;

    if(ty->kind==TY_ARRAY && ty->base->kind==TY_CHAR
    && token->kind==TK_STR){
        token=token->next;
        if(ty->is_incomplete){
            ty->array_size=tok->cont_len;
            ty->is_incomplete=false;
        }
        //len=min(ty->array,tok->cont_len)
        int len=(ty->array_size<tok->cont_len)?ty->array_size:tok->cont_len;
        for(int i=0;i<len;i++)cur=new_init_val(cur,1,tok->contents[i]);

        return new_init_zero(cur,ty->array_size-len);
    }

    if(ty->kind==TY_ARRAY){
        bool open=consume("{");
        int i=0;
        int limit=ty->is_incomplete ? INT_MAX : ty->array_size ;

        do{
            cur=gvar_initializer(cur,ty->base);
            i++;
        }while(i<limit && !peek_end() && consume(","));
        if(open && !consume_end()) skip_excess_elements();
        //set excess array elements to zero
        cur=new_init_zero(cur,size_of(ty->base,tok)*(ty->array_size-i));

        if(ty->is_incomplete){
            ty->array_size=i;
            ty->is_incomplete=false;
        }
        return cur;
    }

    if(ty->kind==TY_STRUCT){
        bool open=consume("{");
        Member*mem=ty->members;

        do{
            cur=gvar_initializer(cur,mem->ty);
            cur=emit_struct_padding(cur,ty,mem);
            mem=mem->next;
        }while(mem && !peek_end() && consume(","));
        if(open && !consume_end()) skip_excess_elements();
        //set excess struct elements to zero
        if(mem){
            int sz=size_of(ty,tok)-mem->offset;
            if(sz) cur=new_init_zero(cur,sz);
        }

        return cur;
    }

    //char*g15={"foo"};
    bool open=consume("{");
    Node*expr=conditional();//代入式の右辺
    if(open) expect("}");

    if(expr->kind==ND_ADDR){
        if(expr->lhs->kind!=ND_VAR)error_tok(tok,"invalid initializer");
        return new_init_label(cur,expr->lhs->var->name);
    }

    //initialize string variable like 'char*gl="abc";'
    if(expr->kind==ND_VAR
        &&expr->var->ty->kind==TY_ARRAY) 
        return new_init_label(cur,expr->var->name);


    return new_init_val(cur,size_of(ty,token),eval(expr));
}

//global-var=type-specifier declarator type-suffix ("=" gvar-initializer)? ";"
void global_var(){
    Type*ty=type_specifier();
    char*name=NULL;
    Token*tok=token;
    ty=declarator(ty,&name);
    ty=type_suffix(ty);
    
    Var*var=push_var(name,ty,false,tok);
    push_scope(name)->var=var;

    if(consume("=")){
        Initializer head;
        head.next=NULL;
        gvar_initializer(&head,ty);
        var->initializer=head.next;
    }
    expect(";");
}

typedef struct Designator Designator;
struct Designator{
    Designator*next;
    int idx;
    Member*mem;//struct
};

//Creates a node for an array access. for example, if var represents
//a varable x and desg represents indices 3 and 4, this function
//return a node representing x[3][4].
/*
int x[2][3]={{1,2,3},{4,5,6}} について
x[1][1]=5 のような初期化式の左辺を作る関数
*/
Node*new_desg_node2(Var*var,Designator*desg){
    Token*tok=var->tok;
    if(!desg) return new_var(var,tok);

    Node*node=new_desg_node2(var,desg->next);
    if(desg->mem){
        node=new_unary(ND_MEMBER,node,desg->mem->tok);
        node->member_name=desg->mem->name;
        return node;
    }
    node=new_binary(ND_ADD,node,new_num(desg->idx,tok),tok);
    return new_unary(ND_DEREF,node,tok);
}

Node*new_desg_node(Var*var,Designator*desg,Node*rhs){
    Node*lhs=new_desg_node2(var,desg);
    Node*node=new_binary(ND_ASSIGN,lhs,rhs,rhs->tok);
    return new_unary(ND_EXPR_STMT,node,rhs->tok);
}

Node*lvar_init_zero(Node*cur,Var*var,Type*ty,Designator*desg){
    if(ty->kind==TY_ARRAY){
        for(int i=0;i<ty->array_size;i++){
            Designator desg2={desg,i};//i++ は間違えだよね??
            cur=lvar_init_zero(cur,var,ty->base,&desg2);
        }
        return cur;
    }

    cur->next=new_desg_node(var,desg,new_num(0,token));
    return cur->next;
}

//lvar-initializer=assign 
//                | "{" lvar-initializer ("," lvar-initializer)* ","? "}" 
/*
an initializer for a local variable is expanded to multiple
assignments. for example, this function creates the following node
for x[2][3]={{1,2,3},{4,5,6}}.

x[0][0]=1;
x[0][1]=2;
x[0][2]=3;
x[1][0]=4;
x[1][1]=5;
x[1][2]=6;

There are a few special rules 
for ambiguous iitializers and shorthand notations.
//ambiguous...曖昧な, shorthand...速記, notation...表記法

if an initializer list is shorter than an array, 
excess array elements are initialized with 0.

if a rhs is an imcomplete array, its size is set by counting 
the number of items on the rhs. 
for example, " 'x' in 'int x[]={1,2,3}; " has type 'int[3]' . '
*/
Node*lvar_initializer(Node*cur,Var*var,Type*ty,Designator*desg){
    if(ty->kind==TY_ARRAY && ty->base->kind==TY_CHAR 
        && token->kind==TK_STR){
            /*
            a char array can be initialized by a string literal.
            for example, 
            'char x[4]="foo";' is equivalent to 'char x[4]={'f','o','o','\0'}' .
            */
            //initialize a char array with a string literal
            Token*tok=token;
            token=token->next;

            if(ty->is_incomplete){
                ty->array_size=tok->cont_len;
                ty->is_incomplete=false;
            }

            int len=(ty->array_size<tok->cont_len)?ty->array_size:tok->cont_len; 
            //     =min( ty->array_size , tok->cont_len )
            int i;

            for(i=0;i<len;i++){
                Designator desg2={desg,i,NULL};
                Node*rhs=new_num(tok->contents[i],tok);
                cur->next=new_desg_node(var,&desg2,rhs);
                cur=cur->next;
            }

            //initialize excess array
            for(;i<ty->array_size;i++){
                Designator desg2={desg,i,NULL};
                cur=lvar_init_zero(cur,var,ty->base,&desg2);
            }

            return cur;
        }

    //initialize array variable
    if(ty->kind==TY_ARRAY){
        /*
        ex)x[2][3]
        node={TY_ARRAY}
                ->base={TY_ARRAY}
                    ->base={TY_INT}...x
                    ->array_size...3
                ->array_size...2
        */
        bool open=consume("{");
        int i=0;
        int limit=ty->is_incomplete ? INT_MAX : ty->array_size ;

        do{
            Designator desg2={desg,i++,NULL};
            //x[0][1] に代入したい場合
            //desg2
            //  .next
            //      ->next...NULL
            //      ->idx...0
            //  .idx...1
            cur=lvar_initializer(cur,var,ty->base,&desg2);
            /*
            ex)x[2][3]={{1,2,3},{4,5,6}}
            desg2(1)={NULL,0}
                desg2(1-1)={&desg2(1),0}
                cur=new_desg_node(var,&desg2(1-1),assign()..."1")
                   ={ND_EXPR_STMT}
                        ->lhs={ND_ASSIGN}
                                ->lhs={ND_DEREF}
                                        ->lhs={ND_ADD}
                                            ->lhs={ND_DEREF}
                                                ->lhs={ND_ADD}
                                                    ->lhs={ND_VAR}
                                                    ->rhs={ND_NUM}...0
                                            ->rhs={ND_NUM}...0
                                ->rhs={ND_NUM}...1
                
                desg2(1-1)={&desg2(1),1}
                cur=new_desg_node(var,&desg2(1-1),assign()..."2")
                   ={ND_EXPR_STMT}
                        ->lhs={ND_ASSIGN}
                                ->lhs={ND_DEREF}
                                        ->lhs={ND_ADD}
                                            ->lhs={ND_DEREF}
                                                ->lhs={ND_ADD}
                                                    ->lhs={ND_VAR}
                                                    ->rhs={ND_NUM}...1
                                            ->rhs={ND_NUM}...0
                                ->rhs={ND_NUM}...2
                ...

                //cur-1 (&head)
                //  ->next=cur-2
                //          ->next ...
            */
        }while(i<limit&&!peek_end()&&consume(","));

        if(open && !consume_end()) skip_excess_elements();

        //set excess array elements to zero.
        /*
        ex)int x[2][3]={{1,2}}; ... x[2][3]={{1,2,0},{0,0,0}};
        */
        for(;i<ty->array_size;i++){
            Designator desg2={desg,i,NULL};
            cur=lvar_init_zero(cur,var,ty->base,&desg2);
        }

        if(ty->is_incomplete){
            ty->array_size=i;
            ty->is_incomplete=false;
        }

        return cur;
    }//ty->kind==TY_ARRAY

    if(ty->kind==TY_STRUCT){
        bool open=consume("{");
        Member*mem=ty->members;
        do{
            Designator desg2={desg,0,mem};
            cur=lvar_initializer(cur,var,mem->ty,&desg2);
            mem=mem->next;
        }while(mem&&!peek_end()&&consume(","));

        if(open && !consume_end()) skip_excess_elements();

        //set excess struct elements to zero
        for(;mem;mem=mem->next){
            Designator desg2={desg,0,mem};
            cur=lvar_init_zero(cur,var,mem->ty,&desg2);
        }

        return cur;
    }

    //char*str={"foo"};
    bool open=consume("{");
    cur->next=new_desg_node(var,desg,assign());
    if(open) expect("}");
    return cur->next;
}

//declaration=type-specifier declarator type-suffix ("=" expr)?";"
//           |btype-specifier ";"
/*
ex)int*num[2]; typedef struct foo foo_t
Type*ty=type_specifier();   //int           //typedef struct foo
ty=declarator(ty,&name);    //int*num       //typedef struct foo foo_t //(char*)name="foo_t"
ty=type_suffix(ty);         //int*num[2]    //typedef struct foo foo_t //変化なし

ex2)struct foo {...} x;
Type*ty=type_specifier();   //struct foo {...}         
ty=declarator(ty,&name);    //struct foo {...} x:
ty=type_suffix(ty);         //struct foo {...} x:
*/
Node*declaration(){
    //変数の型を決める
    Token*tok;
    Type*ty=type_specifier();   
    if(tok=consume(";")){//struct tag //struct foo {...};
        return new_node(ND_NULL,tok);
    }

    tok=token;
    char*name=NULL;
    ty=declarator(ty,&name);
    ty=type_suffix(ty);

    //いろんなオプション
    /*
    typedefについて
    ex) typedef struct foo foo_t
    "foo_t"という名前の変数としてpush_scope//push_scope(name)->type_def=ty;
    これはfind_typedef関数で役立つ。
    処理自体は何もしない//return new_node(ND_NULL,tok);
    */
    if(ty->is_typedef){//typedef struct foo foo_t
        expect(";");
        ty->is_typedef=false;
        push_scope(name)->type_def=ty;
        return new_node(ND_NULL,tok);
    }
    if(ty->kind==TY_VOID){
        error_tok(tok,"variable declared void");
    }
    Var*var;
    if(ty->is_static){
        var=push_var(new_label(),ty,false,tok);
    }else{//通常のローカル変数
        var=push_var(name,ty,true,tok);
    }
    push_scope(name)->var=var;

    if(consume(";")){//declaration without assignment
        return new_node(ND_NULL,tok);
    }
    //so far it's a declaration
    //(Type*)ty, var だけ決定している状態


    //from here on, assignment 
    expect("=");

    Node head;
    head.next=NULL;
    lvar_initializer(&head,var,var->ty,NULL);
    expect(";");
    Node*node=new_node(ND_BLOCK,tok);
    node->body=head.next;
    return node;
}

Node*read_expr_stmt(){
    Token*tok=token;
    Node*node=new_unary(ND_EXPR_STMT,expr(),tok);
    return node;

}

bool is_typename(){
    return peek("void")||peek("_Bool")||peek("char")||peek("short")||peek("int")||peek("long")||
    peek("enum")||peek("struct")||peek("typedef")||peek("static")||
    find_typedef(token);
}

///stmt="return" expr ";" | expr ";"
///    |"if" "(" expr ")" stmt ("else" stmt)?
///    |"switch" "(" expr ")" stmt
///    |"case" const-expr ":" stmt
///    |"default" ":" stmt
///    |"while" "(" expr ")" stmt
///    |"for" "(" ( expr? ";" | declaration ) expr? ";" expr? ")" stmt
///    |"{" stmt* "}"
///    |"break" ";"
///    |"continue" ";"
///    |"goto" ident ";"
///    | ident ":" stmt
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
    if(tok=consume("switch")){
        Node*node=new_node(ND_SWITCH,tok);
        expect("(");
        node->cond=expr();
        expect(")");

        Node*sw=current_switch;
        current_switch=node;
        node->then=stmt();
        current_switch=sw;
        return node;
    }
    if(tok=consume("case")){
        if(!current_switch) error_tok(tok,"stray case");

        Node*node=new_node(ND_CASE,tok);
        node->val=const_expr();
        expect(":");

        node->lhs=stmt();
        node->case_next=current_switch->case_next;//(1)
        current_switch->case_next=node;//(2)
        /*
        current_switch->case_next==>|1|
        node==>|2|
        (1)
        current_switch->case_next==>|1|
        node==>|2|->case_next    ==>|1|
        (2)
        current_switch->case_next==>
        =>node==>|2|->case_next    ==>|1| |1|...NULL
        */
        return node;
    }
    if(tok=consume("default")){
        if(!current_switch) error_tok(tok,"stray default");

        expect(":");

        Node*node=new_unary(ND_CASE,stmt(),tok);
        current_switch->default_case=node;//current_switch は ND_SWITCH の node のアドレス
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
        Scope*sc=enter_scope();
        Node*node=new_node(ND_FOR,tok);
        expect("(");
        if(!consume(";")){
            if(is_typename()){
                node->init=declaration();
            }else{
                node->init=read_expr_stmt();
                expect(";");
            }
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

        leave_scope(sc);
        return node;
    }
    if(tok=consume("{")){
        Node head;
        head.next=NULL;
        Node*cur=&head;

        Scope*sc=enter_scope();
        while(!consume("}")){
            cur->next=stmt();
            cur=cur->next;
        }
        leave_scope(sc);

        Node*node=new_node(ND_BLOCK,tok);
        node->body=head.next;
        return node;
    }
    if(tok=consume("break")){
        expect(";");
        return new_node(ND_BREAK,tok);
    }
    if(tok=consume("continue")){
        expect(";");
        return new_node(ND_CONTINUE,tok);
    }
    if(tok=consume("goto")){
        Node*node=new_node(ND_GOTO,tok);
        node->label_name=expect_ident();
        expect(";");
        return node;
    }
    if(tok=consume_ident()){
        if(consume(":")){
            Node*node=new_unary(ND_LABEL,stmt(),tok);
            node->label_name=strn_dup(tok->str,tok->len);
            return node;
        }
        token=tok;
    }

    if(is_typename()){
        return declaration();
    }
    Node *node=read_expr_stmt();
    expect(";");
    return node;
}

//expr=assign ("," assign)*
Node*expr(){
    Node*node=assign();
    Token*tok;
    while(tok=consume(",")){
        node=new_unary(ND_EXPR_STMT,node,node->tok);
        node=new_binary(ND_COMMA,node,assign(),tok);
    }
    return node;
}

long eval(Node*node){
    switch(node->kind){
        case ND_ADD:
            return eval(node->lhs)+eval(node->rhs);
        case ND_SUB:
            return eval(node->lhs)-eval(node->rhs);
        case ND_MUL:
            return eval(node->lhs)*eval(node->rhs);
        case ND_DIV:
            return eval(node->lhs)/eval(node->rhs);
        case ND_BITAND:
            return eval(node->lhs)&eval(node->rhs);
        case ND_BITOR:
            return eval(node->lhs)|eval(node->rhs);
        case ND_BITXOR:
            return eval(node->lhs)|eval(node->rhs);
        case ND_SHIFT_L:
            return eval(node->lhs)<<eval(node->rhs);
        case ND_SHIFT_R:
            return eval(node->lhs)>>eval(node->rhs);
        case ND_EQ:
            return eval(node->lhs)==eval(node->rhs);
        case ND_NE:
            return eval(node->lhs)!=eval(node->rhs);
        case ND_LT:
            return eval(node->lhs)<eval(node->rhs);
        case ND_LE:
            return eval(node->lhs)<=eval(node->rhs);
        case ND_TERNARY:
            return eval(node->cond)?eval(node->then):eval(node->els);
        case ND_COMMA:
            return eval(node->rhs);
        case ND_NOT:
            return !eval(node->lhs);
        case ND_BITNOT:
            return ~eval(node->lhs);
        case ND_LONGAND:
            return eval(node->lhs)&&eval(node->rhs);
        case ND_LONGOR:
            return eval(node->lhs)||eval(node->rhs);
        case ND_NUM:
            return node->val;
    }

    error_tok(node->tok,"not a constant expression");
}

long const_expr(){
    return eval(conditional());
}

//assign=conditional(assiign-op assign)?
//assign-op= "="|"+="|"-="|"*="|"/="|"<<="|">>="
Node*assign(){
    //printf("assign\n");
    Node*node=conditional();
    Token*tok;
    if(tok=consume("=")){
        node=new_binary(ND_ASSIGN,node,assign(),tok);
    }
    if(tok=consume("+=")){
        node=new_binary(ND_A_ADD,node,assign(),tok);
    }
    if(tok=consume("-=")){
        node=new_binary(ND_A_SUB,node,assign(),tok);
    }
    if(tok=consume("*=")){
        node=new_binary(ND_A_MUL,node,assign(),tok);
    }
    if(tok=consume("/=")){
        node=new_binary(ND_A_DIV,node,assign(),tok);
    }
    if(tok=consume("<<=")){
        node=new_binary(ND_A_SHIFT_L,node,assign(),tok);
    }
    if(tok=consume(">>=")){
        node=new_binary(ND_A_SHIFT_R,node,assign(),tok);
    }
    return node;
}

//conditional=longor ( "?" expr ":" conditional )?
Node*conditional(){
    Node*node=longor();
    Token*tok=consume("?");
    if(!tok) return node;

    Node*ternary=new_node(ND_TERNARY,tok);
    ternary->cond=node;
    ternary->then=expr();
    expect(":");
    ternary->els=conditional();
    return ternary;
}

//longor=lomgand("||" longand)*
Node*longor(){
    Node*node=longand();
    Token*tok;
    while(tok=consume("||")){
        node=new_binary(ND_LONGOR,node,longand(),tok);
    }
    return node;
}

//longand=bitor("&&" bitor)*
Node*longand(){
    Node*node=bitor();
    Token*tok;
    while(tok=consume("&&")){
        node=new_binary(ND_LONGAND,node,bitor(),tok);
    }
    return node;
}

//bitor=bitxor("|" bitxor)*
Node*bitor(){
    Node*node=bitxor();
    Token*tok;
    while(tok=consume("|")){
        node=new_binary(ND_BITOR,node,bitxor(),tok);
    }
    return node;
}

//bitxor=bitand("^" bitand)*
Node*bitxor(){
    Node*node=bitand();
    Token*tok;
    while(tok=consume("^")){
        node=new_binary(ND_BITXOR,node,bitand(),tok);
    }
    return node;
}

//bitand=equality("&" equality)*
Node*bitand(){
    Node*node=equality();
    Token*tok;
    while(tok=consume("&")){
        node=new_binary(ND_BITAND,node,equality(),tok);
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

//relatonal=shift("<" shift | "<=" shift | ">" shift | ">=" shift)*
Node*relational(){
    //printf("relational\n");
    Node*node=shift();
    Token*tok;

    if(tok=consume("<")){
        node=new_binary(ND_LT,node,shift(),tok);
    }else if(tok=consume("<=")){
        node=new_binary(ND_LE,node,shift(),tok);
    }else if(tok=consume(">")){
        node=new_binary(ND_LT,shift(),node,tok);
    }else if(tok=consume(">=")){
        node=new_binary(ND_LE,shift(),node,tok);
    }else{
        return node;
    }
}

//shift=add ( "<<" add | ">>" add )*
Node*shift(){
    Node*node=add();
    Token*tok;

    for(;;){
        if(tok=consume("<<")){
            node=new_binary(ND_SHIFT_L,node,add(),tok);
        }else if(tok=consume(">>")){
            node=new_binary(ND_SHIFT_R,node,add(),tok);
        }else{
            return node;
        }
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

//mul=cast("*" cast | "/" cast)*
Node*mul(){
    //printf("mul\n");
    Node*node=cast();
    Token*tok;
    for(;;){
        if(tok=consume("*")){
            node=new_binary(ND_MUL,node,cast(),tok);
        }else if(tok=consume("/")){
            node=new_binary(ND_DIV,node,cast(),tok);
        }else{
            return node;
        }
    }
}

//cast="(" type-name ")" cast | unary
Node*cast(){
    Token*tok=token;
    if(consume("(")){
        if(is_typename()){
            Type*ty=type_name();
            expect(")");
            Node*node=new_unary(ND_CAST,cast(),tok);
            node->ty=ty;
            return node;
        }
        token=tok;
    }
    return unary();
}

//unary=("+" | "-" | "*" | "&" | "!" | "~" )?cast  
//     |("++"|"--")unary
//     | postfix
Node*unary(){
    //printf("unary\n");
    Token*tok;
    if(consume("+")){
        return cast();
    }
    if(tok=consume("-")){
        return new_binary(ND_SUB,new_num(0,tok),cast(),tok);
    }
    if(tok=consume("&")){
        return new_unary(ND_ADDR,cast(),tok);
    }
    if(tok=consume("*")){
        return new_unary(ND_DEREF,cast(),tok);
    }
    if(tok=consume("!")){
        return new_unary(ND_NOT,cast(),tok);
    }
    if(tok=consume("~")){
        return new_unary(ND_BITNOT,cast(),tok);
    }
    if(tok=consume("++")){
        return new_unary(ND_PRE_INC,unary(),tok);
    }
    if(tok=consume("--")){
        return new_unary(ND_PRE_DEC,unary(),tok);
    }
    return postfix();
}

//postfix=primary ("[" expr "]" | "." ident |"->" ident|"++"|"--")*
Node*postfix(){
    Node*node=primary();
    Token*tok;
    for(;;){
        /*
        y[3][5]==*(*(y+3)+5)
        node={ND_DEREF}
                ->lha={ND_ADD}
                        ->lhs{ND_DEREF}
                            ->lhs={ND_ADD}
                                    ->lhs={ND_VAR}...x
                                    ->rhs={ND_NUM}...3
                        ->rhs={ND_NUM}...5
        */
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
        if(tok=consume("->")){
            //x->y is short for (*x).y
            node=new_unary(ND_DEREF,node,tok);
            node=new_unary(ND_MEMBER,node,tok);
            node->member_name=expect_ident();
            continue;
        }
        if(tok=consume("++")){
            node=new_unary(ND_POST_INC,node,tok);
            continue;
        }
        if(tok=consume("--")){
            node=new_unary(ND_POST_DEC,node,tok);
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
    Scope*sc=enter_scope();

    Node*node=new_node(ND_STMT_EXPR,tok);
    node->body=stmt();
    Node*cur=node->body;

    while(!consume("}")){
        cur->next=stmt();
        cur=cur->next;
    }
    expect(")");
    leave_scope(sc);

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

    ==>関数の引数代入の際に都合がいいから。
    ND_STMT_EXPRのnodeは実行後にadd rsp, 8がされ、スタック上から実行結果の計算値は消される。
    最後のstmtだけND_STMT_EXPRにすることで、
    例えば、以下二つのような関数呼び出しは同じものとなる。
    add(({int a=5;}))
    add(({int a=5;a;}))
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
//primary= "(" "{" stmt-expr-tail
//         | "(" expr ")" 
//         | "sizeof" ("(" type-name ")"| unary)
//         | ident func-args? 
//         | str 
//         | num 
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
        if(consume("(")){
            if(is_typename()){
                Type*ty=type_name();
                expect(")");
                return new_num(size_of(ty,tok),tok);
            }
            token=tok->next;//"("
        }
        return new_unary(ND_SIZEOF,unary(),tok);
    }
    if(tok=consume_ident()){
        if(consume("(")){
            Node*node=new_node(ND_FUNCALL,tok);
            node->funcname=strn_dup(tok->str,tok->len);
            node->args=func_args();

            VarScope*sc=find_var(tok);
            if(sc){
                if(!sc->var||sc->var->ty->kind!=TY_FUNC){
                    error_tok(tok,"not a function");
                }
                node->ty=sc->var->ty->return_ty;
            }else{
                //printf("no sc \n");
                node->ty=int_type();
            }
            return node;
        }
        VarScope*sc=find_var(tok);
        if(sc){
            if(sc->var){
                return new_var(sc->var,tok);
            }
            if(sc->enum_ty){
                return new_num(sc->enum_val,tok);
            }
        }
        error_tok(tok,"undefined variable");
    }
    tok=token;

    if(tok->kind==TK_STR){
        token=token->next;

        Type*ty=array_of(char_type(),tok->cont_len);
        Var*var=push_var(new_label(),ty,false,NULL);
        // var->contents=tok->contents;
        // var->cont_len=tok->cont_len;
        var->initializer=gvar_init_string(tok->contents,tok->cont_len);
        return new_var(var,tok);
    }
    if(tok->kind!=TK_NUM){
        error_tok(tok,"expected expression");
    }
    return new_num(expect_number(),tok);
}
