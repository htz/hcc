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
test 61 'int a=61;int *b=&a;*b;'
test 97 'char *c="ab";*c;'
test 98 'char *c="ab"+1;*c;'

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

echo "All tests passed"
