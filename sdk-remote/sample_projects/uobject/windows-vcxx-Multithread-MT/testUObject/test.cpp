// test.cpp�: d�finit le point d'entr�e pour l'application console.
//

#include "stdafx.h"
#include <urbi/uobject.hh>

using namespace urbi;


class test : public UObject
{
	public:
	test(std::string s) : UObject(s){};
};


UStart(test);

