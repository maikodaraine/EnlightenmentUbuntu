#ifndef C_ELM_GADGET_H
#define C_ELM_GADGET_H

#include "elm.h"
#include "CElmObject.h"

namespace elm {

using namespace v8;

class CElmGadget : public CElmObject {
private:
   static Persistent<FunctionTemplate> tmpl;

protected:
   CElmGadget(Local<Object> _jsObject, CElmObject *parent);
   CElmGadget(Local<Object> _jsObject, Evas_Object *parent);

   static Handle<FunctionTemplate> GetTemplate();

   static Handle<Value> CreateWithEvasObjectAsParent(Evas_Object *parent);

public:
   static Handle<Value> CreateWithExternalizedEvasObjectAsParent(const Arguments& args);

   static void Initialize(Handle<Object> target);

   virtual Handle<Value> Pack(Handle<Value>, Handle<Value>);
   virtual Handle<Value> Unpack(Handle<Value>);

   friend Handle<Value> CElmObject::New<CElmGadget>(const Arguments& args);
};

}

#endif // C_ELM_GADGET_H
