/**
 ** \file ast/parametric-ast.hxx
 ** \brief Inline implementation of ast::ParametricAst.
 */

#ifndef AST_PARAMETRIC_AST_HXX
# define AST_PARAMETRIC_AST_HXX

# include <boost/static_assert.hpp>
# include <boost/type_traits/is_base_of.hpp>

# include <ast/parametric-ast.hh>
# include <ast/print.hh>

namespace ast
{

  template <typename T>
  ParametricAst&
  ParametricAst::operator% (const T& t)
  {
#ifndef NDEBUG
    passert(libport::deref << t, unique_(t));
#endif
    append_(count_, t);
    // Compute the location of the source text we used.
    if (!effective_location_.begin.filename)
      effective_location_ = t->location_get();
    else
      effective_location_ = effective_location_ + t->location_get();
    return *this;
  }

  template <typename T>
  libport::shared_ptr<T>
  ParametricAst::result()
  {
    BOOST_STATIC_ASSERT((boost::is_base_of<Ast, T>::value));
    if (getenv("DESUGAR"))
      LIBPORT_ECHO(*this);
    operator()(ast_.get());
    result_->location_set(effective_location_);
    if (getenv("DESUGAR"))
      LIBPORT_ECHO(result_->location_get() << ": " << *result_);
    clear();
    return assert_exp(result_.unsafe_cast<T>());
  }

  template <typename T>
  T
  ParametricAst::take(unsigned s) throw (std::range_error)
  {
    return parser::MetavarMap<T>::take_(s);
  }

} // namespace ast

#endif // !AST_PARAMETRIC_AST_HH
