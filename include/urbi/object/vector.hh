/*
 * Copyright (C) 2011, Gostai S.A.S.
 *
 * This software is provided "as is" without warranty of any kind,
 * either expressed or implied, including but not limited to the
 * implied warranties of fitness for a particular purpose.
 *
 * See the LICENSE file for more information.
 */

#ifndef OBJECT_VECTOR_HH
# define OBJECT_VECTOR_HH

# include <libport/uvector.hh>
# include <libport/umatrix.hh>
# include <urbi/object/cxx-object.hh>
# include <urbi/object/fwd.hh>

namespace urbi
{
  namespace object
  {

    /*--------------.
    | vector_type.  |
    `--------------*/

    using libport::vector_type;


    /*--------------.
    | matrix_type.  |
    `--------------*/

    using libport::matrix_type;


  }
}


namespace urbi
{
  namespace object
  {

    /*---------.
    | Vector.  |
    `---------*/

    class URBI_SDK_API Vector
      : public CxxObject
      , public EqualityComparable<Vector, vector_type>
    {
      friend class Matrix;

    public:
      typedef Vector self_type;
      typedef ::urbi::object::matrix_type matrix_type;
      typedef ::urbi::object::vector_type vector_type;
      typedef vector_type value_type;
      typedef EqualityComparable<self_type, value_type> super_comparable_type;

      Vector();
      Vector(size_t s);
      Vector(const rVector& v);
      Vector(const value_type& model);
      Vector(const objects_type& model);

      static rVector init(const objects_type& args);
      Vector* fromList(const objects_type& model);

      // Disambiguation operators.
      value_type operator+(const rObject& b) const;
      value_type operator-(const rObject& b) const;
      value_type operator*(const rObject& b) const;
      value_type operator/(const rObject& b) const;

      value_type operator+(const value_type& b) const;
      value_type operator-(const value_type& b) const;
      value_type operator*(const value_type& v) const;
      value_type operator/(const value_type& v) const;

      value_type operator+(ufloat v) const;
      value_type operator-(ufloat v) const;
      value_type operator*(ufloat v) const;
      value_type operator/(ufloat v) const;

      Vector* operator+();
      value_type operator-();
      virtual bool operator<(const Object& rhs) const;
      bool    operator<(const value_type& b) const;
      ufloat  operator[](int) const;

      size_t size() const;
      Vector* resize(size_t s);
      ufloat distance(const value_type& that) const;
      ufloat norm() const;
      ufloat sum() const;
      ufloat set(int i, ufloat v);
      size_t index(int) const;

      virtual std::string as_string() const;

      value_type&       value_get();
      const value_type& value_get() const;

      // Combinatorial operations generating a Matrix of size size*b.size().
      matrix_type combAdd(const value_type& b) const;
      matrix_type combDiv(const value_type& b) const;
      matrix_type combMul(const value_type& b) const;
      matrix_type combSub(const value_type& b) const;

      // Scalar field by field comparison operations
      value_type scalarGT(const value_type &b) const;
      value_type scalarGE(const value_type &b) const;
      value_type scalarLT(const value_type &b) const;
      value_type scalarLE(const value_type &b) const;
      value_type scalarEQ(const value_type &b) const;
      // Return the vector of true indices.
      value_type trueIndexes() const;
      rObject uvalueSerialize() const;
    private:
      value_type value_;
      URBI_CXX_OBJECT(Vector, CxxObject);
    };


    /*-------------.
    | Conversion.  |
    `-------------*/

    template <>
    struct CxxConvert<Vector::value_type>
    {
      typedef       Vector::value_type& target_type;
      typedef const Vector::value_type& source_type;
      static target_type
      to(const rObject& o)
      {
        type_check<Vector>(o);
        return o->as<Vector>()->value_get();
      }

      static rObject
      from(source_type v)
      {
        return new Vector(v);
      }
    };

  };
}

# include <urbi/object/vector.hxx>

#endif
