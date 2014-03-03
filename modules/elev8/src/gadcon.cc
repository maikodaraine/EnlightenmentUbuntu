#include <Eina.h>
#include <Evas.h>
#include <v8.h>
#include <stdlib.h>
#include "gadcon.h"

using namespace v8;

extern "C" {
/*
 * E17 headers fails to compile with a C++ compiler, so provide some
 * prototypes here and use void* types.
 */

void *e_menu_new(void);
void *e_menu_item_new(void *);
void *e_menu_item_nth(void *, int);
void e_menu_item_label_set(void *, const char *);
void e_menu_item_icon_file_set(void *, const char *);
void e_menu_item_submenu_set(void *, void *);
void e_menu_item_separator_set(void *, int);
void e_menu_item_check_set(void *, int);
void e_menu_item_toggle_set(void *, int);
void e_menu_item_radio_set(void *, int);
void e_menu_item_radio_group_set(void *, int);
void e_gadcon_client_menu_set(void *, void *);
void e_util_menu_item_theme_icon_set(void *, const char *);
int e_object_unref(void *);
void *e_object_delfn_add(void *, void (*)(void *, void *), void *);
void *e_object_data_get(void *);
void e_object_data_set(void *, void *);
void e_menu_item_callback_set(void *, void (void *, void *, void *), void *);
void *e_menu_root_get(void *);
}

namespace gadcon {

struct Gadcon {
    Persistent<Object> gadcon;

    static void Register(const char *name) {
        HandleScope scope;
        Gadcon *g = (Gadcon *)calloc(1, sizeof(*g));

        g->gadcon = Persistent<Object>::New(Object::New());
        Context::GetEntered()->Global()->Set(String::NewSymbol("GadCon"), g->gadcon);

        eina_hash_add(Gadcon::all_gadcons, name, g);
    }

    static Handle<Object> Lookup(const char *name) {
        Gadcon *g = (Gadcon *)eina_hash_find(Gadcon::all_gadcons, name);
        return g ? g->gadcon : Handle<Object>();
    }

    static void Initialize() {
        if (Gadcon::all_gadcons) return;
        Gadcon::all_gadcons = eina_hash_string_small_new(Gadcon::Destroy);
    }

private:
    static Eina_Hash *all_gadcons;

