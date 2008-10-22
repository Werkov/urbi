/*! \file uobject.cc
 *******************************************************************************

 File: uobject.cc\n
 Implementation of the UObject class.

 This file is part of LIBURBI\n
 (c) Jean-Christophe Baillie, 2004-2006.

 Permission to use, copy, modify, and redistribute this software for
 non-commercial use is hereby granted.

 This software is provided "as is" without warranty of any kind,
 either expressed or implied, including but not limited to the
 implied warranties of fitness for a particular purpose.

 For more information, comments, bug reports: http://www.urbiforge.com

 **************************************************************************** */

#include <iostream>
#include <sstream>
#include <list>

#include "libport/cli.hh"
#include "libport/package-info.hh"
#include "libport/program-name.hh"
#include "libport/sysexits.hh"

#include "urbi/uobject.hh"
#include "urbi/usyncclient.hh"
#include "urbi/uexternal.hh"

//#define LIBURBIDEBUG

//! Global definition of the starterlist
namespace urbi
{
  UObject* dummyUObject;

  UCallbackAction dispatcher(const UMessage& msg);
  UCallbackAction debug(const UMessage& msg);


  std::ostream& unarmorAndSend(const char* a);

  void uobject_unarmorAndSend(const char* a)
  {
    unarmorAndSend(a);
  }

  void send(const char* a)
  {
    std::ostream& s = getDefaultClient() == 0 ? std::cerr
      : ((UAbstractClient*)getDefaultClient())->getStream();
    s << a;
  }

  void send(void* buf, int size)
  {
    std::ostream& s = getDefaultClient() == 0 ? std::cerr
      : ((UAbstractClient*)getDefaultClient())->getStream();
    s.rdbuf()->sputn(static_cast<char*> (buf), size);
  }

  // **************************************************************************
  //  Monitoring functions

  //! Generic UVar monitoring without callback
  void
  UObject::USync(UVar&)
  {
    //UNotifyChange(v, &UObject::voidfun);
  }

  // **************************************************************************
  //! UObject constructor.
  UObject::UObject(const std::string& s)
    : __name(s),
      classname(s),
      derived(false),
      gc (0),
      remote (true),
      load(s, "load")
  {
    objecthub = 0;
    autogroup = false;
    std::stringstream ss;
    ss << "class " << __name << "{};";
    ss << "external object " << __name << ";";
    URBI(()) << ss.str();
    period = -1;

    // default
    load = 1;
  }

  //! Dummy UObject constructor.
  UObject::UObject(int index)
    : derived(false),
      gc (0),
      remote (true)
  {
    std::stringstream ss;
    ss << "dummy" << index;
    __name = ss.str();
    classname = __name;
    objecthub = 0;
    autogroup = false;
    period = -1;
  }


  //! UObject cleaner
  void
  UObject::clean()
  {
    cleanTable(monitormap, __name);
    cleanTable(accessmap, __name);
    cleanTable(functionmap, __name);
    cleanTable(eventmap, __name);
    cleanTable(eventendmap, __name);

    if (objecthub)
      objecthub->members.remove(this);
  }


  //! UObject destructor.
  UObject::~UObject()
  {
    clean();
  }

  void
  UObject::UJoinGroup(const std::string& gpname)
  {
    std::string groupregister = "addgroup " + gpname +" { "+__name+"};";
    uobject_unarmorAndSend(groupregister.c_str());
  }

  void
  UObject::USetUpdate(ufloat t)
  {
    std::ostringstream os;
    // Forge names for callback and tag
    std::string tagName = "maintimer_" + __name;
    std::string cbName = __name + ".maintimer";
    std::string cbFullName = cbName + "__0";

    // Stop any previous update
    os << "stop " << tagName << ";";
    URBI(()) << os.str ();

    // Find previous update timer on this object
    std::list<UGenericCallback*>& cblist = (*eventmap) [cbFullName];
    std::list<UGenericCallback*>::iterator it = cblist.begin ();
    for (; it != cblist.end () && (*it)->getName () != cbFullName ; ++it)
      ;

    // Delete if found
    if (it != cblist.end ())
    {
      cblist.erase (it);
      delete (*it);
    }

    // Set period value
    period = t;
    // Do nothing more if negative value given
    if (period < 0)
      return;

    // FIXME: setting update at 0 put the kernel in infinite loop
    //        and memory usage goes up to 100%
    if (period == 0)
      period = 1;

    // Create callback
    createUCallback(__name, "event",
		    this, &UObject::update, cbName, eventmap, false);

    // Set update at given period
    os.str ("");
    os.clear ();
    os << tagName << ": every(" << period << ") "
      "{ emit " << cbName << ";},";
    URBI(()) << os.str ();

    return;
  }

