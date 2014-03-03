#ifndef C_ELM_GEN
#define C_ELM_GEN

#include "elm.h"
#include "CElmObject.h"
#include <v8.h>

namespace elm {
namespace gen {

using namespace v8;

template <class T> class ItemClass;

template <class T>
struct Item {
   Persistent<Object> jsObject;
   Persistent<Object> tooltip;
   Elm_Object_Item *object_item;

   static Persistent<String> str_item;
   static Persistent<String> str_before;
   static Persistent<String> str_class;
   static Persistent<String> str_parent;

   Item(Handle<Value> value, Handle<Value> parent)
   {
      static Persistent<ObjectTemplate> tmpl;
      if (tmpl.IsEmpty())
        {
           Local<FunctionTemplate> klass = FunctionTemplate::New();
           klass->SetClassName(String::NewSymbol("GenItem"));

           Local<ObjectTemplate> prototype = klass->PrototypeTemplate();
           prototype->Set(String::NewSymbol("bring_in"), FunctionTemplate::New(T::BringIn));
           prototype->Set(String::NewSymbol("index"), FunctionTemplate::New(T::Index));
           prototype->Set(String::NewSymbol("show"), FunctionTemplate::New(T::Show));
           prototype->Set(String::NewSymbol("update"), FunctionTemplate::New(T::Update));
           prototype->SetAccessor(String::NewSymbol("selected"), T::GetSelected, T::SetSelected);
           prototype->SetAccessor(String::NewSymbol("tooltip"), T::GetTooltip, T::SetTooltip);

           tmpl = Persistent<ObjectTemplate>::New(klass->InstanceTemplate());
           tmpl->SetNamedPropertyHandler(ElementGet, ElementSet);
        }

      jsObject = Persistent<Object>::New(tmpl->NewInstance());
      jsObject->SetHiddenValue(str_item, External::Wrap(this));
      jsObject->SetHiddenValue(str_parent, parent);

      Local<Object> obj = value->ToObject();
      Local<Array> properties = obj->GetOwnPropertyNames();

      for (unsigned int i = 0; i < properties->Length(); ++i)
        {
           Local<Value> key = properties->Get(i);
           jsObject->ForceSet(key, obj->Get(key));
        }
   }

   ~Item()
   {
      tooltip.Dispose();
      jsObject->DeleteHiddenValue(str_item);
      jsObject.Dispose();
   }

   Elm_Gen_Item_Class *GetElmClass()
   {
      HandleScope scope;
      ItemClass<T> *item_class;
      Local<Value> value = this->jsObject->Get(str_class);
      if (value->IsUndefined()) return NULL;
      Local<Value> klass = value->ToObject()->GetHiddenValue(str_class);
      if (klass.IsEmpty())
        item_class = new ItemClass<T>(value);
      else
        item_class = static_cast<ItemClass<T> *>(External::Unwrap(klass));

      return item_class->GetElmClass();
   }

   static void OnSelect(void *data, Evas_Object *, void *)
   {
      HandleScope scope;
      Item<T> *item = static_cast<Item<T> *>(data);
      Local<Value> callback = item->jsObject->Get(String::NewSymbol("on_select"));
      if (!callback->IsFunction()) return;
      Handle<Value> args[1] = { item->jsObject };
      Function::Cast(*callback)->Call(item->jsObject, 1, args);
   }

   static Handle<Value> ElementSet(Local<String> attr, Local<Value> val, const AccessorInfo& info)
   {
      if (!info.This()->GetRealNamedPropertyInPrototypeChain(attr).IsEmpty())
        return Handle<Value>();

      info.This()->ForceSet(attr, val);
      T::UpdateItem(info.This());
      return val;
   }

   static Handle<Value> ElementGet(Local<String> , const AccessorInfo &)
   {
     return Handle<Value>();
   }

   static Item<T> *Unwrap(const AccessorInfo& info)
   {
      return Unwrap(info.This());
   }

