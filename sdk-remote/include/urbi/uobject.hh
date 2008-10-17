/*! \file urbi/uobject.hh
 *******************************************************************************

 File: uobject.hh\n
 Definition of the UObject class and necessary related classes.

 This file is part of UObject Component Architecture\n
 (c) 2006 Gostai S.A.S.

 Permission to use, copy, modify, and redistribute this software for
 non-commercial use is hereby granted.

 This software is provided "as is" without warranty of any kind,
 either expressed or implied, including but not limited to the
 implied warranties of fitness for a particular purpose.

 For more information, comments, bug reports: http://www.urbiforge.com

 **************************************************************************** */

#ifndef URBI_UOBJECT_HH
# define URBI_UOBJECT_HH

# include <string>

# include <libport/fwd.hh>
# include <libport/ufloat.h>
# include <libport/package-info.hh>
# include <urbi/fwd.hh>
# include <urbi/export.hh>
# include <urbi/ucallbacks.hh>
# include <urbi/utimer-callback.hh>
# include <urbi/uvar.hh>

// Tell our users that it is fine to use void returning functions.
#define USE_VOID 1

/** Bind a variable to an object.

 This macro can only be called from within a class inheriting from
 UObject.  It binds the UVar x within the object to a variable
 with the same name in the corresponding URBI object.  */
