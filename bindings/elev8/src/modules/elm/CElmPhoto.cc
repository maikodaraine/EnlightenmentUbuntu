#include "CElmPhoto.h"

namespace elm {

using namespace v8;

GENERATE_PROPERTY_CALLBACKS(CElmPhoto, size);
GENERATE_PROPERTY_CALLBACKS(CElmPhoto, fill);
GENERATE_PROPERTY_CALLBACKS(CElmPhoto, image);
GENERATE_PROPERTY_CALLBACKS(CElmPhoto, fixed_aspect);
GENERATE_PROPERTY_CALLBACKS(CElmPhoto, editable);
GENERATE_PROPERTY_CALLBACKS(CElmPhoto, thumb);

GENERATE_TEMPLATE_FULL(CElmObject, CElmPhoto,
                  PROPERTY(size),
                  PROPERTY(fill),
                  PROPERTY(image),
                  PROPERTY(fixed_aspect),
                  PROPERTY(editable),
                  PROPERTY(thumb));

CElmPhoto::CElmPhoto(Local<Object> _jsObject, CElmObject *parent)
   : CElmObject(_jsObject, elm_photo_add(parent->GetEvasObject()))
{
}

CElmPhoto::~CElmPhoto(){
   editable.Dispose();
   thumb.Dispose();
}

void CElmPhoto::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Photo"), GetTemplate()->GetFunction());
}

Handle <Value> CElmPhoto::image_get() const
{
   //No getter available
   return Undefined();
}

void CElmPhoto::image_set(Handle <Value> val)
{
   if (!val->IsString())
     return;

   String::Utf8Value str(val);

   if (0 > access(*str, R_OK))
     ELM_ERR("warning: can't read image file %s", *str);

   if (!elm_photo_file_set(eo, *str))
     ELM_ERR("Unable to set the image");
}

Handle <Value> CElmPhoto::size_get() const
{
   //No getter available
   return Undefined();
}

void CElmPhoto::size_set(Handle <Value> val)
{
   if (val->IsNumber())
     elm_photo_size_set(eo, val->ToInt32()->Value());
}

Handle <Value> CElmPhoto::fill_get() const
{
   //No getter available
   return Undefined();
}

void CElmPhoto::fill_set(Handle <Value> val)
{
   elm_photo_fill_inside_set(eo, val->BooleanValue());
}

Handle<Value> CElmPhoto::fixed_aspect_get() const
{
   return Boolean::New(elm_photo_aspect_fixed_get(eo));
}

void CElmPhoto::fixed_aspect_set(Handle<Value> val)
{
   elm_photo_aspect_fixed_set(eo, val->BooleanValue());
}

Handle<Value> CElmPhoto::editable_get() const
{
   return editable;
}

void CElmPhoto::editable_set(Handle<Value> val){

   elm_photo_editable_set(eo, val->BooleanValue());

   editable.Dispose();
   editable = Persistent<Value>::New(val);
}

Handle<Value> CElmPhoto::thumb_get() const
{
   return thumb;
}

void CElmPhoto::thumb_set(Handle<Value> val)
{
   if (!val->IsObject())
     return;

   Local<Value> file = val->ToObject()->Get(String::NewSymbol("file"));
   Local<Value> group = val->ToObject()->Get(String::NewSymbol("group"));

   if (!file->IsString() || !group->IsString())
     return;

   elm_photo_thumb_set(eo, *String::Utf8Value(file), *String::Utf8Value(group));

   thumb.Dispose();
   thumb = Persistent<Value>::New(val);
}

}
