/*
 * Copyright (C) 2009, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */
//#define ENABLE_DEBUG_TRACES
#include <libport/compiler.hh>

#include <libport/unistd.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <libport/cstring>

#include <boost/function.hpp>
#include <libport/bind.hh>

#include <libport/asio.hh>
#include <libport/cli.hh>
#include <libport/debug.hh>
#include <libport/exception.hh>
#include <libport/foreach.hh>
#include <libport/format.hh>
#ifndef NO_OPTION_PARSER
# include <libport/option-parser.hh>
# include <libport/input-arguments.hh>
# include <libport/tokenizer.hh>
# define IF_OPTION_PARSER(a, b) a
#else
 #define IF_OPTION_PARSER(a, b) b
#endif
#include <libport/package-info.hh>
#include <libport/program-name.hh>
#include <libport/read-stdin.hh>
#include <libport/semaphore.hh>
#include <libport/sys/socket.h>
#include <libport/sysexits.hh>
#include <libport/thread.hh>
#include <libport/utime.hh>
#include <libport/windows.hh>

#include <libltdl/ltdl.h>

// Inclusion order matters for windows. Leave userver.hh after network.hh.
#include <kernel/uqueue.hh>
#include <kernel/userver.hh>
#include <kernel/uconnection.hh>

#include <sched/configuration.hh>
#include <sched/scheduler.hh>

#include <kernel/connection.hh>
#include <kernel/ubanner.hh>
#include <object/symbols.hh>
#include <urbi/object/object.hh>
#include <urbi/object/float.hh>
#include <object/system.hh>
#include <urbi/export.hh>
#include <urbi/umain.hh>
#include <urbi/uobject.hh>

GD_INIT();
GD_ADD_CATEGORY(URBI);

#define URBI_EXIT(Status, Args)                 \
  throw urbi::Exit(Status, libport::format Args)

class ConsoleServer
  : public kernel::UServer
  , public libport::Socket
{
public:
  ConsoleServer(bool fast)
    : kernel::UServer("console")
    , libport::Socket(kernel::UServer::get_io_service())
    , fast(fast)
    , ctime(0)
  {}

  virtual ~ConsoleServer()
  {}

  virtual void reset()
  {}

  virtual void reboot()
  {}

  virtual libport::utime_t getTime()
  {
    return fast ? ctime : libport::utime();
  }

  virtual
  UErrorValue
  save_file(const std::string& filename, const std::string& content)
  {
    //! \todo check this code
    std::ofstream os(filename.c_str());
    os << content;
    os.close();
    return os.good() ? USUCCESS : UFAIL;
  }

  virtual
  void effectiveDisplay(const char* t)
  {
    std::cout << t;
  }

  boost::asio::io_service&
  get_io_service()
  {
    return kernel::UServer::get_io_service();
  }

  bool fast;
  libport::utime_t ctime;
};

namespace
{
#ifndef NO_OPTION_PARSER
  static
  void
  help(libport::OptionParser& parser)
  {
    std::stringstream output;
    output
      << "usage: " << libport::program_name()
      << " [OPTIONS] [PROGRAM_FILE | -- ] [ARGS...]" << std::endl
      << std::endl
      << "  PROGRAM_FILE   Urbi script to load."
      << "  `-' stands for standard input" << std::endl
      << "  ARGS           user arguments passed to PROGRAM_FILE" << std::endl
      << parser;
    throw urbi::Exit(EX_OK, output.str());
  }
#endif
  static
  void
  version()
  {
    throw urbi::Exit(EX_OK, kernel::UServer::package_info().signature());
  }

  static
  void
  forbid_option(const std::string& arg)
  {
    if (arg.size() > 1 && arg[0] == '-')
      URBI_EXIT(EX_USAGE,
                ("unrecognized command line option: %s", arg));
  }
}

namespace urbi
{

  /// Command line options needed in the main loop.
  struct LoopData
  {
    LoopData()
      : interactive(false)
      , fast(false)
      , network(true)
      , server(0)
    {}

    bool interactive;
    bool fast;
    /// Whether network connections are enabled.
    bool network;
    ConsoleServer* server;
  };

  int main_loop(LoopData& l);

  static
  libport::Socket*
  connectionFactory()
  {
    kernel::Connection* res = new kernel::Connection();
    kernel::urbiserver->connection_add(res);
    return res;
  }

