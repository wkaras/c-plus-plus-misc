/*
Copyright (c) 2025 Walter William Karas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "common.h"

namespace
{

struct SD : public S<Poly_idx>
  {
    S<0> s;

    void const * f() const override
      {
        return this;
      }
  };

SD const sd[Num_set_elems];

void * volatile touch_dummy;

// This is only used to suppress unused variable warnings.
//
void touch(void *p)
  {
    touch_dummy = p;
  }

template <unsigned N>
struct Once
  {
    static void x()
      {
        // Cause these three functions to be instantiated in this compilation unit.
        //
        static auto p1{Get<N>::s};
        static auto p2{set_insert<S<N> >};
        static auto p3{set_erase<S<N> >};

        // Suppress unused variable warnings.
        //
        touch(&p1);
        touch(&p2);
        touch(&p3);
      }
  };

// The plus one also instantiates functions for S<Poly_idx>.
//
Rep<Num_S + 1, Once> dummy;

} // end anonymous namespace

template <unsigned N>
void const * S<N>::f() const
  {
    return this;
  }

template <unsigned N>
S<N> const * Get<N>::s(unsigned i)
  {
    if constexpr (N == Poly_idx)
      return sd + i;
    else
      return reinterpret_cast<S<N> const *>(&(sd[i].s));
  }

template <class ST>
void set_insert(T_set<ST> &t_set, ST const *s)
  {
    t_set.insert(s);
  }

template <class ST>
void set_erase(T_set<ST> &t_set, ST const *s)
  {
    t_set.erase(s);
  }

// This is never called, it only exists to cause the instantiation of template entity functions.
//
void dummy_not_called()
  {
    dummy.x();
  }
