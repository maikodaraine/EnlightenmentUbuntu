#include "elm.h"
#include "CElmGenList.h"

namespace elm {

using namespace v8;
using namespace gen;

GENERATE_PROPERTY_CALLBACKS(CElmGenList, homogeneous);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, decorate_mode);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, tree_effect);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, highlight_mode);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, vertical_bounce);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, horizontal_bounce);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, longpress_timeout);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, block_count);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, select_mode);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, mode);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, reorder_mode);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, multi_select);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, on_longpress);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, scroller_policy);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, on_scrolled_over_top_edge);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, on_scrolled_over_bottom_edge);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, on_scrolled_over_left_edge);
GENERATE_PROPERTY_CALLBACKS(CElmGenList, on_scrolled_over_right_edge);
GENERATE_RO_PROPERTY_CALLBACKS(CElmGenList, items_count);
GENERATE_METHOD_CALLBACKS(CElmGenList, realized_items_update);
GENERATE_METHOD_CALLBACKS(CElmGenList, tooltip_unset);
GENERATE_METHOD_CALLBACKS(CElmGenList, promote_item);
GENERATE_METHOD_CALLBACKS(CElmGenList, demote_item);
GENERATE_METHOD_CALLBACKS(CElmGenList, bring_in_item);
GENERATE_METHOD_CALLBACKS(CElmGenList, clear);

GENERATE_TEMPLATE(CElmGenList,
                  PROPERTY(homogeneous),
                  PROPERTY(decorate_mode),
                  PROPERTY(tree_effect),
                  PROPERTY(highlight_mode),
                  PROPERTY(horizontal_bounce),
                  PROPERTY(vertical_bounce),
                  PROPERTY(longpress_timeout),
                  PROPERTY(block_count),
                  PROPERTY(select_mode),
                  PROPERTY(mode),
                  PROPERTY(reorder_mode),
                  PROPERTY(multi_select),
                  PROPERTY(on_longpress),
                  PROPERTY(scroller_policy),
                  PROPERTY(on_scrolled_over_top_edge),
                  PROPERTY(on_scrolled_over_bottom_edge),
                  PROPERTY(on_scrolled_over_left_edge),
                  PROPERTY(on_scrolled_over_right_edge),
                  PROPERTY_RO(items_count),
                  METHOD(realized_items_update),
                  METHOD(tooltip_unset),
                  METHOD(promote_item),
                  METHOD(demote_item),
                  METHOD(bring_in_item),
                  METHOD(clear));

CElmGenList::CElmGenList(Local<Object> _jsObject, CElmObject *parent)
   : CElmObject(_jsObject, elm_genlist_add(elm_object_top_widget_get(parent->GetEvasObject())))
{
}

CElmGenList::~CElmGenList()
{
   scroller_policy.Dispose();
   on_longpress_set(Undefined());
}

void CElmGenList::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Genlist"), GetTemplate()->GetFunction());
}

Handle<Value> CElmGenList::Pack(Handle<Value> value, Handle<Value> replace)
{
   Item<CElmGenList> *item = new Item<CElmGenList>(value, jsObject);
   if (!item)
     return Undefined();
   Local<Value> before = item->jsObject->Get(Item<CElmGenList>::str_before);

   if (before->IsUndefined() && replace->IsObject())
     before = replace->ToObject()->Get(Item<CElmGenList>::str_before);

   if (before->IsString() || before->IsNumber())
     before = GetJSObject()->Get(String::NewSymbol("elements"))->ToObject()->Get(before);

   Item<CElmGenList> *before_item = Item<CElmGenList>::Unwrap(before);

   Elm_Genlist_Item_Type type;
   if (item->jsObject->Get(String::NewSymbol("group"))->BooleanValue())
     type = ELM_GENLIST_ITEM_GROUP;
   else
     type = ELM_GENLIST_ITEM_NONE;

   Elm_Object_Item *parent = NULL;
   Handle<Value> parent_obj = item->jsObject->Get(String::NewSymbol("parent"));
   if (parent_obj->IsObject())
     parent = Item<CElmGenList>::Unwrap(parent_obj->ToObject())->object_item;

   if (!before_item)
     item->object_item = elm_genlist_item_append(eo, item->GetElmClass(),
                                                 item, parent, type,
                                                 Item<CElmGenList>::OnSelect, item);
   else
     item->object_item = elm_genlist_item_insert_before(eo, item->GetElmClass(),
                                                        item, parent,
                                                        before_item->object_item,
                                                        type,
                                                        Item<CElmGenList>::OnSelect, item);
   if (type == ELM_GENLIST_ITEM_GROUP)
     elm_genlist_item_select_mode_set(item->object_item,
                                         ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);

   elm_object_item_data_set(item->object_item, item);
   return item->jsObject;
}

