#!/bin/bash

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

function testfail {
  expr="$1"
  echo "$expr" | ./hcc > /dev/null 2>&1
  if [ $? -eq 0 ]; then
    echo "Should fail to compile, but succeded: $expr"
    exit
  fi
}

make -s hcc

testast '(f->int [] {1, 2;})' 'int f(){1,2;}'
testast '(f->int [] {1;2;})' 'int f(){1;2;}'
testast '(f->int [] {;})' 'int f(){;}'
testast '(f->int [] {;;1;;})' 'int f(){;;1;;}'

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
