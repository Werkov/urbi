#include <urbi/uclient.hh>

using namespace urbi;

int main (void)
{
      UClient *robotC;
      robotC =new UClient( "localhost");
	  robotC->send( "motors on;" );
	  robotC->send( "play(\"bark.wav\");" );
	  robotC->send( "headPan = 0;" );
	  robotC->send( "neck=0;" );
      urbi::execute();
}


