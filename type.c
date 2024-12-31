#include"chibicc.h"

/*
align(64+1,8)の場合を考える
n    =65  =0b01000001
align=8   =0b00001000
n+align-1 =0b01001000...(a)
align-1   =0b00000111
~(align-1)=0b11111000...(b)
a & b     =0b01001000=64+8
*/
/*
alignはおそらく2^xで、(x+1bit目だけが1)
bはn+align-1のxbit目より上位のbitを反映させるためのもの。
また、n+align-1はalign-1が(下位3bit)111なのでnの下位3bitいずれかが1だった時に
4bit目(8)に繰り上げる。

まとめるとalign_toはalignの倍数のうち、n以上のものの中で最小の数を返す関数。
ただし、alignは2のべき乗
*/
int align_to(int n,int align){
    return (n+align-1)& ~(align-1);//~...not演算子 //~(1011)=(0100)
}


Type*new_type(TypeKind kind,int align){
    Type*ty=calloc(1,sizeof(Type));
    ty->kind=kind;
    ty->align=align;
    return ty;
}

Type*void_type(){
    return new_type(TY_VOID,1);
}

Type*bool_type(){
    return new_type(TY_BOOL,1);
}

Type*short_type(){
    return new_type(TY_SHORT,2);
}

Type*int_type(){
    return new_type(TY_INT,4);
}

Type*long_type(){
    return new_type(TY_LONG,8);
}

Type*char_type(){ 
    return new_type(TY_CHAR,1);
}

Type*enum_type(){
    return new_type(TY_ENUM,4);
}

Type*func_type(Type*return_ty){
    Type*ty=new_type(TY_FUNC,1);
    ty->return_ty=return_ty;
    return ty;
}

Type*pointer_to(Type*base){
    Type*ty=new_type(TY_PTR,8);
    ty->base=base;
    return ty;
}

Type*array_of(Type*base,int size){
    Type*ty=new_type(TY_ARRAY,base->align);
    ty->base=base;
    ty->array_size=size;
    return ty;
}

int size_of(Type*ty){
    assert(ty->kind!=TY_VOID);

    switch(ty->kind){
        case TY_BOOL:
        case TY_CHAR:
            return 1;
        case TY_SHORT:
            return 2;
        case TY_INT:
        case TY_ENUM:
            return 4;
        case TY_LONG:
        case TY_PTR:
            return 8;
        case TY_ARRAY: 
            return size_of(ty->base)*ty->array_size;
        default: 
            assert(ty->kind==TY_STRUCT);
            /*
            return align_to(end,ty->align);
            でend以上の数でty->alignの倍数である数のうち、最小のものを返す

            ただし、struct型においてty->alignは要素内のalignの最大値であり、2^nの形
            */
            Member*mem=ty->members;
            while(mem->next){
                mem=mem->next;
            }
            int end=mem->offset+size_of(mem->ty);
            return align_to(end,ty->align);
    }
}

Member*find_member(Type*ty,char*name){
    assert(ty->kind==TY_STRUCT);
    for(Member*mem=ty->members;mem;mem=mem->next){
        if(!strcmp(mem->name,name)){
            return mem;
        }
    }
    return NULL;
}

void visit(Node*node){
    if(!node)return;

    visit(node->lhs);
    visit(node->rhs);
    visit(node->cond);
    visit(node->then);
    visit(node->els);
    visit(node->init);
    visit(node->inc);

    for(Node*n=node->body;n;n=n->next){
        visit(n);
    }
    for(Node*n=node->args;n;n=n->next){
        visit(n);
    }

    switch(node->kind){
        case ND_MUL:
        case ND_DIV:
        case ND_EQ:
        case ND_NE:
        case ND_LT:
        case ND_LE:
            node->ty=int_type();
            return;
        case ND_NUM:
            if(node->val==(int)node->val){
                node->ty=int_type();
            }else{
                node->ty=long_type();
            }
            return;
        case ND_VAR:
            node->ty=node->var->ty;
            return;
        case ND_ADD:
            if(node->rhs->ty->base){
                Node*tmp=node->lhs;
                node->lhs=node->rhs;
                node->rhs=tmp;
            }

            if(node->rhs->ty->base){
                error_tok(node->tok,"invalid pointer arithmetic operands");
            }

            node->ty=node->lhs->ty;
            return;
        case ND_SUB:
            if(node->rhs->ty->base){
                error_tok(node->tok,"invalid pointer arithmetic operands");
            }
            node->ty=node->lhs->ty;
            return;
        case ND_ASSIGN:
            node->ty=node->lhs->ty;
            return;
        case ND_MEMBER:{
            //ident "." ident
            if(node->lhs->ty->kind!=TY_STRUCT){
                error_tok(node->tok,"not a struct");
            }
            node->member=find_member(node->lhs->ty,node->member_name);
            if(!node->member){
                error_tok(node->tok,"specified member does not exist");
            }
            node->ty=node->member->ty;
            return;
        }
        case ND_ADDR:
            if(node->lhs->ty->kind==TY_ARRAY){
                node->ty=pointer_to(node->lhs->ty->base);
            }else{
                node->ty=pointer_to(node->lhs->ty);
            }
            return;
        case ND_DEREF:
            if(!node->lhs->ty->base){
                error_tok(node->tok,"invailed pointer dereference");
            }
            node->ty=node->lhs->ty->base;//(Type*)ty //(Type*)base
            if(node->ty->kind==TY_VOID){
                error_tok(node->tok,"dereferencing a void pointer");
            }
            return;
        case ND_SIZEOF:
            node->kind=ND_NUM;
            node->ty=int_type();
            node->val=size_of(node->lhs->ty);
            node->lhs=NULL;//忘れてた
            return;
        case ND_STMT_EXPR:
            Node*last=node->body;
            while(last->next){
                last=last->next;
            }
            node->ty=last->ty;
            return;
    }
    
}

void add_type(Program*prog){
    for(Function*fn=prog->fns;fn;fn=fn->next){
        for(Node*node=fn->node;node;node=node->next){
            visit(node);
        }
    }
}