Handle<Value> CElmGenList::Unpack(Handle<Value> value)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(value);

   if (!item)
     return Undefined();

   if (value->IsObject())
     {
        Local<Object> obj = value->ToObject();
        if (obj->Get(Item<CElmGenList>::str_before)->IsUndefined())
          {
             Elm_Object_Item *before = elm_genlist_item_next_get(item->object_item);
             if (before)
               {
                  Item<CElmGenList> *before_item = static_cast< Item<CElmGenList> *>
                     (elm_object_item_data_get(before));
                  obj->ForceSet(Item<CElmGenList>::str_before, before_item->jsObject);
               }
          }
     }
   elm_object_item_del(item->object_item);
   return value;
}

void CElmGenList::UpdateItem(Handle<Value> value)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(value);
   if (item)
     elm_genlist_item_item_class_update(item->object_item, item->GetElmClass());
}

Handle<Value> CElmGenList::tooltip_unset(const Arguments &args)
{
   Item<CElmGenList> *item = static_cast<Item<CElmGenList> *>(External::Unwrap(args[0]));
   if (!item)
     return Undefined();

   if (item->object_item)
     elm_genlist_item_tooltip_unset(item->object_item);

   return Undefined();
}

Handle<Value> CElmGenList::promote_item(const Arguments &args)
{
   Item<CElmGenList> *item = static_cast<Item<CElmGenList> *>(External::Unwrap(args[0]));
   if (!item)
     return Undefined();

   if (item->object_item)
     elm_genlist_item_promote(item->object_item);

   return Undefined();
}

Handle<Value> CElmGenList::demote_item(const Arguments &args)
{
   Item<CElmGenList> *item = static_cast<Item<CElmGenList> *>(External::Unwrap(args[0]));
   if (!item)
     return Undefined();

   if (item->object_item)
     elm_genlist_item_demote(item->object_item);

   return Undefined();
}

Handle<Value> CElmGenList::multi_select_get() const
{
   return Boolean::New(elm_genlist_multi_select_get(eo));
}

void CElmGenList::multi_select_set(Handle<Value> value)
{
   elm_genlist_multi_select_set(eo, value->BooleanValue());
}

Handle<Value> CElmGenList::reorder_mode_get() const
{
   return Boolean::New(elm_genlist_reorder_mode_get(eo));
}

void CElmGenList::reorder_mode_set(Handle<Value> value)
{
   elm_genlist_reorder_mode_set(eo, value->BooleanValue());
}

Handle<Value> CElmGenList::mode_get() const
{
   switch (elm_genlist_mode_get(eo))
     {
        case ELM_LIST_COMPRESS: return String::NewSymbol("compress");
        case ELM_LIST_SCROLL: return String::NewSymbol("scroll");
        case ELM_LIST_LIMIT: return String::NewSymbol("limit");
        case ELM_LIST_EXPAND: return String::NewSymbol("expand");
        default: return String::NewSymbol("unknown");
     }
}

