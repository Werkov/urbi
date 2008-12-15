/*! \file libuco/uvalue-common.cc
 *******************************************************************************

 File: uvalue-common.cc\n
 Implementation of the UValue class and other linked classes

 This file is part of LIBURBI\n
 (c) Jean-Christophe Baillie, 2004-2006.

 Permission to use, copy, modify, and redistribute this software for
 non-commercial use is hereby granted.

 This software is provided "as is" without warranty of any kind,
 either expressed or implied, including but not limited to the
 implied warranties of fitness for a particular purpose.

 For more information, comments, bug reports: http://www.urbiforge.com

 **************************************************************************** */

#include <sstream>
#include <iostream>
#include <cstdlib>

#include <libport/assert.hh>
#include <libport/cstring>
#include <libport/escape.hh>

#include <urbi/uvalue.hh>

namespace urbi
{

  /*---------------.
  | UValue Casts.  |
  `---------------*/

  UVar&
  uvalue_caster<UVar>::operator() (UValue& v)
  {
    return *((UVar*)v.storage);
  }

  UBinary
  uvalue_caster<UBinary>::operator() (UValue& v)
  {
    if (v.type != DATA_BINARY)
      return UBinary();
    return UBinary(*v.binary);
  }

  UList
  uvalue_caster<UList>::operator() (UValue& v)
  {
    if (v.type != DATA_LIST)
      return UList();
    return UList(*v.list);
  }

  UObjectStruct
  uvalue_caster<UObjectStruct>::operator() (UValue& v)
  {
    if (v.type != DATA_OBJECT)
      return UObjectStruct();
    return UObjectStruct(*v.object);
  }

  const char*
  uvalue_caster<const char*>::operator() (UValue& v)
  {
    if (v.type != DATA_STRING)
      return "invalid";
    return v.stringValue->c_str();
  }


  /*---------.
  | UValue.  |
  `---------*/

#define SKIP_SPACES()				\
    while (message[pos] == ' ')			\
      ++pos

  int
  UValue::parse(const char* message, int pos,
		const std::list<BinaryData>& bins,
		std::list<BinaryData>::const_iterator& binpos)
  {
    SKIP_SPACES();
    if (message[pos] == '"')
    {
      //string
      type = DATA_STRING;
      //get terminating '"'
      int p = pos + 1;
      while (message[p] && message[p] != '"')
      {
	if (message[p] == '\\')
	  ++p;
	++p;
      }
      if (!message[p])
	return -p; //parse error

      stringValue = new std::string(
        libport::unescape(std::string(message + pos + 1, p - pos - 1)));
      return p + 1;
    }

    if (message[pos] == '[')
    {
      //list message
      type = DATA_LIST;
      list = new UList();
      ++pos;
      while (message[pos])
      {
	SKIP_SPACES();
	if (message[pos] == ']')
	  break;
	UValue* v = new UValue();
	int p = v->parse(message, pos, bins, binpos);
	if (p < 0)
	  return p;
	list->array.push_back(v);
	pos = p;
	SKIP_SPACES();
	//expect , or rbracket
	if (message[pos] == ']')
	  break;
	if (message[pos] != ',')
	  return -pos;
	++pos;
      }

      if (message[pos] != ']')
	return -pos;
      return pos + 1;
    }

    //OBJ a [x:12, y:4]
    if (STRNEQ(message + pos, "OBJ ", 4))
    {
      //obj message
      pos += 4;
      type = DATA_OBJECT;
      object = new UObjectStruct();
      SKIP_SPACES();
      if (message[pos] != '[')
	return -pos;
      ++pos;

      while (message[pos])
      {
	SKIP_SPACES();
	if (message[pos] == ']')
	  break; //empty object
	//parse name
	int p = pos;
	while (message[p] && message[p] != ':')
	  ++p;
	if (!message[p])
	  return -p; //parse error
	++p;
	UNamedValue nv;
	nv.name = std::string(message + pos, p - pos - 1);
	pos = p;
	SKIP_SPACES();
	UValue* v = new UValue();
	p = v->parse(message, pos, bins, binpos);
	if (p < 0)
	  return p;
	nv.val = v;
	object->array.push_back(nv);
	pos = p;
	SKIP_SPACES();
	//expect , or rbracket
	if (message[pos] == ']')
	  break;
	if (message[pos] != ',')
	  return -pos;
	++pos;
      }

      if (message[pos] != ']')
	return -pos;
      return pos + 1;
    }

    if (STRNEQ(message+pos, "void", 4))
    {
      //void
      type = DATA_VOID;
      pos += 4;
      return pos;
    }

    if (STRNEQ(message + pos, "BIN ", 4))
    {
      //binary message: delegate
      type = DATA_BINARY;
      binary = new UBinary();
      pos += 4;
      //parsing type
      int p = binary->parse(message, pos, bins, binpos);
      return p;
    }

    // true and false used by k2
    if (STRNEQ(message + pos, "true", 4))
    {
      type = DATA_DOUBLE;
      val = 1;
      return pos+4;
    }
    if (STRNEQ(message + pos, "false", 5))
    {
      type = DATA_DOUBLE;
      val = 0;
      return pos+5;
    }

    //last attempt: double
    int p;
    int count = sscanf(message+pos, "%lf%n", &val, &p);
    if (!count)
      return -pos;
    type = DATA_DOUBLE;
    pos += p;
    return pos;
  }

