#include"chibicc.h"


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

void codegen(Node*node){
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");

    for(Node*n=node;n;n=n->next){
        gen(n);
        printf("  pop rax\n");
    }
    
    printf("  ret\n");
    printf(".section .note.GNU-stack,\"\",@progbits\n");

}