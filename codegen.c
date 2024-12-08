#include"chibicc.h"

int labelseq=0;
char*funcname;
char*argreg[]={"rdi","rsi","rdx","rcx","r8","r9"};

void gen(Node*node);

void gen_addr(Node*node){
    switch(node->kind){
        case ND_VAR:
            printf("  lea rax, [rbp-%d]\n",node->var->offset);
            /*
            lea rax, [rbp-%d]
            は

            mov rax, rbp
            sub rax, %d
            と同じ
            */
            printf("  push rax\n");
            return;
        case ND_DEREF:
            gen(node->lhs);
            return;
    }
    error_tok(node->tok,"not a local value");
}

void gen_lval(Node*node){
    if(node->ty->kind==TY_ARRAY){
        error_tok(node->tok,"not an lvalue");
    }
    gen_addr(node);
}

void load(){
    printf("  pop rax\n");
    /*mov dst, [src]
    「srcレジスタの値をアドレスとみなしてそこから値をロードしdstに保存する」
    */
    printf("  mov rax, [rax]\n");
    printf("  push rax\n");
}

void store(){
    printf("  pop rdi\n");
    printf("  pop rax\n");
    printf("  mov [rax], rdi\n");
    printf("  push rdi\n");
}

void gen(Node*node){
    switch(node->kind){
        case ND_NULL:
            return;
        case ND_NUM:
            printf("  push %d\n",node->val);
            return; 
        case ND_EXPR_STMT:
            gen(node->lhs);
            //printf("  add rsp, 8\n");
            return;
        case ND_VAR:
            gen_addr(node);
            if(node->ty->kind!=TY_ARRAY){
                load();
            }
            return;
        case ND_ASSIGN:
            gen_lval(node->lhs);
            gen(node->rhs);
            store();
            return;
        case ND_ADDR:
            gen_addr(node->lhs);
            return;
        case ND_DEREF:
            gen(node->lhs);
            if(node->ty->kind!=TY_ARRAY){
                load();
            }
            return;
        case ND_IF:{
            int seq=labelseq;
            labelseq++;
            if(node->els){
                gen(node->cond);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                /*
                je .L0
                ゼロフラグが1だったらジャンプ
                */
                printf("  je  .Lelse%d\n",seq);
                gen(node->then);
                printf("  jmp  .Lend%d\n",seq);
                printf(".Lelse%d:\n",seq);
                gen(node->els);
                printf(".Lend%d:\n",seq);
            }else{
                gen(node->cond);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je  .Lend%d\n",seq);
                gen(node->then);
                printf(".Lend%d:\n",seq);
            }
            return;
        }
        case ND_WHILE:{
            int seq=labelseq;
            labelseq++;
            printf(".Lbegin%d:\n",seq);
            gen(node->cond);
            printf("  pop rax\n");
            printf("  cmp rax, 0\n");
            printf("  je  .Lend%d\n",seq);
            gen(node->then);
            printf("  jmp  .Lbegin%d\n",seq);
            printf(".Lend%d:\n",seq);
            return;
        }
        case ND_FOR:{
            int seq=labelseq;
            labelseq++;
            if(node->init){
                gen(node->init);
            }
            printf(".Lbegin%d:\n",seq);
            if(node->cond){
                gen(node->cond);
                printf("  pop rax\n");
                printf("  cmp rax, 0\n");
                printf("  je  .Lend%d\n",seq);
            }
            gen(node->then);
            if(node->inc){
                gen(node->inc);
            }
            printf("  jmp  .Lbegin%d\n",seq);
            printf(".Lend%d:\n",seq);
            return;
        }
        case ND_BLOCK:
            for(Node*n=node->body;n;n=n->next){
                gen(n);
            }
            return;
        case ND_FUNCALL:{
            int nargs=0;
            for(Node*arg=node->args;arg;arg=arg->next){
                gen(arg);
                nargs++;
            }
            for(int i=nargs-1;i>=0;i--){
                printf("  pop %s\n",argreg[i]);
            }
            int seq=labelseq;
            labelseq++;
            /*
            x86-64の関数呼び出しのABI（Application Binary Interface）は、
            関数呼び出し前にrspが16の倍数になっている必要がある。
            しかし、rspは8bit単位で変化するので、
            rspを一時的に16の倍数にする必要があるパターンがある。
            */
            printf("  mov rax, rsp\n");
            printf("  and rax, 15\n");//下位4bitが1、その他bitが0のデータとAND
            printf("  jnz .Lcall%d\n",seq);//jnz==jump not zero//rspが16の倍数でないならjump
            printf("  mov rax, 0\n");
            printf("  call %s\n",node->funcname);
            printf("  jmp .Lend%d\n",seq);
            printf(".Lcall%d:\n",seq);
            printf("  sub rsp, 8\n");
            printf("  mov rax, 0\n");
            printf("  call %s\n",node->funcname);
            printf("  add rsp, 8\n");
            printf(".Lend%d:\n",seq);
            printf("  push rax\n");
            return;
        }
        case ND_RETURN:
            gen(node->lhs);
            printf("  pop rax\n");
            printf("  jmp .Lreturn.%s\n",funcname);
            return;
    }
    gen(node->lhs);
    gen(node->rhs);

    printf("  pop rdi\n");
    printf("  pop rax\n");

    switch(node->kind){
        case ND_ADD:
            if(node->ty->base){//base!=NULLならばnode->ty!=TY_INT
                printf("  imul rdi, %d\n",size_of(node->ty->base));
            }
            printf("  add rax, rdi\n");
            break;
        case ND_SUB:
            if(node->ty->base){
                printf("  imul rdi, %d\n",size_of(node->ty->base));
            }
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
            /*
            cmp　rax, rdiはフラグだけを更新するsub 命令と一緒だが、
            cmpと違い、subはraxにrax-rdiを代入する
            */
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

void codegen(Function*prog){
    printf(".intel_syntax noprefix\n");
    for(Function*fn=prog;fn;fn=fn->next){
        printf(".global %s\n",fn->name);
        printf("%s:\n",fn->name);
        funcname=fn->name;

        //prologue
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        printf("  sub rsp, %d\n",fn->stack_size);

        //push argument to the stack when calling function
        int i=0;
        for(VarList*vl=fn->params; vl;vl=vl->next){
            Var*var=vl->var;
            printf("  mov [rbp-%d], %s\n",var->offset,argreg[i]);
            i++;
        }

        //{...}
        for(Node*n=fn->node;n;n=n->next){
            gen(n);
        }

        //epilogue
        printf(".Lreturn.%s:\n",funcname);
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
    }
    printf(".section .note.GNU-stack,\"\",@progbits\n");
}