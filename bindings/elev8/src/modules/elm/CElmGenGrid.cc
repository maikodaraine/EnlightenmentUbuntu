#include "elm.h"
#include "CElmGenGrid.h"

namespace elm {

using namespace v8;
using namespace elm::gen;

GENERATE_PROPERTY_CALLBACKS(CElmGenGrid, item_size_horizontal);
GENERATE_PROPERTY_CALLBACKS(CElmGenGrid, item_size_vertical);
GENERATE_PROPERTY_CALLBACKS(CElmGenGrid, highlight_mode);
GENERATE_PROPERTY_CALLBACKS(CElmGenGrid, select_mode);
GENERATE_PROPERTY_CALLBACKS(CElmGenGrid, reorder_mode);
GENERATE_PROPERTY_CALLBACKS(CElmGenGrid, multi_select);
GENERATE_PROPERTY_CALLBACKS(CElmGenGrid, horizontal);
GENERATE_PROPERTY_CALLBACKS(CElmGenGrid, filled);
GENERATE_PROPERTY_CALLBACKS(CElmGenGrid, item_align);
GENERATE_PROPERTY_CALLBACKS(CElmGenGrid, group_item_size);
GENERATE_RO_PROPERTY_CALLBACKS(CElmGenGrid, items_count);
GENERATE_RO_PROPERTY_CALLBACKS(CElmGenGrid, realized_items);
GENERATE_METHOD_CALLBACKS(CElmGenGrid, realized_items_update);
GENERATE_METHOD_CALLBACKS(CElmGenGrid, bring_in_item);
GENERATE_METHOD_CALLBACKS(CElmGenGrid, clear);

GENERATE_TEMPLATE_FULL(CElmObject, CElmGenGrid,
                  PROPERTY(item_size_horizontal),
                  PROPERTY(item_size_vertical),
                  PROPERTY(highlight_mode),
                  PROPERTY(select_mode),
                  PROPERTY(reorder_mode),
                  PROPERTY(multi_select),
                  PROPERTY(horizontal),
                  PROPERTY(filled),
                  PROPERTY(item_align),
                  PROPERTY(group_item_size),
                  PROPERTY_RO(items_count),
                  PROPERTY_RO(realized_items),
                  METHOD(realized_items_update),
                  METHOD(bring_in_item),
                  METHOD(clear));

CElmGenGrid::CElmGenGrid(Local<Object> _jsObject, CElmObject *p)
   : CElmObject(_jsObject, elm_gengrid_add(elm_object_top_widget_get(p->GetEvasObject())))
{
   elm_gengrid_item_size_set(eo, 64, 64);
}

CElmGenGrid::~CElmGenGrid()
{
   align.Dispose();
}

void CElmGenGrid::Initialize(Handle<Object> target)
{
   target->Set(String::NewSymbol("Gengrid"), GetTemplate()->GetFunction());
}

Handle<Value> CElmGenGrid::Pack(Handle<Value> value, Handle<Value> replace)
{
   Item<CElmGenGrid> *item = new Item<CElmGenGrid>(value, jsObject);
   if (!item)
     return Undefined();

   Local<Value> before = item->jsObject->Get(Item<CElmGenGrid>::str_before);

   if (before->IsUndefined() && replace->IsObject())
     before  = replace->ToObject()->Get(Item<CElmGenGrid>::str_before);

   if (before->IsString() || before->IsNumber())
     before = GetJSObject()->Get(String::NewSymbol("elements"))->ToObject()->Get(before);

    Item<CElmGenGrid> *before_item = Item<CElmGenGrid>::Unwrap(before);

   if (!before_item)
     item->object_item = elm_gengrid_item_append(eo, item->GetElmClass(), item,
                                                 Item<CElmGenGrid>::OnSelect, item);
   else
     item->object_item = elm_gengrid_item_insert_before(eo, item->GetElmClass(), item,
                                                        before_item->object_item,
                                                        Item<CElmGenGrid>::OnSelect, item);

   elm_object_item_data_set(item->object_item, item);
   return item->jsObject;
}

Handle<Value> CElmGenGrid::Unpack(Handle<Value> value)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(value);

