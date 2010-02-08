/*
 * Copyright (C) 2009, 2010, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */
#include <kernel/userver.hh>
#include <urbi/object/code.hh>
#include <urbi/object/float.hh>
#include <object/semaphore.hh>
#include <object/symbols.hh>
#include <runner/runner.hh>
#include <sched/fwd.hh>

namespace urbi
{
  namespace object
  {
    Semaphore::Semaphore()
    {
      proto_add(proto ? rObject(proto) : Object::proto);
    }

    Semaphore::Semaphore(rSemaphore model)
      : value_(model->value_)
    {
      proto_add(proto);
    }

    Semaphore::Semaphore(const value_type& value)
      : value_(value)
    {
      proto_add(proto);
    }

    struct SemaphoreException : public sched::SchedulerException
    {
      COMPLETE_EXCEPTION(SemaphoreException);
    };

    rSemaphore
    Semaphore::_new(rObject, rFloat c)
    {
      return new Semaphore(make_pair(c->to_unsigned_type(),
                                     std::list<sched::rJob>()));
    }

    void
    Semaphore::p()
    {
      runner::Runner& r = ::kernel::urbiserver->getCurrentRunner();

      if (--value_.first < 0)
      {
        value_.second.push_back(&r);
        try
        {
          r.yield_until_terminated(r);
        }
        catch (const SemaphoreException&)
        {
          // Regular wake up from a semaphore wait.
        }
      }
    }

    void
    Semaphore::v()
    {
      if (++value_.first <= 0)
      {
        value_.second.front()->async_throw(SemaphoreException());
        value_.second.pop_front();
      }
    }

    rObject
    Semaphore::criticalSection(rCode f)
    {
      objects_type args;
      args.push_back(this);

      p();
      libport::Finally finally(boost::bind(&Semaphore::v, this));
      return (*f)(args);
    }

    void Semaphore::initialize(CxxObject::Binder<Semaphore>& bind)
    {
      bind(SYMBOL(new),             &Semaphore::_new);
      bind(SYMBOL(criticalSection), &Semaphore::criticalSection);
      bind(SYMBOL(p),               &Semaphore::p);
      bind(SYMBOL(v),               &Semaphore::v);
    }

    URBI_CXX_OBJECT_REGISTER(Semaphore)
    {}

  } // namespace object
}