  /// Data to send to the server.
  struct DataSender : libport::opts::DataVisitor
  {
    typedef libport::opts::DataVisitor super_type;
    DataSender(kernel::UServer& server, kernel::UConnection& connection)
      : server_(server)
      , connection_(connection)
    {}

    using super_type::operator();

    void
    operator()(const libport::opts::TextData& d)
    {
      connection_.recv_queue_get().push(d.command_.c_str());
    }

    void
    operator()(const libport::opts::FileData& d)
    {
      if (server_.load_file(d.filename_, connection_.recv_queue_get())
          != USUCCESS)
        URBI_EXIT(EX_NOINPUT, ("failed to process file %s", d.filename_));
    }

    kernel::UServer& server_;
    kernel::UConnection& connection_;
  };

  static
  int
  init(const libport::cli_args_type& _args, bool errors,
             libport::Semaphore* sem)
  {
    libport::Finally f;
    if (sem)
      f << boost::bind(&libport::Semaphore::operator++, sem);
    if (errors)
    {
      try
      {
        return init(_args, false, sem);
      }
      catch (const urbi::Exit& e)
      {
        std::cerr << libport::program_name() << ": " << e.what() << std::endl;
        return e.error_get();
      }
      catch (const std::exception& e)
      {
        std::cerr << libport::program_name() << ": " << e.what() << std::endl;
        return 1;
      }
    }

    GD_CATEGORY(URBI);

    libport::cli_args_type args = _args;

    object::system_set_program_name(args[0]);
    args.erase(args.begin());
#ifndef NO_OPTION_PARSER
    // Detect shebang mode
    if (!args.empty() && !args[0].empty() && args[0][1] == '-'
        && args[0].find_first_of(' ') != args[0].npos)
    { // All our arguments are in args[0]
      std::string arg0(args[0]);
      libport::cli_args_type nargs;
      foreach(const std::string& arg, libport::make_tokenizer(arg0, " "))
        nargs.push_back(arg);
      for (size_t i = 1; i< args.size(); ++i)
        nargs.push_back(args[i]);
      args = nargs;
    }
#endif
    /// The size of the stacks.
    size_t arg_stack_size = 0;

    // Parse the command line.
    LoopData data;
#ifndef NO_OPTION_PARSER
    libport::OptionFlag
      arg_fast("ignore system time, go as fast as possible",
               "fast", 'F'),
      arg_interactive("read and parse stdin in a nonblocking way",
                      "interactive", 'i'),
      arg_no_net("ignored for backward compatibility", "no-network", 'n');

    libport::OptionValue
      arg_period   ("ignored for backward compatibility", "period", 'P'),
      arg_port_file("write port number to the specified file.",
                    "port-file", 'w', "FILE"),
      arg_stack    ("set the job stack size in KB", "stack-size", 's', "SIZE");

    libport::OptionValues
      arg_exps("run expression", "expression", 'e', "EXP");
    libport::OptionsEnd arg_remaining(true);
    {
      libport::OptionParser parser;
      parser
        << "Options:"
        << arg_fast
        << libport::opts::help
        << libport::opts::version
        << libport::opts::debug
        << arg_period
        << "Tuning:"
        << arg_stack
        << "Networking:"
        << libport::opts::host_l
        << libport::opts::port_l
        << arg_port_file
        << arg_no_net
        << "Execution:"
        << libport::opts::arg_exp
        << libport::opts::arg_file
        << arg_interactive
        << arg_remaining
        ;
      try
      {
        args = parser(args);
      }
      catch (libport::Error& e)
      {
        URBI_EXIT(EX_USAGE, ("command line error: %s", e.what()));
      }

      if (libport::opts::help.get())
        help(parser);
      if (libport::opts::version.get())
        version();
#endif
      data.interactive = IF_OPTION_PARSER(arg_interactive.get(), true);
      data.fast = IF_OPTION_PARSER(arg_fast.get(), false);

#ifndef NO_OPTION_PARSER
      arg_stack_size = arg_stack.get<size_t>(static_cast<size_t>(0));

     // Since arg_remaining ate everything, args should be empty unless the user
     // made a mistake.
     if (!args.empty())
       forbid_option(args[0]);

      // Unrecognized options: script files, followed by user args, or
      // '--' followed by user args.
      libport::OptionsEnd::values_type remaining_args = arg_remaining.get();
      if (!remaining_args.empty())
      {
        unsigned startPos = 0;
        if (!arg_remaining.found_separator())
        {
          // First argument is an input file.
          libport::opts::input_arguments.add_file(remaining_args[0]);
          ++startPos;
        }
        // Anything left is user argument
        for (unsigned i = startPos; i < remaining_args.size(); ++i)
        {
          std::string arg = remaining_args[i];
          object::system_push_argument(arg);
        }
      }
    }
#endif

    // If not defined in command line, use the envvar.
    if (IF_OPTION_PARSER(!arg_stack.filled() && , )  getenv("URBI_STACK_SIZE"))
      arg_stack_size = libport::convert_envvar<size_t> ("URBI_STACK_SIZE");

    if (arg_stack_size)
    {
      // Make sure the result is a multiple of the page size.  This
      // required at least on OSX (which unfortunately fails with errno
      // = 0).
      arg_stack_size *= 1024;
      size_t pagesize = getpagesize();
      arg_stack_size = ((arg_stack_size + pagesize - 1) / pagesize) * pagesize;
      sched::configuration.default_stack_size = arg_stack_size;
    }

    data.server = new ConsoleServer(data.fast);
    ConsoleServer& s = *data.server;

    /*----------------.
    | --port/--host.  |
    `----------------*/
    int port = -1;
    {
      int desired_port = IF_OPTION_PARSER(libport::opts::port_l.get<int>(-1),
                                          UAbstractClient::URBI_PORT);
      if (desired_port != -1)
      {
        std::string host =
          IF_OPTION_PARSER(libport::opts::host_l.value("127.0.0.1"),"");
        if (boost::system::error_code err =
            s.listen(&connectionFactory, host, desired_port))
          URBI_EXIT(EX_UNAVAILABLE,
                    ("cannot listen to port %s:%s: %s",
                     host, desired_port, err.message()));
        port = s.getLocalPort();
        // Port not allocated at all, or port differs from (non null)
        // request.
        if (!port
            || (desired_port && port != desired_port))
          URBI_EXIT(EX_UNAVAILABLE,
                    ("cannot listen to port %s:%s", host, desired_port));
      }
    }
    data.network = 0 < port;
    // In Urbi: System.listenPort = <port>.
    object::system_class->slot_set(SYMBOL(listenPort),
                                   object::to_urbi(port),
                                   true);

    s.initialize(data.interactive);

    /*--------------.
    | --port-file.  |
    `--------------*/
    // Write the port file after initialize returned; that is, after
    // urbi.u is loaded.
    IF_OPTION_PARSER(
    if (arg_port_file.filled())
      std::ofstream(arg_port_file.value().c_str(), std::ios::out)
        << port << std::endl;,
    )

    kernel::UConnection& c = s.ghost_connection_get();
    GD_INFO_TRACE("got ghost connection");

    DataSender send(s, c);
    send(libport::opts::input_arguments);
    libport::opts::input_arguments.clear();

    c.received("");
    GD_INFO_TRACE("going to work...");

    if (sem)
      (*sem)++;

    return main_loop(data);
  }