   if (!item)
     return Undefined();

   if (value->IsObject())
     {
        Local<Object> obj = value->ToObject();
        if (obj->Get(Item<CElmGenGrid>::str_before)->IsUndefined())
          {
             Elm_Object_Item *before = elm_gengrid_item_next_get(item->object_item);
             if (before)
               {
                  Item<CElmGenGrid> *before_item = static_cast< Item<CElmGenGrid> *>
                     (elm_object_item_data_get(before));
                  obj->ForceSet(Item<CElmGenGrid>::str_before, before_item->jsObject);
               }
          }
     }
   elm_object_item_del(item->object_item);
   return value;
}

void CElmGenGrid::UpdateItem(Handle<Value> value)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(value);
   if (item)
     elm_gengrid_item_item_class_update(item->object_item, item->GetElmClass());
}

Handle<Value> CElmGenGrid::item_size_vertical_get() const
{
   Evas_Coord w;
   elm_gengrid_item_size_get(eo, &w, NULL);
   return Integer::New(w);
}

Handle<Value> CElmGenGrid::realized_items_update(const Arguments&)
{
   elm_gengrid_realized_items_update(eo);
   return Undefined();
}

void CElmGenGrid::item_size_vertical_set(Handle<Value> value)
{
   if (value->IsNumber())
     {
        Evas_Coord h;
        elm_gengrid_item_size_get(eo, NULL, &h);
        elm_gengrid_item_size_set(eo, value->IntegerValue(), h);
     }
}

Handle<Value> CElmGenGrid::item_size_horizontal_get() const
{
   Evas_Coord h;
   elm_gengrid_item_size_get(eo, NULL, &h);
   return Integer::New(h);
}

void CElmGenGrid::item_size_horizontal_set(Handle<Value> value)
{
   if (value->IsNumber())
     {
        Evas_Coord w;
        elm_gengrid_item_size_get(eo, &w, NULL);
        elm_gengrid_item_size_set(eo, w, value->IntegerValue());
     }
}

Handle<Value> CElmGenGrid::multi_select_get() const
{
   return Boolean::New(elm_gengrid_multi_select_get(eo));
}

void CElmGenGrid::multi_select_set(Handle<Value> value)
{
   elm_gengrid_multi_select_set(eo, value->BooleanValue());
}

Handle<Value> CElmGenGrid::reorder_mode_get() const
{
   return Boolean::New(elm_gengrid_reorder_mode_get(eo));
}

void CElmGenGrid::reorder_mode_set(Handle<Value> value)
{
   elm_gengrid_reorder_mode_set(eo, value->BooleanValue());
}

Handle<Value> CElmGenGrid::select_mode_get() const
{
   return Number::New(elm_gengrid_select_mode_get(eo));
}

void CElmGenGrid::select_mode_set(Handle<Value> value)
{
   if (value->IsNumber())
     elm_gengrid_select_mode_set(eo, (Elm_Object_Select_Mode)value->ToNumber()->Value());
}

Handle<Value> CElmGenGrid::highlight_mode_get() const
{
   return Boolean::New(elm_gengrid_highlight_mode_get(eo));
}

void CElmGenGrid::highlight_mode_set(Handle<Value> value)
{
   elm_gengrid_highlight_mode_set(eo, value->BooleanValue());
}

Handle<Value> CElmGenGrid::horizontal_get() const
{
   return Boolean::New(elm_gengrid_horizontal_get(eo));
}

void CElmGenGrid::horizontal_set(Handle<Value> value)
{
   elm_gengrid_horizontal_set(eo, value->BooleanValue());
}

