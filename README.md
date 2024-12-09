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
本コンパイラにおけるpush_var()尿な関数の対象内にしていると思われる

※function=basetype ident "(" params? ")" "{" stmt* "}"
ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー