#include "e.h"

static void
_e_comp_canvas_event_compositor_resize_free(void *data EINA_UNUSED, void *event)
{
   E_Event_Compositor_Resize *ev = event;

   e_object_unref(E_OBJECT(ev->comp));
   free(ev);
}

///////////////////////////////////

static void
_e_comp_canvas_cb_first_frame(void *data, Evas *e, void *event_info EINA_UNUSED)
{
   E_Comp *c = data;
   double now = ecore_time_get();

   switch (e_first_frame[0])
     {
      case 'A': abort();
      case 'E':
      case 'D': exit(-1);
      case 'T': fprintf(stderr, "Startup time: '%f' - '%f' = '%f'\n", now, e_first_frame_start_time, now - e_first_frame_start_time);
         break;
     }

   evas_event_callback_del_full(e, EVAS_CALLBACK_RENDER_POST, _e_comp_canvas_cb_first_frame, c);
}

static void
_e_comp_canvas_render_post(void *data, Evas *e EINA_UNUSED, void *event_info EINA_UNUSED)
{
   E_Comp *c = data;
   E_Client *ec;
   //Evas_Event_Render_Post *ev = event_info;
   //Eina_List *l;
   //Eina_Rectangle *r;

   //if (ev)
     //{
        //EINA_LIST_FOREACH(ev->updated_area, l, r)
          //INF("POST RENDER: %d,%d %dx%d", r->x, r->y, r->w, r->h);
     //}
   EINA_LIST_FREE(c->post_updates, ec)
     {
        //INF("POST %p", ec);
        if (!e_object_is_del(E_OBJECT(ec)))
          e_pixmap_image_clear(ec->pixmap, 1);
        e_object_unref(E_OBJECT(ec));
     }
}

///////////////////////////////////

static void
_e_comp_canvas_cb_mouse_in(E_Comp *c EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   E_Client *ec;

   if (e_client_action_get()) return;
   ec = e_client_focused_get();
   if (ec && (!ec->border_menu)) e_focus_event_mouse_out(ec);
}

static void
_e_comp_canvas_cb_mouse_down(E_Comp *c, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   if (e_client_action_get()) return;
   e_bindings_mouse_down_evas_event_handle(E_BINDING_CONTEXT_COMPOSITOR, E_OBJECT(c), event_info);
}

static void
_e_comp_canvas_cb_mouse_up(E_Comp *c, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   if (e_client_action_get()) return;
   e_bindings_mouse_up_evas_event_handle(E_BINDING_CONTEXT_COMPOSITOR, E_OBJECT(c), event_info);
}

static void
_e_comp_canvas_cb_mouse_wheel(E_Comp *c, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   if (e_client_action_get()) return;
   e_bindings_wheel_evas_event_handle(E_BINDING_CONTEXT_COMPOSITOR, E_OBJECT(c), event_info);
}

////////////////////////////////////

static void
_e_comp_canvas_screensaver_active(void *d EINA_UNUSED, Evas_Object *obj, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   E_Comp *c;
   /* thawed in _e_comp_screensaver_off() */
   ecore_animator_frametime_set(10.0);
   c = e_comp_util_evas_object_comp_get(obj);
   if (!c->nocomp)
     ecore_evas_manual_render_set(c->ee, EINA_TRUE);
}

////////////////////////////////////

static int
_e_comp_canvas_cb_zone_sort(const void *data1, const void *data2)
{
   const E_Zone *z1 = data1, *z2 = data2;

   return z2->num - z1->num;
}


