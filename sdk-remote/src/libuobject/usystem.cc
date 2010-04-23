/*
 * Copyright (C) 2006-2010, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

/*! \file libuobject/usystem.cc
 *******************************************************************************

 File: libuobject/usystem.cc
 Implementation of the USystem class.

 This file is part of UObject Component Architecture\n
 (c) Gostai S.A.S., 2004-2006.

 Permission to use, copy, modify, and redistribute this software for
 non-commercial use is hereby granted.

 This software is provided "as is" without warranty of any kind,
 either expressed or implied, including but not limited to the
 implied warranties of fitness for a particular purpose.

 For more information, comments, bug reports: http://www.urbiforge.com

 **************************************************************************** */

#include <urbi/uobject.hh> // echo
#include <urbi/usystem.hh>

namespace urbi
{

  //! USystemMessage constructor.
  USystemMessage::USystemMessage()
  {
  }

  //! USystemMessage destructor.
  USystemMessage::~USystemMessage()
  {
  }

  // **************************************************************************
  // UStringSystemMessage

  //! UStringSystemMessage constructor.
  UStringSystemMessage::UStringSystemMessage(const std::string& s)
    : msg (s)
  {
  }

  //! UStringSystemMessage destructor.
  UStringSystemMessage::~UStringSystemMessage()
  {
  }

  const std::string&
  UStringSystemMessage::getMessage() const
  {
    return msg;
  };

  // **************************************************************************
  // USystem

  //! USystemMessage constructor.
  USystem::USystem()
  {
    echo("Warning: USystem is not available for components in remote mode.\n");
  }

  //! USystemMessage destructor.
  USystem::~USystem()
  {
  }

  void
  USystem::register_channel(const USystemChannel&)
  {
    // nothing to do in remote mode.
  }

  int
  USystem::receive_message(const USystemChannel&,
                           const USystemMessage&)
  {
    // user defined.
    return 0;
  }

} // namespace urbi