    static void Destroy(void *data) {
        Gadcon *g = (Gadcon *)data;
        g->gadcon.Dispose();
        free(g);
    }
};

Eina_Hash *Gadcon::all_gadcons = NULL;

extern "C" Eina_Bool
gadcon_bridge_init(Evas_Object *base, const char *name, const char *id, const char *style)
{
    HandleScope scope;
    Handle<Object> gadcon = Gadcon::Lookup(name);

    if (gadcon.IsEmpty())
      {
         fprintf(stderr, "no gadget by this name: \"%s\"\n", name);
         return EINA_FALSE;
      }
    if (!gadcon->Has(String::NewSymbol("init")))
      {
         fprintf(stderr, "module src has no init fn\n");
         return EINA_FALSE;
      }
    Handle<Value> init_function = gadcon->Get(String::NewSymbol("init"));
    if (!init_function->IsFunction())
      {
         fprintf(stderr, "that's no moon\n");
         return EINA_FALSE;
      }

    if (!Context::GetCurrent()->Global()->Has(String::NewSymbol("elm")))
      {
         fprintf(stderr, "elm module not loaded!\n");
         return EINA_FALSE;
      }

    Handle<Value> elm_module = Context::GetCurrent()->Global()->Get(String::NewSymbol("elm"));
    if (elm_module.IsEmpty())
      {
         fprintf(stderr, "could not find elm module for some reason!\n");
         return EINA_FALSE;
      }
    Handle<Value> new_gadget_function = elm_module->ToObject()->Get(String::NewSymbol("NewGadgetFromEvasObject"));
    if (new_gadget_function.IsEmpty())
      {
         fprintf(stderr, "elm module not supported!\n");
         return EINA_FALSE;
      }

    Local<Function> le_gambiarret(Function::Cast(*new_gadget_function));
    Handle<Value> le_gambiarret_args[1] = { External::New(base) };
    Handle<Value> gadget = le_gambiarret->Call(Context::GetCurrent()->Global(), 1, le_gambiarret_args);

    Local<Function> callback(Function::Cast(*init_function));
    Handle<Value> args[4] = {
        gadget,
        String::New(name),
        String::New(id),
        String::New(style)
    };

    callback->Call(Context::GetCurrent()->Global(), 4, args);

    gadcon->Set(String::Concat(String::New(name), String::NewSymbol("::gadget")), gadget);

    return EINA_TRUE;
}

extern "C" void
gadcon_client_bridge_gadget_move(const char *name, int x, int y)
{
    HandleScope scope;
    Handle<Object> gadcon = Gadcon::Lookup(name);

    if (gadcon.IsEmpty()) return;
    Local<Value> gadget = gadcon->Get(String::Concat(String::New(name), String::NewSymbol("::gadget")));

    if (gadget.IsEmpty()) return;
    if (!gadget->IsObject()) return;

    gadget->ToObject()->Set(String::NewSymbol("x"), Integer::New(x));
    gadget->ToObject()->Set(String::NewSymbol("y"), Integer::New(y));
}

extern "C" void
gadcon_client_bridge_gadget_resize(const char *name, int w, int h)
{
    HandleScope scope;
    Handle<Object> gadcon = Gadcon::Lookup(name);

    if (gadcon.IsEmpty()) return;

    Local<Value> gadget = gadcon->Get(String::Concat(String::New(name), String::NewSymbol("::gadget")));

    if (gadget.IsEmpty()) return;
    if (!gadget->IsObject()) return;

    gadget->ToObject()->Set(String::NewSymbol("width"), Integer::New(w));
    gadget->ToObject()->Set(String::NewSymbol("height"), Integer::New(h));
}

extern "C" void
gadcon_client_bridge_shutdown(const char *name)
{
    HandleScope scope;
    Handle<Object> gadcon = Gadcon::Lookup(name);

    if (gadcon.IsEmpty()) return;
    if (!gadcon->Has(String::NewSymbol("shutdown"))) return;

    Handle<Value> shutdown_function = gadcon->Get(String::NewSymbol("shutdown"));
    if (!shutdown_function->IsFunction()) return;

    Local<Function> callback(Function::Cast(*shutdown_function));
    Handle<Value> args[0] = { };

    callback->Call(Context::GetCurrent()->Global(), 0, args);
}

extern "C" void
gadcon_client_bridge_orient(const char *name, int orient)
{
    HandleScope scope;
    Handle<Object> gadcon = Gadcon::Lookup(name);

    if (gadcon.IsEmpty()) return;
    if (!gadcon->Has(String::NewSymbol("orient"))) return;

    Handle<Value> orient_function = gadcon->Get(String::NewSymbol("orient"));
    if (!orient_function->IsFunction()) return;

    Local<Function> callback(Function::Cast(*orient_function));
    Handle<Value> args[1] = { Integer::New(orient) };

    callback->Call(Context::GetCurrent()->Global(), 1, args);
}

struct MenuItemCallback {
    Persistent<Object> item;

    MenuItemCallback(Handle<Object> item_)
       : item(Persistent<Object>::New(item_)) {}

    ~MenuItemCallback() {
       item.Dispose();
    }

