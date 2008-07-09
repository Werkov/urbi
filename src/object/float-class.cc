/**
 ** \file object/float-class.cc
 ** \brief Creation of the URBI object float.
 */

#include <cmath>

#include <boost/format.hpp>

#include <sdk/config.h>
#ifndef HAVE_ROUND
# include <libport/ufloat.h>
#endif
#include <libport/ufloat.hh>

#include <object/float-class.hh>
#include <object/object.hh>
#include <object/string-class.hh>
#include <object/urbi-exception.hh>

namespace object
{
  rObject float_class;

  Float::Float()
    : value_(0)
  {
    proto_add(float_class);
  }

  Float::Float(value_type value)
    : value_(value)
  {
    proto_add(float_class);
  }

  Float::Float(rFloat model)
    : value_(model->value_get())
  {
    proto_add(model);
    proto_remove(float_class);
  }

  Float::value_type Float::value_get() const
  {
    return value_;
  }

  Float::value_type& Float::value_get()
  {
    return value_;
  }

  int
  Float::to_int(const libport::Symbol func) const
  {
    try
    {
      return libport::ufloat_to_int(value_);
    }
    catch (libport::bad_numeric_cast& ue)
    {
      throw BadInteger(value_, func);
      return 0;  // Keep the compiler happy
    }
  }

  rFloat Float::inf()
  {
    return new Float(std::numeric_limits<libport::ufloat>::infinity());
  }

  rFloat Float::nan()
  {
    return new Float(std::numeric_limits<libport::ufloat>::quiet_NaN());
  }

  rString Float::as_string(rObject from)
  {
    if (from.get() == float_class.get())
      return new String(SYMBOL(LT_Float_GT));
    {
      static boost::format f("%g");
      type_check<Float>(from, SYMBOL(asString));
      rFloat fl = from->as<Float>();
      return new String(libport::Symbol(str(f % float(fl->value_get()))));

    }
  }

