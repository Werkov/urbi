/*
 * Copyright (C) 2007-2012, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

/**
 ** \file object/float.cc
 ** \brief Creation of the Urbi object float.
 */

#include <algorithm>

#include <libport/cmath>
#include <libport/cstdlib>
#include <libport/compiler.hh>
#include <libport/format.hh>

#include <kernel/config.h>
#include <libport/ufloat.hh>

#include <object/cxx-helper.hh>
#if !defined COMPILATION_MODE_SPACE
# include <object/format-info.hh>
#endif
#include <urbi/object/symbols.hh>

#include <urbi/object/float.hh>
#include <urbi/object/hash.hh>
#include <urbi/object/list.hh>
#include <urbi/object/object.hh>
#include <urbi/object/string.hh>
#include <urbi/runner/raise.hh>

#include <runner/job.hh>

namespace urbi
{
  namespace object
  {
    rObject Float::limits;

    Float::Float(value_type value)
      : value_(value)
    {
      proto_add(proto);
    }

    Float::Float(const rFloat& model)
      : value_(model->value_get())
    {
      proto_add(model);
    }

    URBI_CXX_OBJECT_INIT(Float)
      : value_(0)
    {
      BIND(PLUS, plus,
           Float::value_type (Float::*)() const);
      BIND(PLUS, plus,
           Float::value_type (Float::*)(Float::value_type) const);
      BIND(MINUS, minus,
           Float::value_type (Float::*)() const);
      BIND(MINUS, minus,
           Float::value_type (Float::*)(Float::value_type) const);
      BIND(EQ_EQ, operator==,
           bool (self_type::*)(const rObject&) const);

      limits = urbi::object::Object::proto->clone();

#define DECLARE(Urbi, Cxx)                    \
      limits->bind(SYMBOL_(Urbi), &Float::Cxx, 0)

      DECLARE(digits, limit_digits);
      DECLARE(digits10, limit_digits10);
      DECLARE(min, limit_min);
      DECLARE(max, limit_max);
      DECLARE(epsilon, limit_epsilon);
      DECLARE(minExponent, limit_min_exponent);
      DECLARE(minExponent10, limit_min_exponent10);
      DECLARE(maxExponent, limit_max_exponent);
      DECLARE(maxExponent10, limit_max_exponent10);
      DECLARE(radix, limit_radix);

#undef DECLARE

      BIND(CARET,     operator^);
      BIND(GT_GT,     operator>>);
      BIND(LT_EQ,     operator<=);
      BIND(LT,        less_than);
      BIND(GT,        operator>);
      BIND(GT_EQ,     operator>=);
      BIND(BANG_EQ,   operator!=);
      BIND(LT_LT,     operator<<);
      BIND(PERCENT,   operator%);
      BIND(SLASH,     operator/);
      BIND(STAR,      operator*);
      BIND(STAR_STAR, pow);
      BIND(abs,       fabs);
      BIND(acos);
      BIND(asString,  as_string);
      BIND(asin);
      BIND(atan);
      BIND(atan2);
      BIND(bitand,    operator&);
      BIND(bitor,     operator|);
      BIND(ceil);
      BIND(compl,     operator~);
      BIND(cos);
      BIND(each);
      BIND(each_PIPE, each_pipe);
      BIND(exp);
#if !defined COMPILATION_MODE_SPACE
      BIND(format);
#endif
      BIND(floor);
      BIND(hash);
      BINDG(inf);
      BIND(isInf,   is_inf);
      BIND(isNan,   is_nan);
      BIND(log);
      BINDG(nan);
      BIND(random);
      BIND(srandom);
      BIND(round);
      BIND(seq);
      BIND(sign);
      BIND(sin);
      BIND(sqrt);
      BIND(tan);
      BIND(trunc);

      // Hack to avoid proto = 0,
      // proto will redefined after.
      proto = this;

      setSlot("pi", new Float(M_PI));
      setSlot(SYMBOL(limits), Float::limits);
    }

    const Float::value_type&
    Float::value_get() const
    {
      return value_;
    }

    Float::value_type&
    Float::value_get()
    {
      return value_;
    }

#define BOUNCE_COMP_OP(op)                              \
    bool                                                \
    Float::operator op(const value_type& rhs) const     \
    {                                                   \
      return value_get() op rhs;                        \
    }

    BOUNCE_COMP_OP(<=)
    BOUNCE_COMP_OP(>=)
    BOUNCE_COMP_OP(>)
    BOUNCE_COMP_OP(!=)

#undef BOUNCE_COMP_OP

