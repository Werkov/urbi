// test.cpp�: d�finit le point d'entr�e pour l'application console.
//

#include "stdafx.h"
#include <urbi/uclient.hh>

using namespace urbi;


int main (void)
{
  UClient robotC;
  robotC.send( "motors on;" );
  robotC.send( "play(\"bark.wav\");" );
  robotC.send( "headPan = 0 | headPan = 60;" );
  urbi::execute();
}
