#include <ostream>
#include <concepts>
#include <limits>
#include <utility>
#include <stdexcept>

// Wrapper for an ostream for formatted output.
//
class ostream_fmt
  {
  public:

    // If newln_ is true, a new line character is output when the instance is destroyed.
    //
    explicit ostream_fmt(std::ostream &os_, bool newln_ = false) : os{os_}, newln{newln_} {}

    // Return an "outputter" rvalue, which will do output.
    //
    template <typename type, typename ...type_pack>
    auto operator () (type const &value, type_pack && ...arg_pack);

    ~ostream_fmt()
      {
        if (newln)
          os << '\n';
      }

  private:

    std::ostream &os;
    bool const newln;
  };

namespace ostream_fmt_detail
{

// Default outputter type.  Output one variable.
//
template <typename type>
class outputter
  {
  public:

    outputter(std::ostream &os_, type const &ref) : os{os_} { os << ref; }

    // Allow for the daisy chaining of function call operators returning outputter instances.
    //
    template <typename type2, typename ...type_pack>
    auto operator () (type2 const &ref, type_pack && ...arg_pack)
      {
        return outputter<type2>{os, ref, std::forward<type_pack>(arg_pack)...};
      }

  private:

    std::ostream &os;
  };

} // end namespace ostream_fmt_detail

template <typename type, typename ...type_pack>
auto ostream_fmt::operator () (type const &value, type_pack && ...arg_pack)
  {
    return ostream_fmt_detail::outputter<type>(os, value, std::forward<type_pack>(arg_pack)...);
  }

inline ostream_fmt f(std::ostream &os) { return ostream_fmt{os}; }

inline ostream_fmt fn(std::ostream &os) { return ostream_fmt{os, true}; }

namespace ostream_fmt_detail
{

class ostream_fmt_int_data
  {
  protected:

    unsigned padded_width_{1};
    unsigned radix_{10};
    char pad_char_{' '};
    bool right_justified_{true};
  };

// Interface for setting and getting integral formatting parameters.  'derived' will either be a stand-alone formatting
// configuration class or an outputter for an intergral type.  'derived' will publicly inherit from
// ostream_fmt_int_iface<derived>.  If d is an instance of derived, then daisy chains like d.pw(10).rj() are possible.
//
template <class derived>
class ostream_fmt_int_iface : public ostream_fmt_int_data
  {
  public:

    unsigned padded_width() const { return padded_width_; }
    unsigned radix() const { return radix_; }
    char pad_char() const { return pad_char_; }
    bool right_justified() const { return right_justified_; }

    derived & padded_width(unsigned v) { padded_width_ = v; return d(); }
    derived & radix(unsigned v)
      {
        if ((2 > v) || (v > 36))
          throw std::invalid_argument("ostream_fmt_int: radix must be greater than or equal to 2, and less than or equal to 36");

        radix_ = v;
        return d();
      }
    derived & pad_char(char v) { pad_char_ = v; return d(); }
    derived & right_justify() { right_justified_ = true; return d(); }
    derived & left_justify() { right_justified_ = false; return d(); }

    derived & pw(unsigned v) { padded_width_ = v; return d(); }
    derived & r(unsigned v) { radix(v); return d(); }
    derived & pc(char v) { pad_char_ = v; return d(); }
    derived & rj() { right_justified_ = true; return d(); }
    derived & lj() { right_justified_ = false; return d(); }

  private:

    derived & d() { return *static_cast<derived *>(this); }
  };

} // end namespace ostream_fmt_detail

// Formatting paramters for integral output.
//
class ostream_fmt_int : public ostream_fmt_detail::ostream_fmt_int_iface<ostream_fmt_int> {};

namespace ostream_fmt_detail
{

// Convert a one digit value, 0 - 35, to a character.
//
char one_dig_char(unsigned dig)
  {
    if (dig < 10)
      return '0' + dig;
    return 'a' + dig - 10;
  }

// Output an unsigned int using radix 2 - 36.
//
void uint_out(std::ostream &os, unsigned i, unsigned radix)
  {
    if (0 == i)
      os << '0';
    else
      {
        const unsigned digits{16};
        char buf[digits];
        unsigned dig_cnt{0};
        do
          {
            buf[digits - ++dig_cnt] = one_dig_char(i % radix);
            i /= radix;
            if (not i)
              {
                os << std::string_view{buf + (digits - dig_cnt), dig_cnt};
                return;
              }
          }
        while (dig_cnt < digits);
        uint_out(os, i, radix);
        os << std::string_view{buf, digits};
      }
  }

// Return the largest unsigned value that is a power of base.
//
unsigned greatest_power_unsigned(unsigned base)
  {
    unsigned i{base};

    while ((std::numeric_limits<unsigned>::max() / i) >= base)
      i *= base;

    return i;
  }

// Output a nonnegative integral with a radix 2 - 36.
//
template <std::integral type>
void nonnegative_int_out(std::ostream &os, type i, unsigned radix)
  {
    if constexpr (std::numeric_limits<type>::max() <= std::numeric_limits<unsigned>::max())
      {
        uint_out(os, unsigned{i}, radix);
        return;
      }

    if (i <= type{std::numeric_limits<unsigned>::max()})
      {
        uint_out(os, unsigned{i}, radix);
        return;
      }

    unsigned lim{unsigned{greatest_power_unsigned(radix)}};
    unsigned lower{unsigned{i % lim}};
    nonnegative_int_out(os, i / lim, radix);
    uint_out(os, lower, radix);
  }

// Returns number of digits in a nonnegative number for the given radix.
//
template <std::integral type>
unsigned nonnegative_int_num_digits(type i, unsigned radix)
  {
    unsigned num_digits{1};
    while (i >= type{radix})
      {
        ++num_digits;
        i /= radix;
      }
    return num_digits;
  }

// Partial specialization of outputter for integral types other than char.
//
template <std::integral type>
requires (not std::same_as<type, char>)
class outputter<type> : public ostream_fmt_int_iface<outputter<type> >
  {
  public:

