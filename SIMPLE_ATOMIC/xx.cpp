#include "simple_atomic.h"

using namespace Simple_atomic;

T<int> i(no_threads), j(no_threads, 5);

int f()
  {
    T<int> ii, jj(5);

    int n = 5;

    for ( ; ; )
      if (jj.compare_exchange(n, 13)) break;

    j = 10;

    release();

    ii = 20;

    int m = jj;

    acquire();

    return(m + i + j());
  }