# define UBindVar(Obj,X) \
  (X).init(__name, #X)

/** This macro inverts a UVar in/out accesses.

 After this call is made, writes by this module affect the sensed
 value, and reads read the target value. Writes by other modules
 and URBI code affect the target value, and reads get the sensed
 value. Without this call, all operations affect the same
 underlying variable.  */
# define UOwned(X) \
  (X).setOwned()

/// Backward compatibility.
# define USensor(X) \
  UOwned(X)

/** Bind the function x in current URBI object to the C++ member
 function of same name.  The return value and parameters must be of
 a basic integral or floating types, char *, std::string, UValue,
 UBinary, USound or UImage, or any type that can cast to/from
 UValue.  */
# define UBindFunction(Obj, X)						\
  ::urbi::createUCallback(__name, "function", this,			\
			  (&Obj::X), __name + "." #X,			\
			  ::urbi::functionmap, false)

/** Registers a function x in current object that will be called each
 time the event of same name is triggered. The function will be
 called only if the number of arguments match between the function
 prototype and the URBI event.
 */
# define UBindEvent(Obj, X)						\
  ::urbi::createUCallback(__name, "event", this,			\
			  (&Obj::X), __name + "." #X,			\
			  ::urbi::eventmap, false)

/** Registers a function x in current object that will be called each
 * time the event of same name is triggered, and a function fun called
 * when the event ends. The function will be called only if the number
 * of arguments match between the function prototype and the URBI
 * event.
 */
# define UBindEventEnd(Obj, X, Fun)					\
  ::urbi::createUCallback(__name, "eventend", this,			\
			  (&Obj::X),(&Obj::Fun), __name + "." #X,	\
			  ::urbi::eventendmap)

/// Register current object to the UObjectHub named 'hub'.
# define URegister(Hub)						\
  do {								\
    objecthub = ::urbi::getUObjectHub(#Hub);			\
    if (objecthub)						\
      objecthub->addMember(this);				\
    else							\
      ::urbi::echo("Error: hub name '" #Hub "' is unknown\n");	\
  } while (0)


//macro to send urbi commands
# ifndef URBI
/// Send unquoted URBI commands to the server.
/// Add an extra layer of parenthesis for safety.
#   define URBI(A) \
  ::urbi::uobject_unarmorAndSend(# A)
# endif

/// Send \a Args (which is given to a stream and therefore can use <<)
/// to the server.
# define URBI_SEND(Args)			\
  do {						\
    std::ostringstream os;			\
    os << Args;					\
    URBI(()) << os.str();			\
  } while (0)

/// Send "\a Args ; \n".
# define URBI_SEND_COMMAND(Args)		\
  URBI_SEND(Args << ';' << std::endl)

extern "C"
{
  /** Bouncer to urbi::main() for easier access through dlsym()*/
  USDK_API int urbi_main(int argc, const char *argv[], int block);
}

namespace urbi
{

  typedef std::list<UObject*> UObjectList;

  /// Package information about liburbi.
  const libport::PackageInfo& package_info ();

  typedef int UReturn;

  /** Initialisation method.
   * Both plugin and remote libraries include a main function whose only
   * effect is to call urbi::main. If you need to write your own main, call
   * urbi::main(argc, argv) after your work is done.
   * This function returns if block is set to false.
   */
  USDK_API int main(int argc, const char *argv[], bool block = true);

#ifdef URBI_ENV_REMOTE
  /** Initialisation method, for remote mode only, that returns.
   * \param host the host to connect to.
   * \param port the port number to connect to (you can use the constant
   *             UAbstractClient::URBI_PORT).
   * \param buflen receive and send buffer size (you can use the constant
   *               UAbstractClient::URBI_BUFLEN).
   * \param exitOnDisconnect call exit() if we get disconnected from server.
   * \return 0 if no error occured.
   */
   int USDK_API initialize(const char* host, int port, int buflen,
		  bool exitOnDisconnect);
#endif
  /// an empty dummy UObject used by UVar to set a NotifyChange
  /// This avoid coupling a UVar to a particular object
  extern USDK_API UObject* dummyUObject;

  // Global function of the urbi:: namespace to access kernel features

  /// Write a message to the server debug output. Printf syntax.
  USDK_API void echo(const char* format, ... );
  /// Retrieve a UObjectHub based on its name or return 0 if not found.
  USDK_API UObjectHub* getUObjectHub(const std::string& n);
  /// Retrieve a UObject based on its name or return 0 if not found.
  USDK_API UObject* getUObject(const std::string& n);

  /// Send URBI code (ghost connection in plugin mode, default
  /// connection in remote mode).
  USDK_API void uobject_unarmorAndSend(const char* str);

  /// Send the string to the connection hosting the UObject.
  USDK_API void send(const char* str);

  /// Send buf to the connection hosting the UObject.
  USDK_API void send(void* buf, int size);

  /// Possible UObject running modes.
  enum UObjectMode {
       MODE_PLUGIN=1,
       MODE_REMOTE
  };
  /// Return the mode in which the code is running.
  USDK_API UObjectMode getRunningMode();
  /// Return true if the code is running in plugin mode.
  inline bool isPluginMode() { return getRunningMode() == MODE_PLUGIN;}
  /// Return true if the code is running in remote mode.
  inline bool isRemoteMode() { return getRunningMode() == MODE_REMOTE;}
  /** Main UObject class definition
      Each UObject instance corresponds to an URBI object. It provides mechanisms to
      bind variables and functions between C++ and URBI.
  */
  class USDK_API UObject
  {
  public:

    UObject(const std::string&);
    /// dummy UObject constructor
    UObject(int);
    virtual ~UObject();


    // This call registers both an UObject (say of type
    // UObjectDerived), and a callback working on it (named here
    // fun).  createUCallback wants both the object and the callback
    // to have the same type, which is not the casem this is static
    // type of the former is UObject (its runtime type is indeed
    // UObjectDerived though), and the callback wants a
    // UObjectDerived.  So we need a cast, until a more elegant way
    // is found (e.g., using free standing functions instead of a
    // member functions).

    // These macros provide the following callbacks :
    // Notify
    // Access    | const std::string& | int (T::*fun) ()            | const
    // Change    | urbi::UVar&        | int (T::*fun) (urbi::UVar&) | non-const
    // OnRequest |

# ifdef DOXYGEN
    // Doxygen does not handle macros very well so feed it simplified code.
    /*!
    \brief Call a function each time a variable is modified.
    \param v the variable to monitor.
    \param fun the function to call each time the variable \b v is modified.
    The function is called rigth after the variable v is modified.
    */
    void UNotifyChange(UVar& v, int (UObject::*fun)(UVar&));

    /*!
    \brief Call a function each time a new variable value is available.
    \param v the variable to monitor.
    \param fun the function to call each time the variable \b v is modified.
    This function is similar to UNotifyChange(), but it does not monitor the
    changes on \b v. You must explicitly call UVar::requestValue() when you
    want the callback function to be called.
    The function is called rigth after the variable v is updated.
    */
    void UNotifyOnRequest(UVar& v, int (UObject::*fun)(UVar&));
    /*!
    \brief Call a function each time a variable is accessed.
    \param v the variable to monitor.
    \param fun the function to call each time the variable \b v is accessed.
    The function is called rigth \b before the variable v is accessed, giving
    \b fun the oportunity to modify it.
    */
    void UNotifyAccess(UVar& v, int (UObject::*fun)(UVar&));

    /*!
    \brief Setup a callback function that will be called every \t milliseconds.
    */
    template <class T>
    void USetTimer(ufloat t, int (T::*fun) ());
# else

    /// \internal
# define MakeNotify(Type, Notified, Arg, Const,				\
		    TypeString, Name, Map, Owned,			\
		    WithArg, StoreArg)					\
    template <class T>							\
    void UNotify##Type (Notified, int (T::*fun) (Arg) Const)		\
    {									\
      UGenericCallback* cb =						\
	createUCallback (__name, TypeString,				\
			 dynamic_cast<T*>(this),			\
			 fun, Name, Map, Owned);			\
									\
      if (WithArg && cb)						\
	cb->storage = StoreArg;						\
    }

    /// \internal
# define MakeMetaNotifyArg(Type, Notified, TypeString, Map, Owned,	\
			   Name, StoreArg)				\
    MakeNotify (Type, Notified, /**/, /**/,   TypeString, Name,		\
		Map, Owned, false, StoreArg);				\
    MakeNotify (Type, Notified, /**/, const,  TypeString, Name,		\
		Map, Owned, false, StoreArg);				\
    MakeNotify (Type, Notified, UVar&, /**/,  TypeString, Name,		\
		Map, Owned, true, StoreArg);				\
    MakeNotify (Type, Notified, UVar&, const, TypeString, Name,		\
		Map, Owned, true, StoreArg);

    /// \internal
# define MakeMetaNotify(Type, TypeString, Map)				\
    MakeMetaNotifyArg (Type, UVar& v, TypeString,			\
		       Map, v.owned, v.get_name (), &v);		\
    MakeMetaNotifyArg (Type, const std::string& name, TypeString,	\
		       Map, false, name, new UVar(name));

    /// \internal
    MakeMetaNotify (Access, "varaccess", accessmap);

    /// \internal
    MakeMetaNotify (Change, "var", monitormap);

    /// \internal
    MakeMetaNotify (OnRequest, "var_onrequest", monitormap);

# undef MakeNotify
# undef MakeMetaNotifyArg
# undef MakeMEtaNotify

    /// \internal
# define MKUSetTimer(Const, Useless)						\
    template <class T>							\
    void USetTimer(ufloat t, int (T::*fun) () Const)			\
    {									\
      new UTimerCallbackobj<T> (__name, t,				\
				dynamic_cast<T*>(this), fun, timermap);	\
    }

    MKUSetTimer (/**/, /**/);
    MKUSetTimer (const, /**/);

# undef MKUSetTimer

# endif //DOXYGEN

    /// Request permanent synchronization for v.
    void USync(UVar &v);

    /// Name of the object as seen in URBI.
    std::string __name;
    /// Name of the class the object is derived from.
    std::string classname;
    /// True when the object has been newed by an urbi command.
    bool   derived;

    UObjectList members;

    /// The hub, if it exists.
    UObjectHub  *objecthub;

    /// The connection used by send().
    UGhostConnection* gc;
    /// Send a command to URBI.
    int send(const std::string& s);

    /// Set a timer that will call the update function every 'period'
    /// milliseconds
    void USetUpdate(ufloat period);
    virtual int update() {return 0;};
    /// Set autogrouping facility for each new subclass created..
    void UAutoGroup() { autogroup = true; };
    /// Called when a subclass is created if autogroup is true.
    virtual void addAutoGroup() { UJoinGroup(classname+"s"); };

    /// Join the uobject to the 'gpname' group.
    virtual void UJoinGroup(const std::string& gpname);
    /// Void function used in USync callbacks.
    int voidfun() {return  0;};
    /// Add a group with a 's' after the base class name.
    bool autogroup;

    /// Flag to know whether the UObject is in remote mode or not
    bool remote;

    /// Remove all bindings, this method is called by the destructor.
    void clean();

    /// The load attribute is standard and can be used to control the
    /// activity of the object.
    UVar load;

  private:
    /// Pointer to a globalData structure specific to the
    /// remote/plugin architectures who defines it.
    UObjectData*  objectData;
    ufloat period;
  };


  //! Main UObjectHub class definition
  class USDK_API UObjectHub
  {
  public:

    UObjectHub(const std::string&);
    virtual ~UObjectHub();

    void addMember(UObject* obj);

    /// Set a timer that will call update() every 'period' milliseconds.
    void USetUpdate(ufloat period);
    virtual int update() {return 0;}

    UObjectList  members;
    UObjectList* getSubClass(const std::string&);
    //   UObjectList* getAllSubClass(const std::string&); //TODO

  protected:
    /// This function calls update and the subclass update.
    int updateGlobal();

    ufloat period;
    std::string name;
  };

} // end namespace urbi

// This file needs the definition of UObject, so included last.
// To be cleaned later.
# include <urbi/ustarter.hh>

#endif // ! URBI_UOBJECT_HH