   static Item<T> *Unwrap(Handle<Value> value)
   {
      if (!value->IsObject())
        return NULL;
      value = value->ToObject()->GetHiddenValue(str_item);
      if (value.IsEmpty())
        return NULL;
      return static_cast<Item<T> *>(External::Unwrap(value));
   }
};

template <class T>
Persistent<String> Item<T>::str_parent = Persistent<String>::New(String::NewSymbol("elm::gen::parent"));
template <class T>
Persistent<String> Item<T>::str_item = Persistent<String>::New(String::NewSymbol("elm::gen::item"));
template <class T>
Persistent<String> Item<T>::str_class = Persistent<String>::New(String::NewSymbol("class"));
template <class T>
Persistent<String> Item<T>::str_before = Persistent<String>::New(String::NewSymbol("before"));

template <class T>
class ItemClass {
private:
   Elm_Genlist_Item_Class klass;

   static Persistent<String> str_text;
   static Persistent<String> str_state;
   static Persistent<String> str_delete;
   static Persistent<String> str_content;

   static Handle<Value> Call(void *_item, Handle<String> func, const char *part)
   {
      HandleScope scope;
      Item<T> *item = static_cast<Item<T> *>(_item);
      Local<Value> klass = item->jsObject->Get(Item<T>::str_class);
      if (!klass->IsObject()) return Undefined();
      Local<Value> callback = klass->ToObject()->Get(func);
      if (!callback->IsFunction()) return Undefined();
      if (!part) return Function::Cast(*callback)->Call(item->jsObject, 0, NULL);
      Handle<Value> args[2] = { String::New(part), item->jsObject };
      return Function::Cast(*callback)->Call(item->jsObject, 2, args);
   }

   static char *GetTextWrapper(void *item, Evas_Object *, const char *part)
   {
      HandleScope scope;
      Handle<Value> result = Call(item, str_text, part);
      return result->IsUndefined() ? NULL : strdup(*String::Utf8Value(result));
   }

   static Evas_Object *GetContentWrapper(void *_item, Evas_Object *, const char *part)
   {
      HandleScope scope;
      Handle<Value> result = Call(_item, str_content, part);
      if (result.IsEmpty() || !result->IsObject())
         return NULL;

      /* FIXME: This might leak. Investigate. */
      Item<T> *item = static_cast<Item<T> *>(_item);
      Handle<Value> content = CElmObject::Realise(result->ToObject(),
            item->jsObject->GetHiddenValue(Item<T>::str_parent));

      Local<Value> propagate = content->ToObject()->Get(String::New("propagate_events"));

      if (!propagate->IsUndefined())
        evas_object_propagate_events_set(GetEvasObjectFromJavascript(content),
                                         propagate->BooleanValue());

      return content.IsEmpty() ? NULL : GetEvasObjectFromJavascript(content);
   }

   static Eina_Bool GetStateWrapper(void *item, Evas_Object *, const char *part)
   {
      HandleScope scope;
      return Call(item, str_state, part)->BooleanValue();
   }

   static void DeleteItemFromElementary(void *item, Evas_Object *)
   {
      HandleScope scope;
      Call(item, str_delete, NULL);
      delete static_cast<Item<T> *>(item);
   }

   static void Destroy(Persistent<Value>, void *self)
   {
      delete static_cast<ItemClass<T> *>(self);
   }

public:

   ItemClass(Handle<Value> value)
   {
      Handle<Object> obj = value->ToObject();

      klass.func.text_get = GetTextWrapper;
      klass.func.content_get = GetContentWrapper;
      klass.func.state_get = GetStateWrapper;
      klass.func.del = DeleteItemFromElementary;

      Handle<Value> style = obj->Get(String::NewSymbol("style"));
      if (!style->IsString())
         klass.item_style = eina_stringshare_add("default");
      else
         klass.item_style = eina_stringshare_add(*String::Utf8Value(style->ToString()));

      Persistent<Value> self = Persistent<Value>::New(External::Wrap(this));
      self.MakeWeak(this, Destroy);
      obj->SetHiddenValue(Item<T>::str_class, self);
   }

   ~ItemClass()
   {
      eina_stringshare_del(klass.item_style);
   }

   Elm_Gen_Item_Class *GetElmClass() { return &klass; }
};

template <class T>
Persistent<String> ItemClass<T>::str_text = Persistent<String>::New(String::NewSymbol("text"));
template <class T>
Persistent<String> ItemClass<T>::str_state = Persistent<String>::New(String::NewSymbol("state"));
template <class T>
Persistent<String> ItemClass<T>::str_delete = Persistent<String>::New(String::NewSymbol("delete"));
template <class T>
Persistent<String> ItemClass<T>::str_content = Persistent<String>::New(String::NewSymbol("content"));
}
}

#endif
