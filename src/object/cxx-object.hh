#ifndef OBJECT_CXX_OBJECT_HH
# define OBJECT_CXX_OBJECT_HH

# include <vector>

# include <object/object.hh>

namespace object
{
  /// Base class for Urbi bound C++ classes.
  class CxxObject: public Object
  {
  public:

    /// Build a CxxObject.
    CxxObject();

    /// Bind all registered objects.
    /** This function should be called once to bind C++ objects.
     *  \param global Where to store the new classes.
     */
    static void initialize(rObject global);
    static void create();
    static void cleanup();

    /// Register a C++ class to be bound on the urbi side.
    /** \param T      The class to bind.
     *  \param name   Name of the class on the Urbi side.
     *  \param tgt    Where to store the result.
     */
    template<typename T>
    static bool add(const std::string& name, rObject& tgt);

    virtual std::string type_name_get() const = 0;

  private:

    class Initializer
    {
    public:
      Initializer(rObject& tgt);
      virtual ~Initializer();
      virtual rObject make_class() = 0;
      virtual void create() = 0;
      virtual libport::Symbol name() = 0;
    protected:
      rObject& res_;
    };

    template <typename T>
    class TypeInitializer: public Initializer
    {
    public:
      TypeInitializer(const std::string& name, rObject& tgt);
      virtual rObject make_class();
      virtual void create();
      virtual libport::Symbol name();
    private:
      libport::Symbol name_;
    };

    typedef std::vector<Initializer*> initializers_type;
    static initializers_type& initializers_get();

  public:
    /// Functor to bind methods on the urbi side.
    /** An instance of this class is given to the static initialize
     *  method of the bound classes. It can then be used to bind
     *  method and/or attributes on the Urbi side.
     */
    template <typename T>
    class Binder
    {
    public:
      /// Bind \a method with \a name
      template <typename M>
      void operator()(const libport::Symbol& name, M method);

    private:
      Binder(rObject tgt);
      // This method is allowed to construct a Binder.
      // VC++ is not a friend of friend method templates, so friend the class.
      friend class TypeInitializer<T>;
      rObject tgt_;
    };

  };

  /// Raise an exception if this is not of type \a T
  template<typename T>
  void type_check(const rObject& o, const libport::Symbol fun);


}

# if not defined(OBJECT_STRING_CLASS_HH) \
  && not defined(OBJECT_FLOAT_CLASS_HH)
#  include <object/cxx-object.hxx>
# endif

#endif
