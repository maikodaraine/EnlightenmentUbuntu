#include "elm.h"
#include "CElmGadget.h"

namespace elm {

using namespace v8;

GENERATE_TEMPLATE_FULL(CElmObject, CElmGadget);

CElmGadget::CElmGadget(Local <Object> _jsObject, CElmObject *parent)
   : CElmObject(_jsObject, elm_box_add(parent ? parent->GetEvasObject() : NULL))
{
}

CElmGadget::CElmGadget(Local<Object> _jsObject, Evas_Object *parent)
    : CElmObject(_jsObject, elm_box_add(parent))
{
   evas_object_show(eo);
}

Handle<Value> CElmGadget::CreateWithEvasObjectAsParent(Evas_Object *parent)
{
   CElmGadget *g = new CElmGadget(CElmObject::GetTemplate()->InstanceTemplate()->NewInstance(), elm_box_add(parent));
   return g->GetJSObject();
}

Handle<Value> CElmGadget::CreateWithExternalizedEvasObjectAsParent(const Arguments& args)
{
   return CreateWithEvasObjectAsParent((Evas_Object *)External::Cast(*args[0])->Value());
}

Handle<Value> CElmGadget::Pack(Handle<Value> obj, Handle<Value> replace)
{
   Local<Value> before = obj->ToObject()->Get(String::NewSymbol("before"));

   if (before->IsUndefined() && !replace->IsUndefined())
     before = replace->ToObject()->Get(String::NewSymbol("before"));
   else if (before->IsString() || before->IsNumber())
     before = GetJSObject()->Get(String::NewSymbol("elements"))->ToObject()->Get(before);

   obj = Realise(obj, GetJSObject());

   if (before->IsUndefined())
      elm_box_pack_end(eo, GetEvasObjectFromJavascript(obj));
   else
      elm_box_pack_before(eo, GetEvasObjectFromJavascript(obj),
                          GetEvasObjectFromJavascript(before));

   return obj;
}

Handle<Value> CElmGadget::Unpack(Handle<Value> obj)
{
   Handle<Object> replace = Object::New();
   Eina_List *list = elm_box_children_get(eo);
   Eina_List *l = eina_list_data_find_list(list, GetEvasObjectFromJavascript(obj));
   if ((l = eina_list_next(l)))
     {
        Evas_Object *_eo = (Evas_Object *)eina_list_data_get(l);
        CElmObject *before = static_cast<CElmObject *>(evas_object_data_get(_eo, "this"));

        if (before)
          replace->Set(String::NewSymbol("before"), before->GetJSObject());
     }
   eina_list_free(list);
   delete GetObjectFromJavascript(obj);
   return replace;
}

void CElmGadget::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Gadget"), GetTemplate()->GetFunction());
   target->Set(String::NewSymbol("NewGadgetFromEvasObject"), FunctionTemplate::New(CreateWithExternalizedEvasObjectAsParent)->GetFunction());
}

}