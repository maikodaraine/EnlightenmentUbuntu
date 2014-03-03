#ifndef DBUS_PROXY_H
#define DBUS_PROXY_H

#include "dbus-module.h"
#include "dbus-object.h"

using namespace v8;

namespace dbus {

class DProxy : public ObjectWrap {
public:
  static void Init(Handle<Object> target);
  static Handle<Value> NewInstance(DObject *obj, const Arguments &args);

  Eldbus_Proxy *GetProxy() { return proxy; }
  Persistent<Value> properties;

private:
  DProxy(DObject *_obj, const char *_iface);
  ~DProxy();

  static Persistent<Function> constructor;
  static Handle<Value> New(const Arguments& args);

  static Handle<Value> AddSignalHandler(const Arguments &args);
  static Handle<Value> RemoveSignalHandler(const Arguments &args);
  static Handle<Value> Send(const Arguments &args);
  static Handle<Value> GetProperties(const Arguments &args);

  static void Send_Cb(void *data, const Eldbus_Message *msg, Eldbus_Pending *pending);

  static Handle<Value> Getter(Local<String> prop, const AccessorInfo& info);

  Eldbus_Proxy *proxy;
  DObject *obj;
  char *iface;
};

}

#endif