void CElmGenList::mode_set(Handle<Value> value)
{
   String::Utf8Value mode_string(value->ToString());

   if (!strcmp(*mode_string, "compress"))
     elm_genlist_mode_set(eo, ELM_LIST_COMPRESS);
   else if (!strcmp(*mode_string, "scroll"))
     elm_genlist_mode_set(eo, ELM_LIST_SCROLL);
   else if (!strcmp(*mode_string, "limit"))
     elm_genlist_mode_set(eo, ELM_LIST_LIMIT);
   else if (!strcmp(*mode_string, "expand"))
     elm_genlist_mode_set(eo, ELM_LIST_EXPAND);
}

Handle<Value> CElmGenList::select_mode_get() const
{
   return Number::New(elm_genlist_select_mode_get(eo));
}

void CElmGenList::select_mode_set(Handle<Value> value)
{
   if (value->IsNumber())
     elm_genlist_select_mode_set(eo,(Elm_Object_Select_Mode)value->ToNumber()->Value());
}

Handle<Value> CElmGenList::block_count_get() const
{
   return Number::New(elm_genlist_block_count_get(eo));
}

void CElmGenList::block_count_set(Handle<Value> value)
{
   if (value->IsNumber())
     elm_genlist_block_count_set(eo, value->IntegerValue());
}

Handle<Value> CElmGenList::longpress_timeout_get() const
{
   return Number::New(elm_genlist_longpress_timeout_get(eo));
}

void CElmGenList::longpress_timeout_set(Handle<Value> value)
{
   if (value->IsNumber())
     elm_genlist_longpress_timeout_set(eo, value->IntegerValue());
}

void CElmGenList::vertical_bounce_set(Handle<Value> val)
{
   Eina_Bool h;
   elm_scroller_bounce_get(eo, &h, NULL);
   elm_scroller_bounce_set(eo, h, val->BooleanValue());
}

Handle<Value> CElmGenList::vertical_bounce_get() const
{
   Eina_Bool v;
   elm_scroller_bounce_get(eo, NULL, &v);
   return Boolean::New(v);
}

void CElmGenList::horizontal_bounce_set(Handle<Value> val)
{
   Eina_Bool v;
   elm_scroller_bounce_get(eo, NULL, &v);
   elm_scroller_bounce_set(eo, val->BooleanValue(), v);
}

Handle<Value> CElmGenList::horizontal_bounce_get() const
{
   Eina_Bool h;
   elm_scroller_bounce_get(eo, &h, NULL);
   return Boolean::New(h);
}

Handle<Value> CElmGenList::highlight_mode_get() const
{
   return Boolean::New(elm_genlist_highlight_mode_get(eo));
}

void CElmGenList::highlight_mode_set(Handle<Value> value)
{
   elm_genlist_highlight_mode_set(eo, value->BooleanValue());
}

Handle<Value> CElmGenList::tree_effect_get() const
{
   return Boolean::New(elm_genlist_tree_effect_enabled_get(eo));
}

void CElmGenList::tree_effect_set(Handle<Value> value)
{
   elm_genlist_tree_effect_enabled_set(eo, value->BooleanValue());
}

Handle<Value> CElmGenList::decorate_mode_get() const
{
   return Boolean::New(elm_genlist_decorate_mode_get(eo));
}

void CElmGenList::decorate_mode_set(Handle<Value> value)
{
   elm_genlist_decorate_mode_set(eo, value->BooleanValue());
}

Handle<Value> CElmGenList::homogeneous_get() const
{
   return Number::New(elm_genlist_homogeneous_get(eo));
}

void CElmGenList::homogeneous_set(Handle<Value> value)
{
   if (value->IsNumber())
     elm_genlist_homogeneous_set(eo, value->IntegerValue());
}

void CElmGenList::OnLongPress(void *event_info)
{
   HandleScope scope;
   Handle<Function> callback(Function::Cast(*cb.longpress));
   Item<CElmGenList> *item = static_cast< Item<CElmGenList> *>
      (elm_object_item_data_get((Elm_Object_Item *)event_info));
   Handle<Value> args[2] = { item->jsObject, External::Wrap(item) };
   callback->Call(jsObject, 2, args);
}

