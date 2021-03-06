/*
Copyright (c) 2017 Walter William Karas

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

// Example (as tutorial) of use of (clunky) "Go-like" interfaces for C++.
//
// (If intefaces were part of the base language, invocations of IFACE_ENABLE
// and IFACE_CONVERSION would probably be replaced by code implicitly
// generated by the compiler.)

#include "iface.h"

// Identifiers for class instances in this example.
enum Instance
  {
    I_z_x_a_v,
    I_z_x_a_1,
    I_z_x_a_2,
    I_z_x_b_1,
    I_z_x_b_2,
    I_z_y_c_1,
    I_z_y_c_2
  };

// Identifiers for class member functions in this example.
enum Method
  {
    M_a_z_1,
    M_a_x_1,
    M_b_z_1,
    M_b_x_1,
    M_c_z_1,
    M_c_y_1,
    M_c_y_2,
  };

// Define Interface "Z".  (This would probably look like a structure with only
// member function prototypes, if interfaces were part of the base language.)

#define Z1_PARAMS(P) \
P(int *, p1), \
P(int, p2)

#define Z2_PARAMS(P) \
P(void *, p1)

#define Z_FUNC_LIST(F, FNR) \
F(Instance, who, const, IFACE_NO_PARAMS) \
F(Method, z1, , Z1_PARAMS) \
FNR(z2, , Z2_PARAMS)

IFACE_DEF(Z, , Z_FUNC_LIST)

// Exercise a Z interface.
//
bool check(Z iface, Instance who, Method z1_result)
  {
    if (IFACE_CALL_NP(iface, who) != who)
      return(false);

    if (IFACE_CALL(iface, z1, nullptr, 0) != z1_result)
      return(false);

    IFACE_CALL(iface, z2, nullptr);

    return(true);
  }

// Define Interface "Z_X", which has Z as a subset.  (If interfaces were
// part of the base language, inferface inheritance should probably be allowed.
// But interface conversion should not be limited to base interfaces.)

#define X1_PARAMS(P) \
P(int, p1), \
P(int, p2)

#define X2_PARAMS(P) \
P(double, p1)

#define Z_X_FUNC_LIST(F, FNR) \
Z_FUNC_LIST(F, FNR) \
F(Method, x1, , X1_PARAMS) \
FNR(x2, , X2_PARAMS)

IFACE_DEF(Z_X, , Z_X_FUNC_LIST)

// Since Z_X has Z as a subset, a Z_X instance can be converted to an X
// instance
//
IFACE_CONVERSION(Z, Z_FUNC_LIST, Z_X)

// Exercise a Z_X interface.
//
bool check(Z_X iface, Instance who, Method z1_result, Method x1_result)
  {
    if (!check(iface_convert<Z>(iface), who, z1_result))
      return(false);

    if (IFACE_CALL(iface, x1, 0, 0) != x1_result)
      return(false);

    IFACE_CALL(iface, x2, 0.0);

    return(true);
  }

// Define V interface, which is volatile (can only call volatile member
// functions of the interfaced-to class instance).

#define V_PARAMS(P) \
P(int *, p1)

#define V_FUNC_LIST(F, FNR) \
FNR(v, volatile, V_PARAMS)

IFACE_DEF(V, volatile, V_FUNC_LIST)

// Exercise a V interface.
//
void check(V iface) { IFACE_CALL(iface, v, nullptr); }

class Z_X_A
  {
  private:

    Instance who_;

  public:

    constexpr Z_X_A(Instance w) : who_(w) { }

    Instance who() const { return(who_); }

    Method z1(int *, int) { return(M_a_z_1); }

    void z2(void *) { }

    void x2(double) { }

    void v(int *) volatile { }

    Method x1(int, int) { return(M_a_x_1); }
  };

// Enable Z_X instances to interface to Z_X_A instances.
//
IFACE_ENABLE(Z_X, Z_X_FUNC_LIST, Z_X_A)

IFACE_ENABLE(V, V_FUNC_LIST, volatile Z_X_A)

class Z_X_B
  {
  private:

    Instance who_;

  public:

    constexpr Z_X_B(Instance w) : who_(w) { }

    Instance who() const { return(who_); }

    Method z1(int *, int) { return(M_b_z_1); }

    void z2(void *) { }

    void x2(double) { }

    void v(int *) volatile { }

    Method x1(int, int) { return(M_b_x_1); }

    void not_in_any_iface(int) { }
  };

IFACE_ENABLE(Z_X, Z_X_FUNC_LIST, Z_X_B)

// Define Z_Y interface.

#define Y1_PARAMS(P) \
P(unsigned, p1), \
P(float, p2)

#define Y2_PARAMS(P) \
P(unsigned, p1)

#define Z_Y_FUNC_LIST(F, FNR) \
Z_FUNC_LIST(F, FNR) \
F(Method, y1, , Y1_PARAMS) \
F(Method, y2, , Y2_PARAMS)

IFACE_DEF(Z_Y, , Z_Y_FUNC_LIST)

IFACE_CONVERSION(Z, Z_FUNC_LIST, Z_Y)

// Exercise a Z_Y interface.
//
bool check(
  Z_Y iface, Instance who, Method z1_result, Method y1_result, Method y2_result)
  {
    if (!check(iface_convert<Z>(iface), who, z1_result))
      return(false);

    if (IFACE_CALL(iface, y1, 0, 0.0) != y1_result)
      return(false);

    if (IFACE_CALL(iface, y2, 0) != y2_result)
      return(false);

    return(true);
  }

class Z_Y_C
  {
  private:

    Instance who_;

  public:

    constexpr Z_Y_C(Instance w) : who_(w) { }

    Instance who() const { return(who_); }

    Method z1(int *, int) { return(M_c_z_1); }

    void z2(void *) { }

    Method y2(unsigned) { return(M_c_y_2); }

    Method y1(unsigned, float) { return(M_c_y_1); }
  };

IFACE_ENABLE(Z_Y, Z_Y_FUNC_LIST, Z_Y_C)

#include <iostream>

int main()
  {
    volatile Z_X_A v(I_z_x_a_v);

    // Create an interface, and check it.
    //
    check(iface_factory<V>(v)); 

    Z_X_A z_x_a1(I_z_x_a_1);

    // This call should create a vstructure for Z interfaces to class
    // Z_X_A instances.
    //
    if (!check(iface_factory<Z_X>(z_x_a1), I_z_x_a_1, M_a_z_1, M_a_x_1))
      std::cout << "BAD\n";

    Z_X_A z_x_a2(I_z_x_a_2);

    // This call should use the previously created vstructure for Z
    // interfaces to class Z_X_A instances.
    //
    if (!check(iface_factory<Z_X>(z_x_a2), I_z_x_a_2, M_a_z_1, M_a_x_1))
      std::cout << "BAD\n";

    Z_X_B z_x_b1(I_z_x_b_1);

    // This call should create a vstructure for Z interfaces to class
    // Z_X_B instances.
    //
    if (!check(iface_factory<Z_X>(z_x_b1), I_z_x_b_1, M_b_z_1, M_b_x_1))
      std::cout << "BAD\n";

    Z_X_B z_x_b2(I_z_x_b_2);

    // This call should use the previously created vstructure for Z
    // interfaces to class Z_X_B instances.
    //
    if (!check(iface_factory<Z_X>(z_x_b2), I_z_x_b_2, M_b_z_1, M_b_x_1))
      std::cout << "BAD\n";

    Z_Y_C z_y_c1(I_z_y_c_1);

    // This call should create a vstructure for Z interfaces to class
    // Z_Y_C instances.
    //
    if (!check(iface_factory<Z_Y>(z_y_c1), I_z_y_c_1, M_c_z_1, M_c_y_1,
                                  M_c_y_2))
      std::cout << "BAD\n";

    Z_Y_C z_y_c2(I_z_y_c_2);

    // This call should use the previously created vstructure for Z
    // interfaces to class Z_Y_C instances.
    //
    if (!check(iface_factory<Z_Y>(z_y_c2), I_z_y_c_2, M_c_z_1, M_c_y_1,
                                  M_c_y_2))
      std::cout << "BAD\n";

    // Violate encapsulation to check if reuse of vstructures for conversions
    // is working as expected.
    //
    if (Iface_impl::convert_map().size() != 3)
      std::cout << "BAD\n";

    #define X(CLS) \
      std::cout << "sizeof(" << #CLS << ") = " << sizeof(CLS) << '\n';

    X(Z_X_A)
    X(Z_X_B)
    X(Z_Y_C)

    using An_interface_type = Z_Y;

    X(An_interface_type)

    using A_pointer = Z_Y_C *;

    X(A_pointer)

    return(0);
  }
