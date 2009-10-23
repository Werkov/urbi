/*
 * Copyright (C) 2009, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */
/**
 ** \file object/lobby-class.cc
 ** \brief Creation of the URBI object lobby.
 */

#include <boost/algorithm/string.hpp>

#include <libport/cassert>

#include <kernel/uconnection.hh>
#include <kernel/ughostconnection.hh>
#include <kernel/userver.hh>

#include <urbi/object/lobby.hh>
#include <urbi/object/object.hh>
#include <urbi/object/string.hh>
#include <object/symbols.hh>
#include <urbi/object/tag.hh>

#include <runner/raise.hh>
#include <runner/runner.hh>

namespace urbi
{
  namespace object
  {
    static rLobby
    lobby(rObject /*this*/, rLobby l)
    {
      return l;
    }

    Lobby::Lobby(connection_type* c)
      : connection_(c)
    {
      // Only the Lobby prototype is expected to have a null connection.
      assert(!proto || c);
      proto_add(proto ? proto : Object::proto);

      if (c)
      {
        // Don't you DARE change this with a slot pointing to `this', as
        // it would be a circular reference to the lobby from itself,
        // making him un-reclaimable.
        boost::function1<rLobby, rObject> f(boost::bind(lobby, _1, this));
        slot_set(SYMBOL(lobby), make_primitive(f));

        // Initialize the connection tag used to reference local
        // variables.
        slot_set
          (SYMBOL(connectionTag),
           new Tag(new sched::Tag(libport::Symbol(c->connection_tag_))));
      }
    }

    Lobby::Lobby(rLobby)
      : connection_(0)
    {
      RAISE("`Lobby' objects cannot be cloned");
    }

    Lobby::connection_type&
    Lobby::connection_get()
    {
      return *connection_;
    }

    const Lobby::connection_type&
    Lobby::connection_get() const
    {
      return *connection_;
    }

    void
    Lobby::disconnect()
    {
      connection_ = 0;
      call(SYMBOL(handleDisconnect));
    }

    void
    Lobby::send(const objects_type& args)
    {
      if (proto == this)
        RAISE("must be called on Lobby derivative");

      check_arg_count(args.size(), 1, 2);
      if (!connection_)
        return;
      // Second argument is the tag name.
      std::string tag;
      if (args.size() == 2)
      {
        const rString& name = args[1].unsafe_cast<String>();
        if (!name)
          runner::raise_argument_type_error(1, args[1], String::proto);
        tag = name->value_get();
      }
      const rString& rdata = args[0].unsafe_cast<String>();
      if (!rdata)
        runner::raise_argument_type_error(0, args[0], String::proto);
      const std::string data = rdata->value_get() + "\n";
      connection_->send(data.c_str(), data.length(), tag.c_str());
    }

    void
    Lobby::write(const std::string& data)
    {
      if (proto == this)
        RAISE("must be called on Lobby derivative");
      if (!connection_)
        return;
      connection_->send_queue(data.c_str(), data.size());
      connection_->flush();
    }

    rLobby
    Lobby::create()
    {
      kernel::UGhostConnection* g =
        new kernel::UGhostConnection(*kernel::urbiserver, true);
      return g->lobby_get();
    }

    void
    Lobby::receive(const std::string& s)
    {
      connection_->received(s);
    }

    void
    Lobby::resendBanner()
    {
      const std::string& banner = connection_->server_get().banner_get();
      std::vector<std::string> lines;
      boost::split(lines, banner, boost::is_any_of("\n"));
      foreach (const std::string& l, lines)
        call(SYMBOL(send), new String("*** " + l + "\n"),  new String("start"));

      /// Send connection id.
      //call(SYMBOL(send), new String("*** ID: " + connection_tag_ + "\n"),
      //     new String("ident"));
    }

    void
    Lobby::initialize(CxxObject::Binder<Lobby>& bind)
    {
      bind(SYMBOL(send), &Lobby::send);
      bind(SYMBOL(write), &Lobby::write);
      bind(SYMBOL(create), &Lobby::create);
      bind(SYMBOL(receive), &Lobby::receive);
      bind(SYMBOL(resendBanner), &Lobby::resendBanner);
    }

    rObject
    Lobby::proto_make()
    {
      return new Lobby(0);
    }

    URBI_CXX_OBJECT_REGISTER(Lobby);

  }; // namespace object
}
