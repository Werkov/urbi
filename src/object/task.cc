/*
 * Copyright (C) 2009, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */
/**
 ** \file object/task-class.cc
 ** \brief Creation of the URBI object task.
 */

#include <sstream>

#include <boost/any.hpp>

#include <kernel/userver.hh>
#include <urbi/object/float.hh>
#include <urbi/object/list.hh>
#include <urbi/object/object.hh>
#include <urbi/object/string.hh>
#include <object/symbols.hh>
#include <urbi/object/tag.hh>
#include <urbi/object/task.hh>
#include <runner/interpreter.hh>
#include <runner/runner.hh>

namespace urbi
{
  namespace object
  {
    using runner::Runner;

    Task::Task()
      : value_(0)
    {
      proto_add(proto ? proto : Object::proto);
    }

    Task::Task(const value_type& value)
      : value_(value)
    {
      proto_add(Task::proto);
      slot_set(SYMBOL(exceptionHandlerTag), nil_class);
    }

    Task::Task(rTask model)
      : value_(model->value_)
    {
      proto_add(Tag::proto);
    }

    const Task::value_type&
    Task::value_get() const
    {
      return value_;
    }

    libport::Symbol
    Task::name()
    {
      return value_->name_get();
    }

    const runner::tag_stack_type
    Task::tags()
    {
      return dynamic_cast<runner::Interpreter*>(value_.get())->tag_stack_get();
    }

    std::string
    Task::status()
    {
      Runner& r = ::kernel::urbiserver->getCurrentRunner();

      std::stringstream status;
      switch(value_->state_get())
      {
        case sched::to_start:
          status << "starting";
          break;
        case sched::running:
          status << "running";
          break;
        case sched::sleeping:
          status << "sleeping";
          break;
        case sched::waiting:
          status << "idle";
          break;
        case sched::joining:
          status << "waiting";
          break;
        case sched::zombie:
          status << "terminated";
          break;
      }
      if (value_ == &r)
        status << " (current task)";
      if (value_->frozen())
        status << " (frozen)";
      if (value_->has_pending_exception())
        status << " (pending exception)";
      if (value_->side_effect_free_get())
        status << " (side effect free)";
      if (value_->non_interruptible_get())
        status << " (non interruptible)";
      return status.str();
    }

    rList
    Task::backtrace()
    {
      List::value_type res;
      if (const Runner* runner = dynamic_cast<Runner*>(value_.get()))
      {
        foreach(Runner::frame_type line, runner->backtrace_get())
        {
          List::value_type frame;
          frame << new String(line.name)
                << new String(line.location);
          libport::push_front(res, new List(frame));
        }
      }
      return new List(res);
    }

    void
    Task::waitForTermination()
    {
      Runner& r = ::kernel::urbiserver->getCurrentRunner();
      r.yield_until_terminated(*value_);
    }

    void
    Task::waitForChanges()
    {
      Runner& r = ::kernel::urbiserver->getCurrentRunner();
      r.yield_until_things_changed();
    }

    void
    Task::terminate()
    {
      value_->terminate_now();
    }

    void
    Task::setSideEffectFree(rObject b)
    {
      value_->side_effect_free_set(b->as_bool());
    }

    rFloat
    Task::timeShift()
    {
      return new Float(value_->time_shift_get() / 1000000.0);
    }

    void
    Task::initialize(CxxObject::Binder<Task>& bind)
    {
      bind(SYMBOL(backtrace), &Task::backtrace);
      bind(SYMBOL(name), &Task::name);
      bind(SYMBOL(setSideEffectFree), &Task::setSideEffectFree);
      bind(SYMBOL(status), &Task::status);
      bind(SYMBOL(tags), &Task::tags);
      bind(SYMBOL(terminate), &Task::terminate);
      bind(SYMBOL(timeShift), &Task::timeShift);
      bind(SYMBOL(waitForChanges), &Task::waitForChanges);
      bind(SYMBOL(waitForTermination), &Task::waitForTermination);
    }

    rObject
    Task::proto_make()
    {
      return new Task();
    }

    URBI_CXX_OBJECT_REGISTER(Task);
  }; // namespace object
}