    static void Invoke(void *data, void *menu, void *menu_item) {
       HandleScope scope;
       MenuItemCallback *self = (MenuItemCallback *)data;

       Handle<Value> callback_function = self->item->Get(String::NewSymbol("callback"));
       if (!callback_function->IsFunction()) return;

       Local<Function> callback(Function::Cast(*callback_function));
       Handle<Value> args[0] = { };

       callback->Call(self->item, 0, args);
    }
};

static void
destroy_menu_items(void *menu)
{
    for (unsigned i = 0; ; i++)
      {
         void *mi = e_menu_item_nth(menu, i);
         if (!mi) return;
         delete static_cast<MenuItemCallback *>(e_object_data_get(mi));
      }
}

static void
destroy_menu(void *data, void *menu)
{
    Eina_List *submenus = (Eina_List *)data;
    void *submenu;

    EINA_LIST_FREE(submenus, submenu)
      {
         destroy_menu_items(submenu);
         e_object_unref(submenu);
      }

    destroy_menu_items(menu);
}

static void
build_menu_recursively(Local<Array> items, void *menu, Eina_List **submenus)
{
    HandleScope scope;

    if (!menu) return;

    for (unsigned i = 0; i < items->Length(); i++)
      {
         Local<Object> item = items->Get(i)->ToObject();
         void *mi = e_menu_item_new(menu);

         if (!item->Has(String::NewSymbol("label")))
           {
              e_menu_item_separator_set(mi, 1);
              continue;
           }

         e_menu_item_label_set(mi,
                 *String::Utf8Value(item->Get(String::NewSymbol("label"))));

         if (item->Has(String::NewSymbol("icon_theme")))
           e_util_menu_item_theme_icon_set(mi,
                    *String::Utf8Value(item->Get(String::NewSymbol("icon_theme"))));

         if (item->Has(String::NewSymbol("icon_file")))
           e_menu_item_icon_file_set(mi,
                    *String::Utf8Value(item->Get(String::NewSymbol("icon_file"))));

         if (item->Has(String::NewSymbol("items")))
           {
              Local<Array> subitems = Array::Cast(*item->Get(String::NewSymbol("items")));
              void *submenu = e_menu_new();

              *submenus = eina_list_append(*submenus, submenu);

              build_menu_recursively(subitems, submenu, submenus);
              e_menu_item_submenu_set(mi, submenu);
           }

         if (item->Has(String::NewSymbol("callback")))
           {
              e_object_data_set(mi, new MenuItemCallback(item));
              e_menu_item_callback_set(mi, MenuItemCallback::Invoke, e_object_data_get(mi));
           }

         if (item->Has(String::NewSymbol("check")))
           e_menu_item_check_set(mi, item->Get(String::NewSymbol("check"))->Int32Value());

         if (item->Has(String::NewSymbol("radio")))
           e_menu_item_radio_set(mi, item->Get(String::NewSymbol("radio"))->Int32Value());

         if (item->Has(String::NewSymbol("radio_group")))
           e_menu_item_radio_group_set(mi, item->Get(String::NewSymbol("radio_group"))->Int32Value());

         if (item->Has(String::NewSymbol("toggle")))
           e_menu_item_toggle_set(mi, item->Get(String::NewSymbol("toggle"))->Int32Value());
      }
}

extern "C" void *
gadcon_client_bridge_menu_build(const char *name)
{
    HandleScope scope;
    Handle<Object> gadcon = Gadcon::Lookup(name);

    if (gadcon.IsEmpty()) return NULL;
    if (!gadcon->Has(String::NewSymbol("menu"))) return NULL;

    Handle<Value> menu_function = gadcon->Get(String::NewSymbol("menu"));
    if (!menu_function->IsFunction()) return NULL;

    Local<Function> callback(Function::Cast(*menu_function));
    Handle<Value> args[0] = { };
    Local<Value> items_as_value = callback->Call(Context::GetCurrent()->Global(), 0, args);
    if (items_as_value.IsEmpty()) return NULL;
    if (!items_as_value->IsArray()) return NULL;

    Local<Array> items = Array::Cast(*items_as_value);

    void *menu = e_menu_new();
    if (!menu) return NULL;

    Eina_List *submenus = NULL;
    build_menu_recursively(items, menu, &submenus);

    e_object_data_set(menu, submenus);
    e_object_delfn_add(menu, destroy_menu, submenus);

    return menu;
}

extern "C" void
gadcon_client_bridge_register(const char *name)
{
    Gadcon::Register(name);
}

void
RegisterModule(Handle<ObjectTemplate>)
{
    Gadcon::Initialize();
}

}