  rObject Float::operator <(rFloat rhs)
  {
    return value_get() < rhs->value_get() ? true_class : false_class;
  }

#define BOUNCE_OP(Op, Check)                                            \
  rFloat                                                                \
  Float::operator Op(rFloat rhs)                                        \
  {                                                                     \
    static libport::Symbol op(#Op);                                     \
    WHEN(Check,                                                         \
         if (!rhs->value_get())                                         \
           throw PrimitiveError(op, "division by 0"));                  \
    return new Float(value_get() Op rhs->value_get());                  \
  }

  BOUNCE_OP(+, false);
  BOUNCE_OP(*, false);
  BOUNCE_OP(/, true);

#undef BOUNCE_OP

  rFloat
  Float::operator %(rFloat rhs)
  {
    if (!rhs->value_get())
      throw PrimitiveError(SYMBOL(PERCENT), "modulo by 0");
    return new Float(fmod(value_get(), rhs->value_get()));
  }

  rFloat Float::pow(rFloat rhs)
  {
    return new Float(powf(value_get(), rhs->value_get()));
  }


  /*------------------.
  | Unary operators.  |
  `------------------*/
#define BOUNCE_INT_OP(Op)                               \
  rFloat                                                \
  Float::operator Op()                                  \
  {                                                     \
    static libport::Symbol op(#Op);                     \
    return new Float(Op to_int(op));                    \
  }
  BOUNCE_INT_OP(~);
#undef BOUNCE_INT_OP


  /*-------------------.
  | Binary operators.  |
  `-------------------*/
#define BOUNCE_INT_OP(Op)                               \
  rFloat                                                \
  Float::operator Op(rFloat rhs)                        \
  {                                                     \
    static libport::Symbol op(#Op);                     \
    return new Float(to_int(op) Op rhs->to_int(op));    \
  }

  BOUNCE_INT_OP(<<);
  BOUNCE_INT_OP(>>);
  BOUNCE_INT_OP(^);
  BOUNCE_INT_OP(|);
  BOUNCE_INT_OP(&);
#undef BOUNCE_INT_OP



#define CHECK_POSITIVE(F)                                       \
  if (value_ < 0)                                               \
    throw PrimitiveError(F, "argument has to be positive");

#define CHECK_TRIGO_RANGE(F)                                    \
  if (value_ < -1 || value_ > 1)                                \
    throw PrimitiveError(F, "invalid range");

#define BOUNCE(F, Pos, Range)                           \
  rFloat                                                \
  Float::F()                                            \
  {                                                     \
    WHEN(Pos, CHECK_POSITIVE(SYMBOL(F)));               \
    WHEN(Range, CHECK_TRIGO_RANGE(SYMBOL(F)));          \
    return new Float(::F(value_get()));                 \
  }

  BOUNCE(acos,  false, true);
  BOUNCE(asin,  false, true);
  BOUNCE(atan,  false, false);
  BOUNCE(cos,   false, false);
  BOUNCE(exp,   false, false);
  BOUNCE(fabs,  false, false);
  BOUNCE(log,   true,  false);
  BOUNCE(round, false, false);
  BOUNCE(sin,   false, false);
  BOUNCE(sqrt,  true,  false);
  BOUNCE(tan,   false, false);
  BOUNCE(trunc, false, false);

#undef CHECK_POSITIVE
#undef CHECK_TRIGO_RANGE
#undef BOUNCE

  rFloat
  Float::random ()
  {
    value_type res = 0.f;
    const long long range = libport::to_long_long (value_get());
    if (range)
      res = rand () % range;
    return new Float(res);
  }

  rFloat Float::minus(objects_type args)
  {
    CHECK_ARG_COUNT_RANGE(0, 1);
    if (args.empty())
      return new Float(-value_get());
    else
    {
      type_check<Float>(args[0], SYMBOL(MINUS));
      return new Float(value_get() - args[0]->as<Float>()->value_get());
    }
  }

  rFloat Float::set(rFloat rhs)
  {
    value_get() = rhs->value_get();
    return this;
  }

  void Float::initialize(CxxObject::Binder<Float>& bind)
  {
    bind(SYMBOL(CARET), &Float::operator^);
    bind(SYMBOL(GT_GT), &Float::operator>>);
    bind(SYMBOL(LT), static_cast<rObject (Float::*)(rFloat)>(&Float::operator<));
    bind(SYMBOL(LT_LT), &Float::operator<<);
    bind(SYMBOL(MINUS), &Float::minus);
    bind(SYMBOL(PERCENT), &Float::operator%);
    bind(SYMBOL(PLUS), &Float::operator+);
    bind(SYMBOL(SLASH), &Float::operator/);
    bind(SYMBOL(STAR), &Float::operator*);
    bind(SYMBOL(STAR_STAR), &Float::pow);
    bind(SYMBOL(abs), &Float::fabs);
    bind(SYMBOL(acos), &Float::acos);
    bind(SYMBOL(asString), &Float::as_string);
    bind(SYMBOL(asin), &Float::asin);
    bind(SYMBOL(atan), &Float::atan);
    bind(SYMBOL(bitand), &Float::operator&);
    bind(SYMBOL(bitor), &Float::operator|);
    bind(SYMBOL(compl), &Float::operator~);
    bind(SYMBOL(cos), &Float::cos);
    bind(SYMBOL(exp), &Float::exp);
    bind(SYMBOL(inf), &Float::inf);
    bind(SYMBOL(log), &Float::log);
    bind(SYMBOL(nan), &Float::nan);
    bind(SYMBOL(random), &Float::random);
    bind(SYMBOL(round), &Float::round);
    bind(SYMBOL(set), &Float::set);
    bind(SYMBOL(sin), &Float::sin);
    bind(SYMBOL(sqrt), &Float::sqrt);
    bind(SYMBOL(tan), &Float::tan);
    bind(SYMBOL(trunc), &Float::trunc);
  }

  bool Float::float_added = CxxObject::add<Float>("Float", float_class);
  const std::string Float::type_name = "Float";
  std::string Float::type_name_get() const
  {
    return type_name;
  }

}; // namespace object
