#ifndef EO_HH__
# define EO_HH__

#include <exception>

#include <Eo.h>

namespace efl {
  class eo_null_exception : public std::exception
  {
    virtual const char * what() const throw()
    {
      return "Eo source object is NULL";
    }
  } eo_null;

  class eo {
  public:
    eo() : iobj(NULL)
    {
    }
    eo(Eo *cobj) : iobj(cobj)
    {
      if (!iobj) throw efl::eo_null;
    }
    eo(Eo_Class *cclass, Eo *cparent)
    {
      iobj = eo_add(cclass, cparent);
      if (!iobj) throw efl::eo_null;
    }

    ~eo() {};

    void invalidate();
    bool valid() const;

    eo & operator=(Eo *c)
    {
      _unref();
      iobj = c;
      _ref();
    }

    const eo & operator=(const Eo *c)
    {
      _unref();
      iobj = const_cast<Eo*> (c);
      _ref();
    }

    operator Eo *() { return iobj; }

    bool operator==(const eo &c) const { return iobj == c.iobj; }
    bool operator!=(const eo &c) const { return iobj != c.iobj; }

    bool vdo(Eo_Op_Type op_type, ...) {
      va_list ops;
      Eina_Bool ret;

      va_start(ops, op_type);
      ret = eo_vdo_internal(iobj, op_type, &ops);
      va_end(ops);

      return ret ? true : false;
    }

  protected:
    virtual void _unref();
    virtual void _ref();

    void assign(const eo &r);
    void assign(eo &r);

    Eo *iobj;
  };

  template <class X>
  class weak_eo : public eo {

  public:
    weak_eo<X> & operator=(const eo &c)
    {
      assign(c);
      return *this;
    }
    weak_eo<X> & operator=(eo &c)
    {
      assign(c);
      return *this;
    }

    weak_eo<X> & operator=(Eo *c)
    {
      _unref();
      iobj = c;
      _ref();
    }
    const weak_eo<X> & operator=(const Eo *c)
    {
      _unref();
      iobj = c;
      _ref();
    }

  protected:
    virtual void _unref()
    {
      eo_weak_unref(&iobj);
    }

    virtual void _ref()
    {
      eo_weak_ref(&iobj);
    }
  };

  template <class X>
  class shared_eo : public eo {
  public:
    shared_eo<X> & operator=(const eo &c)
    {
      assign(c);
      return *this;
    }
    shared_eo<X> & operator=(eo &c)
    {
      assign(c);
      return *this;
    }

    shared_eo<X> & operator=(Eo *c)
    {
      _unref();
      iobj = c;
      _ref();
    }
    const shared_eo<X> & operator=(const Eo *c)
    {
      _unref();
      iobj = c;
      _ref();
    }

  protected:
    virtual void _unref()
    {
      eo_xunref(iobj, NULL);
    }

    virtual void _ref()
    {
      eo_xref(iobj, NULL);
    }
  };
    
#include "Eo.xx"
};

#endif
