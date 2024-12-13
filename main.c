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

まとめるとalign_toはalignの倍数のうち、n以上のものを返す関数。
ただし、alignは2のべき乗
*/
int align_to(int n,int align){
    return (n+align-1)& ~(align-1);//~...not演算子 //~(1011)=(0100)
}

int main(int argc, char **argv){
    if(argc!=2){
        fprintf(stderr,"%s: invalid number of arguments\n", argv[0]);
        return 1;
    }

    user_input=argv[1];
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