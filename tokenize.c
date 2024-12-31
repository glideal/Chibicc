#include"chibicc.h"

char*filename;
char*user_input;

Token *token;//current token

void error(char*fmt,...){
    va_list ap;
    va_start(ap,fmt);
    vfprintf(stderr,fmt,ap);
    fprintf(stderr,"\n");
    exit(1);
}

void verror_at(char*loc,char*fmt,va_list ap){
    //find a line contatining 'loc'
    char*line=loc;
    while(user_input<line&&line[-1]!='\n'){
        line--;
    }
    char*end=loc;
    while(*end!='\n'){
        end++;
    }

    //get a line number
    int line_num=1;
    for(char*p=user_input;p<line;p++){
        if(*p=='\n'){
            line_num++;
        }
    }

    //print out the line
    int indent=fprintf(stderr,"%s:%d: ",filename,line_num);
    fprintf(stderr,"%.*s\n",(int)(end-line),line);

    //show the error message
    /*
    like this

    /dev/fd/63:5:         retrn 5;
                          ^ undefined variable

    */
    int pos=loc-line+indent;

    fprintf(stderr,"%*s",pos,"");
    fprintf(stderr,"^ ");
    vfprintf(stderr,fmt,ap);
    fprintf(stderr,"\n");

    exit(1);
}

void error_at(char*loc,char*fmt,...){
    va_list ap;
    va_start(ap,fmt);

    verror_at(loc,fmt,ap);
}

void error_tok(Token*tok,char*fmt,...){
    va_list ap;
    va_start(ap,fmt);
    if(tok){
        verror_at(tok->str,fmt,ap);
    }else{
        vfprintf(stderr,fmt,ap);
        fprintf(stderr,"\n");
        exit(1);
    }
}

char*strn_dup(char*p,int len){
    char*buf=calloc(len+1,sizeof(char*));//buf[0]~buf[len]
    strncpy(buf,p,len);//copy
    buf[len]='\0';//NULLはポインタ、\0は値として使う。意味合いは同じ
    return buf;
}

Token*peek(char*s){
    if(token->kind!=TK_RESERVED||strlen(s)!=token->len||memcmp(token->str,s,token->len)){
        return NULL;
    }
    return token;
}


Token*consume(char* s){
    if(!peek(s)){
        return NULL;
    }else{
        Token*t=token;
        token=token->next;
        return t;
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

void expect(char* s){
    if(!peek(s)){
        error_tok(token,"expected \"%s\"",s);
    }

    token=token->next;
}

long expect_number(){
    if(token->kind!=TK_NUM){
        error_tok(token,"expected a number");
    }

    long val=token->val;
    token=token->next;
    return val;
}

char*expect_ident(){
    if(token->kind!=TK_IDENT){
        error_tok(token,"expected an identifier");
    }
    char*s=strn_dup(token->str,token->len);
    token=token->next;
    return s;
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
    static char*kw[]={"return","if","else","while","for","int","char","sizeof","struct","typedef","short","long","void","_Bool","enum","static"};

    for(int i=0;i<sizeof(kw)/sizeof(*kw);i++){
        int len=strlen(kw[i]);
        if(startswith(p,kw[i])&&!is_alnum(p[len])){
            return kw[i];
        }
    }

    static char*punctuator[]={"==","!=","<=",">=","->","++","--"};
    for(int i=0;i<sizeof(punctuator)/sizeof(*punctuator);i++){
        if(startswith(p,punctuator[i])){
            return punctuator[i];
        }
    }

    return NULL;
}

char get_escape_char(char c){
    switch(c){
        case 'a':return '\a';
        case 'b':return '\b';
        case 't':return '\t';
        case 'n':return '\n';
        case 'v':return '\v';
        case 'f':return '\f';
        case 'r':return '\r';
        case 'e':return 27;
        case '0':return 0;
        default:return c;
    }
}

Token*read_string_literal(Token*cur,char*start){
    char*p=start+1;//+1 to skip '"'
    char buf[1024];//buf[0]~buf[2^10-1] //2^10-1=0b1111111111
    int len=0;

    for(;;){
        if(len==sizeof(buf)){
            error_at(start,"string literal too large");
        }
        if(*p=='\0'){
            error_at(start,"unclosed string literal");
        }
        if(*p=='"'){
            break;
        }

        if(*p=='\\'){// '\\' means single '\'
            p++;
            buf[len]=get_escape_char(*p);
            len++;
            p++;
        }else{
            buf[len++]=*p++;
        }
    }

    Token*tok=new_token(TK_STR,cur,start,p-start+1);
    tok->contents=malloc(len+1);
    memcpy(tok->contents,buf,len);
    tok->contents[len]='\0';
    tok->cont_len=len+1;
    return tok;
    /*
    cur->len is the length of ("literal") includiing '"'
    cur->contents is (literal) not including '"'
    cur->cont_len is the length of (literal) not including '"'
    */
}

Token*read_char_literal(Token*cur,char*start){
    //(char)start=='\''
    char*p=start+1;
    if(*p=='\0'){
        error_at(start,"unclosed char literal");
    }

    char c;
    if(*p=='\\'){
        p++;
        c=get_escape_char(*p);
        p++;
    }else{
        c=*p;
        p++;
    }

    if(*p!='\''){
        error_at(start,"char literal too long");
    }
    p++;

    Token*tok=new_token(TK_NUM,cur,start,p-start);
    tok->val=c;
    return tok;
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

        //skip line comments
        if(startswith(p,"//")){
            p++;
            while(*p!='\n'){
                p++;
            }
            continue;
        }

        //skip block comments
        if(startswith(p,"/*")){
            char*q=strstr(p+2,"*/");
            if(!q){
                error_at(p,"unclosed block comments");
            }
            p=q+2;
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
        if(strchr("+-*/()<>;={},&[].",*p)){
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

        //string literal
        //ascii start SP(32) and end DEL(127)
        if(*p=='"'){
            cur=read_string_literal(cur,p);
            p+=cur->len;
            continue;
        }

        //character literal
        if(*p=='\''){
            cur=read_char_literal(cur,p);
            p+=cur->len;
            continue;
        }
        
        if(isdigit(*p)){
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
