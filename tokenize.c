#include"chibicc.h"

char*user_input;

Token *token;//current token

void error(char*fmt,...){
    va_list ap;
    va_start(ap,fmt);
    vfprintf(stderr,fmt,ap);
    fprintf(stderr,"\n");
    exit(1);
}

void error_at(char*loc,char*fmt,...){
    va_list ap;
    va_start(ap,fmt);

    int pos=loc-user_input;
    fprintf(stderr,"%s\n",user_input);
    fprintf(stderr,"%*s",pos,"");
    fprintf(stderr,"^ ");
    vfprintf(stderr,fmt,ap);
    fprintf(stderr,"\n");

    exit(1);
}

char*strn_dup(char*p,int len){
    char*buf=calloc(len+1,sizeof(char*));//buf[0]~buf[len]
    strncpy(buf,p,len);//copy
    buf[len]='\0';//NULLはポインタ、\0は値として使う。意味合いは同じ
    return buf;
}

bool consume(char* op){
    if(token->kind!=TK_RESERVED||strlen(op)!=token->len||memcmp(token->str,op,token->len)){
        return false;
    }else{
        token=token->next;
        return true;
    }
}

Token*consume_ident(){
    if(token->kind!=TK_IDENT){
        return NULL;
    }
    Token*t=token;
    token=token->next;
    return t;
}

void expect(char* op){
    if(token->kind!=TK_RESERVED||strlen(op)!=token->len||memcmp(token->str,op,token->len)){
        error_at(token->str,"expected \"%s\"",op);
    }

    token=token->next;
}

int expect_number(){
    if(token->kind!=TK_NUM){
        error_at(token->str,"expected a number");
    }

    int val=token->val;
    token=token->next;
    return val;
}

bool at_eof(){
    return token->kind==TK_EOF;
}

Token*new_token(TokenKind kind,Token*cur,char*str,int len){
    Token*tok=calloc(1,sizeof(Token));
    tok->kind=kind;
    tok->str=str;
    tok->len=len;
    cur->next=tok;
    return tok;
}

bool startswith(char*p,char*q){
    return memcmp(p,q,strlen(q))==0;
}

bool is_alpha(char c){
    return ('a'<=c&&c<='z')||('A'<=c&&c<='Z')||c=='_';
}

bool is_alnum(char c){
    return is_alpha(c)||('0'<=c&&c<='9');
}

char*starts_with_reserved(char*p){
    static char*kw[]={"return","if","else"};

    for(int i=0;i<sizeof(kw)/sizeof(*kw);i++){
        int len=strlen(kw[i]);
        if(startswith(p,kw[i])&&!is_alnum(p[len])){
            return kw[i];
        }
    }

    static char*punctuator[]={"==","!=","<=",">="};
    for(int i=0;i<sizeof(punctuator)/sizeof(*punctuator);i++){
        if(startswith(p,punctuator[i])){
            return punctuator[i];
        }
    }

    return NULL;
}

Token*tokenize(){
    char*p=user_input;
    Token head;
    head.next=NULL;
    Token*cur=&head;

    while(*p){
        if(isspace(*p)){
            p++;
            continue;
        }

        //keyword or punctuator
        char*kw=starts_with_reserved(p);
        if(kw){
            int len=strlen(kw);
            cur=new_token(TK_RESERVED,cur,p,len);
            p+=len;
            continue;
        }

        /*
        本来は
        strchr(const char*s,int c)
        s=対象の文字列 : c=探す文字 (cをchar型にキャストしたもの)
        であるが、ここでは
        cがsにあるいずれかの文字でないかをチェックしている。
        天才。
        */
        if(strchr("+-*/()<>;=",*p)){
            cur=new_token(TK_RESERVED,cur,p,1);
            p++;
            continue;
        }

        if(is_alpha(*p)){
            char*q=p;
            p++;
            while(is_alnum(*p)){
                p++;
            }
            cur=new_token(TK_IDENT,cur,q,p-q);
            continue;
        }
        
        if(isdigit(p[0])){
            cur=new_token(TK_NUM,cur,p,0);
            char*q=p;
            cur->val=strtol(p,&p,10);
            cur->len=p-q;
            continue;
        }
        error_at(p,"expected a number");
    }
    new_token(TK_EOF,cur,p,0);
    return head.next;
}
