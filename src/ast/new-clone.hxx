/**
 ** \file ast/new-clone.hxx
 ** \brief Inline implementation of ast::new_clone().
 */

#ifndef AST_CLONE_HXX
# define AST_CLONE_HXX

# include <boost/static_assert.hpp>
# include <boost/type_traits/is_base_of.hpp>

# include <ast/cloner.hh>
# include <ast/new-clone.hh>

namespace ast
{

  template <typename T>
  inline
  libport::shared_ptr<T>
  new_clone (libport::shared_ptr<const T> ast)
  {
    BOOST_STATIC_ASSERT((boost::is_base_of<Ast, T>::value));
    Cloner cloner;
    cloner(ast.get());
    libport::shared_ptr<T> res = libport::unsafe_cast<T>(cloner.result_get());
    assert (res);
    return res;
  }

  template <typename T>
  inline
  libport::shared_ptr<T>
  new_clone (libport::shared_ptr<T> ast)
  {
    return new_clone(libport::shared_ptr<const T>(ast));
  }

}

#endif // !AST_CLONE_HXX