void CElmGenList::OnLongPressWrapper(void *data, Evas_Object *, void *event_info)
{
   static_cast<CElmGenList*>(data)->OnLongPress(event_info);
}

Handle<Value> CElmGenList::on_longpress_get() const
{
   return cb.longpress;
}

void CElmGenList::on_longpress_set(Handle<Value> val)
{
   if (!cb.longpress.IsEmpty())
     {
        evas_object_smart_callback_del(eo, "longpressed", &OnLongPressWrapper);
        cb.longpress.Dispose();
        cb.longpress.Clear();
     }

   if (!val->IsFunction())
     return;

   cb.longpress = Persistent<Value>::New(val);
   evas_object_smart_callback_add(eo, "longpressed", &OnLongPressWrapper, this);
}

Handle<Value> CElmGenList::scroller_policy_get() const
{
   return scroller_policy;
}

void CElmGenList::scroller_policy_set(Handle<Value> val)
{
   if (!val->IsArray())
     return;

   Local<Object> policy = val->ToObject();
   elm_scroller_policy_set (eo,
        (Elm_Scroller_Policy) policy->Get(0)->ToNumber()->Value(),
        (Elm_Scroller_Policy) policy->Get(1)->ToNumber()->Value());

   scroller_policy.Dispose();
   scroller_policy = Persistent<Value>::New(val);
}

Handle<Value> CElmGenList::items_count_get() const
{
   return Number::New(elm_genlist_items_count(eo));
}

Handle<Value> CElmGenList::realized_items_update(const Arguments&)
{
   elm_genlist_realized_items_update(eo);
   return Undefined();
}

Handle<Value> CElmGenList::bring_in_item(const Arguments& args)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(args[0]);

   if (!item || !item->object_item)
     return Undefined();

   Elm_Genlist_Item_Scrollto_Type scroll_type = ELM_GENLIST_ITEM_SCROLLTO_NONE;
   if (args[1]->IsString())
     {
        String::Utf8Value s(args[1]->ToString());

        if (!strcmp(*s, "none"))
          scroll_type = ELM_GENLIST_ITEM_SCROLLTO_NONE;
        else if (!strcmp(*s, "in"))
          scroll_type = ELM_GENLIST_ITEM_SCROLLTO_IN;
        else if (!strcmp(*s, "top"))
          scroll_type = ELM_GENLIST_ITEM_SCROLLTO_TOP;
        else if (!strcmp(*s, "middle"))
          scroll_type = ELM_GENLIST_ITEM_SCROLLTO_MIDDLE;
     }

   elm_genlist_item_bring_in(item->object_item, scroll_type);

   return Undefined();
}

void CElmGenList::OnScrolledOverEdge(Persistent<Value> edge_callback)
{
   HandleScope scope;
   Handle<Function> callback(Function::Cast(*edge_callback));
   Handle<Value> args[1] = { };
   callback->Call(jsObject, 0, args);
}

void CElmGenList::OnScrolledOverTopEdgeWrapper(void *data, Evas_Object *, void *)
{
   CElmGenList *self = static_cast<CElmGenList*>(data);
   self->OnScrolledOverEdge(self->cb.scrolled_over_top_edge);
}

Handle<Value> CElmGenList::on_scrolled_over_top_edge_get() const
{
   return cb.scrolled_over_top_edge;
}

void CElmGenList::on_scrolled_over_top_edge_set(Handle<Value> val)
{
   if (!cb.scrolled_over_top_edge.IsEmpty())
     {
        evas_object_smart_callback_del(eo, "edge,top", &OnScrolledOverTopEdgeWrapper);
        cb.scrolled_over_top_edge.Dispose();
        cb.scrolled_over_top_edge.Clear();
     }

   if (!val->IsFunction())
     return;

   cb.scrolled_over_top_edge = Persistent<Value>::New(val);
   evas_object_smart_callback_add(eo, "edge,top", &OnScrolledOverTopEdgeWrapper, this);
}

