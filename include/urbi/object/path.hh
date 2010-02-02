/*
 * Copyright (C) 2009, 2010, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */
#ifndef OBJECT_PATH_HH
# define OBJECT_PATH_HH

# include <libport/compiler.hh>
# include <libport/path.hh>

# include <urbi/object/cxx-object.hh>
# include <urbi/object/directory.hh>
# include <urbi/object/equality-comparable.hh>

namespace urbi
{
  namespace object
  {
    class URBI_SDK_API Path
      : public CxxObject
      , public EqualityComparable<Path, libport::path>
    {

    /*--------------.
    | C++ methods.  |
    `--------------*/

    public:
      typedef Path self_type;
      typedef libport::path value_type;
      const value_type& value_get() const;
      void value_set(const value_type&);

    /*---------------.
    | Urbi methods.  |
    `---------------*/

    public:

      // Construction
      Path();
      Path(rPath model);
      Path(const std::string& value);
      void init(const std::string& path);

      // Comparison.
      bool operator<=(const rPath& rhs) const;

      // Global informations
      static rPath cwd();

      // Operations
      std::string basename();
      rPath cd();
      rPath path_concat(rPath other);
      rPath string_concat(rString other);
      rPath dirname();
      rObject open();

      // Stat
      bool absolute();
      bool exists();
      bool is_dir();
      bool is_reg();
      bool readable();
      bool writable();

      // Conversions
      rList as_list();
      std::string as_string();
      std::string as_printable();

    /*----------.
    | Details.  |
    `----------*/

    private:
      value_type path_;

      ATTRIBUTE_NORETURN
      void handle_any_error();

      friend class Directory;

      // Stat the file and handle all errors
      struct stat stat();

      URBI_CXX_OBJECT_(Path);
    };
  }
}

#endif
