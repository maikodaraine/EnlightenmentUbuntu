#ifndef DBUS_CONNECTION_H
#define DBUS_CONNECTION_H

#include "dbus-module.h"

using namespace v8;

namespace dbus {

class DConnection : public ObjectWrap {
public:
  static void Init(Handle<Object> target);

  Eldbus_Connection *GetConnection() { return conn; }

private:
  DConnection(Eldbus_Connection_Type type);
  ~DConnection();

  static Handle<Value> New(const Arguments& args);
  static Handle<Value> GetObject(const Arguments& args);

  Eldbus_Connection *conn;
};

}

#endif