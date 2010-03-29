/*
 * Copyright (C) 2006, 2009-2010, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

#ifndef MOVE_H
# define MOVE_H

# include <list>
# include <string>

# include "urbi/uclient.hh"

namespace urbi
{
  /**
     Class Move, first rather trivial implementation.
     Call initialize with the UClient to use, and the name of the configuration file.
     Then call move or turn, with the wanted movement value (in degrees or meters), and optionnaly with the name of a tag with which an "end move" message will be generated. On return the value will be filled with the actual movement value by which the robot moved, which may be different.

     The configuration file may have comment lines beginning with '#'. A configuration line has the format:
     <type> <filename> <value> <speed> <precision>
     type can be 'walk' or 'turn'
     filename is the name of a file containing urbi commands, defining an elementary movement.
     value is the value for this movement (in degrees or meters)
     speed and precision are curently not used.
  */

  struct MovementProperties
  {
    float maxSpeed; //  m/s, deg/s
    float minSpeed;
    float resolution; //min move value (m,deg)
    float precision;  //will move by resolution+/-resolution*precision
  };

  struct LoadedFile
  {
    char name[256];
    float speed;
    float value;
    float precision;
  };

  class Move
  {
  public:
    int initialize(UClient * client, bool uploadFiles=true, const char * configFile="moveconfig", bool enableInterrupt=false);
    const MovementProperties& getWalkProperties ()
    {
      return pwalk;
    }
    const MovementProperties& getTurnProperties ()
    {
      return pturn;
    }
    int walk(float &distance, float relativePrecision,const char * tag=NULL);
    int turn(float &angle,  float relativePrecision, const char * tag=NULL);
    /// Stop current movement as soon as possible
    void interrupt(bool notifyEndMove);
    UCallbackAction moveEnd(const UMessage &msg);
    UClient * getConnection ()
    {
      return robot;
    }
    //attempt to break movement, still call endmove

  private:
    UClient * robot;
    UClient * interruptConnection;
    MovementProperties pwalk, pturn;
    std::list<LoadedFile> walks;
    std::list<LoadedFile> turns;

    /// Our unique tag to mark end.
    std::string tag;
    int moving; //curently moving
    std::string usertag;
    std::string execTag;
    std::list<std::string> sequence; //move/walk commands not yet sent to server
  };

} // namespace urbi
#endif