  // This part is specific for standalone linux objects
  // LIBURBI 'Module mode'

  UCallbackAction
  dispatcher(const UMessage& msg)
  {
    //check message type
    if (msg.type != MESSAGE_DATA || msg.value->type != DATA_LIST)
    {
      msg.client.printf("Component Error: "
			"unknown message content, type %d\n",
			(int) msg.type);
      return URBI_CONTINUE;
    }

    UList& array = *msg.value->list;

    if (array.size()<2)
    {
      msg.client.printf("Component Error: Invalid number "
			"of arguments in the server message: %d\n",
			array.size());
      return URBI_CONTINUE;
    }

    if (array[0].type != DATA_DOUBLE)
    {
      msg.client.printf("Component Error: "
			"unknown server message type %d\n",
			(int) array[0].type);
      return URBI_CONTINUE;
    }

    // UEM_ASSIGNVALUE
    if ((USystemExternalMessage)(int)array[0] == UEM_ASSIGNVALUE)
    {
      UVarTable::iterator varmapfind = varmap->find(array[1]);
      if (varmapfind != varmap->end())
	for (std::list<UVar*>::iterator it = varmapfind->second.begin();
	     it != varmapfind->second.end();
	     ++it)
	  (*it)->__update(array[2]);

      UTable::iterator monitormapfind = monitormap->find(array[1]);
      if (monitormapfind != monitormap->end())
	for (std::list<UGenericCallback*>::iterator
	       cbit = monitormapfind->second.begin();
	     cbit != monitormapfind->second.end();
	     ++cbit)
	{
	  // test of return value here
	  UList u;
	  u.array.push_back(new UValue());
	  u[0].storage = (*cbit)->storage;
	  (*cbit)->__evalcall(u);
	}
    }

    // UEM_EVALFUNCTION
    else if ((USystemExternalMessage)(int)array[0] == UEM_EVALFUNCTION)
    {
      /* For the moment, this iteration is useless since the list will
       * contain one and only one element. There is no function overloading
       * yet and still it would probably use a unique name identifier, hence
       * a single element list again. */
      if (functionmap->find(array[1]) != functionmap->end())
      {
	std::list<UGenericCallback*> tmpfun = (*functionmap)[array[1]];
	std::list<UGenericCallback*>::iterator tmpfunit = tmpfun.begin();
	array.setOffset(3);
	UValue retval = (*tmpfunit)->__evalcall(array);
	array.setOffset(0);
	std::stringstream os;
	if (retval.type == DATA_VOID)
	  os << "var " << (std::string) array[2];
	else
	{
	  os << "var " << (std::string) array[2] << "=" << retval;
	}
	os << ";";
	URBI(()) << os.str();
      }
      else
	msg.client.printf("Component Error: %s function unknown.\n",
			  ((std::string) array[1]).c_str());
    }

    // UEM_EMITEVENT
    else if ((USystemExternalMessage)(int)array[0] == UEM_EMITEVENT)
    {
      if (eventmap->find(array[1]) != eventmap->end())
      {
	std::list<UGenericCallback*> tmpfun = (*eventmap)[array[1]];
	for (std::list<UGenericCallback*>::iterator i = tmpfun.begin();
	     i != tmpfun.end();
	     ++i)
	{
	  array.setOffset(2);
	  (*i)->__evalcall(array);
	  array.setOffset(0);
	}
      }
    }

    // UEM_ENDEVENT
    else if ((USystemExternalMessage)(int)array[0] == UEM_ENDEVENT)
    {
      if (eventendmap->find(array[1]) != eventendmap->end())
      {
	std::list<UGenericCallback*> tmpfun = (*eventendmap)[array[1]];
	for (std::list<UGenericCallback*>::iterator i = tmpfun.begin();
	     i != tmpfun.end();
	     ++i)
	{
	  array.setOffset(2);
	  (*i)->__evalcall(array);
	  array.setOffset(0);
	}
      }
    }

    // UEM_NEW
    else if ((USystemExternalMessage)(int)array[0] == UEM_NEW)
    {
      std::list<baseURBIStarter*>::iterator found = objectlist->end();
      for (std::list<baseURBIStarter*>::iterator i = objectlist->begin();
	   i != objectlist->end();
	   ++i)
	if ((*i)->name == (std::string)array[2])
	  if (found != objectlist->end())
	    msg.client.printf("Double object definition %s\n",
			      (*i)->name.c_str());
	  else
	    found = i;

      if (found == objectlist->end())
	msg.client.printf("Unknown object definition %s\n",
			  ((std::string) array[2]).c_str());
      else
	(*found)->copy((std::string) array[1]);

    }

    // UEM_DELETE
    else if ((USystemExternalMessage)(int)array[0] == UEM_DELETE)
    {
      std::list<baseURBIStarter*>::iterator found = objectlist->end();
      for (std::list<baseURBIStarter*>::iterator i = objectlist->begin();
	   i != objectlist->end();
	   ++i)
	if ((*i)->name == (std::string)array[1])
	  if (found != objectlist->end())
	    msg.client.printf("Double object definition %s\n",
			      (*i)->name.c_str());
	  else
	    found = i;

      if (found == objectlist->end())
	msg.client.printf("Unknown object definition %s\n",
			  ((std::string) array[1]).c_str());
      else
      {
	// remove the object from objectlist or terminate
	// the component if there is nothing left
	if (objectlist->size() == 1)
	  exit(0);
	else
	{
	  // delete the object
	  delete (*found);
	}
      }
    }


    // DEFAULT
    else
      msg.client.printf("Component Error: "
			"unknown server message type number %d\n",
			(int)array[0]);

    return URBI_CONTINUE;
  }