Handle<Value> CElmGenGrid::filled_get() const
{
   return Boolean::New(elm_gengrid_filled_get(eo));
}

void CElmGenGrid::filled_set(Handle<Value> value)
{
   elm_gengrid_filled_set(eo, value->BooleanValue());
}

Handle<Value> CElmGenGrid::item_align_get() const
{
   return align;
}

void CElmGenGrid::item_align_set(Handle<Value> val)
{
   if (!val->IsArray())
     return;

   Local<Object> sizes = val->ToObject();
   elm_gengrid_align_set(eo,
        sizes->Get(0)->ToNumber()->Value(),
        sizes->Get(1)->ToNumber()->Value());

   align.Dispose();
   align = Persistent<Value>::New(val);
}

Handle<Value> CElmGenGrid::group_item_size_get(void) const
{
   Local<Object> item_size = Object::New();
   Evas_Coord w, h;

   elm_gengrid_group_item_size_get(eo,  &w, &h);
   item_size->Set(String::NewSymbol("width"), Number::New(w));
   item_size->Set(String::NewSymbol("height"), Number::New(h));

   return item_size;
}

void CElmGenGrid::group_item_size_set(Handle<Value> val)
{
   if (!val->IsObject())
     return;

   Local<Value> w = val->ToObject()->Get(String::NewSymbol("width"));
   Local<Value> h = val->ToObject()->Get(String::NewSymbol("height"));

   if (w->IsInt32() && h->IsInt32())
     elm_gengrid_group_item_size_set(eo, w->Int32Value(), h->Int32Value());
}

Handle<Value> CElmGenGrid::items_count_get() const
{
   return Number::New(elm_gengrid_items_count(eo));
}

Handle<Value> CElmGenGrid::realized_items_get() const
{
   Eina_List *l = elm_gengrid_realized_items_get(eo);
   Handle<Array> arr = Array::New(eina_list_count(l));

   void *d;
   int i = 0;
   EINA_LIST_FREE(l, d)
     {
        arr->Set(i, External::Wrap(d)); ++i;
     }

   return arr;
}

Handle<Value> CElmGenGrid::bring_in_item(const Arguments& args)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(args[0]);

   if (!item || !item->object_item)
     return Undefined();

   Elm_Gengrid_Item_Scrollto_Type scroll_type = ELM_GENGRID_ITEM_SCROLLTO_NONE;
   if (args[1]->IsString())
     {
        String::Utf8Value s(args[1]->ToString());

        if (!strcmp(*s, "none"))
          scroll_type = ELM_GENGRID_ITEM_SCROLLTO_NONE;
        else if (!strcmp(*s, "in"))
          scroll_type = ELM_GENGRID_ITEM_SCROLLTO_IN;
        else if (!strcmp(*s, "top"))
          scroll_type = ELM_GENGRID_ITEM_SCROLLTO_TOP;
        else if (!strcmp(*s, "middle"))
          scroll_type = ELM_GENGRID_ITEM_SCROLLTO_MIDDLE;
     }

   elm_gengrid_item_bring_in(item->object_item, scroll_type);

   return Undefined();
}

Handle<Value> CElmGenGrid::clear(const Arguments &args)
{
   if (args[0]->IsArray())
     {
        Local<Object> obj = args[0]->ToObject();
        Local<Array> props = obj->GetOwnPropertyNames();

        for (unsigned int i = 0, len = props->Length(); i < len; i++)
          {
             Local<Value> val = obj->Get(props->Get(i));
             Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(val);

             if (!item)
               item = Item<CElmGenGrid>::Unwrap
                  (args.This()->Get(String::NewSymbol("elements"))->ToObject()->Get(val));

             if (item)
               elm_object_item_del(item->object_item);
          }

        return Undefined();
     }

   elm_gengrid_clear(eo);
   return Undefined();
}

