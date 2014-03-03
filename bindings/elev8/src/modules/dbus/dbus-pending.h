#ifndef DBUS_PENDING_H
#define DBUS_PENDING_H

#include "dbus-module.h"

using namespace v8;

namespace dbus {

class DPending : public ObjectWrap {
  Eldbus_Pending *pending;
public:
  static void Init(Handle<Object> target);

  Eldbus_Connection *GetConnection() { return conn; }
  static Handle<Value> NewInstance(Eldbus_Pending *pending);
  static Handle<Object> ToObject(Eldbus_Pending *pending);

private:

  static Persistent<Function> constructor;

  DPending(Eldbus_Pending *pending_);

  static void FreeCb(void *data, const void *deadptr);

  static Handle<Value> New(const Arguments& args);
  static Handle<Value> Cancel(const Arguments &args);
  static Handle<Value> OnError(const Arguments &args);
  static Handle<Value> OnComplete(const Arguments &args);
  static Handle<Value> GetObject(const Arguments& args);

  static Handle<Value> Destination(Local<String>, const AccessorInfo& info);
  static Handle<Value> Interface(Local<String>, const AccessorInfo& info);
  static Handle<Value> Method(Local<String>, const AccessorInfo& info);
  static Handle<Value> Path(Local<String>, const AccessorInfo& info);

  Eldbus_Connection *conn;
};

}

#endif
