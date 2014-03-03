#include "dbus-connection.h"
#include "dbus-pending.h"
#include "dbus-object.h"

namespace dbus {

Persistent<Function> DPending::constructor;

DPending::DPending(Eldbus_Pending *pending_)
  : pending(pending_)
{
   eldbus_pending_free_cb_add(pending, FreeCb, this);
   eldbus_pending_data_set(pending, "this", this);
}

void DPending::FreeCb(void *data, const void *)
{
   DPending *self = static_cast<DPending *>(data);
   self->pending = NULL;
   self->Unref();
}

void DPending::Init(Handle<Object>)
{
   HandleScope scope;

   Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
   tpl->SetClassName(String::NewSymbol("DPending"));
   tpl->InstanceTemplate()->SetInternalFieldCount(1);
   Local<ObjectTemplate> proto_t = tpl->PrototypeTemplate();

   proto_t->Set(String::NewSymbol("cancel"),
                FunctionTemplate::New(Cancel)->GetFunction());
   proto_t->Set(String::NewSymbol("onError"),
                FunctionTemplate::New(OnError)->GetFunction());
   proto_t->Set(String::NewSymbol("onComplete"),
                FunctionTemplate::New(OnComplete)->GetFunction());

   proto_t->SetAccessor(String::NewSymbol("destination"), Destination);
   proto_t->SetAccessor(String::NewSymbol("interface"), Interface);
   proto_t->SetAccessor(String::NewSymbol("method"), Method);
   proto_t->SetAccessor(String::NewSymbol("path"), Path);

   constructor = Persistent<Function>::New(tpl->GetFunction());
}

Handle<Value> DPending::New(const Arguments& args)
{
  HandleScope scope;

  Eldbus_Pending *wrapped = static_cast<Eldbus_Pending *>(External::Unwrap(args[0]));
  DPending *pending = new DPending(wrapped);
  pending->Wrap(args.This());
  pending->Ref();

  return args.This();
}

Handle<Value> DPending::NewInstance(Eldbus_Pending *pending)
{
  HandleScope scope;
  const unsigned argc = 1;
  Handle<Value> argv[argc] = { External::Wrap(pending) };
  return scope.Close(constructor->NewInstance(argc, argv));
}

Handle<Value> DPending::Cancel(const Arguments& args)
{
   DPending *self = ObjectWrap::Unwrap<DPending>(args.This());
   eldbus_pending_cancel(self->pending);
   return args.This();
}

Handle<Value> DPending::OnError(const Arguments& args)
{
   if (args[0]->IsFunction())
     args.This()->SetHiddenValue(String::New("onError"), args[0]);
   return args.This();
}

Handle<Value> DPending::OnComplete(const Arguments& args)
{
   if (args[0]->IsFunction())
     args.This()->SetHiddenValue(String::New("onComplete"), args[0]);
   return args.This();
}

Handle<Value> DPending::Destination(Local<String>, const AccessorInfo& info)
{
   DPending *self = ObjectWrap::Unwrap<DPending>(info.This());
   return String::New(eldbus_pending_destination_get(self->pending));
}

Handle<Value> DPending::Interface(Local<String>, const AccessorInfo& info)
{
   DPending *self = ObjectWrap::Unwrap<DPending>(info.This());
   return String::New(eldbus_pending_interface_get(self->pending));
}

Handle<Value> DPending::Method(Local<String>, const AccessorInfo& info)
{
   DPending *self = ObjectWrap::Unwrap<DPending>(info.This());
   return String::New(eldbus_pending_method_get(self->pending));
}

Handle<Value> DPending::Path(Local<String>, const AccessorInfo& info)
{
   DPending *self = ObjectWrap::Unwrap<DPending>(info.This());
   return String::New(eldbus_pending_path_get(self->pending));
}

Handle<Object> DPending::ToObject(Eldbus_Pending *pending)
{
   DPending *self = static_cast<DPending *>(eldbus_pending_data_get(pending, "this"));
   return self->handle_;
}


}