    bool
    Float::less_than(const value_type& rhs) const
    {
      return value_get() < rhs;
    }

    /*--------------.
    | Conversions.  |
    `--------------*/

    // FIXME: For some reason MSVC fails to link when we pass const
    // ref strings here...
#define CONVERSION(Type)                        \
    Float::Type                                 \
    Float::to_##Type(value_type v,              \
                     std::string fmt)           \
    {                                           \
      if (libport::numeric_castable<Type>(v))   \
        return libport::numeric_cast<Type>(v);  \
      runner::raise_bad_integer_error(v, fmt);  \
    }                                           \
                                                \
    Float::Type                                 \
    Float::to_##Type(std::string fmt) const     \
    {                                           \
      return to_##Type(value_get(), fmt);       \
    }

    CONVERSION(int_type);
    CONVERSION(unsigned_type);
    CONVERSION(long_type);
#undef CONVERSION

    static inline bool
    format_manual(Float::value_type value,
                  std::string& res, const std::string& prefix = "")
    {
      // We format nan and inf by ourselves because behavior differs
      // between toolchains, for instance:
      // * Visual gives "1.#INF" and "1.#QNAN".
      // * GCC 4.{4,5} gives "-nan" for negative NaNs.
      // FIXME: this approach does not support %20s and so forth.  The
      // easiest is then probably to reuse the FormatInfo, but with a
      // string "nan" etc.
      if (std::isnan(value))
      {
        // The "+" prefix is not obeyed, but " " is.
        res = prefix == " " ? " nan" : "nan";
        return true;
      }
      else if (std::isinf(value))
      {
        // " inf", "+inf" etc.
        res = prefix + (value < 0 ? "-inf" : "inf");
        return true;
      }
      return false;
    }

#if !defined COMPILATION_MODE_SPACE
    std::string
    Float::format(rFormatInfo finfo) const
    {
      std::string manual;
      if (format_manual(value_, manual, finfo->prefix_get()))
        return manual;

      std::string pattern = finfo->compute_pattern(true);

      if (is_integer())
        return libport::format(pattern, as_integer());
      if (finfo->spec_get() == "x" || finfo->spec_get() == "o")
        runner::raise_bad_integer_error(value_);
      return libport::format(pattern, value_);
    }
#endif

    std::string
    Float::as_string() const
    {
      std::string manual;
      if (format_manual(value_, manual))
        return manual;
      if (is_integer())
        return libport::format("%s", as_integer());
      return libport::format("%s", value_);
    }