    outputter(std::ostream &os_, type const &value_, unsigned pw = 1, unsigned r = 10)
      : os{os_}, value{value_}
      {
        outputter::padded_width(pw);
        outputter::radix(r);
      }

    template <typename type2, typename ...type_pack>
    auto operator () (type2 const &ref, type_pack && ...arg_pack)
      {
        out_val();
        return outputter<type2>{os, ref, std::forward<type_pack>(arg_pack)...};
      }

    outputter & config(ostream_fmt_int_data const &cfg) { *static_cast<ostream_fmt_int_data *>(this) = cfg; return *this; }
    outputter & c(ostream_fmt_int_data const &cfg) { return config(cfg); }

    ~outputter() { out_val(); }

  private:

    void out_val()
      {
        if (out_done)
          return;
        out_done = true;

        unsigned sign_len{0};
        if constexpr (std::numeric_limits<type>::is_signed)
          if (value < 0)
            {
              // Output a negative sign (one character).
              //
              sign_len = 1;
              value = -value;
            }

        int pad{outputter::padded_width() > 1 ?
                  int{outputter::padded_width()} - int{nonnegative_int_num_digits(value, outputter::radix())} - int{sign_len} : 0};

        if (sign_len and (outputter::pad_char() == '0'))
          {
            os << '-';
            sign_len = 0;
          }

        if (outputter::right_justified() and (pad > 0))
          do
            os << outputter::pad_char();
          while (--pad > 0);

        if (sign_len)
          os << '-';

        nonnegative_int_out(os, value, outputter::radix());

        while (pad-- > 0)
          os << outputter::pad_char();
      }

    bool out_done{false};
    std::ostream &os;
    type value;
  };

} // end namespace ostream_fmt_detail

#include <iostream>

int main()
  {
    {
      int i{5};
      f(std::cout) ("i is ") (i) (" decimal, ") (i).r(2)(" binary") ('\n');
    }
    {
      int i{-5};
      fn(std::cout) ("i is ") (i).pc('*').pw(5) (" decimal, ") (i).r(2).pc('0').pw(10) (" binary");
    }
    {
      unsigned i{5};
      fn(std::cout) ("i is ") (i).pc('*').pw(5) (" decimal, ") (i).r(2).pc('0').pw(10) (" binary");
    }
    {
      int i{-5};
      fn(std::cout) ("i is ") (i).pc('*').pw(5).lj() (" decimal, ") (i).r(2).pc('0').pw(10) (" binary");
    }
    {
      int i{-5};
      ostream_fmt_int cfg1, cfg2;
      cfg1.pc('*').pw(5).lj();
      cfg2.r(2).pc('0').pw(10);
      fn(std::cout) ("i is ") (i).c(cfg1) (" decimal, ") (i).c(cfg2) (" binary");
    }
    {
      long long i{-(static_cast<long long>(std::numeric_limits<unsigned>::max()) << 8)};
      fn(std::cout) ("i is ") (i).r(16).pc('0').pw(17) (" hexidecimal, ") (i).r(8).pc('*').lj().pw(23) (" octal, ")
                    (i).r(36) (" base 36");
    }
    {
      auto i{std::numeric_limits<long long>::min() + 1};
      fn(std::cout) ("i is ") (i).r(16).pc('0').pw(17) (" hexidecimal, ") (i).r(8).pc('*').lj().pw(23) (" octal ");
      fn(std::cout) (i).r(2) (" binary, ") (i).r(36) (" base 36");
    }
    {
      long long i{0};
      fn(std::cout) ("i is ") (i, 17, 16).pc('0') (" hexidecimal, ") (i).r(8).pc('*').lj().pw(23) (" octal");
    }

    return 0;
  }
