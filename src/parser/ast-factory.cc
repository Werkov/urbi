#include <ast/all.hh>
#include <ast/new-clone.hh>
#include <ast/parametric-ast.hh>
#include <object/symbols.hh>
#include <parser/ast-factory.hh>
#include <parser/parse.hh>
#include <parser/parse-result.hh>
#include <parser/tweast.hh>

namespace parser
{

  /// Create a new Tree node composing \c Lhs and \c Rhs with \c Op.
  /// \param op must be & or |.
  ast::rExp
  ast_bin(const yy::location& l,
          ast::flavor_type op, ast::rExp lhs, ast::rExp rhs)
  {
    ast::rExp res = 0;
    assert (lhs);
    assert (rhs);
    switch (op)
    {
    case ast::flavor_pipe:
      res = new ast::Pipe (l, lhs, rhs);
      break;
    case ast::flavor_and:
    {
      ast::rAnd rand;
      if (rand = lhs.unsafe_cast<ast::And>())
        rand->children_get().push_back(rhs);
      else
      {
        rand = new ast::And(l, ast::exps_type());
        rand->children_get().push_back(lhs);
        rand->children_get().push_back(rhs);
      }
      res = rand;
      break;
    }
    case ast::flavor_comma:
    case ast::flavor_semicolon:
    {
      ast::rNary nary = new ast::Nary(l);
      nary->push_back(lhs, op);
      nary->push_back(rhs);
      res = nary;
      break;
    }
    case ast::flavor_none:
      pabort(op);
    }
    return res;
  }


  /// "<method> (args)".
  ast::rCall
  ast_call (const yy::location& l,
            libport::Symbol method)
  {
    return ast_call(l, new ast::Implicit(l), method);
  }


  /// "<target> . <method> (args)".
  ast::rCall
  ast_call (const yy::location& l,
            ast::rExp target, libport::Symbol method, ast::exps_type* args)
  {
    ast::rCall res = new ast::Call(l, target, method, args);
    return res;
  }

  /// "<target> . <method> ()".
  ast::rCall
  ast_call(const yy::location& l, ast::rExp target, libport::Symbol method)
  {
    return ast_call(l, target, method, 0);
  }

  /// "<target> . <method> (<arg1>)".
  ast::rCall
  ast_call(const yy::location& l,
           ast::rExp target, libport::Symbol method, ast::rExp arg1)
  {
    ast::rCall res = ast_call(l, target, method, new ast::exps_type);
    res->arguments_get()->push_back(arg1);
    return res;
  }


  /// "<target> . <method> (<arg1>, <arg2>)".
  /// "<target> . <method> (<arg1>, <arg2>, <arg3>)".
  ast::rCall
  ast_call(const yy::location& l,
           ast::rExp target, libport::Symbol method,
           ast::rExp arg1, ast::rExp arg2, ast::rExp arg3)
  {
    ast::rCall res = ast_call(l, target, method, new ast::exps_type);
    res->arguments_get()->push_back(arg1);
    res->arguments_get()->push_back(arg2);
    if (arg3)
      res->arguments_get()->push_back(arg3);
    return res;
  }


  /// "class" lvalue block
  /// "class" lvalue
  ast::rExp
  ast_class(const yy::location& l,
            ast::rCall lvalue, ast::exps_type* protos, ast::rExp block)
  {
    ::parser::Tweast tweast;
    libport::Symbol name = lvalue->name_get();
    tweast << "var " << lvalue << "= {"
           << "var '$tmp' = Object.clone |";
    // Don't call setProtos from inside the do-scope: evaluate the
    // protos in the current scope.
    if (protos)
      tweast
        << "'$tmp'.setProtos([" << *protos << "])|";
    tweast
      << "do '$tmp'"
      << " {"
      <<   "var protoName = " << ast_string(l, name) << "|"
      <<   "function " << ("as" + name.name_get()) << "() { this }|"
      <<   ast_exp(block)
      << "} |"
      << "'$tmp'"
      << "}";
    ast::rExp res = parse(tweast)->ast_get();
    return res;
  }


  ast::rExp
  ast_closure(ast::rExp value)
  {
    static ast::ParametricAst a("closure () { %exp:1 }");
    return exp(a % value);
  }


  /// Build a for loop.
  // Since we don't have "continue", for is really a sugared
  // while:
  //
  // "for OP ( INIT; TEST; INC ) BODY"
  //
  // ->
  //
  // "{ INIT OP WHILE OP (TEST) { BODY | INC } }"
  //
  // OP is either ";" or "|".
  ast::rExp
  ast_for (const yy::location& l, ast::flavor_type op,
           ast::rExp init, ast::rExp test, ast::rExp inc,
           ast::rExp body)
  {
    passert(op, op == ast::flavor_pipe || op == ast::flavor_semicolon);
    assert(init);
    assert(test);
    assert(inc);
    assert(body);

    // BODY | INC.
    ast::rExp loop_body = ast_bin(l, ast::flavor_pipe, body, inc);

    // WHILE OP (TEST) { BODY | INC }.
    ast::While *while_loop =
      new ast::While(l, op, test, ast_scope(l, loop_body));

    // { INIT OP WHILE OP (TEST) { BODY | INC } }.
    return ast_scope(l, ast_bin(l, op, init, while_loop));
  }


