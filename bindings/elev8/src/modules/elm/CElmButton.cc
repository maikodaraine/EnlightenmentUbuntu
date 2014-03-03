#include "elm.h"
#include "CElmButton.h"
#include "CElmIcon.h"

namespace elm {

using namespace v8;

GENERATE_PROPERTY_CALLBACKS(CElmButton, icon);
GENERATE_PROPERTY_CALLBACKS(CElmButton, autorepeat);
GENERATE_PROPERTY_CALLBACKS(CElmButton, autorepeat_initial_timeout);
GENERATE_PROPERTY_CALLBACKS(CElmButton, autorepeat_gap_timeout);

GENERATE_TEMPLATE_FULL(CElmLayout, CElmButton,
                  PROPERTY(icon),
                  PROPERTY(autorepeat),
                  PROPERTY(autorepeat_initial_timeout),
                  PROPERTY(autorepeat_gap_timeout));
CElmButton::CElmButton(Local<Object> _jsObject, CElmObject *_parent)
   : CElmLayout(_jsObject, elm_button_add(_parent->GetEvasObject()))
{
}

CElmButton::CElmButton(Local<Object> _jsObject, Evas_Object *child)
   : CElmLayout(_jsObject, child)
{
}

void CElmButton::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Button"),
               GetTemplate()->GetFunction());
}

CElmButton::~CElmButton()
{
   cached.icon.Dispose();
}

Handle<Value> CElmButton::icon_get() const
{
   return cached.icon;
}

void CElmButton::icon_set(Handle<Value> value)
{
   cached.icon.Dispose();

   cached.icon = Persistent<Value>::New(Realise(value, jsObject));
   elm_object_content_set(eo,
                          GetEvasObjectFromJavascript(cached.icon));
}

Handle<Value> CElmButton::autorepeat_get() const
{
   return Boolean::New(elm_button_autorepeat_get(eo));
}

void CElmButton::autorepeat_set(Handle<Value> value)
{
   elm_button_autorepeat_set(eo, value->BooleanValue());
}

Handle<Value> CElmButton::autorepeat_initial_timeout_get() const
{
   return Number::New(elm_button_autorepeat_initial_timeout_get(eo));
}

void CElmButton::autorepeat_initial_timeout_set(Handle<Value> value)
{
   if (value->IsNumber())
      elm_button_autorepeat_initial_timeout_set(eo, value->NumberValue());
}

Handle<Value> CElmButton::autorepeat_gap_timeout_get() const
{
   return Number::New(elm_button_autorepeat_gap_timeout_get(eo));
}

void CElmButton::autorepeat_gap_timeout_set(Handle<Value> value)
{
   if (value->IsNumber())
      elm_button_autorepeat_gap_timeout_set(eo, value->NumberValue());
}

}
