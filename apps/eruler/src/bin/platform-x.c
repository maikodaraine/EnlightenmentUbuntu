#include "private.h"
#include <Ecore_X.h>

static Eina_Bool has_xinerama = EINA_TRUE;
static Eina_Bool windows_visible = EINA_TRUE;
static Eina_List *zones;
static Eina_List *roots;
static Eina_List *key_grabs;
static const Zone *grabbed_zone;
static Ecore_X_Window temp_xwin;
static Ecore_Event_Handler *mouse_out_handler;
static Ecore_Event_Handler *key_down_handler;

typedef struct _X_Key_Grab X_Key_Grab;
struct _X_Key_Grab {
   const char *keyname;
   void (*cb)(void *data, const char *keyname);
   const void *cb_data;
};

static X_Key_Grab *
_x_key_grab_new(const char *keyname, void (*cb)(void *data, const char *keyname), const void *data)
{
   X_Key_Grab *kg = malloc(sizeof(X_Key_Grab));
   EINA_SAFETY_ON_NULL_RETURN_VAL(kg, NULL);
   kg->keyname = eina_stringshare_add(keyname);
   kg->cb = cb;
   kg->cb_data = data;
   return kg;
}

static void
_x_key_grab_free(X_Key_Grab *kg)
{
   eina_stringshare_del(kg->keyname);
   free(kg);
}