  std::ostream&
  UValue::print (std::ostream& s) const
  {
    switch (type)
    {
      case DATA_DOUBLE:
	s << (float) val;
	break;
      case DATA_STRING:
	s << '"' << libport::escape(*stringValue, '"') << '"';
	break;
      case DATA_BINARY:
	if (binary->type != BINARY_NONE
	    && binary->type != BINARY_UNKNOWN)
	  binary->buildMessage();
	s << "BIN "<< binary->common.size << ' ' << binary->message << ';';
	s.write((char*) binary->common.data, binary->common.size);
	break;
      case DATA_LIST:
      {
	s << '[';
	unsigned sz = list->size();
	for (unsigned i = 0; i < sz; ++i)
	  s << (*list)[i]
	    << (i != sz - 1 ? ", " : "");
	s << ']';
      }
      break;
      case DATA_OBJECT:
      {
	s << "OBJ "<<object->refName<<" [";
	unsigned sz = object->size();
	for (unsigned i = 0; i < sz; ++i)
	  s << (*object)[i].name << ':' << (*object)[i].val
	    << (i != sz - 1 ? ", " : "");
	s << ']';
      }
      break;
      default:
	s << "<<void>>";
    }
    return s;
  }


  /*---------.
  | USound.  |
  `---------*/

  const char* USound::format_string() const
  {
    switch (soundFormat)
    {
      case SOUND_RAW:
	return "raw";
      case SOUND_WAV:
	return "wav";
      case SOUND_MP3:
	return "mp3";
      case SOUND_OGG:
	return "ogg";
      case SOUND_UNKNOWN:
	return "unknown format";
    }
    // To pacify "warning: control reaches end of non-void function".
    // FIXME: This should not abort. UImage should be initialized with IMAGE_UKNOWN.
    //        This is not done because data is stored in an union in UBinary and
    //        union members cannot have constructors.
    return "unknown format";
  }

  USound::operator std::string() const
  {
    std::ostringstream tstr;
    tstr << "sound(format: " << format_string() << ", "
	 << "size: " << size << ", "
	 << "channels: " << channels << ", "
	 << "rate: " << rate << ", "
	 << "sample size: " << sampleSize << ", "
	 << "sample format: ";
    switch (sampleFormat)
    {
      case SAMPLE_SIGNED:
	tstr << "signed";
	break;

      case SAMPLE_UNSIGNED:
	tstr << "unsigned";
	break;

      default:
	tstr << "unknown[" << (int)sampleFormat << "]";
    }
    tstr << ")";
    return tstr.str();
  }

  /*---------.
  | UImage.  |
  `---------*/

  const char* UImage::format_string () const
  {
    switch (imageFormat)
    {
      case IMAGE_RGB:
	return "rgb";
      case IMAGE_JPEG:
	return "jpeg";
      case IMAGE_YCbCr:
	return "YCbCr";
      case IMAGE_PPM:
	return "ppm";
      case IMAGE_UNKNOWN:
	return "unknown format";
    }
    // To pacify "warning: control reaches end of non-void function".
    // pabort(imageFormat);
    // FIXME: This should not abort. UImage should be initialized with IMAGE_UKNOWN.
    //        This is not done because data is stored in an union in UBinary and
    //        union members cannot have constructors.
    return "unknown format";
  }