    Float::value_type
    Float::atan2(value_type y) const
    {
      return ::atan2(value_, y);
    }

#define BOUNCE_OP(Op, ResType, Check)           \
    ResType                                     \
    Float::operator Op(value_type rhs) const    \
    {                                           \
      static libport::Symbol op(#Op);           \
      Check;                                    \
      return value_get() Op rhs;                \
    }

    BOUNCE_OP(*, Float::value_type, )
    BOUNCE_OP(/, Float::value_type, if (!rhs) RAISE("division by 0"))

#undef BOUNCE_OP

    Float::value_type
    Float::operator %(value_type rhs) const
    {
      if (rhs)
        return fmod(value_get(), rhs);
      RAISE("modulo by 0");
    }

    Float::value_type
    Float::pow(value_type rhs) const
    {
      return std::pow(value_get(), rhs);
    }

    /*------------------.
    | Unary operators.  |
    `------------------*/
#define BOUNCE_UNSIGNED_OP(Op)                  \
    Float::unsigned_type                        \
    Float::operator Op() const                  \
    {						\
      return Op to_unsigned_type();             \
    }

    BOUNCE_UNSIGNED_OP(~)
#undef BOUNCE_UNSIGNED_OP


    /*-------------------.
    | Binary operators.  |
    `-------------------*/
#define BOUNCE_UNSIGNED_OP(Op)                  \
    Float::unsigned_type                        \
    Float::operator Op(unsigned_type rhs) const \
    {						\
      return to_unsigned_type() Op rhs;         \
    }

    BOUNCE_UNSIGNED_OP(^)
    BOUNCE_UNSIGNED_OP(|)
    BOUNCE_UNSIGNED_OP(&)
    BOUNCE_UNSIGNED_OP(<<)
    BOUNCE_UNSIGNED_OP(>>)
#undef BOUNCE_UNSIGNED_OP

#define CHECK_POSITIVE(F)                       \
    do {                                        \
      if (value_ < 0)                           \
        RAISE("argument has to be positive");   \
    } while (0)

#define CHECK_TRIGO_RANGE(F)                    \
    do {                                        \
      if (value_ < -1 || 1 < value_)            \
        RAISE("invalid range");                 \
    } while (0)

#define BOUNCE(Namespace, F, Pos, Range)                \
    Float::value_type					\
    Float::F() const                                    \
    {                                                   \
      WHEN(Pos, CHECK_POSITIVE(SYMBOL_(F)));            \
      WHEN(Range, CHECK_TRIGO_RANGE(SYMBOL_(F)));       \
      return Namespace F(value_get());                  \
    }

    BOUNCE(::,        acos,  false, true);
    BOUNCE(::,        asin,  false, true);
    BOUNCE(::,        atan,  false, false);
    BOUNCE(::,        ceil,  false, false);
    BOUNCE(::,        cos,   false, false);
    BOUNCE(::,        exp,   false, false);
    BOUNCE(::,        fabs,  false, false);
    BOUNCE(::,        floor, false, false);
    BOUNCE(::,        log,   true,  false);
    BOUNCE(libport::, round, false, false);
    BOUNCE(::,        sin,   false, false);
    BOUNCE(::,        sqrt,  true,  false);
    BOUNCE(::,        tan,   false, false);
    BOUNCE(libport::, trunc, false, false);

#undef CHECK_POSITIVE
#undef CHECK_TRIGO_RANGE
#undef BOUNCE

    static const std::string positive_error_fmt =
      "expected positive integer: %s";

    Float::unsigned_type
    Float::random() const
    {
      if (unsigned_type upper_bound = to_unsigned_type(positive_error_fmt))
        return libport::rand() % upper_bound;
      runner::raise_bad_integer_error(value_get(), positive_error_fmt);
    }

    void
    Float::srandom() const
    {
      srand(to_unsigned_type());
    }


#define DEFINE(Urbi, Op)                        \
    Float::value_type                           \
    Float::Urbi() const                         \
    {                                           \
      return Op value_get();                    \
    }                                           \
                                                \
    Float::value_type                           \
    Float::Urbi(value_type rhs) const           \
    {                                           \
      return value_get() Op rhs;                \
    }                                           \


    DEFINE(plus, +);
    DEFINE(minus, -);
#undef DEFINE


#define DEFINE(Urbi, Type, Cxx)                         \
    Float::Type                                         \
    Float::limit_ ## Urbi ()                            \
    {                                                   \
      return std::numeric_limits<value_type>::Cxx;      \
    }

    DEFINE(digits         , unsigned_type, digits  )
    DEFINE(digits10       , unsigned_type, digits10)
    DEFINE(min            , value_type,    min()         )
    DEFINE(max            , value_type,    max()         )
    DEFINE(epsilon        , value_type,    epsilon()     )
    DEFINE(min_exponent   , int_type,      min_exponent  )
    DEFINE(min_exponent10 , int_type,      min_exponent10)
    DEFINE(max_exponent   , unsigned_type, max_exponent  )
    DEFINE(max_exponent10 , unsigned_type, max_exponent10)
    DEFINE(radix          , unsigned_type, radix         )
#undef DEFINE

    Float::int_type
    Float::sign() const
    {
      return (0 < value_    ? 1
              : 0 == value_ ? 0
              :               -1);
    }

    void
    Float::each(Executable* action)
    {
      // Check we're non-negative.
      if (value_ < 0)
        runner::raise_bad_integer_error(value_, positive_error_fmt);
      // Iterate, yielding at each iteration.
      runner::Job& r = ::kernel::runner();
      for (int i = 0; i < value_; ++i)
      {
        object::objects_type args;
        args << this << new Float(i);
        (*action)(args);
        r.yield();
      }
    }

    void
    Float::each_pipe(Executable* action)
    {
      // Check we're non-negative.
      if (value_ < 0)
        runner::raise_bad_integer_error(value_, positive_error_fmt);
      // Iterate
      for (int i = 0; i < value_; ++i)
      {
        object::objects_type args;
        args << this << new Float(i);
        (*action)(args);
      }
    }

    rList
    Float::seq() const
    {
      Float::unsigned_type n = to_unsigned_type();
      List::value_type res;
      for (unsigned int i = 0; i < n; i++)
        res << new Float(i);
      return new List(res);
    }

    rHash
    Float::hash() const
    {
      return new Hash(boost::hash_value(value_get()));
    }

    /*----------.
    | Details.  |
    `----------*/

    std::ostream& Float::special_slots_dump(std::ostream& o) const
    {
      o << "value = " << value_ << libport::iendl;
      return o;
    }

  }; // namespace object
}
