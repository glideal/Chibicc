#include"chibicc.h"


char*read_file(char*path){
    //open and read file
    FILE*fp=fopen(path,"r");
    if(!fp){
        error("cannot open %s: %s",path,strerror(errno));
    }
    int filemax=10*1024*1024;
    char*buf=malloc(filemax);
    int size=fread(buf,sizeof(char),filemax-2,fp);
    if(!feof(fp)){
        error("%s: file too large");
    }

    //make sure that the string ends with "\n\0"
    if(size==0||buf[size-1]!='\n'){
        buf[size]='\n';
        size++;
    }
    buf[size]='\0';
    fclose(fp);
    return buf;
}

int main(int argc, char **argv){
    if(argc!=2){
        fprintf(stderr,"%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    filename=argv[1];
    user_input=read_file(argv[1]);
    token=tokenize();
    //printf("tokenizer:ok\n");
    Program*prog=program();
    //printf("parser:ok\n");
    add_type(prog);

    //Assign offsets to local variables.
    for(Function*fn=prog->fns;fn;fn=fn->next){
        int offset=0;
        for(VarList*vl=fn->locals;vl;vl=vl->next){
            Var*var=vl->var;
            offset+=size_of(var->ty);
            var->offset=offset;
        }
        fn->stack_size=align_to(offset,8);
    }

    codegen(prog);
    //printf("code_genrator:ok\n");

    return 0;
}