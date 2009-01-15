/**
 ** \file runner/stacks.hh
 ** \brief Definition of runner::Stacks.
 */

#ifndef RUNNER_STACKS_HH
# define RUNNER_STACKS_HH

# include <boost/function.hpp>
#include <boost/utility.hpp>

# include <ast/fwd.hh>
# include <object/object.hh>

namespace runner
{
  class Stacks: public boost::noncopyable
  {
    public:
      // Import types
      typedef object:: rObject  rObject;
      typedef object::rrObject rrObject;

      /// Type of callable entities
      typedef boost::function0<void> action_type;

      /// Build static stacks
      Stacks(rObject lobby);

      /// Update the given value
      void set(ast::rConstLocalAssignment e, rObject v);
      /// Define the given value
      void def(ast::rConstLocalDeclaration e, rObject v);
      /// Bind given argument
      void def_arg(ast::rConstLocalDeclaration e, rObject v);
      /// Bind given captured variable
      void def_captured(ast::rConstLocalDeclaration e, rrObject v);

      /// Get value from the stack
      rObject get(ast::rConstLocal e);
      /// Get value by double pointer (e must be closed or captured)
      rrObject rget(ast::rConstLocal e);

      /// Set 'this'
      void self_set(rObject v);
      /// Set 'call'
      void call_set(rObject v);
      /// Get 'this'
      rObject self();
      /// Get 'call'
      rObject call();

      /// Switch the current 'this'
      /** \return the action to switch back to the previous 'this'
       */
      action_type switch_self(rObject v);

      /// Signal the stacks a new execution is starting
      void execution_starts(const libport::Symbol& msg);

      /// Push new frames on the stacks to execute a function
      /** \param msg      Name of the function being invoked (debug purpose only)
       *  \param local    Size of the local    variable frame.
       *  \param closed   Size of the closed   variable frame.
       *  \param captured Size of the captured variable frame.
       *  \param self     Value of 'this' in the new frame.
       *  \param call     Value of 'call' in the new frame.
       *  \return The action to restore the previous frame state.
       */
      action_type
      push_frame(const libport::Symbol& msg,
                 unsigned local, unsigned captured,
                 rObject self, rObject call);

    private:

      /// Factored setter
      void set(unsigned local, bool captured, rObject v);
      /// Factored definer
      void def(unsigned local, rObject v);
      void def(unsigned local, bool captured, rrObject v);

      /// Helper to restore a previous frame state
      void pop_frame(const libport::Symbol& msg,
                     unsigned local, unsigned captured);
      /// Helper to restore switched 'this'
      void switch_self_back(rObject v);

      /// The double-indirection local stack, for captured and
      /// closed-over variables
      typedef std::vector<rrObject> local_stack_type;
      local_stack_type local_stack_;
      local_stack_type captured_stack_;
      /// The closed variables frame pointer
      unsigned local_pointer_;
      /// The captured variables frame pointer
      unsigned captured_pointer_;
  };
}

#endif