  int
  UObject::send (const std::string& s)
  {
    URBI(()) << s;
    return 0;
  }

  // **************************************************************************

  UObjectHub::~UObjectHub()
  {
  }

  void
  UObjectHub::USetUpdate(ufloat t)
  {
    period = t;
    // nothing happend in remote mode...
  }

  //! echo method
  void
  echo(const char* format, ...)
  {
    va_list arg;
    va_start(arg, format);
    vfprintf(stderr, format, arg);
    va_end(arg);
  }



  // **************************************************************************
  // Other functions

  UCallbackAction
  debug(const UMessage& msg)
  {
    std::stringstream mesg;
    mesg<<msg;
    msg.client.printf("DEBUG: got a message  : %s\n",
		      mesg.str().c_str());

    return URBI_CONTINUE;
  }

  UCallbackAction
  endProgram(const UMessage& msg)
  {
    std::stringstream mesg;
    mesg<<msg;
    msg.client.printf("ERROR: got a disconnection message  : %s\n",
		      mesg.str().c_str());
    exit(1);
    return URBI_CONTINUE; //stupid gcc
  }

  static
  void
  usage (const char* name)
  {
    std::cout <<
      "usage:\n" << name << " [OPTION]...\n"
      "\n"
      "Options:\n"
      "  -b, --buffer SIZE  input buffer size"
		 << " [" << UAbstractClient::URBI_BUFLEN << "]\n"
      "  -h, --help         display this message and exit\n"
      "  -H, --host ADDR    server host name   [localhost]\n"
      "      --server       put remote in server mode\n"
      "  -p, --port PORT    tcp port URBI will listen to"
		 << " [" << UAbstractClient::URBI_PORT << "]\n"
      "  -v, --version      print version information and exit\n"
      "  -d, --disconnect   exit program if disconnected(defaults)\n"
      "  -s, --stay-alive   do not exit program if disconnected\n"
		 << libport::exit (EX_OK);
  }

