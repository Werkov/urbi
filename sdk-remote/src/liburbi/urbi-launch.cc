/*
 * Copyright (C) 2009, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */
#include <string>
#include <libport/cassert>
#include <cstdarg>
#include <libport/cstdlib>
#include <iostream>
#include <stdexcept>

#include <sdk/config.h>

#include <boost/assign/list_of.hpp>
using namespace boost::assign;

#include <libport/cli.hh>
#include <libport/containers.hh>
#include <libport/debug.hh>
#include <libport/file-system.hh>
#include <libport/foreach.hh>
#include <libport/path.hh>
#include <libport/package-info.hh>
#include <libport/program-name.hh>
#include <libport/sysexits.hh>
#include <libport/unistd.h>
#include <libport/windows.hh>
#include <libport/option-parser.hh>
#include <libport/xltdl.hh>

#include <urbi/exit.hh>
#include <urbi/package-info.hh>
#include <urbi/uclient.hh>

using namespace urbi;
using libport::program_name;

static UCallbackAction
onError(const UMessage& msg)
{
  GD_FERROR("%s: load module error: %s", (program_name())(msg.message));
  return URBI_CONTINUE;
}

static UCallbackAction
onDone(const UMessage&)
{
  ::exit(0);
}

static int
connect_plugin(const std::string& host, int port,
               const libport::cli_args_type& modules)
{
  UClient cl(host, port);
  if (cl.error())
    // UClient already displayed an error message.
    ::exit(1);
  cl.setErrorCallback(callback(&onError));
  cl.setCallback(callback(&onDone), "output");
  foreach (const std::string& m, modules)
    cl << "loadModule(\"" << m << "\");";
  cl << "output << 1;";
  while (true)
    sleep(1);
  return 0;
}

static void
usage(libport::OptionParser& parser)
{
  std::cout <<
    "usage: " << program_name() <<
    " [OPTIONS] MODULE_NAMES ... [-- UOPTIONS...]\n"
    "Start an UObject in either remote or plugin mode.\n"
              << parser <<
    "\n"
    "MODULE_NAMES is a list of modules.\n"
    "UOPTIONS are passed to urbi::main in remote and start modes.\n"
    "\n"
    "Exit values:\n"
    "  0  success\n"
    " " << EX_NOINPUT << "  some of the MODULES are missing\n"
    " " << EX_OSFILE << "  libuobject is missing\n"
    "  *  other kinds of errors\n"
    ;
  ::exit(EX_OK);
}


static
void
version()
{
  std::cout << urbi::package_info() << std::endl
            << libport::exit(EX_OK);
}


static
int
urbi_launch_(int argc, char* argv[])
{
  libport::program_initialize(argc, argv);

  // The options passed to urbi::main.
  libport::cli_args_type args;

  args << argv[0];

  libport::OptionValue
    arg_custom("start using the shared library FILE", "custom", 'c', "FILE"),
    arg_pfile("file containing the port to listen to", "port-file", 0, "FILE");
  libport::OptionFlag
    arg_plugin("start as a plugin uobject on a running server", "plugin", 'p'),
    arg_remote("start as a remote uobject", "remote", 'r'),
    arg_start("start an urbi server and connect as plugin", "start", 's');
  libport::OptionsEnd arg_end;

  libport::OptionParser opt_parser;
  opt_parser << "Urbi-Launch options:"
	     << libport::opts::help
	     << libport::opts::version
	     << arg_custom
	     << libport::opts::debug
	     << "Mode selection:"
	     << arg_plugin
	     << arg_remote
	     << arg_start
	     << "Urbi-Launch options for plugin mode:"
	     << libport::opts::host
	     << libport::opts::port
	     << arg_pfile
	     << arg_end;

  // The list of modules.
  libport::cli_args_type modules;
  try
  {
    modules = opt_parser(libport::program_arguments());
  }
  catch (libport::Error& e)
  {
    const libport::Error::errors_type& err = e.errors();
    foreach (std::string wrong_arg, err)
      libport::invalid_option(wrong_arg);
  }

  if (libport::opts::version.get())
    version();
  if (libport::opts::help.get())
    usage(opt_parser);

  // Connection mode.
  enum ConnectMode
  {
    /// Start a new engine and plug the module
    MODE_PLUGIN_START,
    /// Load the module in a running engine as a plugin
    MODE_PLUGIN_LOAD,
    /// Connect the module to a running engine (remote uobject)
    MODE_REMOTE
  };
  ConnectMode connect_mode = MODE_REMOTE;

  if (arg_plugin.get())
    connect_mode = MODE_PLUGIN_LOAD;
  if (arg_remote.get())
    connect_mode = MODE_REMOTE;
  if (arg_start.get())
    connect_mode = MODE_PLUGIN_START;

  /// Server host name.
  std::string host = libport::opts::host.value(UClient::default_host());
  if (libport::opts::host.filled())
    args << "--host" << host;

  /// Server port.
  int port = libport::opts::port.get<int>(urbi::UClient::URBI_PORT);
  if (libport::opts::port.filled())
    args << "--port" << libport::opts::port.value();

  if (arg_pfile.filled())
  {
    std::string file = arg_pfile.value();
    if (connect_mode == MODE_PLUGIN_LOAD)
      port = libport::file_contents_get<int>(file);
    args << "--port-file" << file;
  }
  args.insert(args.end(), arg_end.get().begin(), arg_end.get().end());

  if (connect_mode == MODE_PLUGIN_LOAD)
    return connect_plugin(host, port, modules);

  libport::path urbi_root = libport::xgetenv("URBI_ROOT", URBI_ROOT);
  libport::path coredir = urbi_root / "gostai" / "core" / URBI_HOST;
  /// Core dll to use.
  std::string dll
    = (arg_custom.filled()
       ? arg_custom.value()
       :  string_cast(coredir
                      / (connect_mode == MODE_REMOTE ? "remote" : "engine")
                      / "libuobject"));

  /* The two other modes are handled the same way:
   * -Dlopen the correct libuobject.
   * -Dlopen the uobjects to load.
   * -Call urbi::main found by dlsym() in libuobject.
   */
  libport::xlt_handle core = libport::xlt_openext(dll, true, EX_OSFILE);

  // If URBI_UOBJECT_PATH is not defined, first look in ., then in the
  // stdlib.
  std::string uobject_path = libport::xgetenv("URBI_UOBJECT_PATH", ".:");

  // Load the modules using our uobject library path.
  libport::xlt_advise dl;
  dl.ext()
    .path().push_back(list_of(uobject_path)(coredir / "uobjects"), ":");
  foreach (const std::string& s, modules)
    dl.open(s);

  typedef int (*umain_type)(const libport::cli_args_type& args,
                            bool block, bool errors);
  umain_type umain = core.sym<umain_type>("urbi_main_args");
  return umain(args, true, true);
}

extern "C" int
URBI_SDK_API
urbi_launch(int argc, char* argv[])
{
  try
  {
    return urbi_launch_(argc, argv);
  }
  catch (const std::exception& e)
  {
    std::cerr << argv[0] << ": " << e.what() << std::endl
              << libport::exit(EX_FAIL);
  }
}
