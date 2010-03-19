/*
 * Copyright (C) 2009, 2010, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

#include <iostream>
#include <libport/debug.hh>
#include "uchange.hh"

GD_ADD_CATEGORY(uchange);

using namespace urbi;

UStart (uchange);

uchange::uchange (const std::string& s)
  : UObject (s)
{
  UBindFunction (uchange, init);
}

int
uchange::init ()
{
  val = new UVar ("uaccess.val");
  UNotifyChange (*val, &uchange::newval);
  return 0;
}

UReturn
uchange::newval (UVar& v)
{
  GD_CATEGORY(uchange);
  GD_FINFO_DEBUG("newval: %s", (int) v);
  return 0;
}
