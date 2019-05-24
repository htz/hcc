#!/bin/bash

function make_driver {
  echo 'int main(){printf("%d\n", mymain());return 0;}' | ./hcc > driver.s
}

function compile {
  echo "$1" | ./hcc > tmp.s 2>/dev/null
  if [ $? -ne 0 ]; then
    echo "Failed to compile $2"
    exit
  fi
  gcc -no-pie -o tmp.out driver.s tmp.s
  if [ $? -ne 0 ]; then
    echo "GCC failed"
    exit
  fi
}

function assertequal {
  if [ "$1" != "$2" ]; then
    echo "Test failed: $2 expected but got $1"
    exit
  fi
}

function testast {
  result="$(echo "$2" | ./hcc -a 2>/dev/null)"
  if [ $? -ne 0 ]; then
    echo "Failed to compile $2"
    exit
  fi
  assertequal "$result" "$1"
}

function test {
  compile "$2"
  assertequal "$(./tmp.out)" "$1"
}

function testfail {
  expr="$1"
  echo "$expr" | ./hcc > /dev/null 2>&1
  if [ $? -eq 0 ]; then
    echo "Should fail to compile, but succeded: $expr"
    exit
  fi
}

make -s hcc
make_driver

testast '(f->int [] {1;})' 'int f(){1;}'
testast '(f->int [] {1, 2;})' 'int f(){1,2;}'
testast '(f->int [] {-1;})' 'int f(){-1;}'
testast "(f->int [] {99;})" "int f(){'c';}"
testast '(f->int [] {"abc";})' 'int f(){"abc";}'
testast '(f->int [] {"a\\b\"c";})' 'int f(){"a\\b\"c";}'
testast '(f->int [] {1;2;})' 'int f(){1;2;}'
testast '(f->int [] {;})' 'int f(){;}'
testast '(f->int [] {;;1;;})' 'int f(){;;1;;}'
testast '(f->int [] {1.250000;})' 'int f(){1.25;}'
testast '(f->int [] {1.250000;})' 'int f(){1.25f;}'
testast '(f->int [] {0.123000;})' 'int f(){.123f;}'

testast '(f->int [] {(+ (- (+ 1 2) 3) 4);})' 'int f(){1+2-3+4;}'
testast '(f->int [] {(+ (+ 1 (* 2 3)) 4);})' 'int f(){1+2*3+4;}'
testast '(f->int [] {(+ (* 1 2) (* 3 4));})' 'int f(){1*2+3*4;}'
testast '(f->int [] {(+ (/ 4 2) (/ 6 3));})' 'int f(){4/2+6/3;}'
testast '(f->int [] {(/ (/ 24 2) 4);})' 'int f(){24/2/4;}'
testast '(f->int [] {(+ (* (+ 1 2) 3) 4);})' 'int f(){(1+2)*3+4;}'
testast '(f->int [] {(% (+ (+ (+ 1 2) 3) 4) 3);})' 'int f(){(1+2+3+4)%3;}'
testast '(f->int [] {(if 1 2 3);})' 'int f(){1?2:3;}'
testast "(f->int [] {(decl int a 3);})" 'int f(){int a=3;}'
testast "(f->int [] {(decl char c 97);})" "int f(){char c='a';}"
testast "(f->int [] {(decl char* s \"abc\");})" 'int f(){char *s="abc";}'
testast "(f->int [] {(decl char[4] s \"abc\");})" 'int f(){char s[4]="abc";}'
testast "(f->int [] {(decl int[3] a [1 2 3]);})" 'int f(){int a[3]={1,2,3};}'
testast "(f->int [] {(decl int[2][3] a [[0 1 2] [3 4 5]]);})" 'int f(){int a[2][3]={{0,1,2},{3,4,5}};}'
testast "(f->int [] {(decl int a 1);(decl int b 2);(= a (= b 3));})" 'int f(){int a=1;int b=2;a=b=3;}'
testast "(f->int [] {(decl int a 3);(& a);})" 'int f(){int a=3;&a;}'
testast "(f->int [] {(decl int a 3);(* (& a));})" 'int f(){int a=3;*&a;}'
testast "(f->int [] {(decl int a 3);(decl int* b (& a));(* b);})" 'int f(){int a=3;int *b=&a;*b;}'
testast '(f->int [] {(if 1 2);})' 'int f(){if(1)2;}'
testast '(f->int [] {(if 1 {2;} (if 3 {4;} {5;}));})' 'int f(){if(1){2;}else if(3){4;}else{5;}}'
testast '(decl int a 3)' 'int a=3;'
testast '(decl int a 3), (decl int b);(decl char c)' 'int a=3,b;char c;'
testast '(decl char a);(decl signed char s);(decl unsigned char u)' 'char a;signed char s;unsigned char u;'
testast '(decl int a);(decl int s);(decl unsigned int u)' 'int a;signed int s;unsigned int u;'

