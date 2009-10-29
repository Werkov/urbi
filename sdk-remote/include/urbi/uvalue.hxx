/*
 * Copyright (C) 2009, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

/// \file urbi/uvalue.hxx

#include <boost/foreach.hpp>

#include <libport/cassert>
#include <libport/cassert>

namespace urbi
{

  /*--------.
  | UList.  |
  `--------*/

# define ULIST_NTH(Const)                       \
  inline                                        \
  Const UValue&                                 \
  UList::operator[](size_t i) Const             \
  {                                             \
    i += offset;                                \
    if (i < size())                             \
      return *array[i];                         \
    else                                        \
      return UValue::error();                   \
  }

  ULIST_NTH(const)
  ULIST_NTH(/* const */)

# undef ULIST_NTH

  inline
  size_t
  UList::size() const
  {
    return array.size();
  }

  inline
  void
  UList::setOffset(size_t n)
  {
    offset = n;
  }

  template<typename T>
  UList&
  UList::push_back(const T& v)
  {
    array.push_back(new UValue(v));
    return *this;
  }

  inline
  UValue&
  UList::front()
  {
   return *array.front();
  }

  inline
  void
  UList::pop_back()
  {
    array.pop_back();
  }

  /*--------------.
  | UNamedValue.  |
  `--------------*/

  inline
  UNamedValue::UNamedValue(const std::string& n, UValue* v)
    : name(n)
    , val(v)
  {}

  inline
  UNamedValue&
  UNamedValue::error()
  {
    static UNamedValue instance("<<UNamedValue::error (denotes an error)>>");
    return instance;
  }



  /*----------------.
  | UObjectStruct.  |
  `----------------*/

# define UOBJECTSTRUCT_NTH(Const)               \
  inline                                        \
  Const UNamedValue&                            \
  UObjectStruct::operator[](size_t i) Const     \
  {                                             \
    if (i < size())                             \
      return array[i];                          \
    else                                        \
      return UNamedValue::error();              \
  }

  UOBJECTSTRUCT_NTH(const)
  UOBJECTSTRUCT_NTH(/* const */)

# undef UOBJECTSTRUCT_NTH

  inline
  size_t
  UObjectStruct::size() const
  {
    return array.size();
  }


  /*---------.
  | UValue.  |
  `---------*/

  /// We use an operator , that behaves like an assignment.  The
  /// only difference is when the rhs is void, in which case it is
  /// the regular comma which is used.  This allows to write "uval,
  /// expr" to mean "compute expr and assign its result to uval,
  /// unless expr is void".
  inline
  UValue&
  UValue::operator, (const UValue &b)
  {
    return *this = b;
  }

#  define CONTAINER_TO_UVALUE_DECLARE(Type)                     \
  template <typename T>                                         \
  inline                                                        \
  UValue&                                                       \
  operator, (UValue &b, const Type<T> &v)                       \
  {                                                             \
    b.type = DATA_LIST;                                         \
    b.list = new UList();                                       \
    for (typename Type<T>::const_iterator i = v.begin(),        \
           i_end = v.end();                                     \
         i != i_end; ++i)                                       \
    {                                                           \
      UValue r;                                                 \
      r, *i;                                                    \
      b.list->array.push_back(new UValue(r));                   \
    }                                                           \
    return b;                                                   \
  }

  CONTAINER_TO_UVALUE_DECLARE(std::list)
  CONTAINER_TO_UVALUE_DECLARE(std::vector)

# undef CONTAINER_TO_UVALUE_DECLARE


# define CTOR_AND_ASSIGN_AND_COMMA(Type)        \
  inline                                        \
  UValue& UValue::operator, (Type rhs)          \
  {						\
    return *this = rhs;                         \
  }

  // UFloats.
  CTOR_AND_ASSIGN_AND_COMMA(ufloat);
  CTOR_AND_ASSIGN_AND_COMMA(int);
  CTOR_AND_ASSIGN_AND_COMMA(long);
  CTOR_AND_ASSIGN_AND_COMMA(unsigned int);
  CTOR_AND_ASSIGN_AND_COMMA(unsigned long);

  // Strings.
  CTOR_AND_ASSIGN_AND_COMMA(const char*);
  CTOR_AND_ASSIGN_AND_COMMA(const void*);
  CTOR_AND_ASSIGN_AND_COMMA(const std::string&);

  // Others.
  CTOR_AND_ASSIGN_AND_COMMA(const UBinary&);
  CTOR_AND_ASSIGN_AND_COMMA(const UList&);
  CTOR_AND_ASSIGN_AND_COMMA(const UObjectStruct&);
  CTOR_AND_ASSIGN_AND_COMMA(const USound&);
  CTOR_AND_ASSIGN_AND_COMMA(const UImage&);

# undef CTOR_AND_ASSIGN_AND_COMMA



