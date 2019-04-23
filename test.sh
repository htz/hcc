#!/bin/bash

function compile {
  echo "$1" | ./hcc > tmp.s 2>/dev/null
  if [ $? -ne 0 ]; then
    echo "Failed to compile $2"
    exit
  fi
  gcc -no-pie -o tmp.out driver.c tmp.s
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

testast '1' '1;'
testast '1, 2' '1,2;'
testast "99" "'c';"
testast '"abc"' '"abc";'
testast '"a\\b\"c"' '"a\\b\"c";'
testast '1;2' '1;2;'
testast '' ';'
testast '1' ';;1;;'

testast '(+ (- (+ 1 2) 3) 4)' '1+2-3+4;'
testast '(+ (+ 1 (* 2 3)) 4)' '1+2*3+4;'
testast '(+ (* 1 2) (* 3 4))' '1*2+3*4;'
testast '(+ (/ 4 2) (/ 6 3))' '4/2+6/3;'
testast '(/ (/ 24 2) 4)' '24/2/4;'
testast '(+ (* (+ 1 2) 3) 4)' '(1+2)*3+4;'
testast '(% (+ (+ (+ 1 2) 3) 4) 3)' '(1+2+3+4)%3;'
testast "(decl int a 3)" 'int a=3;'
testast "(decl char c 97)" "char c='a';"
testast "(decl char* s \"abc\")" 'char *s="abc";'
testast "(decl char[4] s \"abc\")" 'char s[4]="abc";'
testast "(decl int[3] a [1 2 3])" 'int a[3]={1,2,3};'
testast "(decl int[2][3] a [[0 1 2] [3 4 5]])" 'int a[2][3]={{0,1,2},{3,4,5}};'
testast "(decl int a 1);(decl int b 2);(= a (= b 3))" 'int a=1;int b=2;a=b=3;'
testast "(decl int a 3);(& a)" 'int a=3;&a;'
testast "(decl int a 3);(* (& a))" 'int a=3;*&a;'
testast "(decl int a 3);(decl int* b (& a));(* b)" 'int a=3;int *b=&a;*b;'

testast '(a)' 'a();'
testast '(a 1 2 3 4 5 6)' 'a(1,2,3,4,5,6);'

test 0 '0;'

test 3 '1+2;'
test 3 '1 + 2;'
test 10 '1+2+3+4;'
test 11 '1+2*3+4;'
test 14 '1*2+3*4;'
test 4 '4/2+6/3;'
test 3 '24/2/4;'
test 13 '(1+2)*3+4;'
test 1 '(1+2+3+4)%3;'
test 3 'int a=1;a+2;'
test 102 'int a=1,b=48+2;int c=a+b;c*2;'
test 55 'int a[1]={55};int *b=a;*b;'
test 67 'int a[2]={55,67};int *b=a+1;*b;'
test 30 'int a[3]={20,30,40};int *b=a+1;*b;'
test 60 'int a[3];*a=10;*(a+1)=20;*(a+2)=30;*a+*(a+1)+*(a+2);'

test 61 'int a=61;int *b=&a;*b;'
test 97 'char *c="ab";*c;'
test 98 'char *c="ab"+1;*c;'
test 99 'char s[4]="abc";char *c=s+2;*c;'
test 15 'int a[5]={1,2,3,4,5};a[0]+a[1]+a[2]+a[3]+a[4];'
test 15 'int a[2][3]={{0,1,2},{3,4,5}};a[0][0]+a[0][1]+a[0][2]+a[1][0]+a[1][1]+a[1][2];'
test '1 2 0' 'int a[2][3];a[0][1]=1;a[1][1]=2;int *p=a;printf("%d %d ",p[1],p[4]);0;'

test 25 'sum2(20, 5);'
test 55 'sum10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);'
test 55 'sum2(1, sum2(2, sum2(3, sum2(4, sum2(5, sum2(6, sum2(7, sum2(8, sum2(9, 10)))))))));'
test a3 'printf("a");3;'
test abc5 'printf("%s", "abc");5;'
test 70 'int a,b,c,d;a=1;b=2;c=3;d=a+b*c;printf("%d", d);0;'
test 110 'int a,b;a=b=1;printf("%d%d", a, b);0;'

testfail '0abc;'
testfail "'c;"
testfail "'cc';"
testfail "'';"
testfail '1+;'
testfail '(1;'
testfail '1'
testfail 'a(1;'
testfail '1;2'
testfail 'a=;'
testfail '3=1;'
testfail 'a=1;'
testfail 'int a,a;'

testfail '&"a";'
testfail '&1;'
testfail '&a();'
testfail 'int *a;a*1;'
testfail 'int a=10;int b[a];'
testfail 'int a[3];a[];'

echo "All tests passed"