  /*----------.
  | UBinary.  |
  `----------*/

  int
  UBinary::parse(const char* message, int pos,
		 const std::list<BinaryData>& bins,
		 std::list<BinaryData>::const_iterator& binpos)
  {
    SKIP_SPACES();
    //find end of header

    if (binpos == bins.end()) //no binary data available
      return -1;

    //validate size
    int ps;
    unsigned psize;
    int count = sscanf(message+pos, "%u%n", &psize, &ps);
    if (count != 1)
      return -pos;
    if (psize != binpos->size)
    {
      std::cerr << "bin size inconsistency\n";
      return -pos;
    }
    pos += ps;
    common.size = psize;
    common.data = malloc(psize);
    memcpy(common.data, binpos->data, common.size);
    ++binpos;
    while (message[pos] == ' ')
      ++pos;
    int p = pos;
    while (message[p] && message[p] != '\n')
      ++p;
    if (!message[p])
      return -p; //parse error
    this->message = std::string(message + pos, p - pos);
    ++p;

    //trying to parse header to find type
    char type[64];
    memset(type, 0, 64);
    // width, height, sample size.
    size_t p2, p3, p4;
    // sample format.
    int p5;
    count = sscanf(message + pos,
                   "%63s %zu %zu %zu %d",
		   type, &p2, &p3, &p4, &p5);

    if (STREQ(type, "jpeg"))
    {
      this->type = BINARY_IMAGE;
      image.size = common.size;
      image.width = p2;
      image.height = p3;
      image.imageFormat = IMAGE_JPEG;
      return p;
    }

    if (STREQ(type, "YCbCr"))
    {
      this->type = BINARY_IMAGE;
      image.size = common.size;
      image.width = p2;
      image.height = p3;
      image.imageFormat = IMAGE_YCbCr;
      return p;
    }

    if (STREQ(type, "rgb"))
    {
      this->type = BINARY_IMAGE;
      image.size = common.size;
      image.width = p2;
      image.height = p3;
      image.imageFormat = IMAGE_RGB;
      return p;
    }

    if (STREQ(type, "raw"))
    {
      this->type = BINARY_SOUND;
      sound.soundFormat = SOUND_RAW;
      sound.size = common.size;
      sound.channels = p2;
      sound.rate = p3;
      sound.sampleSize = p4;
      sound.sampleFormat = (USoundSampleFormat) p5;
      return p;
    }

    if (STREQ(type, "wav"))
    {
      this->type = BINARY_SOUND;
      sound.soundFormat = SOUND_WAV;
      sound.size = common.size;
      sound.channels = p2;
      sound.rate = p3;
      sound.sampleSize = p4;
      sound.sampleFormat = (USoundSampleFormat) p5;
      return p;
    }

    //unknown binary
    this->type = BINARY_UNKNOWN;
    return p;
  }

  void UBinary::buildMessage()
  {
    message = getMessage();
  }

  std::string UBinary::getMessage() const
  {
    std::ostringstream o;
    switch (type)
    {
      case BINARY_IMAGE:
	o << image.format_string()
	  << ' ' << image.width << ' ' << image.height;
	break;

      case BINARY_SOUND:
	o << sound.format_string()
	  << ' ' << sound.channels
	  << ' ' << sound.rate
	  << ' ' << sound.sampleSize
	  << ' ' << sound.sampleFormat;
	break;

      case BINARY_UNKNOWN:
	o << message;
	break;

      case BINARY_NONE:
	break;
    }
    return o.str();
  }


  /*---------.
  | UValue.  |
  `---------*/

  //! Class UValue implementation
  UValue::UValue()
    : type(DATA_VOID), val (0), storage(0)
  {}

  // All the following mess could be avoid if we used templates...
  // boost::any could be very useful here.  Some day, hopefully...

#define UVALUE_OPERATORS(Args, DataType, Field, Value)	\
  UValue::UValue(Args)					\
    : type(DataType), Field(Value)			\
  {}							\
							\
  UValue& UValue::operator=(Args)			\
  {							\
    passert (type, type == DATA_VOID);			\
    type = DataType;					\
    Field = Value;					\
    return *this;					\
  }

#define UVALUE_DOUBLE(Type)			\
  UVALUE_OPERATORS(Type v, DATA_DOUBLE, val, v)

