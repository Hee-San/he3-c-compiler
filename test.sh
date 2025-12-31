#!/bin/bash

# 一旦、関数定義はgccでコンパイル
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }

int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }

int add8(int a, int b, int c, int d, int e, int f, int g, int h) {
  return a+b+c+d+e+f+g+h;
}
EOF

assert() {
  expected="$1"
  input="$2"

  ./he3cc "$input" > tmp.s
  cc -o tmp tmp.s tmp2.o
  ./tmp
  actual="$?"

  if [ "$actual" = "$expected" ]; then
    echo "$input => $actual"
  else
    echo "$input => $expected expected, but got $actual"
    exit 1
  fi
}

assert 0 'return 0;'
assert 42 'return 42;'
assert 21 'return 5+20-4;'
assert 41 'return  12 + 34 - 5 ;'
assert 47 'return 5+6*7;'
assert 15 'return 5*(9-6);'
assert 4 'return (3+5)/2;'
assert 105 'return (1+(2+3)*4)*5;'
assert 10 'return -3*5+25;'

assert 0 'return 0==1;'
assert 1 'return 42==42;'
assert 1 'return 0!=1;'
assert 0 'return 42!=42;'

assert 1 'return 0<1;'
assert 0 'return 1<1;'
assert 0 'return 2<1;'
assert 1 'return 0<=1;'
assert 1 'return 1<=1;'
assert 0 'return 2<=1;'

assert 1 'return 1>0;'
assert 0 'return 1>1;'
assert 0 'return 1>2;'
assert 1 'return 1>=0;'
assert 1 'return 1>=1;'
assert 0 'return 1>=2;'

assert 0 '1; 2; 3;'
assert 1 'return 1; 2; 3;'
assert 2 '1; return 2; 3;'
assert 3 '1; 2; return 3;'

assert 3 'a=3; return a;'
assert 8 'a=3; z=5; return a+z;'
assert 8 'foo_123=3; bar=5; return foo_123+bar;'

assert 3 'if (0) return 2; return 3;'
assert 3 'if (1-1) return 2; return 3;'
assert 2 'if (1) return 2; return 3;'
assert 2 'if (2-1) return 2; return 3;'
assert 5 'if (0) a=3; else a=5; return a;'
assert 3 'if (1) a=3; else a=5; return a;'
assert 7 'if (0) return 3; else if (0) return 5; else return 7;'

assert 3 '{1; {2;} return 3;}'

assert 10 'i=0; while(i<10) i=i+1; return i;'
assert 55 'i=0; j=0; while(i<=10) {j=i+j; i=i+1;} return j;'

assert 55 'i=0; j=0; for (i=0; i<=10; i=i+1) j=i+j; return j;'
assert 3 'for (;;) return 3; return 5;'

assert 3 'return ret3();'
assert 5 'return ret5();'
assert 8 'return add(3, 5);'
assert 2 'return sub(5, 3);'
assert 36 'return add8(1,2,3,4,5,6,7,8);'

echo OK