EAPI Eina_Bool
e_comp_canvas_init(E_Comp *c)
{
   Evas_Object *o;
   Eina_List *screens;
   unsigned int layer;

   c->evas = ecore_evas_get(c->ee);

   if (e_first_frame)
     evas_event_callback_add(c->evas, EVAS_CALLBACK_RENDER_POST, _e_comp_canvas_cb_first_frame, c);
   ecore_evas_data_set(c->ee, "comp", c);
   o = evas_object_rectangle_add(c->evas);
   c->bg_blank_object = o;
   evas_object_layer_set(o, E_LAYER_BOTTOM);
   evas_object_move(o, 0, 0);
   evas_object_resize(o, c->man->w, c->man->h);
   evas_object_color_set(o, 255, 255, 255, 255);
   evas_object_name_set(o, "comp->bg_blank_object");
   evas_object_data_set(o, "e_comp", c);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb)_e_comp_canvas_cb_mouse_down, c);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, (Evas_Object_Event_Cb)_e_comp_canvas_cb_mouse_up, c);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN, (Evas_Object_Event_Cb)_e_comp_canvas_cb_mouse_in, c);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_WHEEL, (Evas_Object_Event_Cb)_e_comp_canvas_cb_mouse_wheel, c);
   evas_object_show(o);

   ecore_evas_name_class_set(c->ee, "E", "Comp_EE");
   //   ecore_evas_manual_render_set(c->ee, conf->lock_fps);
   ecore_evas_show(c->ee);

   evas_event_callback_add(c->evas, EVAS_CALLBACK_RENDER_POST, _e_comp_canvas_render_post, c);

   c->ee_win = ecore_evas_window_get(c->ee);

   for (layer = 0; layer <= e_comp_canvas_layer_map(E_LAYER_MAX); layer++)
     {
        Evas_Object *o2;

        /* client layers have actual X windows backing them, so we wait */
        if (e_comp_canvas_client_layer_map(e_comp_canvas_layer_map_to(layer)) != 9999)
          continue;
        o2 = c->layers[layer].obj = evas_object_rectangle_add(c->evas);
        evas_object_layer_set(o2, e_comp_canvas_layer_map_to(layer));
        evas_object_name_set(o2, "layer_obj");
     }

   screens = (Eina_List *)e_xinerama_screens_get();
   if (screens)
     {
        E_Screen *scr;
        Eina_List *l;

        EINA_LIST_FOREACH(screens, l, scr)
          {
             e_zone_new(c, scr->screen, scr->escreen, scr->x, scr->y, scr->w, scr->h);
          }
     }
   else
     e_zone_new(c, 0, 0, 0, 0, c->man->w, c->man->h);

   return EINA_TRUE;
}

EINTERN void
e_comp_canvas_clear(E_Comp *c)
{
   evas_event_freeze(c->evas);
   edje_freeze();

   E_FREE_FUNC(c->fps_fg, evas_object_del);
   E_FREE_FUNC(c->fps_bg, evas_object_del);
   E_FREE_FUNC(c->autoclose.rect, evas_object_del);
   E_FREE_FUNC(c->shape_job, ecore_job_del);
   E_FREE_FUNC(c->pointer, e_object_del);
}

//////////////////////////////////////////////

EAPI void
e_comp_all_freeze(void)
{
   Eina_List *l;
   E_Manager *man;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     evas_event_freeze(man->comp->evas);
}

EAPI void
e_comp_all_thaw(void)
{
   Eina_List *l;
   E_Manager *man;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     evas_event_thaw(man->comp->evas);
}

EAPI E_Zone *
e_comp_zone_xy_get(const E_Comp *c, Evas_Coord x, Evas_Coord y)
{
   const Eina_List *l;
   E_Zone *zone;

   if (!c) c = e_comp_get(NULL);
   EINA_LIST_FOREACH(c->zones, l, zone)
     if (E_INSIDE(x, y, zone->x, zone->y, zone->w, zone->h)) return zone;
   return NULL;
}