# define UVALUE_INTEGRAL_CAST(Type)                             \
  inline                                                        \
  UValue::operator Type() const                                 \
  {                                                             \
    return static_cast<Type>(static_cast<ufloat>((*this)));     \
  }

  UVALUE_INTEGRAL_CAST(int)
  UVALUE_INTEGRAL_CAST(unsigned int)
  UVALUE_INTEGRAL_CAST(long)
  UVALUE_INTEGRAL_CAST(unsigned long)
# undef UVALUE_INTEGRAL_CAST


  inline
  UValue::operator bool() const
  {
    return static_cast<int>(static_cast<ufloat>((*this))) != 0;
  }


  inline
  UValue&
  UValue::operator()()
  {
    return *this;
  }

  inline
  std::ostream&
  operator<<(std::ostream& s, const UValue& v)
  {
    // Looks bizarre, but might happen without have the "print" die
    // (it *has* happened).
    assert(&v);
    return v.print(s);
  }



  /*----------.
  | Casters.  |
  `----------*/


  // Run the uvalue_caster<Type> on v.
  template <typename Type>
  typename uvar_ref_traits<typename libport::traits::remove_reference<Type>::type>::type
  uvalue_cast(UValue& v)
  {
    return uvalue_caster<typename libport::traits::remove_reference<Type>::type>()(v);
  }


# define UVALUE_CASTER_DEFINE(Type)		\
  template <>					\
  struct uvalue_caster <Type>			\
  {						\
    Type operator() (UValue& v)			\
    {						\
      return v;					\
    }						\
  };

  UVALUE_CASTER_DEFINE(int);
  UVALUE_CASTER_DEFINE(unsigned int);
  UVALUE_CASTER_DEFINE(long);
  UVALUE_CASTER_DEFINE(unsigned long);
  UVALUE_CASTER_DEFINE(ufloat);
  UVALUE_CASTER_DEFINE(std::string);
  UVALUE_CASTER_DEFINE(const std::string);
  UVALUE_CASTER_DEFINE(bool);
  UVALUE_CASTER_DEFINE(UImage);
  UVALUE_CASTER_DEFINE(USound);

#undef UVALUE_CASTER_DEFINE


  /*-----------------------------------.
  | Casting an UValue into an UValue.  |
  `-----------------------------------*/

# define UVALUE_CASTER_DEFINE(Type)              \
  template <>                                   \
  struct uvalue_caster<Type>                    \
  {                                             \
    Type operator()(UValue& v)                  \
    {                                           \
      return v;                                 \
    }                                           \
  }

  UVALUE_CASTER_DEFINE(const UValue&);
  UVALUE_CASTER_DEFINE(UValue&);
  UVALUE_CASTER_DEFINE(const UValue);
  UVALUE_CASTER_DEFINE(UValue);
# undef UVALUE_CASTER_DEFINE



  // The following ones are defined in uvalue-common.cc.
  template <>
  struct URBI_SDK_API uvalue_caster<UVar>
  {
    UVar& operator () (UValue& v);
  };


# define UVALUE_CASTER_DECLARE(Type)		\
  template <>					\
  struct URBI_SDK_API uvalue_caster<Type>       \
  {                                             \
    Type operator () (UValue& v);		\
  }

  UVALUE_CASTER_DECLARE(UBinary);
  UVALUE_CASTER_DECLARE(UList);
  UVALUE_CASTER_DECLARE(UObjectStruct);
  UVALUE_CASTER_DECLARE(const char*);

# undef UVALUE_CASTER_DECLARE


# ifndef UOBJECT_NO_LIST_CAST

#  define UVALUE_CONTAINER_CASTER_DECLARE(Type)                 \
  template <typename T>                                         \
  struct uvalue_caster< Type<T> >                               \
  {                                                             \
    Type<T>                                                     \
    operator()(UValue& v)                                       \
    {                                                           \
      Type<T> res;                                              \
      if (v.type != DATA_LIST)                                  \
        /* Cast just the element.  */                           \
        res.push_back(uvalue_cast<T>(v));                       \
      else                                                      \
        for (size_t i = 0; i < v.list->size(); ++i)             \
          res.push_back(uvalue_cast<T>(*v.list->array[i]));     \
      return res;                                               \
    }                                                           \
  }

  UVALUE_CONTAINER_CASTER_DECLARE(std::list);
  UVALUE_CONTAINER_CASTER_DECLARE(std::vector);

#  undef UVALUE_CONTAINER_CASTER_DECLARE

# endif

  // Uses casters, must be at the end
  template<typename T>
  UList&
  UList::operator=(const T& container)
  {
    array.clear();
    typedef const typename T::value_type constv;
    BOOST_FOREACH(constv& v, container)
    {
      UValue val;
      val,v;
      array.push_back(new UValue(val));
    }
    return *this;
  }
  template<typename T>
  T
  UList::as()
  {
    T res;
    BOOST_FOREACH(UValue* v, array)
      res.push_back(uvalue_caster<typename T::value_type>()(*v));
    return res;
  }

} // namespace urbi
