#include"chibicc.h"

int main(int argc, char **argv){
    if(argc!=2){
        fprintf(stderr,"%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    user_input=argv[1];
    token=tokenize();
    //printf("tokenizer:ok\n");
    Function*prog=program();
    //printf("parser:ok\n");

    //Assign offsets to local variables.
    for(Function*fn=prog;fn;fn=fn->next){
        int offset=0;
        for(Var*var=prog->locals;var;var=var->next){
            offset+=8;
            var->offset=offset;
        }
        prog->stack_size=offset;
    }

    codegen(prog);
    //printf("code_genrator:ok\n");

    return 0;
}