testast '(f->int [] {(a);})' 'int f(){a();}'
testast '(f->int [] {(a 1 2 3 4 5 6);})' 'int f(){a(1,2,3,4,5,6);}'
testast '(f->int [] {(return 1);})' 'int f(){return 1;}'
testast '(f->void [] {(return);})' 'void f(){return;}'
testast '(f->void [] {})' 'void f(void){}'

testast '(f->void [] {(while 1 4);})' 'void f(){while(1)4;}'
testast '(f->void [] {(do-while 4 1);})' 'void f(){do 4;while(1);}'
testast '(f->void [] {(for 1 2 3 {4;});})' 'void f(){for(1;2;3)4;}'
testast '(f->void [] {(for () () () {1;});})' 'void f(){for(;;)1;}'
testast '(f->void [] {(for () () () {(continue);(break);});})' 'void f(){for(;;){continue;break;}}'

testast '(f->void [] {(cast (long) 1);})' 'void f(){(long)1;}'
testast '(f->void [] {(cast (char**) 1);})' 'void f(){(char**)1;}'

test 0 'int mymain(){return 0;}'
test 0 'int mymain(){return 0u;}'
test 0 'int mymain(){return 0l;}'
test 0 'int mymain(){return 0ul;}'
test 0 'int mymain(){return 0ll;}'
test 0 'int mymain(){return 0ull;}'
test 16 'int mymain(){return 16;}'
test 22 'int mymain(){return 0x16;}'
test 23 'int mymain(){return 0X17;}'
test 15 'int mymain(){return 017;}'

