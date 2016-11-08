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

// For comparision, this code is roughly equilant to example.cpp, but
// it uses current, inheritance-based polymorphism.

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

// Define Interface "Z".
//
struct Z
  {
    virtual Instance who() const = 0;

    virtual Method z1(int *, int) = 0;

    virtual void z2(void *) = 0;
  };

// Exercise a Z interface.
//
bool check(Z &iface, Instance who, Method z1_result)
  {
    if (iface.who() != who)
      return(false);

    if (iface.z1(nullptr, 0) != z1_result)
      return(false);

    iface.z2(nullptr);

    return(true);
  }

// Define Interface "Z_X", which has Z as a subset.
//
struct Z_X : public Z
  {
    virtual Method x1(int, int) = 0;

    virtual void x2(double) = 0;
  };

// Exercise a Z_X interface.
//
bool check(Z_X &iface, Instance who, Method z1_result, Method x1_result)
  {
    if (!check(iface, who, z1_result))
      return(false);

    if (iface.x1(0, 0) != x1_result)
      return(false);

    iface.x2(0.0);

    return(true);
  }

// Define V interface.
//
struct V
  {
    virtual void v(int *) volatile = 0;
  };

// Exercise a V interface.
//
void check(volatile V &iface) { iface.v(nullptr); }

class Z_X_A : public Z_X, public V
  {
  private:

    Instance who_;

  public:

    constexpr Z_X_A(Instance w) : who_(w) { }

    Instance who() const override { return(who_); }

    Method z1(int *, int) override { return(M_a_z_1); }

    void z2(void *) override { }

    void x2(double) override { }

    void v(int *) volatile override { }

    Method x1(int, int) override { return(M_a_x_1); }
  };

class Z_X_B : public Z_X, public V
  {
  private:

    Instance who_;

  public:

    constexpr Z_X_B(Instance w) : who_(w) { }

    Instance who() const override { return(who_); }

    Method z1(int *, int) override { return(M_b_z_1); }

    void z2(void *) override { }

    void x2(double) override { }

    void v(int *) volatile override { }

    Method x1(int, int) override { return(M_b_x_1); }

    void not_in_any_iface(int) { }
  };

// Define Z_Y interface.
//
struct Z_Y : public Z 
  {
    virtual Method y1(unsigned, float) = 0;

    virtual Method y2(unsigned) = 0;
  };

// Exercise a Z_Y interface.
//
bool check(
  Z_Y &iface, Instance who, Method z1_result, Method y1_result,
  Method y2_result)
  {
    if (!check(iface, who, z1_result))
      return(false);

    if (iface.y1(0, 0.0) != y1_result)
      return(false);

    if (iface.y2(0) != y2_result)
      return(false);

    return(true);
  }

class Z_Y_C : public Z_Y
  {
  private:

    Instance who_;

  public:

    constexpr Z_Y_C(Instance w) : who_(w) { }

    Instance who() const override { return(who_); }

    Method z1(int *, int) override { return(M_c_z_1); }

    void z2(void *) override { }

    Method y2(unsigned) override { return(M_c_y_2); }

    Method y1(unsigned, float) override { return(M_c_y_1); }
  };

#include <iostream>

int main()
  {
    volatile Z_X_A v(I_z_x_a_v);

    check(v); 

    Z_X_A z_x_a1(I_z_x_a_1);

    if (!check(z_x_a1, I_z_x_a_1, M_a_z_1, M_a_x_1))
      std::cout << "BAD\n";

    Z_X_A z_x_a2(I_z_x_a_2);

    if (!check(z_x_a2, I_z_x_a_2, M_a_z_1, M_a_x_1))
      std::cout << "BAD\n";

    Z_X_B z_x_b1(I_z_x_b_1);

    if (!check(z_x_b1, I_z_x_b_1, M_b_z_1, M_b_x_1))
      std::cout << "BAD\n";

    Z_X_B z_x_b2(I_z_x_b_2);

    if (!check(z_x_b2, I_z_x_b_2, M_b_z_1, M_b_x_1))
      std::cout << "BAD\n";

    Z_Y_C z_y_c1(I_z_y_c_1);

    if (!check(z_y_c1, I_z_y_c_1, M_c_z_1, M_c_y_1, M_c_y_2))
      std::cout << "BAD\n";

    Z_Y_C z_y_c2(I_z_y_c_2);

    if (!check(z_y_c2, I_z_y_c_2, M_c_z_1, M_c_y_1, M_c_y_2))
      std::cout << "BAD\n";

    #define X(CLS) \
      std::cout << "sizeof(" << #CLS << ") = " << sizeof(CLS) << '\n';

    X(Z_X_A)
    X(Z_X_B)
    X(Z_Y_C)

    return(0);
  }