static Eina_Bool
_x_zone_screen_copy_do(Zone *zone, Evas_Object *img)
{
   Evas_Object *win = zone_win_get(zone);
   Ecore_X_Window xid = elm_win_xwindow_get(win);
   Ecore_X_Window root = ecore_x_window_root_get(xid);
   Ecore_X_Image *capture;
   Ecore_X_Window_Attributes watt;
   int bpl = 0, rows = 0, bpp = 0, x, y, w, h;
   Ecore_X_Colormap cmap;
   Ecore_X_Screen *screen;
   Ecore_X_Display *display;
   void *src, *dst;
   Eina_Bool ret;

   if (!ecore_x_window_attributes_get(root, &watt))
     return EINA_FALSE;

   zone_geometry_get(zone, &x, &y, &w, &h);

   capture = ecore_x_image_new(w, h, watt.visual, watt.depth);
   EINA_SAFETY_ON_NULL_RETURN_VAL(capture, EINA_FALSE);

   ret = ecore_x_image_get(capture, root, x, y, 0, 0, w, h);
   EINA_SAFETY_ON_FALSE_GOTO(ret, error_clean_capture);

   src = ecore_x_image_data_get(capture, &bpl, &rows, &bpp);
   EINA_SAFETY_ON_NULL_GOTO(src, error_clean_capture);

   display = ecore_x_display_get();
   screen = ecore_x_default_screen_get();
   cmap = ecore_x_default_colormap_get(display, screen);

   evas_object_image_colorspace_set(img, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(img, EINA_FALSE);
   evas_object_image_size_set(img, w, h);
   dst = evas_object_image_data_get(img, EINA_TRUE);

   ret = ecore_x_image_to_argb_convert(src, bpp, bpl, cmap, watt.visual,
                                       0, 0, w, h,
                                       dst, w * 4, 0, 0);
   EINA_SAFETY_ON_FALSE_GOTO(ret, error_clean_dst);

   evas_object_image_data_update_add(img, 0, 0, w, h);
   evas_object_image_data_set(img, dst);
   ecore_x_image_free(capture);
   return EINA_TRUE;

 error_clean_dst:
   evas_object_image_data_set(img, dst);
 error_clean_capture:
   ecore_x_image_free(capture);
   return EINA_FALSE;
}

static void
_x_grab(const Zone *zone)
{
   Evas_Object *win = zone_win_get(zone);
   Ecore_X_Window xid = elm_win_xwindow_get(win);

   if (!ecore_x_pointer_grab(xid))
     {
        CRI("Could not grab pointer to xwin %#x", xid);
        show_gui_error("ERuler couldn't grab the mouse pointer");
        return;
     }

   if (!ecore_x_keyboard_grab(xid))
     {
        CRI("Could not grab keyboard to xwin %#x", xid);
        show_gui_error("ERuler couldn't grab the keyboard");
        ecore_x_pointer_ungrab();
        return;
     }

   DBG("Grabbed pointer and keyboard to win %#x (elm_win %p)", xid, win);
   grabbed_zone = zone;
}

static void
_x_ungrab(void)
{
   if (!grabbed_zone) return;
   ecore_x_pointer_ungrab();
   ecore_x_keyboard_ungrab();
   grabbed_zone = NULL;
}

static Eina_Bool
_x_mouse_out(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Mouse_Out *ev = event;
   const Eina_List *l;
   const Zone *zone;

   DBG("Mouse out of zone %p, check new zone containing %d,%d",
       grabbed_zone, ev->root.x, ev->root.y);

   EINA_LIST_FOREACH(zones, l, zone)
     {
        int x, y, w, h;

        if (zone == grabbed_zone) continue;

        zone_geometry_get(zone, &x, &y, &w, &h);
        if (((ev->root.x >= x) && (ev->root.x < x + w)) &&
            ((ev->root.y >= y) && (ev->root.y < y + h)))
          {
             DBG("Mouse now at zone %p (%d,%d + %dx%d)", zone, x, y, w, h);
             _x_ungrab();
             if (windows_visible)
               _x_grab(zone);
             return EINA_TRUE;
          }
     }

   return EINA_TRUE;
}

static void
_x_grab_current(void)
{
   const Eina_List *l;
   const Zone *zone;
   int mx, my;

   ecore_x_pointer_xy_get(ecore_x_window_root_first_get(), &mx, &my);

   EINA_LIST_FOREACH(zones, l, zone)
     {
        int x, y, w, h;

        zone_geometry_get(zone, &x, &y, &w, &h);
        if (((mx >= x) && (mx < x + w)) &&
            ((my >= y) && (my < y + h)))
          {
             DBG("Current zone for mouse at %d,%d is %p (%d,%d + %dx%d)",
                 mx, my, zone, x, y, w, h);
             _x_grab(zone);
             return;
          }
     }

   ERR("Couldn't find current zone for mouse at %d,%d", mx, my);
}

static void
_x_temp_grab(void)
{
   if (mouse_out_handler)
     {
        ecore_event_handler_del(mouse_out_handler);
        mouse_out_handler = NULL;
     }

   if (!temp_xwin)
     temp_xwin = ecore_x_window_input_new(0, 0, 0, 1, 1);

   ecore_x_window_show(temp_xwin);

   ecore_x_pointer_ungrab();
   ecore_x_keyboard_ungrab();
   ecore_x_pointer_grab(temp_xwin);
   ecore_x_keyboard_grab(temp_xwin);

   DBG("Temporary grab for input window %#x", temp_xwin);
}

static void
_x_temp_ungrab(void)
{
   DBG("Ungrab temporary for input window %#x", temp_xwin);

   if ((eina_list_count(zones) > 1) && (!mouse_out_handler))
     {
        mouse_out_handler = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT,
                                                    _x_mouse_out, NULL);
     }

   if (temp_xwin)
     {
        ecore_x_pointer_ungrab();
        ecore_x_keyboard_ungrab();
        ecore_x_window_free(temp_xwin);
        temp_xwin = 0;
     }

   _x_grab_current();
}

static Eina_Bool
x_pre_setup(void)
{
   if (!ecore_x_screen_is_composited(0))
     {
        show_gui_error("ERuler needs a X11 Composite Manager!<br><br>"
                       "Consider running <hilight>xcompmgr</hilight> if your "
                       "Window Manager or Desktop Environment doesn't offer "
                       "you such feature.");
     }

   return EINA_TRUE;
}