test 3 'int mymain(){return 1+2;}'
test 3 'int mymain(){return 1 + 2;}'
test 3 'int mymain(){return 1- -2;}'
test 10 'int mymain(){return 1+2+3+4;}'
test 11 'int mymain(){return 1+2*3+4;}'
test 14 'int mymain(){return 1*2+3*4;}'
test 4 'int mymain(){return 4/2+6/3;}'
test 3 'int mymain(){return 24/2/4;}'
test 13 'int mymain(){return (1+2)*3+4;}'
test 1 'int mymain(){return (1+2+3+4)%3;}'
test 64 'int mymain(){return 127^63;}'
test 4 'int mymain(){return 1<<2;}'
test 31 'int mymain(){return 127>>2;}'
test 51 'int mymain(){return (1+2)?51:52;}'
test 52 'int mymain(){return (1-1)?51:52;}'
test 3 'int mymain(){int a=1;return a+2;}'
test 102 'int mymain(){int a=1,b=48+2;int c=a+b;return c*2;}'
test 11 'int mymain(){int a=1;a+=10;return a;}'
test -9 'int mymain(){int a=1;a-=10;return a;}'
test 10 'int mymain(){int a=1;a*=10;return a;}'
test 2 'int mymain(){int a=11;a/=5;return a;}'
test 2 'int mymain(){int a=11;a%=3;return a;}'
test 64 'int mymain(){int a=127;a^=63;return a;}'
test '257 257' 'int mymain(){int a=256;printf("%d ",++a);return a;}'
test '255 255' 'int mymain(){int a=256;printf("%d ",--a);return a;}'
test '256 257' 'int mymain(){int a=256;printf("%d ",a++);return a;}'
test '256 255' 'int mymain(){int a=256;printf("%d ",a--);return a;}'
test 55 'int mymain(){int a[1]={55};int *b=a;return *b;}'
test 67 'int mymain(){int a[2]={55,67};int *b=a+1;return *b;}'
test 30 'int mymain(){int a[3]={20,30,40};int *b=a+1;return *b;}'
test 60 'int mymain(){int a[3];*a=10;*(a+1)=20;*(a+2)=30;return *a+*(a+1)+*(a+2);}'
test 33 'int mymain(){int a=11;{int a=100;{int a=200;a=222;}a=111;}return a*3;}'
test '10 -10 -11 0 0' 'int mymain(){int a=10;printf("%d %d %d %d ",+a,-a,~a,!a);return 0;}'
test '0 1 0 0' 'int mymain(){printf("%d %d %d ",1==2,1==1,2==1);return 0;}'
test '1 0 1 0' 'int mymain(){printf("%d %d %d ",1!=2,1!=1,2!=1);return 0;}'
test '1 0 0 0' 'int mymain(){printf("%d %d %d ",1<2,1<1,2<1);return 0;}'
test '1 1 0 0' 'int mymain(){printf("%d %d %d ",1<=2,1<=1,2<=1);return 0;}'
test '1 0 0 0' 'int mymain(){printf("%d %d %d ",-1<1,-1< -1,1< -1);return 0;}'
test '1 1 0 0' 'int mymain(){printf("%d %d %d ",-1<=1,-1<=1,1<= -1);return 0;}'
test '0 0 1 0' 'int mymain(){printf("%d %d %d ",1>2,1>1,2>1);return 0;}'
test '0 1 1 0' 'int mymain(){printf("%d %d %d ",1>=2,1>=1,2>=1);return 0;}'
test '0 0 1 0' 'int mymain(){printf("%d %d %d ",-1>1,-1>-1,1>-1);return 0;}'
test '0 1 1 0' 'int mymain(){printf("%d %d %d ",-1>=1,-1>=-1,1>=-1);return 0;}'
test '0 1 0 0' 'int mymain(){int s1=1,s2=2;unsigned int u1=1,u2=2;printf("%d %d %d ",s1==u2,s1==u1,s2==u1);return 0;}'
test '1 0 1 0' 'int mymain(){int s1=1,s2=2;unsigned int u1=1,u2=2;printf("%d %d %d ",s1!=u2,s1!=u1,s2!=u1);return 0;}'
test '1 0 0 0' 'int mymain(){int s1=1,s2=2;unsigned int u1=1,u2=2;printf("%d %d %d ",s1<u2,s1<u1,s2<u1);return 0;}'
test '1 1 0 0' 'int mymain(){int s1=1,s2=2;unsigned int u1=1,u2=2;printf("%d %d %d ",s1<=u2,s1<=u1,s2<=u1);return 0;}'
test '1 0 0 0' 'int mymain(){int s1=1,s2=2;unsigned int u1=1,u2=2;printf("%d %d %d ",-1<1,-1<-1,1<-1);return 0;}'
test '0 0 0 1 0' 'int mymain(){int s1=1,sm1=-1;unsigned int u1=1,um1=-1;printf("%d %d %d %d ",sm1<u1,s1<u1,sm1<um1,s1<um1);return 0;}'
test '1 1 0 0' 'int mymain(){int s1=1,s2=2;unsigned int u1=1,u2=2;printf("%d %d %d ",-1<=1,-1<=-1,1<=-1);return 0;}'
test '0 1 1 1 0' 'int mymain(){int s1=1,sm1=-1;unsigned int u1=1,um1=-1;printf("%d %d %d %d ",sm1<=u1,s1<=u1,sm1<=um1,s1<=um1);return 0;}'
test '0 0 1 0' 'int mymain(){int s1=1,s2=2;unsigned int u1=1,u2=2;printf("%d %d %d ",s1>u2,s1>u1,s2>u1);return 0;}'
test '0 1 1 0' 'int mymain(){int s1=1,s2=2;unsigned int u1=1,u2=2;printf("%d %d %d ",s1>=u2,s1>=u1,s2>=u1);return 0;}'
test '0 0 1 0' 'int mymain(){int s1=1,s2=2;unsigned int u1=1,u2=2;printf("%d %d %d ",-1>1,-1>-1,1>-1);return 0;}'
test '1 0 0 0 0' 'int mymain(){int s1=1,sm1=-1;unsigned int u1=1,um1=-1;printf("%d %d %d %d ",sm1>u1,s1>u1,sm1>um1,s1>um1);return 0;}'
test '0 1 1 0' 'int mymain(){int s1=1,s2=2;unsigned int u1=1,u2=2;printf("%d %d %d ",-1>=1,-1>=-1,1>=-1);return 0;}'
test '1 1 1 0 0' 'int mymain(){int s1=1,sm1=-1;unsigned int u1=1,um1=-1;printf("%d %d %d %d ",sm1>=u1,s1>=u1,sm1>=um1,s1>=um1);return 0;}'
test '0 0 0 1 0' 'int mymain(){printf("%d %d %d %d ",0&&0,0&&1,1&&0,1&&1);return 0;}'
test '0 1 1 1 0' 'int mymain(){printf("%d %d %d %d ",0||0,0||1,1||0,1||1);return 0;}'
test '1 0 0 1 0' 'int mymain() {char a=0x7f,b=0xff;printf("%d %d ",a>=0,b>0);a++;b++;printf("%d %d ",a>=0,b==0);return 0;}'
test '1 1 1 1 0' 'int mymain() {unsigned char a=0x7f,b=0xff;printf("%d %d ",a>=0,b>0);a++;b++;printf("%d %d ",a>=0,b==0);return 0;}'
test '1 0 0 1 0' 'int mymain() {short a=0x7fff,b=0xffff;printf("%d %d ",a>=0,b>0);a++;b++;printf("%d %d ",a>=0,b==0);return 0;}'
test '1 1 1 1 0' 'int mymain() {unsigned short a=0x7fff,b=0xffff;printf("%d %d ",a>=0,b>0);a++;b++;printf("%d %d ",a>=0,b==0);return 0;}'
test '1 0 0 1 0' 'int mymain() {int a=0x7fffffff,b=0xffffffff;printf("%d %d ",a>=0,b>0);a++;b++;printf("%d %d ",a>=0,b==0);return 0;}'
test '1 1 1 1 0' 'int mymain() {unsigned int a=0x7fffffff,b=0xffffffff;printf("%d %d ",a>=0,b>0);a++;b++;printf("%d %d ",a>=0,b==0);return 0;}'
test '1 0 0 1 0' 'int mymain() {long a=0x7fffffffffffffffL,b=0xffffffffffffffffL;printf("%d %d ",a>=0,b>0);a++;b++;printf("%d %d ",a>=0,b==0);return 0;}'
test '1 1 1 1 0' 'int mymain() {unsigned long a=0x7fffffffffffffffUL,b=0xffffffffffffffffUL;printf("%d %d ",a>=0,b>0);a++;b++;printf("%d %d ",a>=0,b==0);return 0;}'
test '1 0 0 1 0' 'int mymain() {long long a=0x7fffffffffffffffLL,b=0xffffffffffffffffLL;printf("%d %d ",a>=0,b>0);a++;b++;printf("%d %d ",a>=0,b==0);return 0;}'
test '1 1 1 1 0' 'int mymain() {unsigned long long a=0x7fffffffffffffffULL,b=0xffffffffffffffffULL;printf("%d %d ",a>=0,b>0);a++;b++;printf("%d %d ",a>=0,b==0);return 0;}'
test '1.2 0' 'int mymain() {float a=1.25f;printf("%.1f ",(double)a);return 0;}'
test '1.2 0' 'int mymain() {double a=1.25;printf("%.1f ",a);return 0;}'
test '1.2 0' 'int mymain() {long double a=1.25;printf("%.1f ",a);return 0;}'