  static
  void
  version ()
  {
    std::cout << urbi::package_info() << std::endl;
    exit (0);
  }

  int
  initialize(const char* addr, int port, int buflen,
	     bool exitOnDisconnect, bool server)
  {
    std::cerr << libport::program_name
	      << ": " << urbi::package_info() << std::endl
	      << libport::program_name
	      << ": Remote Component Running on "
	      << addr << " " << port << std::endl;

    new USyncClient(addr, port, buflen, server);

    if (exitOnDisconnect)
    {
      if (!getDefaultClient() || getDefaultClient()->error())
	std::cerr << "ERROR: failed to connect, exiting..." << std::endl
		  << libport::exit(1);
      getDefaultClient()->setClientErrorCallback(callback (&endProgram));
    }
    if (!getDefaultClient() || getDefaultClient()->error())
      return 1;

#ifdef LIBURBIDEBUG
    getDefaultClient()->setWildcardCallback(callback (&debug));
#else
    getDefaultClient()->setErrorCallback(callback (&debug));
#endif

    getDefaultClient()->setCallback(&dispatcher,
				    externalModuleTag.c_str());

    // Wait for client to be connected if in server mode
    while (getDefaultClient () &&
	   !getDefaultClient()->error () &&
	   !getDefaultClient()->init ())
      usleep(20000);

    dummyUObject = new UObject (0);
    for (UStartlist::iterator i = objectlist->begin();
	 i != objectlist->end();
	 ++i)
      (*i)->init((*i)->name);
    return 0;
  }

  namespace
  {
    static
    void
    argument_with_option(const char* longopt,
                         char shortopt,
                         const char* val)
    {
      std::cerr
        << libport::program_name
        << ": warning: arguments without options are deprecated"
        << std::endl
        << "use `-" << shortopt << ' ' << val << '\''
        << " or `--" << longopt << ' ' << val << "' instead"
        << std::endl
        << "Try `" << libport::program_name << " --help' for more information."
        << std::endl;
    }
  }

  int
  main(int argc, const char* argv[], bool block)
  {
    libport::program_name = argv[0];

    const char* addr = "localhost";
    bool exitOnDisconnect = true;
    int port = UAbstractClient::URBI_PORT;
    bool server = false;
    int buflen = UAbstractClient::URBI_BUFLEN;

    // The number of the next (non-option) argument.
    int argp = 1;
    for (int i = 1; i < argc; ++i)
    {
      std::string arg = argv[i];
      if (arg == "--buffer" || arg == "-b")
	buflen = libport::convert_argument<unsigned> (arg, argv[++i]);
      else if (arg == "--disconnect" || arg == "-d")
	exitOnDisconnect = true;
      else if (arg == "--stay-alive" || arg == "-s")
	exitOnDisconnect = false;
      else if (arg == "--help" || arg == "-h")
	usage (argv[0]);
      else if (arg == "--host" || arg == "-H")
      {
	// libport::convert_argument exits for us on error.
	libport::convert_argument<std::string>(arg, argv[++i]);
	addr = argv[i];
      }
      else if (arg == "--port" || arg == "-p")
	port = libport::convert_argument<unsigned> (arg, argv[++i]);
      else if (arg == "--server")
	server = true;
      else if (arg == "--version" || arg == "-v")
	version ();
      else if (arg[0] == '-')
	libport::invalid_option (arg);
      else
	// A genuine argument.
	switch (argp++)
	{
	  case 1:
	    addr = argv[i];
            argument_with_option("host", 'H', addr);
	    break;
	  case 2:
	    port = libport::convert_argument<unsigned> ("port", argv[i]);
            argument_with_option("port", 'p', addr);
	    break;
	  default:
	    std::cerr << "unexpected argument: " << arg << std::endl
		      << libport::exit (EX_USAGE);
	}
    }

   initialize(addr, port, buflen, exitOnDisconnect, server);

   if (block)
     while (true)
       usleep(30000000);
    return 0;
  }

} // namespace urbi
