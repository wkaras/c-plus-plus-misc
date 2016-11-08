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

#include <chrono>
#include <iostream>
#include <iomanip>

namespace
{

// Insert Num_set_elems elements into the set, and then erase them in a different order.  s() is expected
// to return the same element for successive calls with the same i.
//
template <class SetT, class GetElemT>
void fill_empty_set(SetT &set)
  {
    unsigned i{0};
    do
      {
        set_insert(set, GetElemT::s(i));
        i += 11;
        if (i >= Num_set_elems)
          i -= Num_set_elems;
      }
    while (i);

    i = 0;
    do
      {
        set_erase(set, GetElemT::s(i));
        i += 13;
        if (i >= Num_set_elems)
          i -= Num_set_elems;
      }
    while (i);
  }

template <unsigned N>
struct Fill_empty_set
  {
    static void x()
      {
        static T_set<S<N> > set;

        fill_empty_set<T_set<S<N> >, Get<N> >(set);
      }
  };

// For polymorphic fill/empty, use Num_S different sets, even though they all have the same type.  So that each
// set has the same overhead (as generic programming fill/empty) of a static initialization guard flag, and the
// polymorphic fill/empty can't cache a single set for Num_S runs.
//
template <unsigned>
struct Fill_empty_set_poly
  {
    static void x()
      {
        static T_set<S<Poly_idx> > set;

        fill_empty_set<T_set<S<Poly_idx> >, Get<Poly_idx> >(set);
      }
  };

// Return how long it takes to repeatedly execute a particular function.
//
auto time_func(void (*func)())
  {
    unsigned const Reps_of_func{100000 / Num_S};
    auto start{std::chrono::steady_clock::now()};
    for (unsigned i{0}; i < Reps_of_func; ++i)
      func();
    auto stop{std::chrono::steady_clock::now()};

    return std::chrono::duration<double>(stop - start);
  }

} // end anonymous namespace

int main()
  {
    auto gp{time_func(Rep<Num_S, Fill_empty_set>::x)};

    auto poly{time_func(Rep<Num_S, Fill_empty_set_poly>::x)};

    std::cout << "Generic Programming = " << gp << '\n';
    std::cout << "Polymorphism        = " << poly << '\n';
    std::cout << "Poly / GP           = " << (poly.count() / gp.count()) << '\n';

    return 0;
  }
