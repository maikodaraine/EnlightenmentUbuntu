#include "elm.h"
#include "CElmPrefs.h"

namespace elm {

using namespace v8;

GENERATE_PROPERTY_CALLBACKS(CElmPrefs, file);
GENERATE_PROPERTY_CALLBACKS(CElmPrefs, data);
GENERATE_PROPERTY_CALLBACKS(CElmPrefs, autosave);

GENERATE_TEMPLATE_FULL(CElmObject, CElmPrefs,
                  PROPERTY(file),
                  PROPERTY(data),
                  PROPERTY(autosave));

CElmPrefs::CElmPrefs(Local<Object> _jsObject, CElmObject *_parent)
     : CElmObject(_jsObject, elm_prefs_add(_parent->GetEvasObject()))
{
   HandleScope scope;
   Local<FunctionTemplate> tpl = FunctionTemplate::New();
   tpl->SetClassName(String::NewSymbol("PrefsItems"));

   Local<ObjectTemplate> inst_t = tpl->InstanceTemplate();
   inst_t->SetNamedPropertyHandler(Getter, Setter);

   Local<Value> items = inst_t->NewInstance();
   items->ToObject()->SetHiddenValue(String::NewSymbol("prefs"), jsObject);

   jsObject->Set(String::NewSymbol("items"), items);

   evas_object_smart_callback_add(GetEvasObjectFromJavascript(jsObject),
                                  "page,changed", &PageChangedWrapper, this);
   evas_object_smart_callback_add(GetEvasObjectFromJavascript(jsObject),
                                  "page,saved", &PageSavedWrapper, this);
   evas_object_smart_callback_add(GetEvasObjectFromJavascript(jsObject),
                                  "page,reset", &PageResetWrapper, this);
   evas_object_smart_callback_add(GetEvasObjectFromJavascript(jsObject),
                                  "page,loaded", &PageLoadedWrapper, this);
   evas_object_smart_callback_add(GetEvasObjectFromJavascript(jsObject),
                                  "item,changed", &ItemChangedWrapper, this);
   evas_object_smart_callback_add(GetEvasObjectFromJavascript(jsObject),
                                  "action", &ActionWrapper, this);
}

CElmPrefs::~CElmPrefs()
{
   fileused.Dispose();
   dataused.Dispose();
   evas_object_smart_callback_del(eo, "page,changed", &PageChangedWrapper);
   evas_object_smart_callback_del(eo, "page,saved", &PageSavedWrapper);
   evas_object_smart_callback_del(eo, "page,reset", &PageResetWrapper);
   evas_object_smart_callback_del(eo, "page,loaded", &PageLoadedWrapper);
   evas_object_smart_callback_del(eo, "item,changed", &ItemChangedWrapper);
   evas_object_smart_callback_del(eo, "action", &ActionWrapper);
}

Handle<Value> CElmPrefs::file_get() const
{
   return fileused;
}

void CElmPrefs::file_set(Handle<Value> val)
{
   Local<Object> obj = val->ToObject();
   Local<Value> name = obj->Get(String::NewSymbol("name"));
   Local<Value> page = obj->Get(String::NewSymbol("page"));

   if (!name->IsString())
     return;

   if (!elm_prefs_file_set(eo, *String::Utf8Value(name), page->IsUndefined() ?
                           NULL : *String::Utf8Value(page)))
     return;

   fileused.Dispose();
   fileused = Persistent<Value>::New(val);
}

Eet_File_Mode CElmPrefs::file_mode_from_string(Handle<Value> val)
{
   String::Utf8Value str(val);
   Eet_File_Mode file_mode = EET_FILE_MODE_READ;

   if (!strcmp(*str, "read"))
     file_mode = EET_FILE_MODE_READ;
   else if (!strcmp(*str, "write"))
     file_mode = EET_FILE_MODE_WRITE;
   else if (!strcmp(*str, "read_write"))
     file_mode = EET_FILE_MODE_READ_WRITE;
   else
     ELM_ERR("unknown eet file mode %s", *str);

   return file_mode;
}

Local<Value> CElmPrefs::string_from_file_mode(Eet_File_Mode file_mode)
{
   switch (file_mode) {
   case EET_FILE_MODE_READ:
       return String::NewSymbol("read");
   case EET_FILE_MODE_WRITE:
       return String::NewSymbol("write");
   case EET_FILE_MODE_READ_WRITE:
       return String::NewSymbol("read_write");
   default:
       return String::NewSymbol("unknown");
   }
}

Handle<Value> CElmPrefs::data_get() const
{
   return dataused;
}

void CElmPrefs::data_set(Handle<Value> val)
{
   Local<Object> obj = val->ToObject();
   Local<Value> name = obj->Get(String::NewSymbol("name"));
   Local<Value> page = obj->Get(String::NewSymbol("page"));
   Local<Value> mode = obj->Get(String::NewSymbol("mode"));

   if (!name->IsString() || !mode->IsString())
     return;

   Eet_File_Mode file_mode;
   file_mode = file_mode_from_string(mode);

   if (!elm_prefs_data_set(eo,
                           elm_prefs_data_new(*String::Utf8Value(name),
                                              page->IsUndefined() ?
                                              NULL : *String::Utf8Value(page),
                                              file_mode)))
     return;

   dataused.Dispose();
   dataused = Persistent<Value>::New(val);
}

void CElmPrefs::autosave_set(Handle<Value> val)
{
   elm_prefs_autosave_set(eo, val->BooleanValue());
}

Handle<Value> CElmPrefs::autosave_get() const
{
   return Boolean::New(elm_prefs_autosave_get(eo));
}

Handle<Value> CElmPrefs::VisibleGetter(Local<String>, const AccessorInfo& info)
{
   HandleScope scope;
   Local<Value> prefs = info.This()->GetHiddenValue(String::NewSymbol("prefs"));
   Local<Value> itname = info.This()->GetHiddenValue(String::NewSymbol("name"));
   Evas_Object *eo = GetEvasObjectFromJavascript(prefs);

   return scope.Close
      (Boolean::New(elm_prefs_item_visible_get(eo, *String::Utf8Value(itname))));
}

void CElmPrefs::VisibleSetter(Local<String>, Local<Value> val,
                              const AccessorInfo& info)
{
   Local<Value> prefs = info.This()->GetHiddenValue(String::NewSymbol("prefs"));
   Local<Value> itname = info.This()->GetHiddenValue(String::NewSymbol("name"));
   Evas_Object *eo = GetEvasObjectFromJavascript(prefs);

   elm_prefs_item_visible_set(eo, *String::Utf8Value(itname),
                              val->BooleanValue());
}

Handle<Value> CElmPrefs::DisabledGetter(Local<String>, const AccessorInfo& info)
{
   HandleScope scope;
   Local<Value> prefs = info.This()->GetHiddenValue(String::NewSymbol("prefs"));
   Local<Value> itname = info.This()->GetHiddenValue(String::NewSymbol("name"));
   Evas_Object *eo = GetEvasObjectFromJavascript(prefs);

   return scope.Close
      (Boolean::New(elm_prefs_item_disabled_get(eo, *String::Utf8Value(itname))));
}

void CElmPrefs::DisabledSetter(Local<String>, Local<Value> val,
                               const AccessorInfo& info)
{
   Local<Value> prefs = info.This()->GetHiddenValue(String::NewSymbol("prefs"));
   Local<Value> itname = info.This()->GetHiddenValue(String::NewSymbol("name"));
   Evas_Object *eo = GetEvasObjectFromJavascript(prefs);

   elm_prefs_item_disabled_set(eo, *String::Utf8Value(itname),
                               val->BooleanValue());
}

Handle<Value> CElmPrefs::EditableGetter(Local<String>, const AccessorInfo& info)
{
   HandleScope scope;
   Local<Value> prefs = info.This()->GetHiddenValue(String::NewSymbol("prefs"));
   Local<Value> itname = info.This()->GetHiddenValue(String::NewSymbol("name"));
   Evas_Object *eo = GetEvasObjectFromJavascript(prefs);

   return scope.Close
      (Boolean::New(elm_prefs_item_editable_get(eo, *String::Utf8Value(itname))));
}

void CElmPrefs::EditableSetter(Local<String>, Local<Value> val,
                               const AccessorInfo& info)
{
   Local<Value> prefs = info.This()->GetHiddenValue(String::NewSymbol("prefs"));
   Local<Value> itname = info.This()->GetHiddenValue(String::NewSymbol("name"));
   Evas_Object *eo = GetEvasObjectFromJavascript(prefs);

   elm_prefs_item_editable_set(eo, *String::Utf8Value(itname),
                               val->BooleanValue());
}

Handle<Value> CElmPrefs::Getter(Local<String> name, const AccessorInfo& info)
{
   HandleScope scope;
   Local<Value> prefs = info.This()->GetHiddenValue(String::NewSymbol("prefs"));
   Evas_Object *eo = GetEvasObjectFromJavascript(prefs);

   Local<Value> val;
   Eina_Value value;
   const Eina_Value_Type *type;

   if (elm_prefs_item_value_get(eo, *String::Utf8Value(name), &value))
     type = eina_value_type_get(&value);
   else
     {
        ELM_ERR("cant get type/value of item %s", *String::Utf8Value(name));
        return Undefined();
     }

   if (type == EINA_VALUE_TYPE_INT)
     {
        int cvalue;
        eina_value_get(&value, &cvalue);
        val = NumberObject::New(cvalue);
     }
   else if (type == EINA_VALUE_TYPE_FLOAT)
     {
        float cvalue;
        eina_value_get(&value, &cvalue);
        val = NumberObject::New(cvalue);
     }
   else if (type == EINA_VALUE_TYPE_UCHAR)
     {
        Eina_Bool cvalue;
        eina_value_get(&value, &cvalue);
        val = BooleanObject::New(cvalue);
     }
   else if (type == EINA_VALUE_TYPE_STRINGSHARE)
     {
        const char *cvalue;
        eina_value_get(&value, &cvalue);
        val = StringObject::New(String::New(cvalue));
     }
   else if (type == EINA_VALUE_TYPE_TIMEVAL)
     {
        struct timeval cvalue;
        eina_value_get(&value, &cvalue);
        val = Date::New(cvalue.tv_sec * 1000.0 + cvalue.tv_usec / 1000.0);
     }
   else
     {
        ELM_ERR("unknown/invalid eina value type %s", type->name);
        return Undefined();
     }

   Local<Object> obj = val->ToObject();
   obj->SetAccessor(String::NewSymbol("visible"), VisibleGetter,
                    VisibleSetter);
   obj->SetAccessor(String::NewSymbol("disabled"), DisabledGetter,
                    DisabledSetter);
   obj->SetAccessor(String::NewSymbol("editable"), EditableGetter,
                    EditableSetter);
   obj->SetHiddenValue(String::NewSymbol("prefs"), prefs);
   obj->SetHiddenValue(String::NewSymbol("name"), name);

   return scope.Close(val);
}

Handle<Value> CElmPrefs::Setter(Local<String> name, Local<Value> val,
                                const AccessorInfo& info)
{
   HandleScope scope;
   Local<Value> prefs = info.This()->GetHiddenValue(String::NewSymbol("prefs"));
   Evas_Object *eo = GetEvasObjectFromJavascript(prefs);

   if (!name->IsString() || val->IsUndefined())
     return Handle<Value>();

   Eina_Value value;
   const Eina_Value_Type *type;

   if (elm_prefs_item_value_get(eo, *String::Utf8Value(name), &value))
     type = eina_value_type_get(&value);
   else
     {
        ELM_ERR("cant get type/value of item %s", *String::Utf8Value(name));
        return Undefined();
     }

   if (type == EINA_VALUE_TYPE_INT)
     eina_value_set(&value, val->Int32Value());
   else if (type == EINA_VALUE_TYPE_FLOAT)
     eina_value_set(&value, val->NumberValue());
   else if (type == EINA_VALUE_TYPE_UCHAR)
     eina_value_set(&value, val->BooleanValue());
   else if (type == EINA_VALUE_TYPE_STRINGSHARE)
     eina_value_set(&value, *String::Utf8Value(val));
   else if (type == EINA_VALUE_TYPE_TIMEVAL)
     {
        struct timeval cvalue;
        cvalue.tv_sec = val->NumberValue() / 1000.0;
        cvalue.tv_usec = ((long int) (val->NumberValue() * 1000) % 1000);
        eina_value_set(&value, cvalue);
     }
   else
     {
        ELM_ERR("unknown/invalid eina value type %s", type->name);
        return Undefined();
     }

   elm_prefs_item_value_set(eo, *String::Utf8Value(name), &value);

   return scope.Close(val);
}

void CElmPrefs::PageChangedWrapper(void *data, Evas_Object *, void *event_info)
{
   HandleScope scope;
   Handle<Object> obj = static_cast<CElmPrefs*>(data)->jsObject;
   Handle<Function> callback(Function::Cast(*obj->Get(String::NewSymbol("on_page_changed"))));

   if (!callback->IsFunction()) return;

   Handle<Value> args[1] = { String::New((char*) event_info) };
   callback->Call(obj, 1, args);
}

void CElmPrefs::PageSavedWrapper(void *data, Evas_Object *, void *event_info)
{
   HandleScope scope;
   Handle<Object> obj = static_cast<CElmPrefs*>(data)->jsObject;
   Handle<Function> callback(Function::Cast(*obj->Get(String::NewSymbol("on_page_saved"))));

   if (!callback->IsFunction()) return;

   Handle<Value> args[1] = { String::New((char*) event_info) };
   callback->Call(obj, 1, args);
}

void CElmPrefs::PageResetWrapper(void *data, Evas_Object *, void *event_info)
{
   HandleScope scope;
   Handle<Object> obj = static_cast<CElmPrefs*>(data)->jsObject;
   Handle<Function> callback(Function::Cast(*obj->Get(String::NewSymbol("on_page_reset"))));

   if (!callback->IsFunction()) return;

   Handle<Value> args[1] = { String::New((char*) event_info) };
   callback->Call(obj, 1, args);
}

void CElmPrefs::PageLoadedWrapper(void *data, Evas_Object *, void *event_info)
{
   HandleScope scope;
   Handle<Object> obj = static_cast<CElmPrefs*>(data)->jsObject;
   Handle<Function> callback(Function::Cast(*obj->Get(String::NewSymbol("on_page_loaded"))));

   if (!callback->IsFunction()) return;

   Handle<Value> args[1] = { String::New((char*) event_info) };
   callback->Call(obj, 1, args);
}

void CElmPrefs::ItemChangedWrapper(void *data, Evas_Object *, void *event_info)
{
   HandleScope scope;
   Handle<Object> obj = static_cast<CElmPrefs*>(data)->jsObject;
   Handle<Function> callback(Function::Cast(*obj->Get(String::NewSymbol("on_item_changed"))));

   if (!callback->IsFunction()) return;

   Handle<Value> args[1] = { String::New((char*) event_info) };
   callback->Call(obj, 1, args);
}

void CElmPrefs::ActionWrapper(void *data, Evas_Object *, void *event_info)
{
   HandleScope scope;
   Handle<Object> obj = static_cast<CElmPrefs*>(data)->jsObject;
   Handle<Function> callback(Function::Cast(*obj->Get(String::NewSymbol("on_action"))));

   if (!callback->IsFunction()) return;

   Handle<Value> args[1] = { String::New((char*) event_info) };
   callback->Call(obj, 1, args);
}

void CElmPrefs::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Prefs"), GetTemplate()->GetFunction());
}

}
