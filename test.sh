#!/bin/bash

cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3(){return 3;}
int ret5(){return 5;}
int add(int x,int y){return x+y;}
int sub(int x,int y){return x-y;}

int add6(int a,int b,int c, int d, int e,int f){return a+b+c+d+e+f;}
EOF

assert(){
    expected="$1"
    input="$2"

    ./chibicc "$input" > tmp.s

    cc -g -static -o tmp tmp.s tmp2.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input =>$actual"
    else
        echo "$input =>$expected expected, but got $actual"
        exit 1
    fi
    echo "----------------------------------------------"
}
assert 0 'int main(){return 0;}'
assert 42 'int main(){return 42;}'
assert 21 'int main(){return 5+20-4;}'
assert 41 'int main(){return  12 + 34 - 5;}'
assert 47 'int main(){return 5+6*7;}'
assert 15 'int main(){return 5*(9-6);}'
assert 4 'int main(){return (3+5)/2;}'
assert 10 'int main(){return --10;}'
assert 10 'int main(){return --+10;}'

assert 0 'int main(){return 0==1;}'
assert 1 'int main(){return 42==42;}'
assert 1 'int main(){return 0!=1;}'
assert 0 'int main(){return 42!=42;}'

assert 1 'int main(){return 0<1;}'
assert 0 'int main(){return 1<1;}'
assert 0 'int main(){return 2<1;}'
assert 1 'int main(){return 0<=1;}'
assert 1 'int main(){return 1<=1;}'
assert 0 'int main(){return 2<=1;}'

assert 1 'int main(){return 1>0;}'
assert 0 'int main(){return 1>1;}'
assert 0 'int main(){return 1>2;}'
assert 1 'int main(){return 1>=0;}'
assert 1 'int main(){return 1>=1;}'
assert 0 'int main(){return 1>=2;}'

assert 1 'int main(){return 1; 2; 3;}'
assert 2 'int main(){ 1;return 2; 3;}'
assert 3 'int main(){1; 2;return 3;}'

assert 3 'int main(){int a=3; return a;}'
assert 8 'int main(){int a=3;int b=5;return a+b;}'

assert 3 'int main(){if(0)return 2;return 3;}'
assert 3 'int main(){if(1-1)return 2;return 3;}'
assert 2 'int main(){if(1)return 2;return 3;}'
assert 2 'int main(){if(2-1)return 2;return 3;}'
assert 3 'int main(){if(5-2>4)return 2;else return 3;}'

#if(a=1)はコンパイルエラーにならず、左辺値をifの判定に用いる
assert 5 '
int main(){
    int a=0;
    if(a=1){
        return 5;
    }else{
        return 0;
    }
}'
assert 0 '
int main(){
    int a=1;
    if(a=0){
        return 5;
    }else{
        return 0;
    }
}'

assert 10 'int main(){int i=0;while(i<10) i=i+1; return i;}'

assert 3 'int main(){for(;;) return 3;return 5;}'
assert 10 'int main(){int i;for(i=0;i<10;i=i+1)1;return i;}'

assert 17 '
int main(){
    int j=0;
    int i;
    for(i=0;i<10;i=i+1){
        if(i>6){
            j=j+2;
            if(j==4){
                j=j+1;
            }
        }
    }
    return i+j;
}'

assert 3 'int main(){return ret3();}'
assert 5 'int main(){return ret5();}'
assert 8 'int main(){return add(5,3);}'
assert 2 'int main(){return sub(5,3);}'
assert 21 'int main(){return add6(1,2,3,4,5,6);}'

assert 42 '
int main(){
    return 10+ret32();
}
int ret32(){
    return 32;
}'
assert 42 '
int ret32(){
    return 32;
}
int main(){
    return 10+ret32();
}'

assert 7 '
int add2(int x,int y){
    return x+y;
}
int main(){
    return add2(3,4);
}'
assert 7 '
int main(){
    return add2(3,4);
}
int add2(int x,int y){
    return x+y;
}'
assert 55 '
int main(){
    return fib(9);
}
int fib(int x){
    if(x<=1){
        return 1;
    }
    return fib(x-1)+fib(x-2);
}'

assert 5 '
int main(){
    int x=3;
    int y=5;
    return *(&x+1);
}'

assert 7 '
int main(){
    int x=3;
    int y=5;
    *(&x+1)=7;
    return y;
}'

assert 5 '
int main(){
    int*p;
    int a=2;
    int b=5;
    p=&a;
    return *(p+1);
}'

# #文法上はいいけど
# #node->stack_sizeで表されるメモリ領域以上のメモリ領域を使うことになるので
# #長いc言語コードを書いていたらどこかで意図しないメモリ参照が生まれるはず。
# assert 4 '
# int main(){
#     int*x;
#     *(x+2)=4;
#     return *(x+2);
# }'


# #文法上はあってる？
# assert 4 '
# int main(){
#     int p=5;
#     return &(*p);
# }'

assert 3 '
int main(){
    int x[2];
    int *y=&x;
    *y=3;
    return *x;
}'

assert 4 '
int main(){
    int x[3];
    *x=3;
    *(x+1)=4;
    *(x+2)=5;
    return *(x+1);
}'

assert 1 '
int main(){
    int x[2][3];
    int *y=x;
    *(y+1)=1;
    return *(*x+1);
}'

assert 4 '
int main(){
    int x[2][3];
    int *y=x;
    *(y+4)=4;
    return *(*(x+1)+1);
}'

assert 6 '
int main(){
    int x[2][3];
    int *y=x;
    *(y+6)=6;
    return **(x+2);
}'

assert 3 '
int main(){
    int x[3];
    x[0]=2;
    x[1]=3;
    x[2]=4;
    return *(x+1);
}'

assert 4 '
int main(){
    int x[3];
    x[0]=2;
    x[1]=3;
    x[2]=4;
    return x[2];
}'

#x[0][1]=*(*(x+0)+1)

# x[0][0]=*y=y[0]
# x[0][1]
# x[0][2]
# x[1][0]=*(y+3)=y[3]
# x[1][1]
# x[1][2]
assert 5 '
int main(){
    int x[2][3];
    int *y=x;
    y[4]=5;
    return x[1][1];
}'

assert 8 '
int main(){
    int x;
    return sizeof x;
}'

assert 8 '
int main(){
    int x;
    return sizeof x;
}'

assert 32 '
int main(){
    int x[4];
    return sizeof x;
}'

assert 96 '
int main(){
    int x[3][4];
    return sizeof x;
}'

assert 32 '
int main(){
    int x[3][4];
    return sizeof (*x);
}'

assert 8 '
int main(){
    int x[3][4];
    return sizeof (**x);
}'

assert 9 '
int main(){
    int x[3][4];
    return sizeof (**x)+1;
}'

assert 8 '
int main(){
    int x[3][4];
    return sizeof (**x+1);
}'

assert 0 '
int x;
int main(){
    return x;
}'

assert 2 '
int x[4];
int main(){
    x[0]=0;
    x[1]=1;
    x[2]=2;
    x[3]=3;
    return x[2];
}'

assert 8 '
int x;
int main(){
    return sizeof x;
}'

echo OK