  UVALUE_DOUBLE(ufloat)
  UVALUE_DOUBLE(int)
  UVALUE_DOUBLE(unsigned int)
  UVALUE_DOUBLE(long)
  UVALUE_DOUBLE(unsigned long)

#undef UVALUE_DOUBLE


  /*----------------------.
  | UValue: DATA_STRING.  |
  `----------------------*/

namespace
{
  std::string*
  ptr_string (const void* v)
  {
    std::ostringstream i;
    i << "%ptr_" << (unsigned long) v;
    return new std::string(i.str());
  }
}

#define UVALUE_STRING(Type)			\
  UVALUE_OPERATORS(Type v, DATA_STRING, stringValue, new std::string(v))

  UVALUE_STRING(const char*)
  UVALUE_STRING(const std::string&)
  UVALUE_OPERATORS(const void* v, DATA_STRING, stringValue, ptr_string(v))

#undef UVALUE_STRING

  /*----------------------.
  | UValue: DATA_BINARY.  |
  `----------------------*/

#define UVALUE_BINARY(Type)			\
  UVALUE_OPERATORS(Type v, DATA_BINARY, binary, new UBinary(v))

  UVALUE_BINARY(const UBinary&)
  UVALUE_BINARY(const USound&)
  UVALUE_BINARY(const UImage&)

#undef UVALUE_BINARY

  UVALUE_OPERATORS(const UList& v, DATA_LIST, list, new UList(v))
  UVALUE_OPERATORS(const UObjectStruct& v,
		   DATA_OBJECT, object, new UObjectStruct(v))

#undef UVALUE_OPERATORS

  UValue::~UValue()
  {
    switch(type)
    {
      case DATA_STRING:
	delete stringValue;
	break;
      case DATA_BINARY:
	delete binary;
	break;
      case DATA_LIST:
	delete list;
	break;
      case DATA_OBJECT:
	delete object;
	break;
      case DATA_DOUBLE:
      case DATA_VOID:
	break;
    }
  }

  UValue::operator ufloat() const
  {
    switch (type)
    {
      case DATA_DOUBLE:
	return val;

      case DATA_STRING:
      {
	std::istringstream is(*stringValue);
	ufloat v;
	is >> v;
	return v;
      }
      break;

      case DATA_BINARY:
      case DATA_LIST:
      case DATA_OBJECT:
      case DATA_VOID:
	break;
    }

    return ufloat(0);
  }


  UValue::operator std::string() const
  {
    switch (type)
    {
      case DATA_DOUBLE:
	{
	  std::ostringstream tstr;
	  tstr << val;
	  return tstr.str();
	}

      case DATA_STRING:
	return *stringValue;

      case DATA_BINARY:
        // We cannot convert to UBinary because it is ambigous so we
        // try until we found the right type.
      {
        USound snd(*this);
        if (snd.soundFormat != SOUND_UNKNOWN)
          return snd;
        goto invalid;
      }

      invalid:
      default:
        return "invalid";
    }
  }

  UValue::operator UBinary() const
  {
    if (type != DATA_BINARY)
      return UBinary();
    else
      return *binary;
  }

  UValue::operator UImage() const
  {
    if (type != DATA_BINARY
	|| binary->type != BINARY_IMAGE)
    {
      UImage i;
      i.data = 0;
      i.size = i.width = i.height = 0;
      i.imageFormat = IMAGE_UNKNOWN;
      return i;
    }
    else
      return binary->image;
  }

  UValue::operator USound() const
  {
    if (type == DATA_BINARY && binary->type == BINARY_SOUND)
      return binary->sound;

    USound res;
    res.data = 0;
    res.size = res.sampleSize = res.channels = res.rate = 0;
    res.soundFormat = SOUND_UNKNOWN;
    res.sampleFormat = SAMPLE_UNSIGNED;
    return res;
  }

  UValue::operator UList() const
  {
    if (type != DATA_LIST)
      return UList();
    return UList(*list);
  }

