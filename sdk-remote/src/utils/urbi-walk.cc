/*
 * Copyright (C) 2009, 2010, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */
#include "urbi/usyncclient.hh"
#include "move.hh"

// FIXME: those return value should not be ignored
static char* ignore;

int
main (int argc, char * argv[])
{
  if (argc<2)
  {
    printf("usage: %s robotname [initialize] [configfile]\n"
	   "\tSet initialize to 0 to skip upload of command files.(needed the first time, and each time changes are made to the configuration\n"
	   "\tThen send commands to stdin with the syntax 'w[alk] value_in_meters relativePrecision' or 't[urn] value_in_degrees relativePrecision'\n",argv[0]);
    exit(1);
  }

  urbi::USyncClient cl(argv[1]);
  cl.start();
  if (cl.error())
    exit(2);
  urbi::Move mv;
  const char * config;
  bool init=true;
  if (argc>2 && argv[2][0]=='0')
    init=false;
  if (argc>3)
    config=argv[3];
  else
    config="moveset/moveconfig";
  cl.send("motoron;");
  if (init)
    printf("Uploading elementary movement commands, this may take a few seconds...\n");
  if (mv.initialize(&cl,init,config))
    exit(1);
  double useless;
  cl.syncGetDevice("neck",useless);
  if (init)
    printf("Done.\n");
  printf("Ready to accept commands\n");

  while (1)
  {
    char line[1024];
    ignore = fgets(line,1024,stdin);
    char cmd[64];
    float val,prec;
    int arg=sscanf(line,"%s%f%f",cmd,&val,&prec);
    if (arg<2)
      continue;
    if (arg<3)
      prec=0.2;
    if (cmd[0]=='w')
      mv.walk(val,prec);
    else
      mv.turn(val,prec);
    printf(cmd[0]=='w'?"walked %f meters(%f)\n":"turned %f degrees (%f)\n",
	   val,prec);
  }
}