test 61 'int mymain(){int a=61;int *b=&a;return *b;}'
test 62 'int a=62;int mymain(){int *b=&a;return *b;}'
test 97 'int mymain(){char *c="ab";return *c;}'
test 98 'int mymain(){char *c="ab"+1;return *c;}'
test 99 'int mymain(){char s[4]="abc";char *c=s+2;return *c;}'
test 15 'int mymain(){int a[5]={1,2,3,4,5};return a[0]+a[1]+a[2]+a[3]+a[4];}'
test 15 'int mymain(){int a[2][3]={{0,1,2},{3,4,5}};return a[0][0]+a[0][1]+a[0][2]+a[1][0]+a[1][1]+a[1][2];}'
test '1 2 0' 'int mymain(){int a[2][3];a[0][1]=1;a[1][1]=2;int *p=a;printf("%d %d ",p[1],p[4]);return 0;}'

test a3 'int mymain(){printf("a");return 3;}'
test abc5 'int mymain(){printf("%s", "abc");return 5;}'
test 70 'int mymain(){int a,b,c,d;a=1;b=2;c=3;d=a+b*c;printf("%d", d);return 0;}'
test 110 'int mymain(){int a,b;a=b=1;printf("%d%d", a, b);return 0;}'
test 111 'int mymain(){int a=0;if(1){a=111;}else{a=222;}return a;}'
test 999 'int mymain(){int a=999;if(0){a=111;}else if(0){a=222;}return a;}'
test 998 'int mymain(){return 998;return 999;}'