void CElmGenList::OnScrolledOverRightEdgeWrapper(void *data, Evas_Object *, void *)
{
   CElmGenList *self = static_cast<CElmGenList*>(data);
   self->OnScrolledOverEdge(self->cb.scrolled_over_right_edge);
}

Handle<Value> CElmGenList::on_scrolled_over_right_edge_get() const
{
   return cb.scrolled_over_right_edge;
}

void CElmGenList::on_scrolled_over_right_edge_set(Handle<Value> val)
{
   if (!cb.scrolled_over_right_edge.IsEmpty())
     {
        evas_object_smart_callback_del(eo, "edge,right", &OnScrolledOverRightEdgeWrapper);
        cb.scrolled_over_right_edge.Dispose();
        cb.scrolled_over_right_edge.Clear();
     }

   if (!val->IsFunction())
     return;

   cb.scrolled_over_right_edge = Persistent<Value>::New(val);
   evas_object_smart_callback_add(eo, "edge,right", &OnScrolledOverRightEdgeWrapper, this);
}

void CElmGenList::OnScrolledOverBottomEdgeWrapper(void *data, Evas_Object *, void *)
{
   CElmGenList *self = static_cast<CElmGenList*>(data);
   self->OnScrolledOverEdge(self->cb.scrolled_over_bottom_edge);
}

Handle<Value> CElmGenList::on_scrolled_over_bottom_edge_get() const
{
   return cb.scrolled_over_bottom_edge;
}

void CElmGenList::on_scrolled_over_bottom_edge_set(Handle<Value> val)
{
   if (!cb.scrolled_over_bottom_edge.IsEmpty())
     {
        evas_object_smart_callback_del(eo, "edge,bottom", &OnScrolledOverBottomEdgeWrapper);
        cb.scrolled_over_bottom_edge.Dispose();
        cb.scrolled_over_bottom_edge.Clear();
     }

   if (!val->IsFunction())
     return;

   cb.scrolled_over_bottom_edge = Persistent<Value>::New(val);
   evas_object_smart_callback_add(eo, "edge,bottom", &OnScrolledOverBottomEdgeWrapper, this);
}

void CElmGenList::OnScrolledOverLeftEdgeWrapper(void *data, Evas_Object *, void *)
{
   CElmGenList *self = static_cast<CElmGenList*>(data);
   self->OnScrolledOverEdge(self->cb.scrolled_over_left_edge);
}

Handle<Value> CElmGenList::on_scrolled_over_left_edge_get() const
{
   return cb.scrolled_over_left_edge;
}

void CElmGenList::on_scrolled_over_left_edge_set(Handle<Value> val)
{
   if (!cb.scrolled_over_left_edge.IsEmpty())
     {
        evas_object_smart_callback_del(eo, "edge,left", &OnScrolledOverLeftEdgeWrapper);
        cb.scrolled_over_left_edge.Dispose();
        cb.scrolled_over_left_edge.Clear();
     }

   if (!val->IsFunction())
     return;

   cb.scrolled_over_left_edge = Persistent<Value>::New(val);
   evas_object_smart_callback_add(eo, "edge,left", &OnScrolledOverLeftEdgeWrapper, this);
}

Handle<Value> CElmGenList::clear(const Arguments &args)
{
   if (args[0]->IsArray())
     {
        Local<Object> obj = args[0]->ToObject();
        Local<Array> props = obj->GetOwnPropertyNames();

        for (unsigned int i = 0, len = props->Length(); i < len; i++)
          {
             Local<Value> val = obj->Get(props->Get(i));
             Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(val);

             if (!item)
               item = Item<CElmGenList>::Unwrap
                  (args.This()->Get(String::NewSymbol("elements"))->ToObject()->Get(val));

             if (item)
               elm_object_item_del(item->object_item);
          }

        return Undefined();
     }

   elm_genlist_clear(eo);
   return Undefined();
}

