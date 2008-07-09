/**
 ** \file object/string-class.cc
 ** \brief Creation of the URBI object string.
 */

#include <libport/escape.hh>
#include <libport/lexical-cast.hh>
#include <libport/tokenizer.hh>

#include <object/float-class.hh>
#include <object/list-class.hh>
#include <object/object.hh>
#include <object/primitives.hh>
#include <runner/runner.hh>
#include <object/string-class.hh>

namespace object
{
  rObject string_class;

  /*--------------------.
  | String primitives.  |
  `--------------------*/

  String::String()
    : content_()
  {
    proto_add(string_class);
  }

  String::String(rString model)
    : content_(model->content_)
  {
    proto_add(string_class);
  }

  String::String(const value_type& v)
    : content_(v)
  {
    assert(string_class);
    proto_add(string_class);
  }

  const String::value_type& String::value_get() const
  {
    return content_;
  }

  String::value_type& String::value_get()
  {
    return content_;
  }

  std::string String::plus (runner::Runner& r, rObject rhs)
  {
    rObject str = urbi_call(r, rhs, SYMBOL(asString));
    type_check<String>(str, SYMBOL(PLUS));
    return content_.name_get() + str->as<String>()->value_get().name_get();
  }

  rFloat String::size ()
  {
    return new Float(content_.name_get().length());
  }

  rString as_printable (rObject from)
  {
    if (from.get() == string_class.get())
      return as_string(from);
    else
    {
      type_check<String>(from, SYMBOL(asPrintable));
      rString str = from->as<String>();
      return new String(
        libport::Symbol('"'
                        + string_cast(libport::escape(str->value_get()))
                        + '"'));
    }
  }

  rString
  as_string (rObject from)
  {
    if (from.get() == string_class.get())
      return new String(SYMBOL(LT_String_GT));
    else
    {
      type_check<String>(from, SYMBOL(asString));
      return from->as<String>();
    }
  }

  bool
  String::lt(const std::string& rhs)
  {
    return value_get().name_get() < rhs;
  }

  std::string
  String::set(const std::string& rhs)
  {
    content_ = libport::Symbol(rhs);
    return rhs;
  }

  std::string
  String::fresh ()
  {
    return libport::Symbol::fresh(value_get()).name_get();
  }

  rList
  String::split(const std::string& sep)
  {
    boost::tokenizer< boost::char_separator<char> > tok =
      libport::make_tokenizer(value_get().name_get(),
                              sep.c_str());
    List::value_type ret;
    foreach(const std::string& i, tok)
      ret.push_back(new String(libport::Symbol(i)));
    return new List(ret);
  }

  std::string String::format(runner::Runner& r, rList _values)
  {
    const char* str = content_.name_get().c_str();
    const char* next;
    std::string res;

    const objects_type& values = _values->value_get();
    objects_type::const_iterator it  = values.begin();
    objects_type::const_iterator end = values.end();

    while ((next = strchr(str, '%')))
    {
      res += std::string(str, next - str);
      str = next + 2;
      const char format = next[1];
      if (format == 0)
        throw PrimitiveError(SYMBOL(PERCENT), "Trailing %");
      switch (format)
      {
        case '%':
          res += '%';
          break;
        case 's':
        {
          if (it == end)
            throw PrimitiveError(SYMBOL(PERCENT), "Too few arguments for format");
          rObject as_string = urbi_call(r, *it, SYMBOL(asString));
          res += as_string->as<String>()->value_get();
          it++;
          break;
        }
        default:
          throw PrimitiveError(SYMBOL(PERCENT),
                               std::string("Unrecognized format: %") + format);
          break;
      }
    }
    if (it != end)
      throw PrimitiveError(SYMBOL(PERCENT), "Too many arguments for format");
    res += str;
    return res;
  }

  void String::initialize(CxxObject::Binder<String>& bind)
  {
    bind(SYMBOL(asPrintable), &as_printable);
    bind(SYMBOL(asString), &as_string);
    bind(SYMBOL(fresh), &String::fresh);
    bind(SYMBOL(LT), &String::lt);
    bind(SYMBOL(PERCENT), &String::format);
    bind(SYMBOL(PLUS), &String::plus);
    bind(SYMBOL(set), &String::set);
    bind(SYMBOL(size), &String::size);
    bind(SYMBOL(split), &String::split);
  }

  bool String::string_added = CxxObject::add<String>("String", string_class);
  const std::string String::type_name = "String";
  std::string String::type_name_get() const
  {
    return type_name;
  }

}; // namespace object