static Eina_Bool
x_post_setup(void)
{
   const Eina_List *l;
   const Zone *zone;
   void *ptr;

   if (eina_list_count(zones) > 1)
     {
        mouse_out_handler = ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT,
                                                    _x_mouse_out, NULL);
     }

   EINA_LIST_FOREACH(zones, l, zone)
     {
        Evas_Object *win = zone_win_get(zone);
        Ecore_X_Window xid = elm_win_xwindow_get(win);
        Ecore_X_Window root_xid;
        int x, y, w, h;

        zone_geometry_get(zone, &x, &y, &w, &h);
        evas_object_show(win);
        ecore_x_window_move_resize(xid, x, y, w, h);

        root_xid = ecore_x_window_root_get(xid);
        if (!eina_list_data_find(roots, (void*)(long)root_xid))
          roots = eina_list_append(roots, (void*)(long)root_xid);
     }

   _x_grab_current();

   return EINA_TRUE;
}

static int
x_zones_count(void)
{
   int c = ecore_x_xinerama_screen_count_get();

   if (c < 1)
     {
        ERR("No Xinerama support, assume single screen");
        has_xinerama = EINA_FALSE;
        c = 1;
     }

   return c;
}

static Eina_Bool
x_zone_geometry_get(int idx, int *x, int *y, int *w, int *h)
{
   if (!has_xinerama)
     {
        if (x) *x = 0;
        if (y) *y = 0;
        ecore_x_screen_size_get(ecore_x_default_screen_get(), w, h);
        return EINA_TRUE;
     }

   return ecore_x_xinerama_screen_geometry_get(idx, x, y, w, h);
}

static Eina_Bool
x_zone_setup(Zone *zone)
{
   Evas_Object *win = zone_win_get(zone);
   Evas_Object *img;

   DBG("setup for zone %p", zone);

   elm_win_override_set(win, EINA_TRUE);

   img = zone_screen_copy_object_get(zone);
   if (img)
     zone_screen_copy_ready_set(zone, _x_zone_screen_copy_do(zone, img));

   zones = eina_list_append(zones, zone);

   return EINA_TRUE;
}

typedef struct _X_Zone_Screen_Copy_Data X_Zone_Screen_Copy_Data;
struct _X_Zone_Screen_Copy_Data
{
   Zone *zone;
   Evas_Object *img;
   Ecore_X_Window xid;
   void (*cb)(void *data, Eina_Bool success);
   const void *cb_data;
   Ecore_Timer *shot_timer;
};

static Eina_Bool
_x_zone_screen_copy_shot_timer_cb(void *data)
{
   X_Zone_Screen_Copy_Data *ctx = data;
   Eina_Bool ret;

   DBG("do the screen shot to image %p", ctx->img);
   ret = _x_zone_screen_copy_do(ctx->zone, ctx->img);

   DBG("Screen shot done with ret=%d, show %#x, ungrab temporary %#x",
       ret, ctx->xid, temp_xwin);

   ecore_x_window_show(ctx->xid);
   _x_temp_ungrab();

   ctx->cb((void *)ctx->cb_data, ret);
   free(ctx);

   return EINA_FALSE;
}

static void
x_zone_screen_copy(Zone *zone, Evas_Object *img, void (*cb)(void *data, Eina_Bool success), const void *data)
{
   X_Zone_Screen_Copy_Data *ctx;
   Evas_Object *win = zone_win_get(zone);
   Ecore_X_Window xid = elm_win_xwindow_get(win);

   EINA_SAFETY_ON_NULL_RETURN(cb);

   ctx = calloc(1, sizeof(X_Zone_Screen_Copy_Data));
   EINA_SAFETY_ON_NULL_GOTO(ctx, error);

   ctx->zone = zone;
   ctx->img = img;
   ctx->xid = xid;
   ctx->cb = cb;
   ctx->cb_data = data;

   ctx->shot_timer = ecore_timer_add(1.0, _x_zone_screen_copy_shot_timer_cb,
                                     ctx);
   EINA_SAFETY_ON_NULL_GOTO(ctx->shot_timer, error_free_ctx);

   _x_temp_grab();
   ecore_x_window_hide(xid);

   DBG("xid %#x hidden, grab to temporary %#x, start 1.0s shot timer %p",
       xid, temp_xwin, ctx->shot_timer);
   return;

 error_free_ctx:
   free(ctx);
 error:
   cb((void *)data, EINA_FALSE);
}