  ast::rCall
  ast_lvalue_once(ast::rCall lvalue, Tweast& tweast)
  {
    if (!lvalue->target_implicit())
    {
      libport::Symbol tmp = libport::Symbol::fresh("__tmp__");
      const yy::location& l = lvalue->location_get();
      tweast << "var " << tmp << " = " << lvalue->target_get() << "|";
      lvalue = ast_call(l, ast_call(l, tmp), lvalue->name_get());
    }
    return lvalue;
  }


  /// Return \a e in a ast::Scope unless it is already one.
  ast::rAbstractScope
  ast_scope(const yy::location& l, ast::rExp target, ast::rExp e)
  {
    if (ast::rAbstractScope res = e.unsafe_cast<ast::AbstractScope>())
      return res;
    else if (target)
      return new ast::Do(l, e, target);
    else
      return new ast::Scope(l, e);
  }

  ast::rAbstractScope
  ast_scope(const yy::location& l, ast::rExp e)
  {
    return ast_scope(l, 0, e);
  }

  /*-----------------.
  | Changing slots.  |
  `-----------------*/

  /// Factor slot_set, slot_update, and slot_remove.
  /// \param l        source location.
  /// \param lvalue   object and slot to change.  This object is
  ///                 destroyed by this function: its contents is
  ///                 stolen, and its counter is decreased.  So the
  ///                 comments in the code for details.
  /// \param change   the Urbi method to invoke.
  /// \param value    optional assigned value.
  /// \param modifier optional time modifier object.
  /// \return The AST node calling the slot assignment.
  static
  inline
  ast::rExp
  ast_slot_change(const yy::location& l,
                  ast::rCall lvalue, libport::Symbol change,
                  ast::rExp value = 0,
                  ast::rExp modifier = 0)
  {
    ast::rExp res = 0;
    bool implicit = lvalue->target_implicit();

    /// FIXME: This is far from being enough, but it is hard to finish
    /// the implementation since the conversion to local vars
    /// vs. field is not finished yet.  In particular things might go
    /// wrong for the target (%exp:4) depending whether the assignment
    /// is to a local or to a field.
    if (modifier)
    {
      // Currently, we cannot have lvalues as meta-variables, which
      // prevents the use of ParametricAst :(
# if 0
      static ast::ParametricAst
        traj("TrajectoryGenerator"
             // startValue, targetValue, args.
             ".new(%exp:1, %exp:2, %exp:3)"
             // getter, setter.
             ".run(function () { %exp:4 },"
             "     function (v){ %exp:5 = v })");
      res = exp(traj
                %lvalue %value %modifier
                %new_clone(lvalue) %new_clone(lvalue));
# else
    Tweast tweast;
    lvalue = ast_lvalue_once(lvalue, tweast);
    tweast
      << "TrajectoryGenerator"
      << ".new("
      // getter.
      << "closure () { " << new_clone(lvalue) << " }, "
      // setter.
      << "closure (v){ " << lvalue << " = v }, "
      // targetValue, args.
      << value << ", " << modifier
      << ").run";
    res = parse(tweast)->ast_get();
# endif
    }
    else
    {
      if (implicit && change == SYMBOL(updateSlot))
        res = new ast::Assignment(l, lvalue->name_get(), value, 0);
      else if (implicit && change == SYMBOL(setSlot))
        res = new ast::Declaration(l, lvalue->name_get(), value);
      else
      {
        ast::rCall call =
          ast_call(l,
                   lvalue->target_get(), change,
                   ast::rString(new ast::String(lvalue->location_get(),
                                                lvalue->name_get())));
        if (value)
          call->arguments_get()->push_back(value);
        res = call;
      }
    }
    return res;
  }

  ast::rExp
  ast_slot_set(const yy::location& l, ast::rCall lvalue,
               ast::rExp value,
               ast::rExp modifier)
  {
    return ast_slot_change(l, lvalue, SYMBOL(setSlot), value, modifier);
  }

  ast::rExp
  ast_slot_update(const yy::location& l, ast::rCall lvalue,
                  ast::rExp value,
                  ast::rExp modifier)
  {
    return ast_slot_change(l, lvalue, SYMBOL(updateSlot), value, modifier);
  }

  ast::rExp
  ast_slot_remove(const yy::location& l, ast::rCall lvalue)
  {
    return ast_slot_change(l, lvalue, SYMBOL(removeSlot));
  }

  ast::rExp
  ast_string(const yy::location& l, libport::Symbol s)
  {
    return new ast::String(l, s);
  }

  ast::rExp
  ast_switch(const yy::location& l, ast::rExp cond, const cases_type& cases)
  {
    ::parser::Tweast tweast;
    libport::Symbol switched = libport::Symbol::fresh("switched");
    tweast << "var " << switched << " = ";
    tweast << cond << ";";
    static ast::ParametricAst nil("nil");
    ast::rExp inner = exp(nil);
    rforeach (const case_type& c, cases)
    {
      static ast::ParametricAst a(
        "if (Pattern.new(%exp:1).match(%exp:2)) %exp:3 else %exp:4");
      a % c.first
        % ast_call(l, switched)
        % c.second
        % inner;
      inner = ast::exp(a);
    }
    tweast << inner;
    return ::parser::parse(tweast)->ast_get();
  }
}
