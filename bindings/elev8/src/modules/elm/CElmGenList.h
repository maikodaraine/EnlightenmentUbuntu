#ifndef C_ELM_GEN_LIST_H
#define C_ELM_GEN_LIST_H

#include "elm.h"
#include "CElmGen.h"
#include "CElmObject.h"

namespace elm {

class CElmGenList : public CElmObject {
private:
   static Persistent<FunctionTemplate> tmpl;

protected:
   CElmGenList(Local<Object> _jsObject, CElmObject *parent);
   ~CElmGenList();

   struct {
      Persistent<Value> longpress;
      Persistent<Value> scrolled_over_top_edge;
      Persistent<Value> scrolled_over_bottom_edge;
      Persistent<Value> scrolled_over_left_edge;
      Persistent<Value> scrolled_over_right_edge;
   } cb;

   Persistent<Value> scroller_policy;

   static Handle<FunctionTemplate> GetTemplate();

public:
   static void Initialize(Handle<Object> target);
   static void UpdateItem(Handle<Value> value);

   Handle<Value> realized_items_update(const Arguments& args);
   Handle<Value> tooltip_unset(const Arguments& args);
   Handle<Value> promote_item(const Arguments& args);
   Handle<Value> demote_item(const Arguments& args);
   Handle<Value> bring_in_item(const Arguments& args);
   Handle<Value> clear(const Arguments& args);

   virtual Handle<Value> Pack(Handle<Value>, Handle<Value>);
   virtual Handle<Value> Unpack(Handle<Value>);

   Handle<Value> multi_select_get() const;
   void multi_select_set(Handle<Value> value);

   Handle<Value> reorder_mode_get() const;
   void reorder_mode_set(Handle<Value> value);

   Handle<Value> mode_get() const;
   void mode_set(Handle<Value> value);

   Handle<Value> select_mode_get() const;
   void select_mode_set(Handle<Value> value);

   Handle<Value> block_count_get() const;
   void block_count_set(Handle<Value> value);

   Handle<Value>longpress_timeout_get() const;
   void longpress_timeout_set(Handle<Value> value);

   void vertical_bounce_set(Handle<Value> val);
   Handle<Value> vertical_bounce_get() const;

   void horizontal_bounce_set(Handle<Value> val);
   Handle<Value> horizontal_bounce_get() const;

   Handle<Value> highlight_mode_get() const;
   void highlight_mode_set(Handle<Value> value);

   Handle<Value> tree_effect_get() const;
   void tree_effect_set(Handle<Value> value);

   Handle<Value> decorate_mode_get() const;
   void decorate_mode_set(Handle<Value> value);

   Handle<Value> homogeneous_get() const;
   void homogeneous_set(Handle<Value> value);

   void OnLongPress(void *event_info);
   static void OnLongPressWrapper(void *data, Evas_Object *, void *event_info);
   Handle<Value> on_longpress_get() const;
   void on_longpress_set(Handle<Value> val);

   void OnScrolledOverEdge(Persistent<Value> edge_callback);

   static void OnScrolledOverTopEdgeWrapper(void *data, Evas_Object *, void *);
   Handle<Value> on_scrolled_over_top_edge_get() const;
   void on_scrolled_over_top_edge_set(Handle<Value> val);

   static void OnScrolledOverBottomEdgeWrapper(void *data, Evas_Object *, void *);
   Handle<Value> on_scrolled_over_bottom_edge_get() const;
   void on_scrolled_over_bottom_edge_set(Handle<Value> val);

   static void OnScrolledOverLeftEdgeWrapper(void *data, Evas_Object *, void *);
   Handle<Value> on_scrolled_over_left_edge_get() const;
   void on_scrolled_over_left_edge_set(Handle<Value> val);

   static void OnScrolledOverRightEdgeWrapper(void *data, Evas_Object *, void *);
   Handle<Value> on_scrolled_over_right_edge_get() const;
   void on_scrolled_over_right_edge_set(Handle<Value> val);

   Handle<Value> scroller_policy_get() const;
   void scroller_policy_set(Handle<Value> val);

   Handle<Value> items_count_get() const;

   static Handle<Value> GetSelected(Local<String>, const AccessorInfo &info);
   static void SetSelected(Local<String>, Local<Value> value, const AccessorInfo &info);

   static Handle<Value> GetTooltip(Local<String>, const AccessorInfo &info);
   static void SetTooltip(Local<String>, Local<Value> value, const AccessorInfo &info);

   static Handle<Value> BringIn(const Arguments &args);

   static Handle<Value> Index(const Arguments &args);

   static Handle<Value> Show(const Arguments &args);

   static Handle<Value> Update(const Arguments &args);

   friend Handle<Value> CElmObject::New<CElmGenList>(const Arguments&);
};

}

#endif

