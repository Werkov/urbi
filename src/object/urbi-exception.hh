/**
 ** \file object/urbi-exception.hh
 ** \brief Definition of UrbiException
 */

#ifndef OBJECT_URBI_EXCEPTION_HH
# define OBJECT_URBI_EXCEPTION_HH

# include <string>

# include <libport/ufloat.hh>
# include <libport/symbol.hh>

# include <ast/fwd.hh>
# include <ast/call.hh>
# include <ast/loc.hh>
# include <kernel/exception.hh>
# include <object/object.hh>

namespace object
{
  /// The call stack
  typedef std::pair<libport::Symbol,
                    boost::optional<ast::loc> > call_type;
  typedef std::vector<call_type> call_stack_type;

  class MyException: public kernel::exception
  {
  public:
    MyException(rObject value, const call_stack_type& bt);
    rObject value_get();
    const call_stack_type& backtrace_get();

  private:
    rObject value_;
    call_stack_type bt_;
    COMPLETE_EXCEPTION(MyException);
  };

  /// This class defines an exception used when an error
  /// occurs in an URBI primitive.
  class UrbiException : public kernel::exception
  {
  public:
    /// Destructor.
    virtual ~UrbiException() throw ();

    /// Initialize message (add debug information if required).
    void initialize_msg() throw ();

    /// Return the exception's error message.
    virtual std::string what() const throw ();

    /// Returns true if the exception was allready displayed.
    bool was_displayed() const;

    /// Mark the exception as allready displayed.
    void set_displayed();

    ADD_FIELD(ast::loc, location)
    ADD_FIELD(call_stack_type, backtrace)
    ADD_FIELD(std::string, msg)
    ADD_FIELD(libport::Symbol, function);

  protected:
    /**
     * \brief Construct an exception which contains a raw message.
     * \param msg raw error message.  */
    explicit UrbiException(const std::string& msg);

    /**
     * \brief Construct an exception which contains a raw message.
     * \param msg raw Error message.
     * \param loc Error's location.  */
    UrbiException(const std::string& msg, const ast::loc&);

    /**
     * \brief Construct an exception which contains a raw message.
     * \param msg raw Error message.
     * \param fun C++ function that raised.  */
    UrbiException(const std::string& msg, const libport::Symbol fun);

    private:
    /// Was the exception displayed
    bool displayed_;
    COMPLETE_EXCEPTION(UrbiException)
  };


  /** Exception for lookup failures.
   * \param slot Searched slot.  */
  struct LookupError: public UrbiException
  {
    explicit LookupError (libport::Symbol slot);
    COMPLETE_EXCEPTION (LookupError)
  };

  /// Explicit for slots redefined.
  struct RedefinitionError: public UrbiException
  {
    explicit RedefinitionError (libport::Symbol slot);
    COMPLETE_EXCEPTION (RedefinitionError)
  };

  /// Exception raised when the stack space in a task is almost exhausted
  struct StackExhaustedError: public UrbiException
  {
    explicit StackExhaustedError (const std::string& msg);
    COMPLETE_EXCEPTION (StackExhaustedError)
  };

  /** Exception for errors related to primitives usage.
   * \param primitive   primitive that has thrown the error.
   * \param msg         error message which will be sent.  */
  struct PrimitiveError: public UrbiException
  {
    PrimitiveError(const libport::Symbol primitive,
                   const std::string& msg);
    COMPLETE_EXCEPTION(PrimitiveError)
  };

  /** Exception for type mismatch in a primitive usage.
   * \param formal      Expected type.
   * \param effective   Real type.
   * \param fun         Primitive's name.  */
  struct WrongArgumentType: public UrbiException
  {
    WrongArgumentType (const std::string& formal,
		       const std::string& effective,
		       const libport::Symbol fun);
    /// Invalid use of void.
    WrongArgumentType(const libport::Symbol fun);

    COMPLETE_EXCEPTION (WrongArgumentType)
  };

  /** Exception used for calls with wrong argument count.
   * \param effective  Number of arguments given.
   * \param formal     Number of arguments expected.
   * A version of the constructor also exists for functions taking a
   * variable number of arguments, between \param minformal and
   * \param maxformal.
   */
  struct WrongArgumentCount: public UrbiException
  {
    WrongArgumentCount (unsigned formal, unsigned effective,
			const libport::Symbol fun);
    WrongArgumentCount (unsigned minformal, unsigned maxformal,
			unsigned effective, const libport::Symbol fun);
    COMPLETE_EXCEPTION (WrongArgumentCount)
  };

  /** Exception used when a non-integer is provided to a primitive expecting
   * an integer.
   * \param effective  Effective floating point value that failed conversion.
   */
  struct BadInteger: public UrbiException
  {
    BadInteger (libport::ufloat effective, const libport::Symbol fun,
		std::string fmt = "expected integer, got %1%");
    COMPLETE_EXCEPTION (BadInteger)
  };

  /** Exception used when building an implicit tag name (k1 style).
   */
  struct ImplicitTagComponentError: public UrbiException
  {
    ImplicitTagComponentError (const ast::loc&);
    COMPLETE_EXCEPTION (ImplicitTagComponentError);
  };

  /** Exception used when a non-interruptible block of code tries to
   * get interrupted or blocked.
   */
  struct SchedulingError: public UrbiException
  {
    SchedulingError (const std::string& msg);
    COMPLETE_EXCEPTION (SchedulingError);
  };

  /** Exception used for a path which should be impossible to take,
   * for example when a template method doesn't make sense.
   */
  struct InternalError: public UrbiException
  {
    InternalError (const std::string& msg);
    COMPLETE_EXCEPTION (InternalError);
  };

  /** Exception used when there is a parser/binder/flower/... error.
   */
  struct ParserError: public UrbiException
  {
    ParserError(const ast::loc&, const std::string& msg);
    COMPLETE_EXCEPTION(ParserError);
  };

  /// Throw an exception if formal != effective.
  /// \note: \c self is included in the count.
  void check_arg_count (unsigned formal, unsigned effective,
		       const libport::Symbol fun);

  /// Same as above, with a minimum and maximum number of
  /// formal parameters.
  void check_arg_count (unsigned minformal, unsigned maxformal,
			unsigned effective, const libport::Symbol fun);

} // namespace object

# include <object/urbi-exception.hxx>
#endif //! OBJECT_URBI_EXCEPTION_HH
