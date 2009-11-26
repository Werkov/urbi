/*
 * Copyright (C) 2009, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */
#ifndef RUNNER_RAISE_HH
# define RUNNER_RAISE_HH

# include <libport/compiler.hh>
# include <libport/ufloat.hh>

# include <urbi/object/fwd.hh>

namespace runner
{
  extern const object::rObject& raise_current_method;

  /// Raise an Urbi exception denoted by its name, looked up in
  /// "Global.Exception".  If arg1 is "raise_current_method", the
  /// innermost method name will be looked up in the current runner
  /// and used instead.
  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_urbi(libport::Symbol exn_name,
		  object::rObject arg1 = 0,
		  object::rObject arg2 = 0,
		  object::rObject arg3 = 0,
		  object::rObject arg4 = 0,
                  bool skip = false);

  /// Like raise_urbi, but skip the last callstack element.
  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_urbi_skip(libport::Symbol exn_name,
                       object::rObject arg1 = 0,
                       object::rObject arg2 = 0,
                       object::rObject arg3 = 0,
                       object::rObject arg4 = 0);

  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_argument_type_error
    (unsigned idx,
     object::rObject effective,
     object::rObject expected,
     object::rObject method_name = raise_current_method);

  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_arity_error(unsigned effective, unsigned expected);

  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_arity_error(unsigned effective,
			 unsigned minimum,
			 unsigned maximum);

  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_bad_integer_error(libport::ufloat effective,
			       const std::string& msg
                               = "expected integer, got %s");

  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_const_error();

  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_lookup_error(libport::Symbol msg, const object::rObject& obj);

  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_primitive_error(const std::string& message);

  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_type_error(object::rObject effective,
                        object::rObject expected);

  ATTRIBUTE_NORETURN
  URBI_SDK_API
  void raise_unexpected_void_error();
}

#endif