  UValue& UValue::operator= (const UValue& v)
  {
    // TODO: optimize
    if (this == &v)
      return *this;
    switch (type)
    {
      case DATA_STRING:
	delete stringValue;
	break;
      case DATA_BINARY:
	delete binary;
	break;
      case DATA_LIST:
	delete list;
	break;
      case DATA_OBJECT:
	delete object;
      case DATA_DOUBLE:
      case DATA_VOID:
	break;
    }

    type = v.type;
    switch (type)
    {
      case DATA_DOUBLE:
	val = v.val;
	break;
      case DATA_STRING:
	stringValue = new std::string(*v.stringValue);
	break;
      case DATA_BINARY:
	binary = new UBinary(*v.binary);
	break;
      case DATA_LIST:
	list = new UList(*v.list);
	break;
      case DATA_OBJECT:
	object = new UObjectStruct(*v.object);
	break;
      case DATA_VOID:
	break;
    }
    return *this;
  }


  UValue::UValue(const UValue& v)
  {
    type = DATA_VOID;
    *this = v;
  }

  /*----------.
  | UBinary.  |
  `----------*/

  UBinary::UBinary()
  {
    common.data = 0;
    common.size = 0;
    type = BINARY_NONE;
  }

  UBinary::~UBinary()
  {
    if (common.data)
      free(common.data);
  }

  UBinary::UBinary(const UBinary& b)
  {
    type = BINARY_NONE;
    common.data = 0;
    *this = b;
  }

  UBinary::UBinary(const UImage& i)
  {
    type = BINARY_IMAGE;
    image = i;
    image.data = static_cast<unsigned char*> (malloc (image.size));
    memcpy(image.data, i.data, image.size);
  }

  UBinary::UBinary(const USound& i)
  {
    type = BINARY_SOUND;
    sound = i;
    sound.data = static_cast<char*> (malloc (sound.size));
    memcpy(sound.data, i.data, sound.size);
  }

  UBinary& UBinary::operator= (const UBinary& b)
  {
    if (this == &b)
      return *this;

    free(common.data);

    type = b.type;
    message = b.message;
    common.size = b.common.size;
    switch(type)
    {
      case BINARY_IMAGE:
	image = b.image;
	break;
      case BINARY_SOUND:
	sound = b.sound;
	break;
      case BINARY_NONE:
      case BINARY_UNKNOWN:
	break;
    }
    common.data = malloc(common.size);
    memcpy(common.data, b.common.data, b.common.size);
    return *this;
  }

  /*--------.
  | UList.  |
  `--------*/

  UList::UList()
    : offset(0)
  {}

  UList::UList(const UList& b)
    : offset(0)
  {
    *this = b;
  }

  UList& UList::operator= (const UList& b)
  {
    if (this == &b)
      return *this;
    clear();
    for (std::vector<UValue*>::const_iterator it = b.array.begin();
	 it !=b.array.end();
	 ++it)
      array.push_back(new UValue(**it));
    offset = b.offset;

    return *this;
  }

  UList::~UList()
  {
    clear();
  }

  void UList::clear()
  {
    offset = 0;
    for (int i = 0; i < size(); ++i) //relax, its a vector
      delete array[i];
    array.clear();
  }

  /*----------------.
  | UObjectStruct.  |
  `----------------*/

  UObjectStruct::UObjectStruct()
  {}

  UObjectStruct::UObjectStruct(const UObjectStruct& b)
  {
    *this = b;
  }

  UObjectStruct::~UObjectStruct()
  {
    for (int i = 0; i < size(); ++i) //relax, it's a vector
      delete array[i].val;
    array.clear();
  }

  UObjectStruct&
  UObjectStruct::operator= (const UObjectStruct& b)
  {
    if (this == &b)
      return *this;

    for (int i = 0; i < size(); ++i) // Relax, it's a vector.
      delete array[i].val;
    array.clear();

    for (std::vector<UNamedValue>::const_iterator it = b.array.begin();
	 it != b.array.end();
	 ++it)
      array.push_back(UNamedValue(it->name, new UValue(*(it->val))));

    return *this;
  }

  UValue&
  UObjectStruct::operator[] (const std::string& s)
  {
    for (int i = 0; i < size(); ++i)
      if (array[i].name == s)
	return *array[i].val;
    static UValue n;
    return n; // Gni?
  }

} // namespace urbi
