#include <boost/lexical_cast.hpp>

#include <kernel/uvalue-cast.hh>

#include <object/float-class.hh>
#include <object/global-class.hh>
#include <object/list-class.hh>
#include <object/string-class.hh>
#include <object/urbi-exception.hh>

#include <urbi/uvalue.hh>

urbi::UValue uvalue_cast(object::rObject o)
{
  urbi::UValue res;
  if (object::rFloat f = o->as<object::Float>())
    res = f->value_get();
  else if (object::rString s = o->as<object::String>())
    res = s->value_get();
  else if (object::rList s = o->as<object::List>())
  {
    res.type = urbi::DATA_LIST;
    res.list = new urbi::UList;
    object::List::value_type& t = o.cast<object::List>()->value_get();
    foreach (const object::rObject& co, t)
      res.list->array.push_back(new urbi::UValue(uvalue_cast(co)));
  }
  else
  {
    if (!is_a(o, object::global_class->slot_get(SYMBOL(Binary))))
    {
      libport::shared_ptr<object::String> rs =
        o->slot_get(SYMBOL(protoName))->as<object::String>();
      std::string t = rs->value_get().name_get();
      throw object::WrongArgumentType("Binary", t, SYMBOL(cast));
    }
    else
    {
      const std::string& data =
        o->slot_get(SYMBOL(data))->
        as<object::String>()->value_get().name_get();
      const std::string& keywords =
        o->slot_get(SYMBOL(keywords))->
        as<object::String>()->value_get().name_get();
      std::list<urbi::BinaryData> l;
      l.push_back(urbi::BinaryData(const_cast<char*>(data.c_str()),
                                   data.size()));
      std::list<urbi::BinaryData>::iterator i = l.begin();
      res.type = urbi::DATA_BINARY;
      res.binary = new urbi::UBinary();
      res.binary->parse(
        (boost::lexical_cast<std::string>(data.size()) +
         " " + keywords + '\n').c_str(), 0, l, i);
      return res;
    }
  }
  return res;
}

object::rObject
object_cast(const urbi::UValue& v)
{
  object::rObject res;
  switch(v.type)
  {
    case urbi::DATA_DOUBLE:
      return new object::Float(v.val);
      break;

    case urbi::DATA_STRING:
      return new object::String(libport::Symbol(*v.stringValue));
      break;

    case urbi::DATA_LIST:
    {
      object::rList l = new object::List();
      foreach (urbi::UValue *cv, v.list->array)
	l->value_get().push_back(object_cast(*cv));
      res = l;
      break;
    }

    case urbi::DATA_BINARY:
    {
      res = new object::Object();
      res->proto_add(object::global_class->slot_get(SYMBOL(Binary)));
      std::string msg = v.binary->getMessage();
      // Trim it.
      if (!msg.empty() && msg[0] == ' ')
        msg = msg.substr(1, msg.npos);
      res->slot_set(SYMBOL(keywords),
                    new object::String(libport::Symbol(msg)));
      res->slot_set(SYMBOL(data),
                    new object::String
                    (libport::Symbol(
                      std::string(static_cast<char*>(v.binary->common.data),
                                  v.binary->common.size))));
    }
    break;

    default:
      throw
	object::WrongArgumentType("Object",
				  "Not yet supported",
                                  SYMBOL(backcast));
      break;
  }
  return res;
}