static void
_x_global_key_ungrab(const char *keyname)
{
   const Eina_List *l;
   const void *ptr;

   EINA_LIST_FOREACH(roots, l, ptr)
     {
        Ecore_X_Window xid = (Ecore_X_Window)(long)ptr;
        ecore_x_window_key_ungrab(xid, keyname, 0, 0);
     }
}

static void
x_pre_teardown(void)
{
   X_Key_Grab *kg;

   _x_ungrab();

   if (temp_xwin)
     {
        ecore_x_window_free(temp_xwin);
        temp_xwin = 0;
     }

   if (mouse_out_handler)
     {
        ecore_event_handler_del(mouse_out_handler);
        mouse_out_handler = NULL;
     }

   if (key_down_handler)
     {
        ecore_event_handler_del(key_down_handler);
        key_down_handler = NULL;
     }

   EINA_LIST_FREE(key_grabs, kg)
     {
        _x_global_key_ungrab(kg->keyname);
        _x_key_grab_free(kg);
     }

   eina_list_free(roots);
   roots = NULL;
   eina_list_free(zones);
   zones = NULL;
   grabbed_zone = NULL;
}

static void
x_mouse_move_by(Zone *zone, int dx, int dy)
{
   Evas_Object *win = zone_win_get(zone);
   Ecore_X_Window xid = elm_win_xwindow_get(win);
   int x, y, zx, zy, zw, zh;

   ecore_x_pointer_xy_get(xid, &x, &y);
   zone_geometry_get(zone, &zx, &zy, &zw, &zh);

   x += dx;
   y += dy;

   if (x < 0)
     x = 0;
   if (y < 0)
     y = 0;

   if (x >= zw)
     x = zw - 1;

   if (y >= zh)
     y = zh - 1;

   ecore_x_pointer_warp(xid, x, y);
}

static void
x_windows_visibility_set(Eina_Bool visible)
{
   const Zone *zone;
   const Eina_List *l;

   windows_visible = visible;

   if (!visible)
     _x_ungrab();

   EINA_LIST_FOREACH(zones, l, zone)
     {
        Evas_Object *win = zone_win_get(zone);
        Ecore_X_Window xid = elm_win_xwindow_get(win);

        if (visible)
          ecore_x_window_show(xid);
        else
          ecore_x_window_hide(xid);
     }

   if (visible)
     _x_grab_current();
}

static Eina_Bool
_x_key_down(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_Event_Key *ev = event;
   const Eina_List *l;
   const X_Key_Grab *kg;

   if (!ev->key)
     return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FOREACH(key_grabs, l, kg)
     {
        if (strcmp(ev->key, kg->keyname) == 0)
          kg->cb((void *)kg->cb_data, kg->keyname);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
x_global_key_grab(const char *keyname, void (*cb)(void *data, const char *keyname), const void *data)
{
   X_Key_Grab *kg;
   const Eina_List *l;
   const void *ptr;

   if (!key_down_handler)
     {
        key_down_handler = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                                                   _x_key_down, NULL);
     }

   EINA_LIST_FOREACH(roots, l, ptr)
     {
        Ecore_X_Window xid = (Ecore_X_Window)(long)ptr;
        ecore_x_window_key_grab(xid, keyname, 0, 0);
        ecore_x_event_mask_set(xid, ECORE_X_EVENT_MASK_KEY_DOWN);
     }

   kg = _x_key_grab_new(keyname, cb, data);
   key_grabs = eina_list_append(key_grabs, kg);
}

const Platform_Funcs *
platform_funcs_x_get(void)
{
   static const Platform_Funcs funcs = {
     x_pre_setup,
     x_post_setup,
     x_zones_count,
     x_zone_geometry_get,
     x_zone_setup,
     x_zone_screen_copy,
     x_pre_teardown,
     x_mouse_move_by,
     x_windows_visibility_set,
     x_global_key_grab,
   };
   return &funcs;
}
