#include"chibicc.h"

void gen_addr(Node*node){
    if(node->kind==ND_VAR){
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
    }
    error("not a local value");
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
        case ND_NUM:
            printf("  push %d\n",node->val);
            return; 
        case ND_EXPR_STMT:
            gen(node->lhs);
            //printf("  add rsp, 8\n");
            return;
        case ND_VAR:
            gen_addr(node);
            load();
            return;
        case ND_ASSIGN:
            gen_addr(node->lhs);
            gen(node->rhs);
            store();
            return;
        case ND_RETURN:
            gen(node->lhs);
            printf("  pop rax\n");
            printf("  jmp .Lreturn\n");
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

void codegen(Program*prog){
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    //prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n",prog->stack_size);

    for(Node*n=prog->node;n;n=n->next){
        gen(n);
    }

    //epilogue
    printf(".Lreturn:\n");
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
    printf(".section .note.GNU-stack,\"\",@progbits\n");

}