test '1 2 3 4 5 6 7 8 9 10 0' 'int mymain(){printf("%d %d %d %d %d %d %d %d %d %d ",1,2,3,4,5,6,7,8,9,10);return 0;}'

test 21 'int a=21;int mymain(){return a;}'
test 22 'int a;int mymain(){a=22;return a;}'
test 23 'int a[3];int mymain(){a[1]=23;return a[1];}'
test 25 'int a[3]={24,25,26};int mymain(){return a[1];}'

test 100 'int mymain(){int i=0;while(i<100)i++;return i;}'
test 100 'int mymain(){int i=0;do{i++;}while(i<100);return i;}'
test 100 'int mymain(){int i;for(i=0;i<100;i++){i++;}return i;}'
test 1 'int mymain(){int i=0;while(i<100){i++;break;}return i;}'
test '1 3 5 7 9 0' 'int mymain(){int i;for(i=0;i<10;i++){if(i%2==0){continue;}printf("%d ",i);}return 0;}'

test 1 'int mymain(){return 1==(int)1.1;}'
test 1 'int mymain(){return 1.0==(double)1;}'
test 1 'int mymain(){return 1.0f==(float)1;}'
test 1 'int mymain(){return 1.0f==(float)1.0;}'
test 1 'int mymain(){return 1.0==(double)1.0f;}'

test 61 'int mymain(){struct {int a;} x;x.a=61;return x.a;}'
test 63 'int mymain(){struct {int a;int b;} x;x.a=61;x.b=2;return x.a+x.b;}'
test 67 'int mymain(){struct {int a;struct {char b;int c;} y;} x;x.a=61;x.y.b=3;x.y.c=3;return x.a+x.y.b+x.y.c;}'
test 67 'int mymain(){struct tag {int a;struct {char b;int c;} y;} x;struct tag s;s.a=61;s.y.b=3;s.y.c=3;return s.a+s.y.b+s.y.c;}'
test 68 'int mymain(){struct tag {int a;} x;struct tag *p=&x;x.a=68;return (*p).a;}'
test 69 'int mymain(){struct tag {int a;} x;struct tag *p=&x;(*p).a=69;return x.a;}'
test 71 'int mymain(){struct tag {int a;int b;} x;struct tag *p=&x;x.b=71;return (*p).b;}'
test 72 'int mymain(){struct tag {int a;int b;} x;struct tag *p=&x;(*p).b=72;return x.b;}'
test 67 'struct {int a;struct {char b;int c;} y;} x;int mymain(){x.a=61;x.y.b=3;x.y.c=3;x.a+x.y.b+x.y.c;}'
test 78 'struct tag {int a;} x;int mymain(){struct tag *p=&x;x.a=78;(*p).a;}'
test 79 'struct tag {int a;} x;int mymain(){struct tag *p=&x;(*p).a=79;x.a;}'
test 78 'struct tag {int a;} x;int mymain(){struct tag *p=&x;x.a=78;return p->a;}'
test 79 'struct tag {int a;} x;int mymain(){struct tag *p=&x;p->a=79;return x.a;}'
test 81 'struct tag {int a;int b;} x;int mymain(){struct tag *p=&x;x.b=81;(*p).b;}'
test 82 'struct tag {int a;int b;} x;int mymain(){struct tag *p=&x;(*p).b=82;x.b;}'
test 66 'int mymain(){struct {int a;struct{int b;struct{int c;};};} x;x.a=11;x.b=22;x.c=33;return x.a+x.b+x.c;}'

