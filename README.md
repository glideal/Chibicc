```
24/12/02
RSPの変更単位を知るためのアセンブリコードを書いた
main:
  mov rax, rsp
  push 12
  push rsp 
  pop rdi
  sub rax, rdi
  ret

はsegmentation fault.
  mov rax, rspの書き方はruiさんもしてるのに
main:
  push rsp
  push rsp 
  pop rdi
  pop rax
  sub rax, rdi
  ret
はok。出力は8。
ちなみに
main:
  mov rax, rsp
  //push 12
  push rsp 
  pop rdi
  sub rax, rdi
  ret
  
だとsegmentation faultは起きず出力ははゼロ
そもそもRSPとスタックの関係がまたよくわからなくなった。chibiccにintを導入してからまた考えて。わからなければ質問。
2024/12/10
=>レジスタtoレジスタの値の移動はダメらしい

12/10
配列の添え字を実装する
Cでは x[y]は*(x+y)と等価であるものとして定義されています。したがって添字の実装は比較的簡単です。単純にx[y]をパーザの中で*(x+y)として読み換えるようにしてください。たとえばa[3]は*(a+3)になります。

この文法では、3[a]は*(3+a)に展開されるので、a[3]が動くなら3[a]も動くはずですが、なんとCでは3[a]のような式は実際に合法です

ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー
12/10
int main(){
    return func();
}
int func(){
    return 3;
}
int func(){
    return 5;
}
のような命令は現段階ではアセンブラがエラーメッセージを出す。
ｃコンパイラがエラーメッセージ出さなくていいの？

追記
ちなみにgccでローカル変数funcと関数func()を用意してコンパイルしたところ、
エラーになったので、少なくともgccでは関数の際に出てくるidentも(※)
本コンパイラにおけるpush_var()のような関数の対象内にしていると思われる
>>
12/29
introduce return type. //(Type*)ty->return_ty

※function=basetype ident "(" params? ")" "{" stmt* "}"
ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー
12/29

in codegen.c, case ND_EXPR_STMT
why "add rsp, 8"

|_|...8 byte

//movement of stack until function call

//no add rsp
|_|<-2|          |          |
|_|   |(<-[a])<-3|          |
|_|   |(<-3)     |(<-[b])<-6|
|_|   |          |(<-6)     |<-a|   |<-a+b
|_|   |          |          |   |<-b|
|_|
|_|
|_|

// add rsp
|_|<-2|            |            |   |
|_|   |(<-[a])(<-3)|(<-[b])(<-6)|<-a|   |<-a+b
|_|   |(<-3)       |(<-6)       |   |<-b|
|_|   |            |            |   |
|_|
|_|
|_|
|_|

int main(){
    return num(2,({int a=3;int b=6;a+b;}));
}

int num(int x,int y){
    return x+y;
}

>>

main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  push 2

  lea rax, [rbp-16]
  push rax
  push 3
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
  (add rsp, 8)

  lea rax, [rbp-8]
  push rax
  push 6
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
  (add rsp, 8)

  lea rax, [rbp-16]
  push rax
  pop rax
  mov rax, [rax]
  push rax

  lea rax, [rbp-8]
  push rax
  pop rax
  mov rax, [rax]
  push rax

  pop rdi
  pop rax
  add rax, rdi
  push rax

//function call
  pop rsi
  pop rdi
  mov rax, rsp
  and rax, 15
  jnz .Lcall0
  mov rax, 0
  call num
  jmp .Lend0
.Lcall0:
  sub rsp, 8
  mov rax, 0
  call num
  (add rsp, 8)
.Lend0:
  push rax
  pop rax
  jmp .Lreturn.main
.Lreturn.main:
  mov rsp, rbp
  pop rbp
  ret
.global num
num:
  push rbp
  mov rbp, rsp
  sub rsp, 16
  mov [rbp-16], rdi
  mov [rbp-8], rsi
  lea rax, [rbp-16]
  push rax
  pop rax
  mov rax, [rax]
  push rax
  lea rax, [rbp-8]
  push rax
  pop rax
  mov rax, [rax]
  push rax
  pop rdi
  pop rax
  add rax, rdi
  push rax
  pop rax
  jmp .Lreturn.num
.Lreturn.num:
  mov rsp, rbp
  pop rbp
  ret
------------------------------------------------------------
12/29
about typedef

code{
typedef int t;
t x;
}

parse.c
line 1 typedef int t;>>add new var_scope

stmt(){
  ...
  if(tok=consume("typedef")){

  }
  ...
}
>>
(VarScope*){
  (VarScope*)next=/*existing*/var_scope
  (char*)name="t";
  (Var*)var=NULL
  (Type*)type_def={
    kind=TY_INT;
    align=8;
    base=NULL;
    array_size=0;
    members=NULL;
  }
}

line 2 "t x;">> compile as "int x;"

stmt(){
  ...
  if(is_typename()){
    return declaration();
  }
  ...
}
=>is_typename(){
  return ...||find_typedef(Token*tok)
}
==>find_typedef

=>declaration()
>>
ty=basetype(){find_var("t")}
push_var((char*name)"x",(Type*)ty,(bool is_local)true);
--------------------------------------------------------------------
12/29

when exerting "pop", sub rsp, 8
but all size of variable is not 8.
doesn't unintentional stack interface happen?

|_|...1 byte

|_|<-rbp
|_|
|_|<-(rbp-2)==5
|_|
|_|<-(rbp-4)==3
~~
(|_|...8 byte)
|_|<-[rbp-4]==3
|_|<-[rbp-2]==5
|_|


int main(){
    short a=3;
    short b=5;
    return a+b;
}
>>
main:
  push rbp
  mov rbp, rsp
  sub rsp, 8 //function stack size is a multiple of 8

  lea rax, [rbp-4]
  push rax
  push 3
  pop rdi
  pop rax
  mov [rax], di
  push rdi
  add rsp, 8

  lea rax, [rbp-2]
  push rax
  push 5
  pop rdi
  pop rax
  mov [rax], di
  push rdi
  add rsp, 8

  lea rax, [rbp-4]
  push rax
  pop rax
  movsx rax, word ptr [rax]
  push rax

  lea rax, [rbp-2]
  push rax
  pop rax
  movsx rax, word ptr [rax]
  push rax

  pop rdi
  pop rax
  add rax, rdi
  push rax
  pop rax
  jmp .Lreturn.main
.Lreturn.main:
  mov rsp, rbp
  pop rbp
  ret
-----------------------------------------------------------------------
12/31
'
int main(){
  return num();
}
'>>assembler error

'
int main(){
  int num=1;
  return num();
}
int num(){
  return 2;
}
'>>compiler error :"not a function"

'
int num(){
  int num=2;
  return num;
}
int main(){
  return num();
}
'>> compiler error :in main, "not a function"

'
int main(){
  return num();
}
int num(){
  return 2;
}
'>>
else{
    printf("no sc \n");
    node->ty=int_type();
}を通ってコンパイルされた。
int num() > char num()
に変更しても同様だった。return_tyは何だったの？


int inc(){
    int num=0;
    num=num+1;
    return num;
}
int main(){
    return num();
}>>compiler error : num() "not a function"

int inc(){
    static num;
    num=num+1;
    return num;
}
int main(){
    return num();
}>>compiler error : num() "not a function"
関数が変わってもリセットされるのは(VarList*)localsだけで(VarScope*)var_scopeはそのまま
localsはmain.cにてoffsetを確保する際にのみ使用
staticはis_local=falseであり、global変数扱い
-----------------------------------------------------------------------
12/31
'
int main(){
    enum{a=5,b=7};
    int a=2;
    return a;
}
'>>2

'
int main(){
    int a=2;
    enum{a=5,b=7};
    return a;
}
'>>5
-----------------------------------------------------------------------
1/20
int *val[8];
return val[1];


int *val[8];
|_|...int*     (|_| means 8 bytes)
|_|_|_|_|...int  (|_|means 1 byte)

0xXX01|_|...val...> 0xXX10|_|...*val==0xX100...>0xX100|_|_|_|_|...*val[0]
                    0xXX18|_|...            ...>0xX104|_|_|_|_|
                    0xXX26|_|...            ...>0xX108|_|_|_|_|
                    0xXX34|_|...            ...>0xX112|_|_|_|_|
                    0xXX42|_|...            ...>0xX116|_|_|_|_|
                    0xXX50|_|...            ...>0xX120|_|_|_|_|
                    0xXX58|_|...            ...>0xX124|_|_|_|_|
                    0xXX66|_|...            ...>0xX128|_|_|_|_|
/*
valはあるアドレス上に*val[0]というint型の値を格納しているアドレス

以下、参考
int num[10];
num is equal to &num[0]
|_|...num...> |_|...*Val==val[0]
              |_|
              |_|
              |_|
              |_|
              |_|
              |_|
              |_|
              |_|
              |_|
*/

右辺値*val[1]
ND_DEREF/*
->ND_DEREF/[]
ND_ADD<-
ND_VAR/int<- ->ND_NUM/1

=>val[1]に格納されている値をアドレスとみなし、
そのアドレスに格納されている値*val[1]を返す

ex)

int main(){
    char*s[2];
    s[0]="hello world";
    s[1]="minecraft";
}

return s[0][1] => 'e' //(s[0])[1]
return s[1][0] => 'm' 
return *s[1]   => 'm' // *(s[1])
return (*s)[1] => 'e'
-----------------------------------------------------------------------
1/23
int a[5] と宣言したとき
スタック上のメモリ確保数は
int a;
int b;
int c;
int d;
int e;
の時と変わらない。
ではなぜ、int a[5];としたときaは&a[0]を表すのか

==>codegen.c gen(Node*node)

        case ND_VAR:
        case ND_MEMBER:
            gen_addr(node);
            if(node->ty->kind!=TY_ARRAY){
                load(node->ty);
            }
            return;
上のif分のおかげ但しこのせいで、関数の返り値をreturn a[2];
のようにしても返ってくるのはa[2]ではなく&a[0]。
これはc言語の使用

以下は上記を理解するために書いたものなどで別に読まなくていいよ

parse.c にて
宣言時
TY_ARRAY -> TY_INT

a[2]=11;
          ND_ASSIGN
ND_DEREF<-        ->TY_NUM 11
ND_ADD<-
ND_VAR<- ->ND_NUM

type.cにて
6,ND_ASSIGN...TY_INT
5,TY_NUM 11...TY_INT
4,ND_DEREF...TY_INT
3,ND_ADD...TY_ARRAY -> TY_INT
1,ND_VAR...TY_ARRAY -> TY_INT
2,ND_NUM 2...TY_INT
-----------------------------------------------------------------------------------------------
2025/3/6
codegen.c の case ND_FOR,ND_WHILE の時に brkseq=brk; するけどこれ必要??
(ND_BREAKを導入したときに brkseq や brk を書いた)

=>これがないと for や while のブロック内に breakを伴う条件分 が複数出てくるとおかしくなるよ

int main(){
    int a=0; 
    for(;;){              ...(A)
        for(;;)break;     ...(B)
        if(a++==4) break; ...(C)
    } 
    return a;
}
/*codegen.c の ND_IF での labelseq,brkseq の動き
(1)
  int seq=labelseq;
  labelseq++;
(2)
*/
/*codegen.c の ND_FOR,ND_WHILE での labelseq,brkseq の動き
(1)
  int seq=labelseq;
  labelseq++;
  int brk=brkseq;
  brkseq=seq;
(2)
  ...
  gen(node->then)
  ...
  printf(".L.break.%d:\n",seq);
  brkseq=brk;
(3)

brkseqの使い道
case ND_BREAK:
    ...
    printf("  jmp .L.break.%d\n",brkseq);
    ...
*/

//brkseq=brk; 有り
//ex)labelseq=1,brkseq=1
//(1)labelseq=1,brkseq=1...(A)
//(2)labelseq=2,brkseq=1,(seq=1,brk=1)
// //gen(node->then) の中の for構文{
//     (B)
//      (1)labelseq=2,brkseq=1
//      (2)labelseq=3,brkseq=2,(seq=2,brk=1)
//       //gen(node->then){
//            jmp .L.break.2 // printf("  jmp .L.break.%d\n",brkseq) //ND_BREAK
//         }
//         .L.break.2: // printf(".L.break.%d:\n",seq);
//      (3)labelseq=3,brkseq=1,(seq=2,brk=1)
//       
//     (C)
//      (1)labelseq=3,brkseq=1
//      (2)labelseq=4,brkseq=1,(seq=3)
//            jmp .L.break.1 // printf("  jmp .L.break.%d\n",brkseq) //ND_BREAK
//   }
//   .L.break.1: // printf(".L.break.%d:\n",seq);
//(3)labelseq=4,brkseq=1,(seq=1,brk=1)


//brkseq=brk; 無し
//ex)labelseq=1,brkseq=1
//(1)labelseq=1,brkseq=1
//(2)labelseq=2,brkseq=1,(seq=1,brk=1)
// //gen(node->then) の中の for構文{
//     (B)
//      (1)labelseq=2,brkseq=1
//      (2)labelseq=3,brkseq=2,(seq=2,brk=1)
//       //gen(node->then){
//            jmp .L.break.2 // printf("  jmp .L.break.%d\n",brkseq) //ND_BREAK
//         }
//         .L.break.2: // printf(".L.break.%d:\n",seq);
//      (3)labelseq=3,brkseq=2,(seq=2,brk=1)  //brkseq=2 (違う点)
//      
//     (C)
//      (1)labelseq=3,brkseq=2
//      (2)labelseq=4,brkseq=2,(seq=3)
//            jmp .L.break.2 // printf("  jmp .L.break.%d\n",brkseq) //ND_BREAK
//   }
//   .L.break.1: // printf(".L.break.%d:\n",seq);
//(2)labelseq=3,brkseq=2,(seq=1,brk=1)        //brkseq=2 (違う点)

main:
  push rbp
  mov rbp, rsp
  sub rsp, 8
  lea rax, [rbp-4]
  push rax
  push 0
  pop rdi
  pop rax
  mov [rax], edi
  push rdi
  add rsp, 8
.Lbegin1:
.Lbegin2:
  jmp .L.break.2
  jmp  .Lbegin2
.L.break.2:
  lea rax, [rbp-4]
  push rax
  push [rsp]
  pop rax
  movsxd rax, dword ptr [rax]
  push rax
  pop rax
  add rax, 1
  push rax
  pop rdi
  pop rax
  mov [rax], edi
  push rdi
  pop rax
  sub rax, 1
  push rax
  push 4
  pop rdi
  pop rax
  cmp rax, rdi
  sete al
  movzb rax, al
  push rax
  pop rax
  cmp rax, 0
  je  .Lend3
  jmp .L.break.1// brkseq=brk; 有り
  jmp .L.break.2// brkseq=brk; 無し
.Lend3:
  jmp  .Lbegin1
.L.break.1:
  lea rax, [rbp-4]
  push rax
  pop rax
  movsxd rax, dword ptr [rax]
  push rax
  pop rax
  jmp .Lreturn.main
.Lreturn.main:
  mov rsp, rbp
  pop rbp
  ret


------------------------------------------------------------------------
グローバル、ローカル変数における文字列。//左辺がアドレス型か、配列型かによるメモリ確保の変化
=>
ローカルのchar型配列のみローカル変数的なメモリ配置。それ以外はグローバルな感じ

char*global="abc";
char gl[]="cd";
int main(){
    char*string="bcdef";
    char str[]="defgh";
    return str[1];
}

intel_syntax noprefix
.data
.L.date.1:
  .byte 98  ...'b'
  .byte 99  ...'c'
  .byte 100 ...'d'
  .byte 101 ...'e'
  .byte 102 ...'f'
  .byte 0
gl:
  .byte 99  ...'c'
  .byte 100 ...'d'
  .byte 0
.L.date.0:
  .byte 97  ...'a'
  .byte 98  ...'b'
  .byte 99  ...'c'
  .byte 0
global:
  .quad .L.date.0
.text
.global main
main:
  push rbp
  mov rbp, rsp
  sub rsp, 16
.Lcccccc0:
  lea rax, [rbp-16]
  push rax
  mov rax, offset .L.date.1
  push rax
  pop rdi
  pop rax
  mov [rax], rdi
  push rdi
  add rsp, 8
.Lcccccc1:
  lea rax, [rbp-6]
  push rax
  push 0
  pop rdi
  pop rax
  imul rdi, 1
  add rax, rdi
  push rax
  push 100  ...'d'
  pop rdi
  pop rax
  mov [rax], dil
  push rdi
  add rsp, 8
.Lcccccc2:
  lea rax, [rbp-6]
  push rax
  push 1
  pop rdi
  pop rax
  imul rdi, 1
  add rax, rdi
  push rax
  push 101  ...'e'
  pop rdi
  pop rax
  mov [rax], dil
  push rdi
  add rsp, 8
.Lcccccc3:
  lea rax, [rbp-6]
  push rax
  push 2
  pop rdi
  pop rax
  imul rdi, 1
  add rax, rdi
  push rax
  push 102  ...'f'
  pop rdi
  pop rax
  mov [rax], dil
  push rdi
  add rsp, 8
.Lcccccc4:
  lea rax, [rbp-6]
  push rax
  push 3
  pop rdi
  pop rax
  imul rdi, 1
  add rax, rdi
  push rax
  push 103  ...'g'
  pop rdi
  pop rax
  mov [rax], dil
  push rdi
  add rsp, 8
.Lcccccc5:
  lea rax, [rbp-6]
  push rax
  push 4
  pop rdi
  pop rax
  imul rdi, 1
  add rax, rdi
  push rax
  push 104  ...'h'
  pop rdi
  pop rax
  mov [rax], dil
  push rdi
  add rsp, 8
.Lcccccc6:
  lea rax, [rbp-6]
  push rax
  push 5
  pop rdi
  pop rax
  imul rdi, 1
  add rax, rdi
  push rax
  push 0  ...'\0'
  pop rdi
  pop rax
  mov [rax], dil
  push rdi
  add rsp, 8

  lea rax, [rbp-6]
  push rax
  push 1
  pop rdi
  pop rax
  imul rdi, 1
  add rax, rdi
  push rax
  pop rax
  movsx rax, byte ptr [rax]
  push rax
  pop rax
  jmp .Lreturn.main
.Lreturn.main:
  mov rsp, rbp
  pop rbp
  ret


```//斜体をブロック