  int
  main_loop(LoopData& data)
  {
    ConsoleServer& s = *data.server;
    libport::utime_t next_time = 0;
    while (true)
    {
      if (data.interactive)
      {
        std::string input;
        try
        {
          input = libport::read_stdin();
        }
        catch (const libport::exception::Exception& e)
        {
          std::cerr << libport::program_name() << ": "
                    << e.what() << std::endl;
          data.interactive = false;
        }
        if (!input.empty())
          s.ghost_connection_get().received(input);
      }

      if (data.network)
      {
        libport::utime_t select_time = 0;
        if (!data.fast)
        {
          select_time = std::max(next_time - libport::utime(), select_time);
          if (data.interactive)
            select_time = std::min(100000LL, select_time);
        }
        if (select_time)
          libport::pollFor(select_time, true, s.get_io_service());
        else
        {
          s.get_io_service().reset();
          s.get_io_service().poll();
        }
      }

      next_time = s.work();
      if (next_time == sched::SCHED_EXIT)
        break;
      s.ctime = std::max(next_time, s.ctime + 1000L);
    }

    return EX_OK;
  }

  int
  main(const libport::cli_args_type& args, bool block, bool errors)
  {
    if (block)
      return init(args, errors, 0);
    else
    {
      // The semaphore must survive this block, as init will use it when
      // exiting.
      libport::Semaphore* s = new libport::Semaphore;
      libport::startThread(boost::bind(&init, boost::ref(args),
                                       errors, s));
      (*s)--;
      return 0;
    }
  }

}