test 90 'int mymain(){union {int a;int b;} x;x.a=90;return x.b;}'
test 256 'int mymain(){union {char a[4];int b;} x;x.b=0;x.a[1]=1;return x.b;}';
test 256 'int mymain(){union {char a[4];int b;} x;x.a[0]=x.a[1]=x.a[2]=x.a[3]=0;x.a[1]=1;return x.b;}';
test 256 'union {char a[4];int b;} x;int mymain(){x.b=0;x.a[1]=1;return x.b;}';

testfail 'int f(){0abc;}'
testfail 'int f(){0xg;}'
testfail 'int f(){08;}'
testfail "int f(){'c;}"
testfail "int f(){'cc';}"
testfail "int f(){'';}"
testfail 'int f(){1+;}'
testfail 'int f(){(1;}'
testfail 'int f(){1}'
testfail 'int f(){a(1;}'
testfail 'int f(){1;2}'
testfail 'int f(){a=;}'
testfail 'int f(){3=1;}'
testfail 'int f(){a=1;}'
testfail 'int f(){int a,a;}'
testfail 'int f(){{int a;}a;}'
testfail 'int f(){{{int a;}a;}}'

testfail 'int f(){&"a";}'
testfail 'int f(){&1;}'
testfail 'int f(){&a();}'
testfail 'int f(){int *a;a*1;}'
testfail 'int f(){int a=10;int b[a];}'
testfail 'int f(){int a[3];a[];}'
testfail 'int f(){return;}'
testfail 'void f(){return 0;}'

testfail 'int i,i;'
testfail 'void f(){for(int a;;){1;}a;}'
testfail 'void f(){continue;}'
testfail 'void f(){break;}'

testfail 'void f(){(int[])1;}'
testfail 'void f(){(int[1])1;}'

testfail 'void f(){1.0|1;}'
testfail 'void f(){1.0|1.0;}'
testfail 'void f(){1.0|1.0f;}'
testfail 'void f(){1.0f|1;}'
testfail 'void f(){1.0f|1.0;}'
testfail 'void f(){1.0f|1.0f;}'
testfail 'void f(){1|1.0;}'
testfail 'void f(){1|1.0f;}'
testfail 'void f(){1.0^1;}'
testfail 'void f(){1.0^1.0;}'
testfail 'void f(){1.0^1.0f;}'
testfail 'void f(){1.0f^1;}'
testfail 'void f(){1.0f^1.0;}'
testfail 'void f(){1.0f^1.0f;}'
testfail 'void f(){1^1.0;}'
testfail 'void f(){1^1.0f;}'
testfail 'void f(){1.0&1;}'
testfail 'void f(){1.0&1.0;}'
testfail 'void f(){1.0&1.0f;}'
testfail 'void f(){1.0f&1;}'
testfail 'void f(){1.0f&1.0;}'
testfail 'void f(){1.0f&1.0f;}'
testfail 'void f(){1&1.0;}'
testfail 'void f(){1&1.0f;}'
testfail 'void f(){1.0<<1;}'
testfail 'void f(){1.0<<1.0;}'
testfail 'void f(){1.0<<1.0f;}'
testfail 'void f(){1.0f<<1;}'
testfail 'void f(){1.0f<<1.0;}'
testfail 'void f(){1.0f<<1.0f;}'
testfail 'void f(){1<<1.0;}'
testfail 'void f(){1<<1.0f;}'

echo "All tests passed"
