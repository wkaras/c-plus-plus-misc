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

#pragma once

#include <set>

// Number of instatiations of S where f() is not virtual.
//
unsigned const Num_S{11};

unsigned const Poly_idx{Num_S};

unsigned const Num_set_elems{256};

template <unsigned N>
struct S
  {
    // f() is defined in other.cpp to prevent it from being inlined in main.cpp
    //
    void const * f() const;
  };

template <>
struct S<Poly_idx>
  {
    virtual void const *f() const = 0;
  };

template <class ST>
struct S_less
  {
    bool operator () (ST const *s1, ST const *s2) const
      {
        return s1->f() < s2->f();
      }
  };

template <class ST>
using T_set = std::set<ST const *, S_less<ST> >;

// set_insert/erase() simply call the respective member functions of the set.  These functions are defined
// in other.cpp so they will not be inlined in main.cpp.

template <class ST>
extern void set_insert(T_set<ST> &t_set, ST const *s);

template <class ST>
extern void set_erase(T_set<ST> &t_set, ST const *s);

template <unsigned N>
struct Get
  {
    static S<N> const * s(unsigned i);
  };

#if 1
// Use recursive templates for repetition.

template <unsigned Base, unsigned Count, template <unsigned N> class Once>
struct Rep_
  {
    static void x()
      {
        Rep_<Base, Count / 2, Once>::x();
        Rep_<Base + (Count / 2), Count / 2, Once>::x();
        Rep_<Base + 2 * (Count / 2), Count % 2, Once>::x();
      }
  };

template <unsigned Base, template <unsigned N> class Once>
struct Rep_<Base, 1, Once>
  {
    static void x()
      {
        Once<Base>::x();
      }
  };

template <unsigned Base, template <unsigned N> class Once>
struct Rep_<Base, 0, Once>
  {
    static void x()
      {
      }
  };

// Calling Rep<Count, Once>::x() calls:
//
// Once<0>::x();
// Once<1>::x();
//   .
//   .
//   .
// Once<Count - 1>::x();
//
template <unsigned Count, template <unsigned N> class Once>
using Rep = Rep_<0, Count, Once>;

#else
// Use preprocessor for repetition.

#undef REP0
#define REP0(I, COUNT) if constexpr ((I) < COUNT) Once<I>::x();

#undef REP1
#define REP1(I, COUNT) REP0(I, COUNT) REP0(I + 1, COUNT)
#undef REP2
#define REP2(I, COUNT) REP1(I, COUNT) REP1(I + (1 << 1), COUNT)
#undef REP3
#define REP3(I, COUNT) REP2(I, COUNT) REP2(I + (1 << 2), COUNT)
#undef REP4
#define REP4(I, COUNT) REP3(I, COUNT) REP3(I + (1 << 3), COUNT)
#undef REP5
#define REP5(I, COUNT) REP4(I, COUNT) REP4(I + (1 << 4), COUNT)
#undef REP6
#define REP6(I, COUNT) REP5(I, COUNT) REP5(I + (1 << 5), COUNT)
#undef REP7
#define REP7(I, COUNT) REP6(I, COUNT) REP6(I + (1 << 6), COUNT)
#undef REP8
#define REP8(I, COUNT) REP7(I, COUNT) REP7(I + (1 << 7), COUNT)
#undef REP9
#define REP9(I, COUNT) REP8(I, COUNT) REP8(I + (1 << 8), COUNT)
#undef REP10
#define REP10(I, COUNT) REP9(I, COUNT) REP9(I + (1 << 9), COUNT)

template <unsigned Count, template <unsigned N> class Once>
struct Rep
  {
    static void x() { REP10(0, Count) }
  };

#endif
