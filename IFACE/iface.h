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

/*

"Go-like" interfaces for C++.  See example.cpp for example use.

The primarily intent of this code is to explore the idea of adding
interfaces to the C++ base language.

It's possible that this facility could be changed to allow class public data
members to be accessed through interfaces.

Definitions:

- A "param list" is a macro that defines the parameters to a (member)
function.  It must take (only) one parameter.  If the parameter is named P,
then the param list must be defined as a blank-separated sequence of:

P(TYPE, NAME)

Where TYPE specifies the type of the parameter, and NAME is the name
of the parameter.  IFACE_NO_PARAMS is an empty param list.

- A "function list" is a macro that encodes the interfaces to multiple
(member) functions.  It must take (only) two parameters.  If the two
parameters are named F and FNR, then the function list must be defined as a
blank-separated sequence of:

F(TYPE, NAME, CV, PARAM_LIST)

and/or

FNR(NAME, CV, PARAM_LIST)

TYPE specifies the return type of the function (use FNR for void functions).
NAME is the name of the function.  CV is either blank or a CV type qualifier.
PARAM_LIST is a param list (defined above) specifying the function parameters.

*/

#ifndef IFACE_20170223
#define IFACE_20170223

// Empty param list.
//
#define IFACE_NO_PARAMS(P) P(..., ) 

// Defines an interface (type).  IF_NAME is the name of the interface type.
// CV is a CV type qualifier for the object being interfaced to (not mutable).
// FUNC_LIST is a function list, specifying the member functions of the
// interface.
//
#define IFACE_DEF(IF_NAME, CV, FUNC_LIST) \
 \
struct IF_NAME \
  { \
    /* Do not access the contents of this directly */ \
 \
    struct Vstruct \
      { \
        FUNC_LIST(IFACE_IMPL_FUNC_POINTER, IFACE_IMPL_FUNC_NR_POINTER) \
 \
        Iface_impl::Class_id class_id; \
      }; \
 \
    CV void * const this_; \
 \
    const Vstruct * const vptr; \
 \
    IF_NAME(CV void * t, const Vstruct *vptr_) : this_(t), vptr(vptr_) { } \
  };

// Enable instance of a class to be interfaced to by instances of an
// interface.  IF_SPEC is the qualified name of the interface type.
// FUNC_LIST is the function list that was used to define the interface type.
// CLS_SPEC is the qualified name of the class.  This can only be invoked
// at global scope.
//
#define IFACE_ENABLE(IF_SPEC, FUNC_LIST, CLS_SPEC) \
 \
namespace Iface_impl \
{ \
 \
template <> \
class Enable<IF_SPEC, CLS_SPEC> \
  { \
  private: \
 \
    using Cls = CLS_SPEC; \
 \
    FUNC_LIST(IFACE_IMPL_THUNK, IFACE_IMPL_THUNK_NR) \
 \
  public: \
 \
    static const IF_SPEC::Vstruct * vptr() \
      { \
        static const IF_SPEC::Vstruct IFACE_v = \
          { \
            FUNC_LIST(IFACE_IMPL_ENB_FUNC_ADDR, \
                      IFACE_IMPL_ENB_FUNC_ADDR_NR) \
 \
            id<CLS_SPEC>() \
          }; \
 \
        return(&IFACE_v); \
      } \
  }; \
 \
}

// Enable conversion from one interface instance to another of a different
// type (by iface_convert).  DEST_IF_SPEC is the qualified name of the type
// of the interace instance to be returned by iface_convert.  DEST_FUNC_LIST
// is the function list used to define the destination destination interface
// type.  SRC_IF_SPEC is the qualified name of the type of the interface
// interface that will be the parameter to iface_convert for this conversion.
// The functions in the destination interface must be a subset of those in
// the source interface.  This macro can only be invoked at global scope.
//
#define IFACE_CONVERSION(DEST_IF_SPEC, DEST_FUNC_LIST, SRC_IF_SPEC) \
 \
namespace Iface_impl \
{ \
 \
template <> \
struct Convert<DEST_IF_SPEC, SRC_IF_SPEC> \
  { \
    static const DEST_IF_SPEC::Vstruct * vptr( \
      const SRC_IF_SPEC::Vstruct *src_vptr) \
      { \
        Convert_key k(id<DEST_IF_SPEC>(), src_vptr->class_id); \
 \
        auto map_iter = convert_map().find(k); \
 \
        if (map_iter != convert_map().end()) \
          return(static_cast<const DEST_IF_SPEC::Vstruct *>( \
            map_iter->second)); \
        else \
          { \
            DEST_IF_SPEC::Vstruct *vp = new DEST_IF_SPEC::Vstruct; \
 \
            DEST_FUNC_LIST(IFACE_IMPL_CONVERT, IFACE_IMPL_CONVERT_NR) \
 \
            vp->class_id = src_vptr->class_id; \
 \
            convert_map()[k] = vp; \
 \
            return(vp); \
          }; \
      } \
  }; \
 \
}

