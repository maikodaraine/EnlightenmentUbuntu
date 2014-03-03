#include <v8.h>
#include <Ecore.h>
#include "utils_js.h"

using namespace v8;

namespace utils {

int log;

#define UTILS_DBG(...) EINA_LOG_DOM_DBG(log, __VA_ARGS__)
#define UTILS_INF(...) EINA_LOG_DOM_INFO(log, __VA_ARGS__)
#define UTILS_WRN(...) EINA_LOG_DOM_WARN(log, __VA_ARGS__)
#define UTILS_ERR(...) EINA_LOG_DOM_ERR(log, __VA_ARGS__)
#define UTILS_CRT(...) EINA_LOG_DOM_CRITICAL(log, __VA_ARGS__)

static Handle<Value>
deep_clone(Local<Value> value) {
  HandleScope scope;
  if (!value->IsObject())
    return Undefined();

  Local<Object> obj = value->ToObject();
  Local<Object> clone =obj->Clone();
  Local<Array> props = clone->GetOwnPropertyNames();

  for (unsigned int i = 0; i < props->Length(); i++)
    {
       Local<Value> key = props->Get(i);
       if (clone->Get(key)->IsObject())
         clone->Set(key, deep_clone(obj->Get(key)));
    }

  return scope.Close(clone);
}

static Handle<Value>
clone(const Arguments &args)
{
  Local<Value> object = args[0];
  Local<Value> deep = args[1];

  if (object->IsUndefined())
    return Undefined();

  if (!deep->BooleanValue())
    return object->ToObject()->Clone();

  return deep_clone(object);
}

static Handle<Value>
isObjectEmpty(const Arguments& args)
{
   HandleScope scope;
   Local<Object> object = args[0]->ToObject();

   if (object->IsUndefined())
     return Undefined();

   return Boolean::New(object->GetOwnPropertyNames()->Length() == 0);
}

void RegisterModule(Handle<ObjectTemplate> global)
{
   log = eina_log_domain_register("elev8-utils", EINA_COLOR_ORANGE);
   if (!log)
     {
        UTILS_ERR("could not register elev8-utils log domain.");
        log = EINA_LOG_DOMAIN_GLOBAL;
     }

   UTILS_INF("elev8-utils Logging initialized. %d", log);

   Handle<ObjectTemplate> tmpl = ObjectTemplate::New();
   tmpl->Set("clone", FunctionTemplate::New(clone));
   tmpl->Set("isEmpty", FunctionTemplate::New(isObjectEmpty));

   global->Set("utils", tmpl);
}

}
