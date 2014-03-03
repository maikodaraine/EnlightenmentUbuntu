#include "e.h"
#include "e_mod_main.h"
#include "elev8.h"

typedef struct _Gadget Gadget;

/*
 * The problem with this structure is that it maps the script name to
 * this structure. This means you can't have various instances of the
 * same gadget running at the same time (so, for example, various 
 * instances of a Weather gadget would display the same location).
 *
 * We need a way to fix this, by providing an Elev8_Context for each
 * instance.
 *
 * As a proof of concept, though, this works fairly well.
 */
struct _Gadget {
   E_Gadcon_Client_Class *klass;
   Elev8_Context *ctx;
   int clients;
};

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class);

static e17_elev8_log_domain = -1;

static Eina_Hash *name_to_g = NULL;

static Gadget *
gadget_name_lookup(const char *name)
{
    if (!name) return NULL;
    return eina_hash_find(name_to_g, name);
}

static void
gadget_destroy(Gadget *g)
{
    if (!g) return;

    g->clients--;
    if (g->clients) return;

    e_gadcon_provider_unregister(g->klass);
    free((char *)g->klass->name);
    free(g->klass);
    elev8_context_del(g->ctx);
    free(g);
}

static Eina_Bool
gadget_setup(const char *file, const char *name)
{
    Gadget *g;

    name = strdup(name);
    if (!name) return EINA_FALSE;

    g = calloc(1, sizeof(*g));
    if (!g) goto out_nog;

    g->ctx = elev8_context_add();
    if (!g->ctx) goto out_noctx;

    elev8_context_enter(g->ctx);
    gadcon_client_bridge_register(name);
    elev8_context_leave(g->ctx);

    if (!elev8_context_script_exec(g->ctx, file)) goto out_noklass;

    g->klass = calloc(1, sizeof(*g->klass));
    if (!g->klass) goto out_noklass;
    if (!eina_hash_add(name_to_g, name, g)) goto out;

    memcpy(g->klass, (E_Gadcon_Client_Class[]) {{
       .version = GADCON_CLIENT_CLASS_VERSION,
       .name = name,
       .func = {
          .init = _gc_init,
          .shutdown = _gc_shutdown,
          .orient = _gc_orient,
          .label = _gc_label,
          .icon = _gc_icon,
          .id_new = _gc_id_new,
          .id_del = NULL,
          .is_site = NULL
       },
       .default_style = E_GADCON_CLIENT_STYLE_PLAIN
    }}, sizeof(E_Gadcon_Client_Class));

    g->clients = 0;
    e_gadcon_provider_register(g->klass);

    return EINA_TRUE;

out:
    free(g->klass);
out_noklass:
    elev8_context_del(g->ctx);
out_noctx:
    free(g);
out_nog:
    free((char *)name);

    return EINA_FALSE;
}

static void
handle_mouse_down(void *data, Evas *e, Evas_Object *eo, void *event_info)
{
    Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;
    E_Gadcon_Client *gcc = (E_Gadcon_Client *)data;
    Gadget *g = gadget_name_lookup(gcc->name);
    void *menu, *zone;
    int x, y;

    if (!g) return;

    elev8_context_enter(g->ctx);

    menu = gadcon_client_bridge_menu_build(gcc->name);
    if (!menu) goto end;

    menu = e_gadcon_client_util_menu_items_append(gcc, menu, 0);

    e_gadcon_canvas_zone_geometry_get(gcc->gadcon, &x, &y, NULL, NULL);

    zone = e_util_zone_current_get(e_manager_current_get());
    e_menu_activate_mouse(menu, zone, x + ev->output.x, y + ev->output.y, 1, 1,
            E_MENU_POP_DIRECTION_AUTO, ev->timestamp);

    evas_event_feed_mouse_up(e, ev->button, EVAS_BUTTON_NONE,
            ev->timestamp, NULL);

end:
    elev8_context_leave(g->ctx);
}

static void
install_menu_handler(Evas_Object *base, void *gcc)
{
    evas_object_event_callback_add(base, EVAS_CALLBACK_MOUSE_DOWN,
             handle_mouse_down, gcc);
}

static void
base_object_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    E_Gadcon_Client *gcc = data;
    Gadget *g = gadget_name_lookup(gcc->name);
    int x, y;

    if (!g) return;

    elev8_context_enter(g->ctx);
    evas_object_geometry_get(obj, &x, &y, NULL, NULL);
    gadcon_client_bridge_gadget_move(gcc->name, x, y);
    elev8_context_leave(g->ctx);
}

static void
base_object_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    E_Gadcon_Client *gcc = data;
    Gadget *g = gadget_name_lookup(gcc->name);
    int w, h;

    if (!g) return;

    elev8_context_enter(g->ctx);
    evas_object_geometry_get(obj, NULL, NULL, &w, &h);
    gadcon_client_bridge_gadget_resize(gcc->name, w, h);
    elev8_context_leave(g->ctx);
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Gadget *g = gadget_name_lookup(name);

   if (!g) return NULL;

   o = evas_object_rectangle_add(gc->evas);
   if (!o) return NULL;

   evas_object_color_set(o, 255, 255, 255, 0);
   evas_object_show(o);

   elev8_context_enter(g->ctx);
   if (!gadcon_bridge_init(o, name, id, style))
     {
        elev8_context_leave(g->ctx);
        evas_object_del(o);
        return NULL;
     }

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   e_gadcon_client_min_size_set(gcc, 64, 64);
   install_menu_handler(o, gcc);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE, base_object_move, gcc);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, base_object_resize, gcc);

   g->clients++;

   elev8_context_leave(g->ctx);

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Gadget *g = gadget_name_lookup(gcc->name);
   if (!g) return;

   elev8_context_enter(g->ctx);
   evas_object_del(gcc->o_base);
   gadcon_client_bridge_shutdown(gcc->name);
   elev8_context_leave(g->ctx);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Gadget *g = gadget_name_lookup(gcc->name);
   if (!g) return;

   elev8_context_enter(g->ctx);
   gadcon_client_bridge_orient(gcc->name, orient);
   elev8_context_leave(g->ctx);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *klass)
{
   return klass->name;
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   return NULL;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *klass)
{
   static char buf[4096];
   static int instances = 0;

   snprintf(buf, sizeof(buf) - 1, "%s:%d", klass->name, instances++);
   return buf;
}

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Elev8 Module"
};

static void
gadget_dir_list_setup(const char *name, const char *path, void *data)
{
   char full_path[PATH_MAX];

   if (!eina_str_has_suffix(name, ".js")) return;
   if (snprintf(full_path, sizeof(full_path) - 1, "%s/%s", path, name) < 0) return;

   if (!gadget_setup(full_path, name))
     {
        printf("***** ERROR SETTING UP GADGET %s\n", full_path);
     }
}

EAPI void *
e_modapi_init(E_Module *m)
{
   e17_elev8_log_domain = eina_log_domain_register("e17_elev8", EINA_COLOR_RED);
   elev8_init();

   name_to_g = eina_hash_string_superfast_new((Eina_Free_Cb)gadget_destroy);

   if (!eina_file_dir_list("/tmp", EINA_FALSE, gadget_dir_list_setup, NULL))
     {
        elev8_shutdown();
        return NULL;
     }

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   eina_hash_free(name_to_g);

   elev8_shutdown();
   eina_log_domain_unregister(e17_elev8_log_domain);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
