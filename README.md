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
  
