/**
 ** \file runner/interpreter.hh
 ** \brief Definition of runner::Interpreter.
 */

#ifndef RUNNER_INTERPRETER_HH
# define RUNNER_INTERPRETER_HH

# include <boost/tuple/tuple.hpp>

# include <libport/hash.hh>

# include <ast/default-visitor.hh>
# include <ast/fwd.hh>
# include <object/object.hh>
# include <scheduler/scheduler.hh>
# include <scheduler/job.hh>

# include <runner/runner.hh>
# include <runner/stacks.hh>

namespace runner
{

  /// Ast executor.
  class Interpreter : public ast::DefaultConstVisitor,
		      public runner::Runner
  {
  public:
    /// \name Useful shorthands.
    /// \{
    /// Super class type.
    typedef ast::DefaultConstVisitor super_type;
    typedef object::rObject rObject;
    typedef object::rrObject rrObject;
    typedef object::rRoutine rRoutine;
    typedef object::rLobby rLobby;
    typedef object::objects_type objects_type;
    /// \}

    /// \name Ctor & dtor.
    /// \{
    /// Construct a \c Interpreter in the \a lobby.  The runner needs
    /// to know its \a scheduler and will execute \a ast.  Memory
    /// ownership of \a ast is transferred to the Interpreter.  The
    /// new runner has no parent.
    Interpreter(rLobby lobby,
                scheduler::Scheduler& scheduler,
                ast::rConstAst ast,
                const libport::Symbol& name = SYMBOL());

    Interpreter(const Interpreter&,
                rObject code,
                const libport::Symbol& name = SYMBOL(),
                const objects_type& args = objects_type());

    /// Create a copy of a runner starting with another ast.
    Interpreter(const Interpreter&,
                ast::rConstAst ast,
                const libport::Symbol& name = SYMBOL());

    /// Destroy a Interpreter.
    virtual ~Interpreter();
    /// \}

    /// The entry point: visit \a e.
    virtual void operator() (ast::rConstAst e);

    /// \ name Accessors.
    /// \{
  public:
    /// \}

    /// Execute the code of function \a func with arguments \a args in
    /// the local runner.
    ///
    /// \param func  the "function" to call: Primitive, Delegate, Code
    ///              or even rObject, in which case it is returned.
    ///
    /// \param msg   the name of the called method, as it should be stored
    ///              in a call message in case further processing is
    ///              required.
    ///
    /// \param args  the arguments.  Used as given, they are not evaluated
    ///              here.
    ///
    /// \param call_message  the callMessage.  Valid only for \a func
    ///             being Code.
    ///
    /// One cannot have both a call message and args.
    virtual rObject apply(const rObject& func,
                          const libport::Symbol msg,
                          object::objects_type args,
                          rObject call_message = 0);

    virtual rObject apply(const rObject& func,
                          const libport::Symbol msg,
                          object::objects_type args,
                          boost::optional<ast::loc> loc,
                          rObject call_message = 0);

    /// Helpers to apply a function with the arguments as ast chunks
    rObject apply(rObject tgt, const libport::Symbol& msg,
                  const ast::exps_type* args,
                  boost::optional<ast::loc> loc);
    rObject apply(rObject tgt, rObject f, const libport::Symbol& msg,
                  const ast::exps_type* args,
                  boost::optional<ast::loc> loc);

    /// Return the result of the latest evaluation.
    virtual rObject result_get();

    /// Evaluate an expression in the current scope and return its result.
    rObject eval(ast::rConstAst);

    /// Evaluate a tag and create it as well as the intermediate levels
    /// if needed.
    rObject eval_tag(ast::rConstExp);

    /// Make an urbi function from an ast chunk
    object::rRoutine make_routine(ast::rConstRoutine f) const;

  protected:
    /// \name Evaluation.
    /// \{
    /// Build an evaluated arguments list containing \a tgt and
    /// arguments coming from \a args evaluated in the current context.
    /// Raise an error if any argument is void.
    void push_evaluated_arguments (object::objects_type& args,
				   const ast::exps_type& ue_args);

    /// Build a call message
    virtual rObject
    build_call_message(const rObject& tgt,
		       const rObject& code,
		       const libport::Symbol& msg,
                       const object::objects_type& args);

    /// Build a call message
    rObject build_call_message(const rObject& tgt,
			       const rObject& code,
			       const libport::Symbol& msg,
                               const ast::exps_type& args);

    /// Import from super.
    using super_type::visit;

    CONST_VISITOR_VISIT_NODES((And)
                              (Assignment)
                              (Call)
                              (CallMsg)
                              (Closure)
                              (Do)
                              (Declaration)
                              (Float)
                              (Foreach)
                              (Function)
                              (If)
                              (Lazy)
                              (List)
                              (Local)
                              (Message)
                              (Nary)
                              (Noop)
                              (Pipe)
                              (Scope)
                              (Stmt)
                              (String)
                              (Tag)
                              (TaggedStmt)
                              (This)
                              (While))

    /// Factor handling of Scope and Do
    void visit (ast::rConstAbstractScope e);
    /// Factor handling of Function and Closure
    void visit (ast::rConstRoutine e, bool closure);
    /// \}


    /// Do the actual work.  Implementation of \c Job::run.
    virtual void work ();


    virtual void show_backtrace(const std::string& chan);
    virtual backtrace_type backtrace_get() const;

  protected:
    void show_error_ (object::UrbiException& ue);

  private:
    void init();
    /// Reset result_, set the location and call stack of ue.
    void propagate_error_(object::UrbiException& ue, const ast::loc& l);
    rObject apply_urbi (rRoutine func,
			const libport::Symbol& msg,
			const object::objects_type& args,
			rObject call_message);

  private:
    /// The root of the AST being executed.
    ast::rConstAst ast_;

    /// The urbi Code object to execute
    rObject code_;
    /// Its arguments
    objects_type args_;

    /// The current value during the evaluation of the AST.
    rObject result_;

    /// The call stack.
    typedef object::UrbiException::call_stack_type call_stack_type;
    typedef object::UrbiException::call_type call_type;
    call_stack_type call_stack_;
    void show_backtrace(const call_stack_type& bt,
                        const std::string& chan);

    /// The local variable stacks
    Stacks stacks_;
  };

} // namespace runner

# include <runner/interpreter.hxx>

#endif // !RUNNER_INTERPRETER_HH
