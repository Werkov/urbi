/*
 * Copyright (C) 2009, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */
/// \file urbi/uobject.hh

#ifndef URBI_UOBJECT_HH
# define URBI_UOBJECT_HH

# include <libport/warning-push.hh>

# include <string>

# include <libport/compiler.hh>
# include <libport/fwd.hh>
# include <libport/ufloat.h>
# include <libport/utime.hh>

# include <urbi/export.hh>
# include <urbi/fwd.hh>
# include <urbi/kernel-version.hh>
# include <urbi/ucallbacks.hh>
# include <urbi/utimer-callback.hh>
# include <urbi/uvar.hh>
# include <urbi/uobject-hub.hh>
# include <urbi/ucontext.hh>

// Tell our users that it is fine to use void returning functions.
#define USE_VOID 1

#define URBI_UOBJECT_VERSION 2

/** Bind a variable to an object.

 This macro can only be called from within a class inheriting from
 UObject.  It binds the UVar x within the object to a variable
 with the same name in the corresponding Urbi object.  */
# define UBindVar(Obj,X) \
  (X).init(__name, #X, ctx_)

/** This macro inverts a UVar in/out accesses.

 After this call is made, writes by this module affect the sensed
 value, and reads read the target value. Writes by other modules
 and Urbi code affect the target value, and reads get the sensed
 value. Without this call, all operations affect the same
 underlying variable.  */
# define UOwned(X) \
  (X).setOwned()

/// Backward compatibility.
# define USensor(X) \
  UOwned(X)

/** Bind the function X in current Urbi object to the C++ member
 function of same name.  The return value and arguments must be of
 a basic integral or floating types, char *, std::string, UValue,
 UBinary, USound or UImage, or any type that can cast to/from
 UValue.  */
# define UBindFunction(Obj, X)                                          \
  ::urbi::createUCallback(*this, 0, "function", static_cast<Obj*>(this), \
                          (&Obj::X), __name + "." #X)

/** Registers a function X in current object that will be called each
 time the event of same name is triggered. The function will be
 called only if the number of arguments match between the function
 prototype and the Urbi event.  */
# define UBindEvent(Obj, X)                             \
::urbi::createUCallback(*this, 0, "event", this,  \
			  (&Obj::X), __name + "." #X)

/** Registers a function \a X in current object that will be called each
 * time the event of same name is triggered, and a function fun called
 * when the event ends. The function will be called only if the number
 * of arguments match between the function prototype and the Urbi
 * event.
 */
# define UBindEventEnd(Obj, X, Fun)					\
  ::urbi::createUCallback(*this, 0, "eventend", this,			\
			  (&Obj::X),(&Obj::Fun), __name + "." #X)

/// Register current object to the UObjectHub named \a Hub.
# define URegister(Hub)						\
  do {								\
    objecthub = ::urbi::baseURBIStarterHub::find(#Hub);         \
    if (objecthub)						\
      objecthub->addMember(this);				\
    else							\
      ::urbi::echo("Error: hub name '" #Hub "' is unknown\n");	\
  } while (0)


//macro to send urbi commands
# ifndef URBI
/// Send unquoted Urbi commands to the server.
/// Add an extra layer of parenthesis for safety.
#   define URBI(A) \
  uobject_unarmorAndSend(# A)
# endif


/// Send \a Args (which is given to a stream and therefore can use <<)
/// to \a C.
# define URBI_SEND_C(C, Args)			\
  do {						\
    std::ostringstream os;			\
    os << Args;					\
    C << os.str();                              \
  } while (false)

/// Send \a Args (which is given to a stream and therefore can use <<)
/// to the server.
# define URBI_SEND(Args)			\
  URBI_SEND_C(URBI(()), Args)

/// Send "\a Args ; \n" to \a C.
# define URBI_SEND_COMMAND_C(C, Args)		\
  URBI_SEND_C(C, Args << ';' << std::endl)

# define URBI_SEND_COMMAND(Args)		\
  URBI_SEND_COMMAND_C(URBI(()), Args)

/** Send "\a Args | \n" to \a C.
  * \b Warning: nothing is executed until a ';' or ',' is sent.
  */
# define URBI_SEND_PIPED_COMMAND_C(C, Args)     \
  URBI_SEND_C(C, Args << '|' << std::endl)

# define URBI_SEND_PIPED_COMMAND(Args)          \
  URBI_SEND_PIPED_COMMAND_C(URBI(()), Args)

# define URBI_SEND_COMMA_COMMAND_C(C, Args)     \
  URBI_SEND_C(C, Args << ',' << std::endl)

# define URBI_SEND_COMMA_COMMAND(Args)          \
  URBI_SEND_COMMA_COMMAND_C(URBI(()), Args)

namespace urbi
{

  UObjectHub* getUObjectHub(const std::string& n);
  UObject* getUObject(const std::string& n);
  void uobject_unarmorAndSend(const char* str);
  void send(const char* str);
  void send(const std::string&s);
  void send(const void* buf, size_t size);
  UObjectMode getRunningMode();
  bool isPluginMode();
  bool isRemoteMode();

  typedef int UReturn;
  /** Main UObject class definition
      Each UObject instance corresponds to an URBI object.
      It provides mechanisms to bind variables and functions between
      C++ and Urbi.
  */
  class URBI_SDK_API UObject: public UContext
  {
  public:

    UObject(const std::string&, impl::UContextImpl* impl = 0);
    /// Reserved for internal use
    UObject(int, impl::UContextImpl* impl = 0);
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

    /// Call \a fun every \a t milliseconds.
    template <class T>
    void USetTimer(ufloat t, int (T::*fun)());
# else

    /// \internal
# define MakeNotify(Type, Notified, Arg, Const,                 \
		    TypeString, Name, Owned,			\
		    WithArg, StoreArg)                          \
    template <class T>                                          \
    void UNotify##Type(Notified, int (T::*fun) (Arg) Const)     \
    {                                                           \
	createUCallback(*this, StoreArg, TypeString,            \
                        dynamic_cast<T*>(this),                 \
                        fun, Name);	\
    }

    /// \internal Handle functions taking a UVar& or nothing, const or not.
# define MakeMetaNotifyArg(Type, Notified, TypeString, Owned,	\
			   Name, StoreArg)                      \
    MakeNotify(Type, Notified, /**/, /**/,   TypeString, Name,  \
               Owned, false, StoreArg);				\
    MakeNotify(Type, Notified, /**/, const,  TypeString, Name,  \
               Owned, false, StoreArg);				\
    MakeNotify(Type, Notified, UVar&, /**/,  TypeString, Name,  \
               Owned, true, StoreArg);				\
    MakeNotify(Type, Notified, UVar&, const, TypeString, Name,  \
               Owned, true, StoreArg);

    /// \internal Define notify by name or by passing an UVar.
# define MakeMetaNotify(Type, TypeString)				\
    MakeMetaNotifyArg(Type, UVar& v, TypeString,			\
                      v.owned, v.get_name (),                           \
                      v.get_temp()?new UVar(v.get_name()):&v);          \
    MakeMetaNotifyArg(Type, const std::string& name, TypeString,	\
                      false, name, new UVar(name));

    /// \internal
    MakeMetaNotify(Access, "varaccess");

    /// \internal
    MakeMetaNotify(Change, "var");

    /// \internal
    MakeMetaNotify(OnRequest, "var_onrequest");

# undef MakeNotify
# undef MakeMetaNotifyArg
# undef MakeMEtaNotify

    /// \internal
# define MKUSetTimer(Const, Useless)                                    \
    template <class T>							\
    void USetTimer(ufloat t, int (T::*fun) () Const)			\
    {									\
      new UTimerCallbackobj<T> (__name, t,				\
				dynamic_cast<T*>(this), fun, ctx_);     \
    }

    MKUSetTimer (/**/, /**/);
    MKUSetTimer (const, /**/);

# undef MKUSetTimer

# endif //DOXYGEN

    /// Request permanent synchronization for v.
    void USync(UVar &v);

    /// Name of the object as seen in Urbi.
    std::string __name;
    /// Name of the class the object is derived from.
    std::string classname;
    /// True when the object has been newed by an urbi command.
    bool derived;

    UObjectList members;

    /// The hub, if it exists.
    UObjectHub* objecthub;

    /// Set a timer that will call the update function every 'period'
    /// milliseconds.
    void USetUpdate(ufloat period);
    virtual int update();


    /// \name Autogroup.
    /// \{
    /// These functions are obsoleted, they are not supported
    /// in Urbi SDK 2.0.

    /// Set autogrouping facility for each new subclass created.
#ifdef BUILDING_URBI_SDK
# define URBI_SDK_DEPRECATED
#else
# define URBI_SDK_DEPRECATED ATTRIBUTE_DEPRECATED
#endif
    URBI_SDK_DEPRECATED
    void UAutoGroup();
    /// Called when a subclass is created if autogroup is true.
    URBI_SDK_DEPRECATED
    virtual void addAutoGroup();

    /// Join the uobject to the 'gpname' group.
    URBI_SDK_DEPRECATED
    virtual void UJoinGroup(const std::string& gpname);

    /// Add a group with a 's' after the base class name.
    URBI_SDK_DEPRECATED
    bool autogroup;
#undef DEPRECATED
    /// \}

    /// Void function used in USync callbacks.
    int voidfun();

    /// Flag to know whether the UObject is in remote mode or not
    bool remote;

    /// Remove all bindings, this method is called by the destructor.
    void clean();


    /// The load attribute is standard and can be used to control the
    /// activity of the object.
    UVar load;

    baseURBIStarter* cloner;

    impl::UObjectImpl* impl_get();
  private:
    /// Pointer to a globalData structure specific to the
    /// remote/plugin architectures who defines it.
    UObjectData* objectData;

    impl::UObjectImpl* impl_;
  };

  // Provide cast support to UObject*
  template<> struct uvalue_caster<UObject*>
  {
    UObject* operator()(urbi::UValue& v);
  };
} // end namespace urbi

// This file needs the definition of UObject, so included last.
// To be cleaned later.
# include <urbi/ustarter.hh>

# include <urbi/uobject.hxx>

# include <libport/warning-pop.hh>

#endif // ! URBI_UOBJECT_HH
