#include<stdio.h>
#include<stdlib.h>

//test

int main(int argc, char **argv){
    if(argc!=2){
        fprintf(stderr,"%s: invalid number of arguments\n", argv[0]);
        return 1;
    }
    printf(".intel_syntax noprefix\n");
    printf(".global main\n");
    printf("main:\n");
    printf("  mov rax, %d\n",atoi(argv[1]));
    printf("  ret\n");
    printf("  ret\n");
    printf(".section .note.GNU-stack,\"\",@progbits\n");


    return 0;
}