// Call the member function NAME through the interface instance IF_INST.  The
// variable arguments must be the member function's actual arguments.
//
#define IFACE_CALL(IF_INST, NAME, ...) \
(IF_INST).vptr->NAME((IF_INST).this_, __VA_ARGS__)

// Call the member function NAME, which takes no arguments, through the
// interface instance IF_INST.
//
#define IFACE_CALL_NP(IF_INST, NAME) (IF_INST).vptr->NAME((IF_INST).this_)

namespace Iface_impl
{

// Don't use anything in this namespace directly.

template <class Iface, class Target>
class Enable;

template <class Dest_iface, class Src_iface>
class Convert;

} // end namespace Iface_impl

// Return an interface instance of type Iface that interfaces to an
// object of type Target.
//
template <class Iface, class Target>
Iface iface_factory(Target &t)
  { return(Iface(&t, Iface_impl::Enable<Iface, Target>::vptr())); }

// Convert an interface instance of type Src_iface to one of type Dest_iface.
//
template <class Dest_iface, class Src_iface>
Dest_iface iface_convert(Src_iface src_if)
  {
    return(
      Dest_iface(
        src_if.this_,
        Iface_impl::Convert<Dest_iface, Src_iface>::vptr(src_if.vptr)));
  }

/*

Bad implementation artifcats of this facility include:

- No member function name overloading in interfaces.

- If you call a parameterless interface function with parameters, they
will be ignored at run-time, but you will not get a compile error.

*/

// ----- Private stuff (don't use directly) --------------------------

#define IFACE_IMPL_FUNC_POINTER(TYPE, NAME, CV, PARAMS) \
TYPE (*NAME) (CV void *this_, PARAMS(IFACE_IMPL_FULL_PARAM) );

#define IFACE_IMPL_FUNC_NR_POINTER(NAME, CV, PARAMS) \
IFACE_IMPL_FUNC_POINTER(void, NAME, CV, PARAMS)

#define IFACE_IMPL_THUNK(TYPE, NAME, CV, PARAMS) \
static TYPE NAME(CV void *this_, PARAMS(IFACE_IMPL_FULL_PARAM) ) \
  { \
    return( \
      static_cast<CV Cls *>(this_)->NAME( PARAMS(IFACE_IMPL_PARAM_NAME) )); \
  }

#define IFACE_IMPL_THUNK_NR(NAME, CV, PARAMS) \
static void NAME(CV void *this_, PARAMS(IFACE_IMPL_FULL_PARAM) ) \
  { \
    static_cast<CV Cls *>(this_)->NAME( PARAMS(IFACE_IMPL_PARAM_NAME) ); \
  }

#define IFACE_IMPL_CONVERT(TYPE, NAME, CV, PARAMS) \
vp->NAME = src_vptr->NAME;

#define IFACE_IMPL_CONVERT_NR(NAME, CV, PARAMS) \
IFACE_IMPL_CONVERT(, NAME, , PARAMS)

#define IFACE_IMPL_ENB_FUNC_ADDR(TYPE, NAME, CV, PARAMS) NAME,

#define IFACE_IMPL_ENB_FUNC_ADDR_NR(NAME, CV, PARAMS) NAME,

#define IFACE_IMPL_FULL_PARAM(TYPE, NAME) TYPE NAME

#define IFACE_IMPL_PARAM_NAME(TYPE, NAME) NAME

#include <functional>
#include <unordered_map>

namespace Iface_impl
{

template <class C>
int * id()
  {
    // Dummy, purpose is to provide a unique address for each class C.
    static int i;

    return(&i);
  }

using Class_id = int *;

using Iface_id = int *;

// Key for lookup table from interface type / class type combo to a pointer
// to a pointer to the vstructure for the class for that interface.
//
class Convert_key
  {
  private:

    // ID of interface resulting from interface conversion.
    const Iface_id dest_iface_id;

    // ID of class of instance interfaced to.
    const Class_id class_id;

  public:

    Convert_key(Iface_id i, Class_id c) : dest_iface_id(i), class_id(c) { }

    // "Pre-hash" this instance to produce a value of a type for which
    // std::hash is already defined.  Hopefully never a loss of nominal
    // or actual precision in any C++ implementation.
    //
    long long hash_me() const { return(dest_iface_id - class_id); }

    friend bool operator == (Convert_key a, Convert_key b)
      { return((a.dest_iface_id == b.dest_iface_id) and
               (a.class_id == b.class_id)); }
  };

} // end namespace Iface_impl

namespace std
{

template<>
struct hash<Iface_impl::Convert_key>
  {
    using argument_type = Iface_impl::Convert_key;

    using result_type = std::size_t;

    result_type operator()(const argument_type &k) const
      {
        return(std::hash<long long>()(k.hash_me()));
      }
  };

} // end namespace std

namespace Iface_impl
{

using Convert_map = std::unordered_map<Convert_key, const void *>;

// Map to get vpointer for result (destination) interface in interface
// conversion.
//
inline Convert_map & convert_map()
  {
    static Convert_map m;

    return(m);
  }

} // end namespace Iface_impl

#endif // Include once.