EAPI E_Zone *
e_comp_zone_number_get(E_Comp *c, int num)
{
   Eina_List *l = NULL;
   E_Zone *zone = NULL;

   E_OBJECT_CHECK_RETURN(c, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(c, E_COMP_TYPE, NULL);
   EINA_LIST_FOREACH(c->zones, l, zone)
     {
        if ((int)zone->num == num) return zone;
     }
   return NULL;
}

EAPI E_Zone *
e_comp_zone_id_get(E_Comp *c, int id)
{
   Eina_List *l = NULL;
   E_Zone *zone = NULL;

   E_OBJECT_CHECK_RETURN(c, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(c, E_COMP_TYPE, NULL);
   EINA_LIST_FOREACH(c->zones, l, zone)
     {
        if (zone->id == id) return zone;
     }
   return NULL;
}

EAPI E_Comp *
e_comp_number_get(unsigned int num)
{
   const Eina_List *l;
   E_Comp *c;

   EINA_LIST_FOREACH(e_comp_list(), l, c)
     if (c->num == num) return c;
   return NULL;
}

EAPI E_Desk *
e_comp_desk_window_profile_get(E_Comp *c, const char *profile)
{
   Eina_List *l = NULL;
   E_Zone *zone = NULL;
   int x, y;

   E_OBJECT_CHECK_RETURN(c, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(c, E_COMP_TYPE, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(profile, NULL);

   EINA_LIST_FOREACH(c->zones, l, zone)
     {
        for (x = 0; x < zone->desk_x_count; x++)
          {
             for (y = 0; y < zone->desk_y_count; y++)
               {
                  E_Desk *desk = e_desk_at_xy_get(zone, x, y);
                  if (!e_util_strcmp(desk->window_profile, profile))
                    return desk;
               }
          }
     }

   return NULL;
}

EAPI void
e_comp_canvas_zone_update(E_Zone *zone)
{
   Evas_Object *o;
   const char *const over_styles[] =
   {
      "e/comp/screen/overlay/default",
      "e/comp/screen/overlay/noeffects"
   };
   const char *const under_styles[] =
   {
      "e/comp/screen/base/default",
      "e/comp/screen/base/noeffects"
   };
   E_Comp_Config *conf = e_comp_config_get();

   if (zone->over && zone->base)
     {
        e_theme_edje_object_set(zone->base, "base/theme/comp",
                                under_styles[conf->disable_screen_effects]);
        edje_object_part_swallow(zone->base, "e.swallow.background",
                                 zone->transition_object ?: zone->bg_object);
        e_theme_edje_object_set(zone->over, "base/theme/comp",
                                over_styles[conf->disable_screen_effects]);
        return;
     }
   E_FREE_FUNC(zone->base, evas_object_del);
   E_FREE_FUNC(zone->over, evas_object_del);
   zone->base = o = edje_object_add(zone->comp->evas);
   evas_object_repeat_events_set(o, 1);
   evas_object_name_set(zone->base, "zone->base");
   e_theme_edje_object_set(o, "base/theme/comp", under_styles[conf->disable_screen_effects]);
   edje_object_part_swallow(zone->base, "e.swallow.background", zone->transition_object ?: zone->bg_object);
   evas_object_move(o, zone->x, zone->y);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_layer_set(o, E_LAYER_BG);
   evas_object_show(o);

   zone->over = o = edje_object_add(zone->comp->evas);
   edje_object_signal_callback_add(o, "e,state,screensaver,active", "e", _e_comp_canvas_screensaver_active, NULL);
   evas_object_layer_set(o, E_LAYER_MAX);
   evas_object_raise(o);
   evas_object_name_set(zone->over, "zone->over");
   evas_object_pass_events_set(o, 1);
   e_theme_edje_object_set(o, "base/theme/comp", over_styles[conf->disable_screen_effects]);
   evas_object_move(o, zone->x, zone->y);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_raise(o);
   evas_object_show(o);
}

EAPI void
e_comp_canvas_update(E_Comp *c)
{
   E_Event_Compositor_Resize *ev;
   Eina_List *l, *screens, *zones = NULL, *ll;
   E_Zone *zone;
   E_Screen *scr;
   int i;
   Eina_Bool changed = EINA_FALSE;

   screens = (Eina_List *)e_xinerama_screens_get();

   if (screens)
     {
        zones = c->zones;
        c->zones = NULL;
        EINA_LIST_FOREACH(screens, l, scr)
          {
             zone = NULL;

             EINA_LIST_FOREACH(zones, ll, zone)
               {
                  if (zone->id == scr->escreen) break;
                  zone = NULL;
               }
             if (zone)
               {
                  changed |= e_zone_move_resize(zone, scr->x, scr->y, scr->w, scr->h);
                  if (changed)
                    printf("@@@ FOUND ZONE %i %i [%p]\n", zone->num, zone->id, zone);
                  zones = eina_list_remove(zones, zone);
                  c->zones = eina_list_append(c->zones, zone);
                  zone->num = scr->screen;
               }
             else
               {
                  zone = e_zone_new(c, scr->screen, scr->escreen,
                                    scr->x, scr->y, scr->w, scr->h);
                  printf("@@@ NEW ZONE = %p\n", zone);
                  changed = EINA_TRUE;
               }
             if (changed)
               printf("@@@ SCREENS: %i %i | %i %i %ix%i\n",
                      scr->screen, scr->escreen, scr->x, scr->y, scr->w, scr->h);
          }
        c->zones = eina_list_sort(c->zones, 0, _e_comp_canvas_cb_zone_sort);
        if (zones)
          {
             E_Zone *spare_zone;

             changed = EINA_TRUE;
             spare_zone = eina_list_data_get(c->zones);

             EINA_LIST_FREE(zones, zone)
               {
                  E_Client *ec;

                  /* delete any shelves on this zone */
                  E_CLIENT_FOREACH(c, ec)
                    {
                       if (ec->zone == zone)
                         {
                            if (spare_zone)
                              e_client_zone_set(ec, spare_zone);
                            else
                              printf("EEEK! should not be here - but no\n"
                                     "spare zones exist to move this\n"
                                     "window to!!! help!\n");
                         }
                    }
                  e_object_del(E_OBJECT(zone));
               }
          }
        if (changed) e_shelf_config_update();
     }
   else
     {
        E_Zone *z;

        z = e_comp_zone_number_get(c, 0);
        if (z)
          {
             changed |= e_zone_move_resize(z, 0, 0, c->man->w, c->man->h);
             if (changed) e_shelf_zone_move_resize_handle(z);
          }
     }

   if (!changed) return;
   if (!starting)
     {
        ev = calloc(1, sizeof(E_Event_Compositor_Resize));
        ev->comp = c;
        e_object_ref(E_OBJECT(c));
        ecore_event_add(E_EVENT_COMPOSITOR_RESIZE, ev, _e_comp_canvas_event_compositor_resize_free, NULL);
     }

   EINA_LIST_FOREACH(c->zones, l, zone)
     {
        E_FREE_FUNC(zone->base, evas_object_del);
        E_FREE_FUNC(zone->over, evas_object_del);
        if (zone->bloff)
          {
             if (!e_comp_config_get()->nofade)
               {
                  if (e_backlight_mode_get(zone) != E_BACKLIGHT_MODE_NORMAL)
                    e_backlight_mode_set(zone, E_BACKLIGHT_MODE_NORMAL);
                  e_backlight_level_set(zone, e_config->backlight.normal, -1.0);
               }
          }
        e_comp_canvas_zone_update(zone);
     }

   for (i = 0; i < 11; i++)
     {
        Eina_List *tmp = NULL;
        E_Client *ec;

        if (!c->layers[i].clients) continue;
        /* Make temporary list as e_client_res_change_geometry_restore
         * rearranges the order. */
        EINA_INLIST_FOREACH(c->layers[i].clients, ec)
          {
             if (!e_client_util_ignored_get(ec))
               tmp = eina_list_append(tmp, ec);
          }

        EINA_LIST_FREE(tmp, ec)
          {
             e_client_res_change_geometry_save(ec);
             e_client_res_change_geometry_restore(ec);
          }
     }
}

EAPI void
e_comp_canvas_fake_layers_init(E_Comp *comp)
{
   unsigned int layer;

   /* init layers */
   for (layer = e_comp_canvas_layer_map(E_LAYER_CLIENT_DESKTOP); layer <= e_comp_canvas_layer_map(E_LAYER_CLIENT_PRIO); layer++)
     {
        Evas_Object *o2;

        o2 = comp->layers[layer].obj = evas_object_rectangle_add(comp->evas);
        evas_object_layer_set(o2, e_comp_canvas_layer_map_to(layer));
        evas_object_name_set(o2, "layer_obj");
     }
}

EAPI void
e_comp_canvas_fps_toggle(void)
{
   E_Comp_Config *conf = e_comp_config_get();

   conf->fps_show = !conf->fps_show;
   e_comp_internal_save();
   E_LIST_FOREACH(e_comp_list(), e_comp_render_queue);
}

EAPI E_Layer
e_comp_canvas_layer_map_to(unsigned int layer)
{
   switch (layer)
     {
      case 0: return E_LAYER_BOTTOM;
      case 1: return E_LAYER_BG;
      case 2: return E_LAYER_DESKTOP;
      case 3: return E_LAYER_DESKTOP_TOP;
      case 4: return E_LAYER_CLIENT_DESKTOP;
      case 5: return E_LAYER_CLIENT_BELOW;
      case 6: return E_LAYER_CLIENT_NORMAL;
      case 7: return E_LAYER_CLIENT_ABOVE;
      case 8: return E_LAYER_CLIENT_EDGE;
      case 9: return E_LAYER_CLIENT_FULLSCREEN;
      case 10: return E_LAYER_CLIENT_EDGE_FULLSCREEN;
      case 11: return E_LAYER_CLIENT_POPUP;
      case 12: return E_LAYER_CLIENT_TOP;
      case 13: return E_LAYER_CLIENT_DRAG;
      case 14: return E_LAYER_CLIENT_PRIO;
      case 15: return E_LAYER_POPUP;
      case 16: return E_LAYER_MENU;
      case 17: return E_LAYER_DESKLOCK;
      case 18: return E_LAYER_MAX;
      default: break;
     }
   return -INT_MAX;
}

EAPI unsigned int
e_comp_canvas_layer_map(E_Layer layer)
{
   switch (layer)
     {
      case E_LAYER_BOTTOM: return 0;
      case E_LAYER_BG: return 1;
      case E_LAYER_DESKTOP: return 2;
      case E_LAYER_DESKTOP_TOP: return 3;
      case E_LAYER_CLIENT_DESKTOP: return 4;
      case E_LAYER_CLIENT_BELOW: return 5;
      case E_LAYER_CLIENT_NORMAL: return 6;
      case E_LAYER_CLIENT_ABOVE: return 7;
      case E_LAYER_CLIENT_EDGE: return 8;
      case E_LAYER_CLIENT_FULLSCREEN: return 9;
      case E_LAYER_CLIENT_EDGE_FULLSCREEN: return 10;
      case E_LAYER_CLIENT_POPUP: return 11;
      case E_LAYER_CLIENT_TOP: return 12;
      case E_LAYER_CLIENT_DRAG: return 13;
      case E_LAYER_CLIENT_PRIO: return 14;
      case E_LAYER_POPUP: return 15;
      case E_LAYER_MENU: return 16;
      case E_LAYER_DESKLOCK: return 17;
      case E_LAYER_MAX: return 18;
      default: break;
     }
   return 9999;
}

EAPI unsigned int
e_comp_canvas_client_layer_map(E_Layer layer)
{
   switch (layer)
     {
      case E_LAYER_CLIENT_DESKTOP: return 0;
      case E_LAYER_CLIENT_BELOW: return 1;
      case E_LAYER_CLIENT_NORMAL: return 2;
      case E_LAYER_CLIENT_ABOVE: return 3;
      case E_LAYER_CLIENT_EDGE: return 4;
      case E_LAYER_CLIENT_FULLSCREEN: return 5;
      case E_LAYER_CLIENT_EDGE_FULLSCREEN: return 6;
      case E_LAYER_CLIENT_POPUP: return 7;
      case E_LAYER_CLIENT_TOP: return 8;
      case E_LAYER_CLIENT_DRAG: return 9;
      case E_LAYER_CLIENT_PRIO: return 10;
      default: break;
     }
   return 9999;
}

EAPI E_Layer
e_comp_canvas_client_layer_map_nearest(int layer)
{
#define LAYER_MAP(X) \
   if (layer <= X) return X

   LAYER_MAP(E_LAYER_CLIENT_DESKTOP);
   LAYER_MAP(E_LAYER_CLIENT_BELOW);
   LAYER_MAP(E_LAYER_CLIENT_NORMAL);
   LAYER_MAP(E_LAYER_CLIENT_ABOVE);
   LAYER_MAP(E_LAYER_CLIENT_EDGE);
   LAYER_MAP(E_LAYER_CLIENT_FULLSCREEN);
   LAYER_MAP(E_LAYER_CLIENT_EDGE_FULLSCREEN);
   LAYER_MAP(E_LAYER_CLIENT_POPUP);
   LAYER_MAP(E_LAYER_CLIENT_TOP);
   LAYER_MAP(E_LAYER_CLIENT_DRAG);
   return E_LAYER_CLIENT_PRIO;
}
