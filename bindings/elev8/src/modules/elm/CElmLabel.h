#ifndef C_ELM_LABEL_H
#define C_ELM_LABEL_H

#include "elm.h"
#include "CElmObject.h"

namespace elm {

using namespace v8;

class CElmLabel : public CElmObject {
private:
   static Persistent<FunctionTemplate> tmpl;

protected:
   CElmLabel(Local<Object> _jsObject, CElmObject *parent);
   static Handle<FunctionTemplate> GetTemplate();

public:
   static void Initialize(Handle<Object> target);

   virtual void wrap_set(Handle<Value> wrap);
   virtual Handle<Value> wrap_get() const;

   void wrap_width_set(Handle<Value> value);
   Handle<Value> wrap_width_get() const;

   void ellipsis_set(Handle<Value> value);
   Handle<Value> ellipsis_get() const;

   void slide_mode_set(Handle<Value> value);
   Handle<Value> slide_mode_get() const;

   void slide_duration_set(Handle<Value> value);
   Handle<Value> slide_duration_get() const;

   friend Handle<Value> CElmObject::New<CElmLabel>(const Arguments& args);
};

}

#endif // C_ELM_LABEL_H