void CElmGenGrid::SetSelected(Local<String>, Local<Value> value, const AccessorInfo &info)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(info);
   elm_gengrid_item_selected_set(item->object_item, value->BooleanValue());
}

Handle<Value> CElmGenGrid::GetSelected(Local<String>, const AccessorInfo &info)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(info);
   return Boolean::New(elm_gengrid_item_selected_get(item->object_item));
}

void CElmGenGrid::SetTooltip(Local<String>, Local<Value> value, const AccessorInfo &info)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(info);
   item->tooltip.Dispose();

   if (value->IsUndefined() || !value->ToObject()->IsObject())
     {
        item->tooltip.Clear();
        elm_gengrid_item_tooltip_unset(item->object_item);
        return;
     }
   item->tooltip = Persistent<Object>::New(value->ToObject()->Clone());

   Local<Object> obj = value->ToObject();
   Handle<Value> style = obj->Get(String::NewSymbol("style"));
   Handle<Value> text = obj->Get(String::NewSymbol("text"));
   Handle<Value> window_mode = obj->Get(String::NewSymbol("window_mode"));

   elm_gengrid_item_tooltip_text_set(item->object_item, *String::Utf8Value(text));

   if (style->IsString())
     elm_gengrid_item_tooltip_style_set(item->object_item, *String::Utf8Value(style));

   elm_gengrid_item_tooltip_window_mode_set(item->object_item, window_mode->BooleanValue());

}

Handle<Value> CElmGenGrid::GetTooltip(Local<String>, const AccessorInfo &info)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(info);
   return item->tooltip;
}

Handle<Value> CElmGenGrid::BringIn(const Arguments &args)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(args.This());

   if (!args.Length())
     {
        elm_gengrid_item_show(item->object_item, ELM_GENGRID_ITEM_SCROLLTO_NONE);
        return Undefined();
     }

   String::Utf8Value mode(args[0]);

   if (!strcmp(*mode, "in"))
     elm_gengrid_item_bring_in(item->object_item, ELM_GENGRID_ITEM_SCROLLTO_IN);
   else if (!strcmp(*mode, "top"))
     elm_gengrid_item_bring_in(item->object_item, ELM_GENGRID_ITEM_SCROLLTO_TOP);
   else if (!strcmp(*mode, "middle"))
     elm_gengrid_item_bring_in(item->object_item, ELM_GENGRID_ITEM_SCROLLTO_MIDDLE);
   else
     elm_gengrid_item_bring_in(item->object_item, ELM_GENGRID_ITEM_SCROLLTO_NONE);

   return Undefined();
}

Handle<Value> CElmGenGrid::Index(const Arguments &args)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(args.This());
   return Uint32::New(elm_gengrid_item_index_get(item->object_item));
}

Handle<Value> CElmGenGrid::Show(const Arguments &args)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(args.This());

   if (!args.Length())
     {
        elm_gengrid_item_show(item->object_item, ELM_GENGRID_ITEM_SCROLLTO_NONE);
        return Undefined();
     }

   String::Utf8Value mode(args[0]);

   if (!strcmp(*mode, "in"))
     elm_gengrid_item_show(item->object_item, ELM_GENGRID_ITEM_SCROLLTO_IN);
   else if (!strcmp(*mode, "top"))
     elm_gengrid_item_show(item->object_item, ELM_GENGRID_ITEM_SCROLLTO_TOP);
   else if (!strcmp(*mode, "middle"))
     elm_gengrid_item_show(item->object_item, ELM_GENGRID_ITEM_SCROLLTO_MIDDLE);
   else
     elm_gengrid_item_show(item->object_item, ELM_GENGRID_ITEM_SCROLLTO_NONE);

   return Undefined();
}

Handle<Value> CElmGenGrid::Update(const Arguments &args)
{
   Item<CElmGenGrid> *item = Item<CElmGenGrid>::Unwrap(args.This());
   elm_gengrid_item_update(item->object_item);

   return Undefined();
}

}
