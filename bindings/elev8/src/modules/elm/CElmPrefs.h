#ifndef C_ELM_PREFS_H
#define C_ELM_PREFS_H

#include "elm.h"
#include "CElmObject.h"

namespace elm {

using namespace v8;

class CElmPrefs : public CElmObject {
private:
   static Persistent<FunctionTemplate> tmpl;
   static Handle<Value> Getter(Local<String> prop, const AccessorInfo& info);
   static Handle<Value> Setter(Local<String> prop, Local<Value> val,
                               const AccessorInfo& info);

   static Handle<Value> VisibleGetter(Local<String> prop,
                                      const AccessorInfo& info);
   static void VisibleSetter(Local<String> prop, Local<Value> val,
                             const AccessorInfo& info);

   static Handle<Value> DisabledGetter(Local<String> prop,
                                       const AccessorInfo& info);
   static void DisabledSetter(Local<String> prop, Local<Value> val,
                              const AccessorInfo& info);

   static Handle<Value> EditableGetter(Local<String> prop,
                                       const AccessorInfo& info);
   static void EditableSetter(Local<String> prop, Local<Value> val,
                              const AccessorInfo& info);

   static void PageChangedWrapper(void *data, Evas_Object *, void *event_info);
   static void PageSavedWrapper(void *data, Evas_Object *, void *event_info);
   static void PageResetWrapper(void *data, Evas_Object *, void *event_info);
   static void PageLoadedWrapper(void *data, Evas_Object *, void *event_info);
   static void ItemChangedWrapper(void *data, Evas_Object *, void *event_info);
   static void ActionWrapper(void *data, Evas_Object *, void *event_info);

protected:
   CElmPrefs(Local<Object> _jsObject, CElmObject *parent);
   CElmPrefs(Local<Object> _jsObject, Evas_Object *child);
   ~CElmPrefs();

   static Handle<FunctionTemplate> GetTemplate();
   static Eet_File_Mode file_mode_from_string(Handle<Value> val);
   static Local<Value> string_from_file_mode(Eet_File_Mode file_mode);

   Persistent<Value> fileused;
   Persistent<Value> dataused;
public:
   static void Initialize(Handle<Object> target);

   Handle<Value> file_get() const;
   void file_set(Handle<Value> val);

   Handle<Value> data_get() const;
   void data_set(Handle<Value> val);

   void autosave_set(Handle<Value> val);
   Handle<Value> autosave_get() const;

   friend Handle<Value> CElmObject::New<CElmPrefs>(const Arguments& args);
};

}
#endif // C_ELM_PREFS_H
