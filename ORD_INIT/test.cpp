#include <iostream>

#include "ord_init.h"

template <char C>
struct X
  {
    X() { std::cout << "construct " << C << std::endl; }
    ~X() { std::cout << "destroy " << C << std::endl; }
  };

template <char C>
class Dep
  {
  private:
    void cons();

  public:
    Dep() { cons(); }
  };

template <char C>
struct Y : private Dep<C>, public X<C> { };

template <char C>
struct Z : public Ord_init< Y<C> > { };

extern Z<'a'> a;
extern Z<'b'> b;
extern Z<'c'> c;
extern Z<'d'> d;
extern Z<'e'> e;
extern Z<'f'> f;
extern Z<'g'> g;

#if CU == 1

// I don't understand why the destructor for this is not called in
// the first failure case in 'cmd'.
//
Z<'_'> underscore;

template <>
void Dep<'_'>::cons() { }

Z<'a'> a;

template <>
void Dep<'a'>::cons() { e.init(); c.init(); }

#elif CU == 2

Z<'b'> b;

template <>
void Dep<'b'>::cons() { g.init(); }

Z<'c'> c;

template <>
void Dep<'c'>::cons() { b.init(); }

Z<'d'> d;

template <>
void Dep<'d'>::cons() { f.init(); }

Z<'e'> e;

template <>
void Dep<'e'>::cons() { d.init(); }

#elif CU == 3

Z<'f'> f;

#ifdef FAIL

template <>
void Dep<'f'>::cons() { a.init(); }

#else

template <>
void Dep<'f'>::cons() { }

#endif

Z<'g'> g;

template <>
void Dep<'g'>::cons() { f.init(); }

int main() { return(0); }

#endif
