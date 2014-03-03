#include "CElmLabel.h"

namespace elm {

using namespace v8;

GENERATE_PROPERTY_CALLBACKS(CElmLabel, wrap);
GENERATE_PROPERTY_CALLBACKS(CElmLabel, wrap_width);
GENERATE_PROPERTY_CALLBACKS(CElmLabel, ellipsis);
GENERATE_PROPERTY_CALLBACKS(CElmLabel, slide_mode);
GENERATE_PROPERTY_CALLBACKS(CElmLabel, slide_duration);

GENERATE_TEMPLATE(CElmLabel,
                  PROPERTY(wrap),
                  PROPERTY(wrap_width),
                  PROPERTY(ellipsis),
                  PROPERTY(slide_mode),
                  PROPERTY(slide_duration));

CElmLabel::CElmLabel(Local<Object> _jsObject, CElmObject *parent)
   : CElmObject(_jsObject, elm_label_add(parent->GetEvasObject()))
{
}

void CElmLabel::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Label"), GetTemplate()->GetFunction());
}

void CElmLabel::wrap_set(Handle<Value> wrap)
{
   String::Utf8Value mode_string(wrap->ToString());

   if (!strcmp(*mode_string, "none"))
     elm_label_line_wrap_set(eo, ELM_WRAP_NONE);
   else if (!strcmp(*mode_string, "char"))
     elm_label_line_wrap_set(eo, ELM_WRAP_CHAR);
   else if (!strcmp(*mode_string, "word"))
     elm_label_line_wrap_set(eo, ELM_WRAP_WORD);
   else if (!strcmp(*mode_string, "mixed"))
     elm_label_line_wrap_set(eo, ELM_WRAP_MIXED);
}

Handle<Value> CElmLabel::wrap_get() const
{
   switch (elm_label_line_wrap_get(eo)) {
     case ELM_WRAP_NONE:
       return String::NewSymbol("none");
     case ELM_WRAP_CHAR:
       return String::NewSymbol("char");
     case ELM_WRAP_WORD:
       return String::NewSymbol("word");
     case ELM_WRAP_MIXED:
       return String::NewSymbol("mixed");
     default:
       return String::NewSymbol("unknown");
   }
}

void CElmLabel::wrap_width_set(Handle<Value> value)
{
   if (value->IsNumber())
     elm_label_wrap_width_set(eo, value->IntegerValue());
}

Handle<Value> CElmLabel::wrap_width_get() const
{
   return Integer::New(elm_label_wrap_width_get(eo));
}

void CElmLabel::ellipsis_set(Handle<Value> value)
{
   elm_label_ellipsis_set(eo, value->BooleanValue());
}

Handle<Value> CElmLabel::ellipsis_get() const
{
   return Boolean::New(elm_label_ellipsis_get(eo));
}

void CElmLabel::slide_mode_set(Handle<Value> value)
{
   elm_label_slide_mode_set(eo, (Elm_Label_Slide_Mode)value->ToNumber()->Value());
   elm_label_slide_go(eo);
}

Handle<Value> CElmLabel::slide_mode_get() const
{
   return Integer::New(elm_label_slide_mode_get(eo));
}

void CElmLabel::slide_duration_set(Handle<Value> value)
{
   if (value->IsNumber())
     elm_label_slide_duration_set(eo, value->ToNumber()->Value());
}

Handle<Value> CElmLabel::slide_duration_get() const
{
   return Number::New(elm_label_slide_duration_get(eo));
}

}
