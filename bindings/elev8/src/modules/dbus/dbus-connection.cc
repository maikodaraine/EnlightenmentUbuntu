#include "dbus-connection.h"
#include "dbus-object.h"

namespace dbus {

DConnection::DConnection(Eldbus_Connection_Type type)
  : conn(eldbus_connection_ref(eldbus_connection_get(type)))
{
}

DConnection::~DConnection()
{
  eldbus_connection_unref(conn);
}

void DConnection::Init(Handle<Object> target)
{
  HandleScope scope;

  Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
  tpl->SetClassName(String::NewSymbol("DConnection"));
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  tpl->PrototypeTemplate()->Set(String::NewSymbol("getObject"),
     FunctionTemplate::New(GetObject)->GetFunction());

  Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
  target->Set(String::NewSymbol("Connection"), constructor);
}

Handle<Value> DConnection::New(const Arguments& args)
{
  HandleScope scope;
  Eldbus_Connection_Type type;

  if (args[0]->IsString()) {
    String::Utf8Value v(args[0]->ToString());

    if (!strcmp(*v, "session"))
      type = ELDBUS_CONNECTION_TYPE_SESSION;
    else if (!strcmp(*v, "system"))
      type = ELDBUS_CONNECTION_TYPE_SYSTEM;
    else if (!strcmp(*v, "starter"))
      type = ELDBUS_CONNECTION_TYPE_STARTER;
    else
      THROW_EXCEPTION("Wrong connection type");
  } else {
    type = ELDBUS_CONNECTION_TYPE_SESSION;
  }

  DConnection *obj = new DConnection(type);
  if (!obj->conn)
    THROW_EXCEPTION("Could not obtain DBus connection");

  obj->Wrap(args.This());

  return args.This();
}

Handle<Value> DConnection::GetObject(const Arguments& args)
{
  DConnection *conn = ObjectWrap::Unwrap<DConnection>(args.This());
  return DObject::NewInstance(conn, args);
}

}