void CElmGenList::SetSelected(Local<String>, Local<Value> value, const AccessorInfo &info)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(info);
   elm_genlist_item_selected_set(item->object_item, value->BooleanValue());
}

Handle<Value> CElmGenList::GetSelected(Local<String>, const AccessorInfo &info)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(info);
   return Boolean::New(elm_genlist_item_selected_get(item->object_item));
}

void CElmGenList::SetTooltip(Local<String>, Local<Value> value, const AccessorInfo &info)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(info);
   item->tooltip.Dispose();

   if (value->IsUndefined() || !value->ToObject()->IsObject())
     {
        item->tooltip.Clear();
        elm_genlist_item_tooltip_unset(item->object_item);
        return;
     }
   item->tooltip = Persistent<Object>::New(value->ToObject()->Clone());

   Local<Object> obj = value->ToObject();
   Handle<Value> style = obj->Get(String::NewSymbol("style"));
   Handle<Value> text = obj->Get(String::NewSymbol("text"));
   Handle<Value> window_mode = obj->Get(String::NewSymbol("window_mode"));

   elm_genlist_item_tooltip_text_set(item->object_item, *String::Utf8Value(text));

   if (style->IsString())
     elm_genlist_item_tooltip_style_set(item->object_item, *String::Utf8Value(style));

   elm_genlist_item_tooltip_window_mode_set(item->object_item, window_mode->BooleanValue());

}

Handle<Value> CElmGenList::GetTooltip(Local<String>, const AccessorInfo &info)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(info);
   return item->tooltip;
}

Handle<Value> CElmGenList::BringIn(const Arguments &args)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(args.This());

   if (!args.Length())
     {
        elm_genlist_item_bring_in(item->object_item, ELM_GENLIST_ITEM_SCROLLTO_NONE);
        return Undefined();
     }

   String::Utf8Value mode(args[0]);

   if (!strcmp(*mode, "in"))
     elm_genlist_item_bring_in(item->object_item, ELM_GENLIST_ITEM_SCROLLTO_IN);
   else if (!strcmp(*mode, "top"))
     elm_genlist_item_bring_in(item->object_item, ELM_GENLIST_ITEM_SCROLLTO_TOP);
   else if (!strcmp(*mode, "middle"))
     elm_genlist_item_bring_in(item->object_item, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
   else
     elm_genlist_item_bring_in(item->object_item, ELM_GENLIST_ITEM_SCROLLTO_NONE);

   return Undefined();
}

Handle<Value> CElmGenList::Index(const Arguments &args)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(args.This());
   return Uint32::New(elm_genlist_item_index_get(item->object_item));
}

Handle<Value> CElmGenList::Show(const Arguments &args)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(args.This());

   if (!args.Length())
     {
        elm_genlist_item_bring_in(item->object_item, ELM_GENLIST_ITEM_SCROLLTO_NONE);
        return Undefined();
     }

   String::Utf8Value mode(args[0]);

   if (!strcmp(*mode, "in"))
     elm_genlist_item_show(item->object_item, ELM_GENLIST_ITEM_SCROLLTO_IN);
   else if (!strcmp(*mode, "top"))
     elm_genlist_item_show(item->object_item, ELM_GENLIST_ITEM_SCROLLTO_TOP);
   else if (!strcmp(*mode, "middle"))
     elm_genlist_item_show(item->object_item, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
   else
     elm_genlist_item_show(item->object_item, ELM_GENLIST_ITEM_SCROLLTO_NONE);

   return Undefined();
}

Handle<Value> CElmGenList::Update(const Arguments &args)
{
   Item<CElmGenList> *item = Item<CElmGenList>::Unwrap(args.This());
   elm_genlist_item_update(item->object_item);

   return Undefined();
}

}
