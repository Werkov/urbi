/*
 * Copyright (C) 2011-2012, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

#ifndef OBJECT_LOGGER_HH
# define OBJECT_LOGGER_HH

# include <boost/optional.hpp>

# include <libport/debug.hh>
# include <libport/symbol.hh>

# include <object/urbi/export.hh>
# include <urbi/object/enumeration.hh>
# include <urbi/object/tag.hh>

namespace urbi
{
  namespace object
  {
    class URBI_MODULE_API Logger: public Tag
    {
      URBI_CXX_OBJECT(Logger, Tag);
    public:
      typedef libport::debug::category_type category_type;
      typedef libport::Debug::types types;
      typedef libport::Debug::levels levels;
      Logger();
      Logger(std::string category);
      Logger(rLogger model);
      virtual ~Logger();

      void init();
      void init(category_type name);
      void init(category_type name, rObject level);
      std::string as_printable() const;
      Logger* operator<<(rObject o);

      /// The list of the known categories.
      /// Changes in this Dictionary have no impact.
      rDictionary categories() const;
      /// Enable all the categories (existing/future) that match \a pattern.
      void enable(const std::string& pattern);
      /// Disable all the categories (existing/future) that match \a pattern.
      void disable(const std::string& pattern);
      /// Change the status of all the categories based on \a specs.
      void set(const std::string& pattern);

      /// The current level.
      levels::Level level_get() const;
      /// Set the current level.
      void level_set(levels::Level level) const;


    private:
      void init_helper(category_type name);

    /*-------------------.
    | Logger functions.  |
    `-------------------*/

#define LEVEL(Level)                                                    \
    public:                                                             \
      Logger* Level(const std::string& msg, const std::string& category); \
      Logger* Level(const std::string& msg);                            \
      Logger* Level();

      LEVEL(log);
      LEVEL(trace);
      LEVEL(debug);
      LEVEL(dump);
      LEVEL(warn);
      LEVEL(err);
#undef LEVEL

    /*--------------.
    | Indentation.  |
    `--------------*/
    public:
      void onEnter();
      void onLeave();

    /*----------.
    | Details.  |
    `----------*/
    private:
      void msg_(types::Type type,
                levels::Level level,
                const std::string& msg,
                boost::optional<std::string> category =
                boost::optional<std::string>());
      boost::optional<category_type> category_;
      types::Type type_;
      levels::Level level_;
    };
  }
}

URBI_ENUM_DECLARE(::libport::Debug::levels::Level);

#endif
