#include "private.h"

#include <Ecore_Getopt.h>
#include <Elementary_Cursor.h>
#include <Ecore_File.h>

static void
_group_smart_set_user(Evas_Smart_Class *sc EINA_UNUSED)
{
}

EVAS_SMART_SUBCLASS_NEW("Group", _group,
                        Evas_Smart_Class, Evas_Smart_Class,
                        evas_object_smart_clipped_class_get, NULL);

static Evas_Object *
group_add(Evas *evas)
{
   return evas_object_smart_add(evas, _group_smart_class_new());
}


struct _Zone {
   Evas *evas;
   Evas_Object *win;
   Evas_Object *event;
   Evas_Object *handling;
   Evas_Object *hint;
   Evas_Object *guide_h;
   Evas_Object *guide_v;
   Evas_Object *display_pos;
   Evas_Object *cmdbox;
   struct {
      Evas_Object *frame;
      Evas_Object *main_box;
      Evas_Object *create;
      Evas_Object *clear;
      Evas_Object *style;
      Evas_Object *zoom;
      Evas_Object *show_guides;
      Evas_Object *show_distances;
      Evas_Object *hex_colors;
      struct {
         int x, y, dx, dy;
         Ecore_Animator *anim;
      } moving;
   } gui;
   struct {
      Evas_Object *frame;
      Evas_Object *image;
      struct {
         int w, h;
      } ideal_size;
      int factor;
      Eina_Bool ready : 1;
   } zoom;
   struct {
      Ecore_Evas *ee;
      Evas_Object *image; /* lives inside ee */
      /* the following are inside zone->win */
      Evas_Object *popup;
      Evas_Object *save_bt;
      Evas_Object *cancel_bt;
      struct {
         Evas_Object *entry;
         Evas_Object *button;
         Evas_Object *popup;
         Evas_Object *selector;
      } file;
      Evas_Object *preview;
      Ecore_Timer *timer;
   } screenshot;
   struct {
      Evas_Object *rulers;
      Evas_Object *distances;
   } group;
   Eina_List *rulers;
   Eina_List *distances;
   Ecore_Animator *tracker;
   int idx;
   int x, y, w, h;
   struct {
      int x, y;
   } last_mouse;
   Eina_Bool last_ruler_used : 1;
   Eina_Bool keyboard_move : 1;
};

static Eina_List *zones;
static const Platform_Funcs *platform_funcs;
static Eina_Bool show_distances = EINA_TRUE;
static Eina_Bool show_guides = EINA_TRUE;
static Eina_Bool hex_colors = EINA_FALSE;
static Eina_Bool visible = EINA_TRUE;
static int retval = EXIT_SUCCESS;

int _log_dom = -1;

enum ruler_type {
  RULER_TYPE_DEFAULT = 0,
  RULER_TYPE_LIGHT,
  RULER_TYPE_DARK,
  RULER_TYPE_LIGHT_FILLED,
  RULER_TYPE_DARK_FILLED,
  RULER_TYPE_GOOD,
  RULER_TYPE_WARNING,
  RULER_TYPE_BAD,
  RULER_TYPE_SENTINEL
};

static enum ruler_type initial_ruler_type = RULER_TYPE_DEFAULT;
static char theme_file[PATH_MAX];

#define RULER_TYPE_PREFIX "eruler/rule/"

static const char *ruler_type_names_strs[] = {
  [RULER_TYPE_DEFAULT] = "default",
  [RULER_TYPE_LIGHT] = "light",
  [RULER_TYPE_DARK] = "dark",
  [RULER_TYPE_LIGHT_FILLED] = "light-filled",
  [RULER_TYPE_DARK_FILLED] = "dark-filled",
  [RULER_TYPE_GOOD] = "good",
  [RULER_TYPE_WARNING] = "warning",
  [RULER_TYPE_BAD] = "bad",
  [RULER_TYPE_SENTINEL] = NULL,
};
#define N_RULER_TYPES (EINA_C_ARRAY_LENGTH(ruler_type_names_strs) - 1)


typedef struct _Distance Distance;
typedef struct _Ruler_Data Ruler_Data;

struct _Distance {
   Zone *zone;
   Evas_Object *a, *b;
   Evas_Object *top, *bottom, *left, *right;
};

struct _Ruler_Data {
   Zone *zone;
   struct {
      int x, y;
   } start, stop;
   Eina_List *distances;
   enum ruler_type type;
};

static Eina_Bool
theme_apply(Evas_Object *edje, const char *group)
{
   const char *errmsg;

   EINA_SAFETY_ON_NULL_RETURN_VAL(edje, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(group, EINA_FALSE);

   if (edje_object_file_set(edje, theme_file, group))
     return EINA_TRUE;

   errmsg = edje_load_error_str(edje_object_load_error_get(edje));
   CRI("Cannot find theme: file=%s group=%s error='%s'",
       theme_file, group, errmsg);
   return EINA_FALSE;
}

static void
show_guides_apply(void)
{
   const Eina_List *l;
   const Zone *zone;

   EINA_LIST_FOREACH(zones, l, zone)
     {
        elm_check_state_set(zone->gui.show_guides, show_guides);
     }
}

static void
show_distances_apply(void)
{
   const Eina_List *l;
   const Zone *zone;

   EINA_LIST_FOREACH(zones, l, zone)
     {
        if (show_distances && zone->distances)
          evas_object_show(zone->group.distances);
        else
          evas_object_hide(zone->group.distances);
        elm_check_state_set(zone->gui.show_distances,
                            show_distances);
     }
}

static Ruler_Data *
ruler_data_get(const Evas_Object *o)
{
   return evas_object_data_get(o, "ruler_data");
}

static Evas_Object *
zone_ruler_last_get(const Zone *zone)
{
   return eina_list_data_get(eina_list_last(zone->rulers));
}

static void distance_update(Distance *d);

static void
_ruler_distances_update(Evas_Object *ruler)
{
   Ruler_Data *rd = ruler_data_get(ruler);
   const Eina_List *l;
   Distance *d;

   EINA_LIST_FOREACH(rd->distances, l, d)
     distance_update(d);
}

static void
zone_last_ruler_used_set(Zone *zone, Eina_Bool used)
{
   Evas_Object *ruler;

   zone->last_ruler_used = used;
   ruler = zone_ruler_last_get(zone);
   _ruler_distances_update(ruler);

   if (zone->gui.frame)
     {
        elm_object_disabled_set(zone->gui.create, !used);

        if ((!used) && (eina_list_count(zone->rulers) == 1))
          elm_object_disabled_set(zone->gui.clear, EINA_TRUE);
        else if (eina_list_count(zone->rulers) > 1)
          elm_object_disabled_set(zone->gui.clear, EINA_FALSE);
        else
          elm_object_disabled_set(zone->gui.clear, !used);
     }
}

static void
zone_rulers_clear(Zone *zone)
{
   Evas_Object *ruler;
   EINA_LIST_FREE(zone->rulers, ruler)
     evas_object_del(ruler);
}

static void
_ruler_state_update(Evas_Object *ruler)
{
   const Ruler_Data *rd = ruler_data_get(ruler);
   Edje_Message_Int_Set *msg;
   int x, y, w, h, dx, dy;

   x = rd->start.x;
   y = rd->start.y;
   dx = w = rd->stop.x - rd->start.x;
   dy = h = rd->stop.y - rd->start.y;

   if (w < 0)
     {
        w = -w;
        x -= w;
     }

   if (h < 0)
     {
        h = -h;
        y -= h;
     }

   w++;
   h++;

   dx = dx < 0 ? -w : w;
   dy = dy < 0 ? -h : h;

   evas_object_move(ruler, x, y);
   evas_object_resize(ruler, w, h);

   msg = alloca(sizeof(Edje_Message_Int_Set) + sizeof(int));
   msg->count = 2;
   msg->val[0] = rd->start.x;
   msg->val[1] = rd->start.y;
   edje_object_message_send(ruler, EDJE_MESSAGE_INT_SET, 0, msg);

   msg->val[0] = dx;
   msg->val[1] = dy;
   edje_object_message_send(ruler, EDJE_MESSAGE_INT_SET, 1, msg);
}

static void _zone_gui_style_label_update(Zone *zone);

static void
ruler_type_apply(Evas_Object *ruler)
{
   Ruler_Data *rd = ruler_data_get(ruler);
   char buf[128];

   eina_strlcpy(buf, RULER_TYPE_PREFIX, sizeof(buf));
   eina_strlcat(buf, ruler_type_names_strs[rd->type], sizeof(buf));
   theme_apply(ruler, buf);
   _ruler_state_update(ruler);

   if (rd->zone)
     _zone_gui_style_label_update(rd->zone);
}

static void
ruler_move_relative(Evas_Object *ruler, int dx, int dy)
{
   Ruler_Data *rd;

   DBG("place ruler %p relative by %d, %d", ruler, dx, dy);

   rd = ruler_data_get(ruler);
   rd->start.x += dx;
   rd->start.y += dy;
   rd->stop.x += dx;
   rd->stop.y += dy;

   _ruler_state_update(ruler);
   evas_object_show(ruler);
}

static void
ruler_resize_relative(Evas_Object *ruler, int dw, int dh)
{
   Ruler_Data *rd;

   DBG("resize ruler %p relative by %d, %d", ruler, dw, dh);

   rd = ruler_data_get(ruler);

   rd->stop.x += dw;
   rd->stop.y += dh;

   _ruler_state_update(ruler);
   evas_object_show(ruler);
}

static void
ruler_place(Evas_Object *ruler, int x, int y, int w, int h)
{
   Ruler_Data *rd;

   DBG("place ruler %p at %d,%d size %dx%d", ruler, x, y, w, h);

   rd = ruler_data_get(ruler);
   rd->start.x = x;
   rd->start.y = y;
   rd->stop.x = x + w - 1;
   rd->stop.y = y + h - 1;

   _ruler_state_update(ruler);
   evas_object_show(ruler);
}

static Eina_Bool
_event_mouse_tracker(void *data)
{
   Zone *zone = data;
   Evas_Coord x, y, dx, dy, dw, dh, gx, gy, gw, gh;
   char buf[64];

   if (zone->screenshot.ee) return EINA_TRUE;

   evas_pointer_canvas_xy_get(zone->evas, &x, &y);
   if ((x < 0) || (x >= zone->w) || (y < 0) || (y >= zone->h))
     return EINA_TRUE;

   if ((x == zone->last_mouse.x) && (y == zone->last_mouse.y))
     return EINA_TRUE;
   zone->last_mouse.x = x;
   zone->last_mouse.y = y;

   evas_object_geometry_get(zone->gui.frame, &gx, &gy, &gw, &gh);
   if (((x >= gx) && (x < gx + gw)) &&
       ((y >= gy) && (y < gy + gh)))
     {
        evas_object_hide(zone->display_pos);
        evas_object_hide(zone->guide_v);
        evas_object_hide(zone->guide_h);
        if (zone->zoom.frame)
          evas_object_hide(zone->zoom.frame);
        return EINA_TRUE;
     }
   evas_object_show(zone->display_pos);

   if (zone->handling)
     {
        Ruler_Data *rd = ruler_data_get(zone->handling);
        rd->stop.x = x;
        rd->stop.y = y;
        _ruler_state_update(zone->handling);
        evas_object_show(zone->handling);
     }

   if (show_guides)
     {
        evas_object_move(zone->guide_v, x, 0);
        evas_object_resize(zone->guide_v, 1, zone->h);
        evas_object_show(zone->guide_v);

        evas_object_move(zone->guide_h, 0, y);
        evas_object_resize(zone->guide_h, zone->w, 1);
        evas_object_show(zone->guide_h);
     }

   snprintf(buf, sizeof(buf), "%d,%d", x, y);
   edje_object_part_text_set(zone->display_pos, "text", buf);
   edje_object_size_min_calc(zone->display_pos, &dw, &dh);
   dx = x - dw - 10;
   if (dx < 0)
     dx = x + 10;
   dy = y - dh - 10;
   if (dy < 0)
     dy = y + 10;
   evas_object_move(zone->display_pos, dx, dy);
   evas_object_resize(zone->display_pos, dw, dh);

   if (zone->zoom.ready)
     {
        int fx, fy, fw, fh, zx, zy, zw, zh, stride, iw, ih;
        unsigned int *pixels, color;
        char buf[32];

        zw = zone->zoom.ideal_size.w < zone->w / 4 ?
          zone->zoom.ideal_size.w : zone->w / 4;
        zh = zone->zoom.ideal_size.h < zone->h / 4 ?
          zone->zoom.ideal_size.h : zone->h / 4;

        fx = -x * zone->zoom.factor + zw / 2 - zone->zoom.factor / 2;
        fy = -y * zone->zoom.factor + zh / 2 - zone->zoom.factor / 2;
        fw = zone->w * zone->zoom.factor;
        fh = zone->h * zone->zoom.factor;

        zx = x + 10;
        zy = y + 10;

        if (zx + zw > zone->w)
          zx = dx - zw - 10;

        if (zy + zh > zone->h)
          zy = dy - zh - 10;

        evas_object_move(zone->zoom.frame, zx, zy);
        evas_object_resize(zone->zoom.frame, zw, zh);
        evas_object_image_fill_set(zone->zoom.image, fx, fy, fw, fh);

        pixels = evas_object_image_data_get(zone->zoom.image, EINA_FALSE);
        stride = evas_object_image_stride_get(zone->zoom.image);
        evas_object_image_size_get(zone->zoom.image, &iw, &ih);

        EINA_SAFETY_ON_NULL_RETURN_VAL(pixels, EINA_TRUE);

        EINA_SAFETY_ON_FALSE_GOTO(x >= 0, position_invalid);
        EINA_SAFETY_ON_FALSE_GOTO(y >= 0, position_invalid);
        EINA_SAFETY_ON_FALSE_GOTO(x < iw, position_invalid);
        EINA_SAFETY_ON_FALSE_GOTO(y < ih, position_invalid);

        color = pixels[y * (stride / sizeof(unsigned int)) + x];
        color = color & 0xffffff;
        if (hex_colors)
          snprintf(buf, sizeof(buf), "#%06x", color);
        else
          {
             snprintf(buf, sizeof(buf), "%d %d %d",
                      (color >> 16) & 0xff,
                      (color >> 8) & 0xff,
                      (color & 0xff));
          }
        edje_object_part_text_set(zone->zoom.frame, "color", buf);

     position_invalid:
        evas_object_image_data_set(zone->zoom.image, pixels);
        evas_object_show(zone->zoom.frame);
     }

   return EINA_TRUE;
}

static void
_handling_start(Zone *zone)
{
   Evas_Object *ruler;
   Ruler_Data *rd;
   Edje_Message_Int_Set *msg;

   ruler = zone_ruler_last_get(zone);
   EINA_SAFETY_ON_NULL_RETURN(ruler);

   zone->handling = ruler;

   rd = ruler_data_get(ruler);

   zone_last_ruler_used_set(zone, EINA_TRUE);

   evas_pointer_canvas_xy_get(zone->evas, &rd->start.x, &rd->start.y);
   evas_object_move(ruler, rd->start.x, rd->start.y);
   evas_object_show(ruler);

   msg = alloca(sizeof(Edje_Message_Int_Set) + sizeof(int));
   msg->count = 2;
   msg->val[0] = rd->start.x;
   msg->val[1] = rd->start.y;
   edje_object_message_send(ruler, EDJE_MESSAGE_INT_SET, 0, msg);

   zone->last_mouse.x = -1;
   zone->last_mouse.y = -1;
   _event_mouse_tracker(zone);
}

static void
_handling_stop(Zone *zone)
{
   zone->handling = NULL;
}

static void
_event_mouse_down(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event)
{
   Zone *zone = data;
   Evas_Event_Mouse_Down *ev = event;

   if (zone->handling)
     return;

   if (ev->button != 1)
     return;

   if (zone->cmdbox)
     evas_object_del(zone->cmdbox);

   zone->keyboard_move = EINA_FALSE;
   _handling_start(zone);
}

static void
_event_mouse_up(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   _handling_stop(zone);
}

static void
_event_del(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   if (zone->tracker)
     {
        ecore_animator_del(zone->tracker);
        zone->tracker = NULL;
     }

   zone_rulers_clear(zone);
}

static void
zone_hint_setup(Zone *zone)
{
   zone->hint = edje_object_add(zone->evas);
   if (!theme_apply(zone->hint, "eruler/hint"))
     return;
   evas_object_repeat_events_set(zone->hint, EINA_TRUE);
   evas_object_show(zone->hint);

   edje_object_part_text_set(zone->hint, "hint",
                             "<title>ERuler</><br>"
                             "Press <hilight>F1</hilight> for help or "
                             "<hilight>Escape</hilight> to quit.");

   evas_object_size_hint_weight_set(zone->hint,
                                    EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(zone->hint, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(zone->win, zone->hint);

   zone->guide_h = evas_object_rectangle_add(zone->evas);
   evas_object_pass_events_set(zone->guide_h, EINA_TRUE);
   evas_object_color_set(zone->guide_h, 128, 0, 0, 128);

   zone->guide_v = evas_object_rectangle_add(zone->evas);
   evas_object_pass_events_set(zone->guide_v, EINA_TRUE);
   evas_object_color_set(zone->guide_v, 128, 0, 0, 128);

   zone->display_pos = edje_object_add(zone->evas);
   if (!theme_apply(zone->display_pos, "eruler/display_pos"))
     return;
   evas_object_pass_events_set(zone->display_pos, EINA_TRUE);
   evas_object_show(zone->display_pos);
}

static void
_ruler_free(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Ruler_Data *rd = data;
   eina_list_free(rd->distances);
   free(rd);
}

static void
_distance_source_changed(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Distance *d = data;
   distance_update(d);
}

static void
distance_update(Distance *d)
{
   int ax1, ay1, ax2, ay2, aw, ah;
   int bx1, by1, bx2, by2, bw, bh;
   int x, y, w, h;
   char buf[32];

   if (!d->zone->last_ruler_used)
     {
        Evas_Object *last = zone_ruler_last_get(d->zone);
        if ((last == d->a) || (last == d->b))
          {
             DBG("Hide distance to unused ruler %p (a: %p, b: %p)",
                 last, d->a, d->b);
             evas_object_hide(d->top);
             evas_object_hide(d->bottom);
             evas_object_hide(d->left);
             evas_object_hide(d->right);
             return;
          }
     }

   evas_object_geometry_get(d->a, &ax1, &ay1, &aw, &ah);
   evas_object_geometry_get(d->b, &bx1, &by1, &bw, &bh);

   ax2 = ax1 + aw;
   ay2 = ay1 + ah;

   bx2 = bx1 + bw;
   by2 = by1 + bh;

   /* horizontal distances */
   if (ax2 < bx1)
     {
        x = ax2;
        w = bx1 - ax2;
     }
   else if (ax1 > bx2)
     {
        x = bx2;
        w = ax1 - bx2;
     }
   else
     {
        int x2 = ax2 < bx2 ? ax2 : bx2; /* the left one */
        x = ax1 < bx1 ? bx1 : ax1; /* the right one */
        w = x2 - x;
     }

   if (ay2 < by1)
     {
        evas_object_hide(d->top);
        evas_object_move(d->bottom, x, ay2);
        evas_object_resize(d->bottom, w, (by1 - ay2));
        snprintf(buf, sizeof(buf), "%d", (by1 - ay2));
        edje_object_part_text_set(d->bottom, "display", buf);
        evas_object_show(d->bottom);
     }
   else if (ay1 > by2)
     {
        evas_object_hide(d->bottom);
        evas_object_move(d->top, x, by2);
        evas_object_resize(d->top, w, (ay1 - by2));
        snprintf(buf, sizeof(buf), "%d", (ay1 - by2));
        edje_object_part_text_set(d->top, "display", buf);
        evas_object_show(d->top);
     }
   else
     {
        if (ay1 < by1)
          {
             y = ay1;
             h = by1 - ay1 + 1;
          }
        else
          {
             y = by1;
             h = ay1 - by1 + 1;
          }
        evas_object_move(d->top, x, y);
        evas_object_resize(d->top, w, h);
        snprintf(buf, sizeof(buf), "%d", h);
        edje_object_part_text_set(d->top, "display", buf);
        evas_object_show(d->top);

        if (ay2 < by2)
          {
             y = ay2;
             h = by2 - ay2 + 1;
          }
        else
          {
             y = by2;
             h = ay2 - by2 + 1;
          }
        evas_object_move(d->bottom, x, y);
        evas_object_resize(d->bottom, w, h);
        snprintf(buf, sizeof(buf), "%d", h);
        edje_object_part_text_set(d->bottom, "display", buf);
        evas_object_show(d->bottom);
     }

   /* horizontal distances */
   if (ay2 < by1)
     {
        y = ay2;
        h = by1 - ay2;
     }
   else if (ay1 > by2)
     {
        y = by2;
        h = ay1 - by2;
     }
   else
     {
        int y2 = ay2 < by2 ? ay2 : by2; /* the top one */
        y = ay1 < by1 ? by1 : ay1; /* the bottom one */
        h = y2 - y;
     }

   if (ax2 < bx1)
     {
        evas_object_hide(d->left);
        evas_object_move(d->right, ax2, y);
        evas_object_resize(d->right, (bx1 - ax2), h);
        snprintf(buf, sizeof(buf), "%d", (bx1 - ax2));
        edje_object_part_text_set(d->right, "display", buf);
        evas_object_show(d->right);
     }
   else if (ax1 > bx2)
     {
        evas_object_hide(d->right);
        evas_object_move(d->left, bx2, y);
        evas_object_resize(d->left, (ax1 - bx2), h);
        snprintf(buf, sizeof(buf), "%d", (ax1 - bx2));
        edje_object_part_text_set(d->left, "display", buf);
        evas_object_show(d->left);
     }
   else
     {
        if (ax1 < bx1)
          {
             x = ax1;
             w = bx1 - ax1 + 1;
          }
        else
          {
             x = bx1;
             w = ax1 - bx1 + 1;
          }
        evas_object_move(d->left, x, y);
        evas_object_resize(d->left, w, h);
        snprintf(buf, sizeof(buf), "%d", w);
        edje_object_part_text_set(d->left, "display", buf);
        evas_object_show(d->left);

        if (ax2 < bx2)
          {
             x = ax2;
             w = bx2 - ax2 + 1;
          }
        else
          {
             x = bx2;
             w = ax2 - bx2 + 1;
          }
        evas_object_move(d->right, x, y);
        evas_object_resize(d->right, w, h);
        snprintf(buf, sizeof(buf), "%d", w);
        edje_object_part_text_set(d->right, "display", buf);
        evas_object_show(d->right);
     }
}

static void _distance_source_del(void *data, Evas *e EINA_UNUSED, Evas_Object *o, void *event EINA_UNUSED);

static void
distance_del(Distance *d)
{
   Ruler_Data *rd;
   Zone *zone = d->zone;

   /* 'a' or 'b' may have vanished (_distance_source_del) and will be NULL */

   if (d->a)
     {
        rd = ruler_data_get(d->a);
        rd->distances = eina_list_remove(rd->distances, d);

        evas_object_event_callback_del_full(d->a, EVAS_CALLBACK_DEL,
                                            _distance_source_del, d);
        evas_object_event_callback_del_full(d->a, EVAS_CALLBACK_MOVE,
                                            _distance_source_changed, d);
        evas_object_event_callback_del_full(d->a, EVAS_CALLBACK_RESIZE,
                                            _distance_source_changed, d);
     }

   if (d->b)
     {
        rd = ruler_data_get(d->b);
        rd->distances = eina_list_remove(rd->distances, d);

        evas_object_event_callback_del_full(d->b, EVAS_CALLBACK_DEL,
                                            _distance_source_del, d);
        evas_object_event_callback_del_full(d->b, EVAS_CALLBACK_MOVE,
                                            _distance_source_changed, d);
        evas_object_event_callback_del_full(d->b, EVAS_CALLBACK_RESIZE,
                                            _distance_source_changed, d);
     }

   evas_object_del(d->top);
   evas_object_del(d->bottom);
   evas_object_del(d->left);
   evas_object_del(d->right);

   zone->distances = eina_list_remove(zone->distances, d);
   free(d);

   if (!zone->distances)
     evas_object_hide(zone->group.distances);
}

static void
_distance_source_del(void *data, Evas *e EINA_UNUSED, Evas_Object *o, void *event EINA_UNUSED)
{
   Distance *d = data;

   /* if we delete 'a' we need to stop monitoring 'b' to avoid double free
    * and remove from 'b' distances to not mess with deleted distance
    */
   if (o == d->a)
     d->a = NULL;
   else
     d->b = NULL;

   distance_del(d);
}

static void
distance_create(Zone *zone, Evas_Object *a, Evas_Object *b)
{
   Ruler_Data *rd;
   Distance *d;

   d = calloc(1, sizeof(Distance));
   EINA_SAFETY_ON_NULL_RETURN(d);

   d->zone = zone;
   d->a = a;
   d->b = b;

   evas_object_event_callback_add(a, EVAS_CALLBACK_DEL,
                                  _distance_source_del, d);
   evas_object_event_callback_add(a, EVAS_CALLBACK_MOVE,
                                  _distance_source_changed, d);
   evas_object_event_callback_add(a, EVAS_CALLBACK_RESIZE,
                                  _distance_source_changed, d);

   rd = ruler_data_get(d->a);
   rd->distances = eina_list_append(rd->distances, d);

   evas_object_event_callback_add(b, EVAS_CALLBACK_DEL,
                                  _distance_source_del, d);
   evas_object_event_callback_add(b, EVAS_CALLBACK_MOVE,
                                  _distance_source_changed, d);
   evas_object_event_callback_add(b, EVAS_CALLBACK_RESIZE,
                                  _distance_source_changed, d);
   rd = ruler_data_get(d->b);
   rd->distances = eina_list_append(rd->distances, d);

   /* clipper was hidden if clipping nothing */
   if ((!zone->distances) && (show_distances))
     evas_object_show(zone->group.distances);

   zone->distances = eina_list_append(zone->distances, d);

   d->top = edje_object_add(zone->evas);
   theme_apply(d->top, "eruler/distance_vertical");
   evas_object_smart_member_add(d->top, zone->group.distances);

   d->bottom = edje_object_add(zone->evas);
   theme_apply(d->bottom, "eruler/distance_vertical");
   evas_object_smart_member_add(d->bottom, zone->group.distances);

   d->left = edje_object_add(zone->evas);
   theme_apply(d->left, "eruler/distance_horizontal");
   evas_object_smart_member_add(d->left, zone->group.distances);

   d->right = edje_object_add(zone->evas);
   theme_apply(d->right, "eruler/distance_horizontal");
   evas_object_smart_member_add(d->right, zone->group.distances);
}

static void
zone_ruler_create(Zone *zone)
{
   Evas_Object *ruler;
   Ruler_Data *rd;

   rd = calloc(1, sizeof(Ruler_Data));
   EINA_SAFETY_ON_NULL_RETURN(rd);

   rd->zone = zone;
   ruler = edje_object_add(zone->evas);
   evas_object_smart_member_add(ruler, zone->group.rulers);
   evas_object_event_callback_add(ruler, EVAS_CALLBACK_FREE,
                                  _ruler_free, rd);
   evas_object_pass_events_set(ruler, EINA_TRUE);
   evas_object_data_set(ruler, "ruler_data", rd);
   rd->type = initial_ruler_type;

   if (zone->rulers)
     {
        Evas_Object *other;
        Eina_List *l;
        EINA_LIST_FOREACH(zone->rulers, l, other)
          distance_create(zone, other, ruler);
     }

   zone->rulers = eina_list_append(zone->rulers, ruler);

   ruler_type_apply(ruler);
   zone_last_ruler_used_set(zone, EINA_FALSE);
}

static void
zone_ruler_setup(Zone *zone)
{
   zone->event = evas_object_rectangle_add(zone->evas);
   evas_object_color_set(zone->event, 0, 0, 0, 0);
   evas_object_repeat_events_set(zone->event, EINA_TRUE);
   evas_object_show(zone->event);

   evas_object_event_callback_add(zone->event, EVAS_CALLBACK_MOUSE_DOWN,
                                  _event_mouse_down, zone);
   evas_object_event_callback_add(zone->event, EVAS_CALLBACK_MOUSE_UP,
                                  _event_mouse_up, zone);
   evas_object_event_callback_add(zone->event, EVAS_CALLBACK_DEL,
                                  _event_del, zone);
   evas_object_size_hint_weight_set(zone->event,
                                    EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(zone->event, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(zone->win, zone->event);

   zone->group.distances = group_add(zone->evas);
   evas_object_size_hint_weight_set(zone->group.distances,
                                    EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(zone->group.distances,
                                   EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(zone->win, zone->group.distances);

   zone->group.rulers = group_add(zone->evas);
   evas_object_size_hint_weight_set(zone->group.rulers,
                                    EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(zone->group.rulers,
                                   EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_win_resize_object_add(zone->win, zone->group.rulers);
   evas_object_show(zone->group.rulers);

   zone_ruler_create(zone);

   zone->tracker = ecore_animator_add(_event_mouse_tracker, zone);
}

static void
_zone_gui_resize_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   int x, y, w, h;

   evas_object_geometry_get(zone->gui.frame, &x, &y, NULL, NULL);
   evas_object_size_hint_min_get(zone->gui.frame, &w, &h);

   if (x + w > zone->w - 10)
     x = zone->w - 10 - w;

   if (y + h > zone->h - 10)
     y = zone->h - 10 - h;

   if (x < 10)
     x = 10;
   if (y < 10)
     y = 10;

   evas_object_move(zone->gui.frame, x, y);
   evas_object_resize(zone->gui.frame, w, h);
}

static void
_zone_gui_create(void *data, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   if (zone->last_ruler_used)
     zone_ruler_create(zone);
}

static void
_zone_gui_clear(void *data, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   zone_rulers_clear(zone);
   zone_ruler_create(zone);
}

static void create_ruler_from_cmdbox(Zone *zone);
static void
_zone_gui_type(void *data, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   create_ruler_from_cmdbox(zone);
}

static void
_zone_gui_style_label_update(Zone *zone)
{
   Evas_Object *ruler;
   Ruler_Data *rd;
   char buf[128];

   if (!zone->gui.style) return;

   ruler = zone_ruler_last_get(zone);
   rd = ruler_data_get(ruler);
   snprintf(buf, sizeof(buf), "Style: %s", ruler_type_names_strs[rd->type]);
   elm_object_text_set(zone->gui.style, buf);
}

static void
_zone_gui_style_changed(void *data, Evas_Object *o EINA_UNUSED, void *event)
{
   Zone *zone = data;
   Evas_Object *ruler;
   Ruler_Data *rd;
   Elm_Object_Item *it = event;
   const char *style = elm_object_item_text_get(it);
   unsigned int i;

   for (i = 0; ruler_type_names_strs[i] != NULL; i++)
     {
        if (strcmp(ruler_type_names_strs[i], style) == 0)
          break;
     }

   EINA_SAFETY_ON_NULL_RETURN(ruler_type_names_strs[i]);

   ruler = zone_ruler_last_get(zone);
   EINA_SAFETY_ON_NULL_RETURN(ruler);
   rd = ruler_data_get(ruler);

   rd->type = i;
   ruler_type_apply(ruler);
}

static void zone_zoom_pre_setup(Zone *zone);
static void _zone_screen_copy_cb(void *data, Eina_Bool success);

static void
_zone_gui_zoom_changed(void *data, Evas_Object *o, void *event EINA_UNUSED)
{
   Zone *zone = data;
   Eina_Bool state = elm_check_state_get(o);

   if (state)
     {
        if (!zone->zoom.image)
          {
             zone_zoom_pre_setup(zone);
             platform_funcs->zone_screen_copy(zone, zone->zoom.image,
                                              _zone_screen_copy_cb,
                                              zone);
          }
     }
   else
     {
        if (zone->zoom.image)
          {
             evas_object_del(zone->zoom.image);
             zone->zoom.image = NULL;
             evas_object_del(zone->zoom.frame);
             zone->zoom.frame = NULL;
             zone->zoom.ready = EINA_FALSE;
          }
     }
}

static void
_zone_gui_show_hex_colors_changed(void *data EINA_UNUSED, Evas_Object *o, void *event EINA_UNUSED)
{
   Eina_Bool state = elm_check_state_get(o);
   hex_colors = state;
}

static void
_zone_gui_show_guides_changed(void *data EINA_UNUSED, Evas_Object *o, void *event EINA_UNUSED)
{
   Eina_Bool state = elm_check_state_get(o);
   show_guides = state;
   show_guides_apply();
}

static void
_zone_gui_show_distances_changed(void *data EINA_UNUSED, Evas_Object *o, void *event EINA_UNUSED)
{
   Eina_Bool state = elm_check_state_get(o);
   show_distances = state;
   show_distances_apply();
}

static void show_gui_help(Zone *zone);
static void
_zone_gui_help(void *data, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   show_gui_help(zone);
}

static void
_zone_gui_exit(void *data EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   elm_exit();
}

static void create_screenshot(Zone *zone);
static void
_zone_gui_shot(void *data, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   create_screenshot(zone);
}

static void
_zone_gui_widget_setup(Evas_Object *o)
{
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   elm_object_focus_allow_set(o, EINA_FALSE);
}

static void
_zone_gui_icon_set(Evas_Object *o, const char *part, const char *iconname)
{
   Evas_Object *ic = elm_icon_add(o);

   if (!elm_icon_standard_set(ic, iconname))
     {
        evas_object_del(ic);
        return;
     }

   elm_object_part_content_set(o, part, ic);
}

static Evas_Object *
_zone_gui_check_add(Zone *zone, Eina_Bool state, const char *label, Evas_Smart_Cb cb)
{
   Evas_Object *ck = elm_check_add(zone->gui.main_box);
   _zone_gui_widget_setup(ck);
   elm_object_text_set(ck, label);
   elm_check_state_set(ck, state);
   evas_object_smart_callback_add(ck, "changed", cb, zone);
   evas_object_show(ck);
   elm_box_pack_end(zone->gui.main_box, ck);
   return ck;
}

static Evas_Object *
_zone_gui_button_add(Zone *zone, const char *icon, const char *label, Evas_Smart_Cb cb)
{
   Evas_Object *bt = elm_button_add(zone->gui.main_box);
   _zone_gui_widget_setup(bt);
   _zone_gui_icon_set(bt, NULL, icon);
   elm_object_text_set(bt, label);
   evas_object_smart_callback_add(bt, "clicked", cb, zone);
   evas_object_show(bt);
   elm_box_pack_end(zone->gui.main_box, bt);
   return bt;
}

static Eina_Bool
_zone_gui_frame_moving(void *data)
{
   Zone *zone = data;
   int x, y, w, h;

   evas_pointer_canvas_xy_get(zone->evas, &x, &y);
   evas_object_size_hint_min_get(zone->gui.frame, &w, &h);

   x -= zone->gui.moving.dx;
   y -= zone->gui.moving.dy;

   if (x + w > zone->w - 10)
     x = zone->w - 10 - w;

   if (y + h > zone->h - 10)
     y = zone->h - 10 - h;

   if (x < 10)
     x = 10;
   if (y < 10)
     y = 10;

   evas_object_move(zone->gui.frame, x, y);
   evas_object_resize(zone->gui.frame, w, h);

   return EINA_TRUE;
}

static void
_zone_gui_frame_mouse_down(void *data, Evas *e EINA_UNUSED, Evas_Object *o, void *event)
{
   Zone *zone = data;
   Evas_Event_Mouse_Down *ev = event;
   int x, y;

   if ((zone->gui.moving.anim) || (ev->button != 1))
     return;

   if (!elm_frame_collapse_get(o))
     {
        int bx, by, bw, bh;
        evas_object_geometry_get(zone->gui.main_box, &bx, &by, &bw, &bh);
        if (((ev->canvas.x >= bx) && (ev->canvas.x < bx + bw)) &&
            ((ev->canvas.y >= by) && (ev->canvas.y < by + bh)))
          return;
     }

   evas_object_geometry_get(o, &x, &y, NULL, NULL);

   zone->gui.moving.x = ev->canvas.x;
   zone->gui.moving.y = ev->canvas.y;
   zone->gui.moving.dx = ev->canvas.x - x;
   zone->gui.moving.dy = ev->canvas.y - y;
   zone->gui.moving.anim = ecore_animator_add(_zone_gui_frame_moving, zone);
}

static void
_zone_gui_frame_mouse_up(void *data, Evas *e EINA_UNUSED, Evas_Object *o, void *event)
{
   Zone *zone = data;
   Evas_Event_Mouse_Up *ev = event;
   int fingersize, dx, dy;

   if ((!zone->gui.moving.anim) || (ev->button != 1))
     return;

   fingersize = elm_config_finger_size_get();

   dx = zone->gui.moving.x - ev->canvas.x;
   dy = zone->gui.moving.y - ev->canvas.y;

   if (dx < 0)
     dx = -dx;
   if (dy < 0)
     dy = -dy;

   if ((dx < fingersize) && (dy < fingersize))
     {
        evas_object_move(o, zone->gui.moving.x - zone->gui.moving.dx,
                         zone->gui.moving.y - zone->gui.moving.dy);
        elm_frame_collapse_set(o, !elm_frame_collapse_get(o));
     }

   zone->gui.moving.x = 0;
   zone->gui.moving.y = 0;
   zone->gui.moving.dx = 0;
   zone->gui.moving.dy = 0;
   ecore_animator_del(zone->gui.moving.anim);
   zone->gui.moving.anim = NULL;
}

static void
zone_gui_setup(Zone *zone)
{
   Evas_Object *o;
   unsigned int i;

   zone->gui.frame = elm_frame_add(zone->win);
   elm_object_focus_allow_set(zone->gui.frame, EINA_FALSE);
   evas_object_event_callback_add(zone->gui.frame,
                                  EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _zone_gui_resize_cb, zone);
   if (eina_list_count(zones) == 1)
     elm_object_text_set(zone->gui.frame, "ERuler");
   else
     {
        char buf[128];
        snprintf(buf, sizeof(buf), "ERuler (Zone #%d)", zone->idx);
        elm_object_text_set(zone->gui.frame, buf);
     }

   evas_object_event_callback_add(zone->gui.frame, EVAS_CALLBACK_MOUSE_DOWN,
                                  _zone_gui_frame_mouse_down, zone);
   evas_object_event_callback_add(zone->gui.frame, EVAS_CALLBACK_MOUSE_UP,
                                  _zone_gui_frame_mouse_up, zone);

   zone->gui.main_box = elm_box_add(zone->gui.frame);
   elm_box_horizontal_set(zone->gui.main_box, EINA_FALSE);

   zone->gui.create = _zone_gui_button_add
     (zone, "add", "Create ruler", _zone_gui_create);
   elm_object_disabled_set(zone->gui.create, EINA_TRUE);
   zone->gui.clear = _zone_gui_button_add
     (zone, "editclear", "Clear rulers", _zone_gui_clear);
   elm_object_disabled_set(zone->gui.clear, EINA_TRUE);
   _zone_gui_button_add(zone, "keyboard", "Type coordinates", _zone_gui_type);

   zone->gui.style = o = elm_hoversel_add(zone->gui.main_box);
   elm_hoversel_hover_parent_set(o, zone->win);
   _zone_gui_widget_setup(o);
   for (i = 0; ruler_type_names_strs[i] != NULL; i++)
     {
        const char *label = ruler_type_names_strs[i];
        elm_hoversel_item_add(o, label, NULL, ELM_ICON_NONE, NULL, NULL);
     }
   evas_object_smart_callback_add(o, "selected", _zone_gui_style_changed, zone);
   _zone_gui_style_label_update(zone);
   evas_object_show(o);
   elm_box_pack_end(zone->gui.main_box, o);

   zone->gui.zoom = _zone_gui_check_add
     (zone, !!zone->zoom.image, "Zoom", _zone_gui_zoom_changed);
   zone->gui.hex_colors = _zone_gui_check_add
     (zone, hex_colors, "Show hexadecimal colors",
      _zone_gui_show_hex_colors_changed);
   zone->gui.show_guides = _zone_gui_check_add
     (zone, show_guides, "Show guidelines", _zone_gui_show_guides_changed);
   zone->gui.show_distances = _zone_gui_check_add
     (zone, show_distances, "Show distances", _zone_gui_show_distances_changed);

   _zone_gui_button_add(zone, "filesaveas", "Save screenshot", _zone_gui_shot);
   _zone_gui_button_add(zone, "help", "Help", _zone_gui_help);
   _zone_gui_button_add(zone, "exit", "Exit", _zone_gui_exit);

   elm_object_content_set(zone->gui.frame, zone->gui.main_box);
   evas_object_show(zone->gui.main_box);

   evas_object_move(zone->gui.frame, 10, 10);
   evas_object_show(zone->gui.frame);
}

static void
zone_zoom_pre_setup(Zone *zone)
{
   const char *str;

   if (zone->zoom.frame) return;

   zone->zoom.ideal_size.w = 256;
   zone->zoom.ideal_size.h = 256;
   zone->zoom.factor = 10;

   zone->zoom.frame = edje_object_add(zone->evas);
   theme_apply(zone->zoom.frame, "eruler/zoom_viewfinder");

   zone->zoom.image = evas_object_image_add(zone->evas);
   evas_object_image_smooth_scale_set(zone->zoom.image, EINA_FALSE);
   edje_object_part_swallow(zone->zoom.frame, "content", zone->zoom.image);

   str = edje_object_data_get(zone->zoom.frame, "ideal_size");
   DBG("Zoom viewfinder ideal size: %s", str);
   if (str)
     {
        int n = sscanf(str, "%d %d",
                       &zone->zoom.ideal_size.w,
                       &zone->zoom.ideal_size.h);

        if (n == 1)
          zone->zoom.ideal_size.h = zone->zoom.ideal_size.w;

        if (zone->zoom.ideal_size.w < 1)
          zone->zoom.ideal_size.w = 256;
        if (zone->zoom.ideal_size.h < 1)
          zone->zoom.ideal_size.h = 256;
     }

   str = edje_object_data_get(zone->zoom.frame, "factor");
   DBG("Zoom factor: %s", str);
   if (str)
     {
        zone->zoom.factor = atoi(str);
        if (zone->zoom.factor < 1)
          zone->zoom.factor = 10;
     }

   zone->zoom.ready = EINA_FALSE;
}

static void
_popup_dismiss_cb(void *data, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Object *popup = data;
   evas_object_del(popup);
}

static void
show_gui_help(Zone *zone)
{
   Evas_Object *popup, *help, *bt;

   popup = elm_popup_add(zone->win);
   elm_popup_content_text_wrap_type_set(popup, ELM_WRAP_MIXED);

   elm_object_part_text_set(popup, "title,text", "ERuler");

   help = elm_entry_add(popup);
   elm_entry_editable_set(help, EINA_FALSE);

#if (ELM_VERSION_MAJOR >= 1) && (ELM_VERSION_MINOR >= 8)
   elm_scroller_policy_set(help, ELM_SCROLLER_POLICY_OFF,
                           ELM_SCROLLER_POLICY_AUTO);
#else
   elm_entry_scrollbar_policy_set(help, ELM_SCROLLER_POLICY_OFF,
                                  ELM_SCROLLER_POLICY_AUTO);
#endif

   elm_object_text_set
     (help,
      "Press your mouse's left-button to start measuring, "
      "release to stop.<br><br>"
      "Keyboard shortcuts:<br>"
      "  • <b>Escape:</b> quit;<br>"
      "  • <b>F2:</b> toggle ERuler visibility;<br>"
      "  • <b>p:</b> print size to stdout;<br>"
      "  • <b>c:</b> create new ruler;<br>"
      "  • <b>Control-c:</b> clear current zone rulers;<br>"
      "  • <b>g:</b> toggle display of guide lines;<br>"
      "  • <b>d:</b> toggle display of distances between boxes;<br>"
      "  • <b>t:</b> toggle ruler type (dark, light, filled...);<br>"
      "  • <b>z:</b> toggle zoom;<br>"
      "  • <b>x:</b> toggle display of colors in hexadecimal;<br>"
      "  • <b>@:</b> open command box to type ruler placement;<br>"
      "  • <b>Space:</b> start or stop measure using keyboard;<br>"
      "  • <b>Left:</b> move ruler start point to the left (<b>Shift</b> to use 10px step);<br>"
      "  • <b>Control-Left:</b> move ruler end point to the left (<b>Shift</b> to use 10px step);<br>"
      "  • <b>Right:</b> move ruler start point to the right (<b>Shift</b> to use 10px step);<br>"
      "  • <b>Control-Right:</b> move ruler end point to the right (<b>Shift</b> to use 10px step);<br>"
      "  • <b>Up:</b> move ruler start point up (<b>Shift</b> to use 10px step);<br>"
      "  • <b>Control-Up:</b> move ruler end point up (<b>Shift</b> to use 10px step);<br>"
      "  • <b>Down:</b> move ruler start point down (<b>Shift</b> to use 10px step);<br>"
      "  • <b>Control-Down:</b> move ruler end point down (<b>Shift</b> to use 10px step);<br>"
      "");

   elm_object_content_set(popup, help);

   bt = elm_button_add(popup);
   elm_object_text_set(bt, "Close");
   evas_object_smart_callback_add(bt, "clicked", _popup_dismiss_cb, popup);
   elm_object_part_content_set(popup, "button1", bt);

   evas_object_show(popup);
   elm_object_focus_set(bt, EINA_TRUE);
}

static void
zone_print_measurements(const Zone *zone)
{
   const Evas_Object *ruler, *last_ruler;
   const Eina_List *l;
   unsigned int i;

   printf("Zone %d,%d size %dx%d\n", zone->x, zone->y, zone->w, zone->h);
   if ((!zone->rulers) ||
       ((eina_list_count(zone->rulers) == 1) && (!zone->last_ruler_used)))
     {
        puts("\tnothing measured.");
        return;
     }

   last_ruler = zone_ruler_last_get(zone);
   i = 0;
   EINA_LIST_FOREACH(zone->rulers, l, ruler)
     {
        const Ruler_Data *rd = ruler_data_get(ruler);

        if ((ruler == last_ruler) && (!zone->last_ruler_used))
          break;

        i++;
        printf("\tmeasure #%d: %+dx%+d from %d,%d to %d,%d\n",
               i,
               rd->stop.x - rd->start.x + 1,
               rd->stop.y - rd->start.y + 1,
               zone->x + rd->start.x,
               zone->y + rd->start.y,
               zone->x + rd->stop.x,
               zone->y + rd->stop.y);
     }
}

static void
print_measurements(void)
{
   const Eina_List *l;
   const Zone *zone;
   EINA_LIST_FOREACH(zones, l, zone)
     zone_print_measurements(zone);
}

static void
_create_ruler_cmdbox_activated(void *data, Evas_Object *o, void *event_info EINA_UNUSED)
{
   Zone *zone = data;
   Evas_Object *ruler;
   int x, y, w, h;
   const char *str;

   str = elm_entry_entry_get(o);
   EINA_SAFETY_ON_NULL_GOTO(str, end);

   if (sscanf(str, "%d %d %d %d", &x, &y, &w, &h) != 4)
     {
        ERR("Invalid creation format. Expected 'x y w h', got '%s'", str);
        goto end;
     }
   if (w < 1 || h < 1)
     {
        ERR("Invalid size: %dx%d", w, h);
        goto end;
     }

   ruler = zone_ruler_last_get(zone);
   EINA_SAFETY_ON_NULL_GOTO(ruler, end);

   zone_last_ruler_used_set(zone, EINA_TRUE);
   ruler_place(ruler, x, y, w, h);

 end:
   evas_object_del(zone->cmdbox);
}

static void
_create_ruler_cmdbox_aborted(void *data, Evas_Object *o EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Zone *zone = data;
   evas_object_del(zone->cmdbox);
}

static void
_create_ruler_cmdbox_size_changed(void *data, Evas *e EINA_UNUSED, Evas_Object *frame, void *event_info EINA_UNUSED)
{
   Zone *zone = data;
   int mh;

   evas_object_size_hint_min_get(frame, NULL, &mh);
   evas_object_move(frame, 0, 0);
   evas_object_resize(frame, zone->w, mh);
}

static void
_create_ruler_cmdbox_del(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Zone *zone = data;
   zone->cmdbox = NULL;
}

static void
create_ruler_from_cmdbox(Zone *zone)
{
   Evas_Object *frame, *entry;

   if (zone->cmdbox) return;

   frame = elm_frame_add(zone->win);
   /* frame has delayed min size calc, listen for size hint changed */
   evas_object_event_callback_add(frame, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
                                  _create_ruler_cmdbox_size_changed, zone);
   evas_object_event_callback_add(frame, EVAS_CALLBACK_DEL,
                                  _create_ruler_cmdbox_del, zone);

   if (!zone->last_ruler_used)
     elm_object_text_set(frame, "Create ruler (x y w h):");
   else
     elm_object_text_set(frame, "Edit ruler (x y w h):");

   entry = elm_entry_add(frame);
   elm_entry_single_line_set(entry, EINA_TRUE);
   elm_entry_editable_set(entry, EINA_TRUE);
   elm_entry_scrollable_set(entry, EINA_TRUE);
   evas_object_show(entry);

   evas_object_smart_callback_add(entry, "activated",
                                  _create_ruler_cmdbox_activated, zone);
   evas_object_smart_callback_add(entry, "aborted",
                                  _create_ruler_cmdbox_aborted, zone);

   elm_object_focus_set(entry, EINA_TRUE);
   elm_object_content_set(frame, entry);

   evas_object_show(frame);
   zone->cmdbox = frame;
}

static void
_create_screenshot_select_done_cb(void *data, Evas_Object *o EINA_UNUSED, void *event)
{
   Zone *zone = data;
   const char *str = event;

   if (str)
     elm_entry_entry_set(zone->screenshot.file.entry, str);

   evas_object_del(zone->screenshot.file.popup);
   zone->screenshot.file.popup = NULL;
   zone->screenshot.file.selector = NULL;
   elm_object_focus_set(zone->screenshot.popup, EINA_TRUE);
}

static void
_create_screenshot_select_cb(void *data, Evas_Object *btn EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   Evas_Object *popup, *bx, *sizer, *sel;
   const char *fname = elm_entry_entry_get(zone->screenshot.file.entry);

   zone->screenshot.file.popup = popup = elm_popup_add(zone->win);
   elm_object_part_text_set(popup, "title,text", "Select file to save");

   bx = elm_box_add(popup);
   elm_box_layout_set(bx, evas_object_box_layout_stack, NULL, NULL);

   /* stack + sizer to make popup bigger */
   sizer = evas_object_rectangle_add(zone->evas);
#if (ELM_VERSION_MAJOR >= 1) && (ELM_VERSION_MINOR >= 8)
   evas_object_size_hint_min_set(sizer, zone->w * 0.5, zone->h * 0.5);
#else
   /* 1.7 popup theme is badly written and won't adapt base to contents,
    * instead the contents is "limited" to 400px in width. So use this :-(
    */
   evas_object_size_hint_min_set(sizer, 400, (zone->h * 400.0) / zone->w);
#endif
   evas_object_color_set(sizer, 0, 0, 0, 0);
   evas_object_show(sizer);
   elm_box_pack_end(bx, sizer);

   zone->screenshot.file.selector = sel = elm_fileselector_add(popup);
   evas_object_size_hint_weight_set(sel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(sel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_fileselector_is_save_set(sel, EINA_TRUE);
   evas_object_show(sel);

   if (fname)
     {
        if (ecore_file_is_dir(fname))
          elm_fileselector_path_set(sel, fname);
        else
          {
             char *dname = ecore_file_dir_get(fname);
             elm_fileselector_path_set(sel, dname);
             free(dname);
          }
     }

   evas_object_smart_callback_add(sel, "done",
                                  _create_screenshot_select_done_cb, zone);

   elm_box_pack_end(bx, sel);
   elm_object_content_set(popup, bx);

   evas_object_show(popup);
}

static void
_create_screenshot_cleanup(Zone *zone)
{
   ecore_evas_free(zone->screenshot.ee);
   zone->screenshot.ee = NULL;
   zone->screenshot.image = NULL;

   evas_object_del(zone->screenshot.popup);
   zone->screenshot.preview = NULL;
   zone->screenshot.popup = NULL;
   zone->screenshot.file.entry = NULL;
   zone->screenshot.file.button = NULL;
   zone->screenshot.file.popup = NULL;
   zone->screenshot.file.selector = NULL;

   evas_object_focus_set(zone->win, EINA_TRUE);
}

static void
_create_screenshot_notify_dismiss_cb(void *data, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   evas_object_del(data);
}

static void
_create_screenshot_notify_show(Zone *zone, double timeout, const char *msg)
{
   Evas_Object *notify, *bx, *o;

   notify = elm_notify_add(zone->win);
   elm_notify_allow_events_set(notify, EINA_FALSE);
#if (ELM_VERSION_MAJOR >= 1) && (ELM_VERSION_MINOR >= 8)
   elm_notify_align_set(notify, 0.5, 0.5);
#else
   elm_notify_orient_set(notify, ELM_NOTIFY_ORIENT_CENTER);
#endif
   if (timeout > 0.0)
     elm_notify_timeout_set(notify, timeout);

   bx = elm_box_add(notify);
   elm_box_horizontal_set(bx, EINA_FALSE);

   o = elm_label_add(bx);
   elm_object_text_set(o, msg);
   evas_object_show(o);
   elm_box_pack_end(bx, o);

   o = elm_button_add(bx);
   elm_object_text_set(o, "Close");
   evas_object_smart_callback_add(o, "clicked",
                                  _create_screenshot_notify_dismiss_cb, notify);
   evas_object_show(o);
   elm_box_pack_end(bx, o);

   evas_object_show(bx);
   elm_object_content_set(notify, bx);

   evas_object_smart_callback_add(notify, "timeout",
                                  _create_screenshot_notify_dismiss_cb, notify);
   evas_object_show(notify);
}

static Eina_Bool
_create_screenshot_save_timer_cb(void *data)
{
   Zone *zone = data;
   const char *fname, *ext, *opts = NULL;
   char msg[1024];
   char *dname;

   fname = elm_object_text_get(zone->screenshot.file.entry);
   EINA_SAFETY_ON_NULL_GOTO(fname, error);

   dname = ecore_file_dir_get(fname);
   if ((dname) && (!ecore_file_is_dir(dname)))
     {
        if (ecore_file_mkpath(dname))
          {
             snprintf(msg, sizeof(msg),
                      "Could not create save directory: \"%s\"", dname);
             _create_screenshot_notify_show(zone, -1, msg);
             free(dname);
             goto error;
          }
     }
   free(dname);

   ext = strrchr(fname, '.');
   if (ext)
     {
        ext++;
        if (strcasecmp(ext, "png") == 0)
          opts = "compress=9";
        else if ((strcasecmp(ext, "jpg") == 0) ||
                 (strcasecmp(ext, "jpeg") == 0))
          opts = "quality=95";
     }

   if (!evas_object_image_save(zone->screenshot.preview, fname, NULL, opts))
     {
        snprintf(msg, sizeof(msg), "Could not save image: \"%s\"", fname);
        _create_screenshot_notify_show(zone, -1, msg);
     }
   else
     {
        snprintf(msg, sizeof(msg), "Saved as: \"%s\"", fname);
        _create_screenshot_notify_show(zone, 5.0, msg);
     }

 error:
   _create_screenshot_cleanup(zone);
   zone->screenshot.timer = NULL;
   return EINA_FALSE;
}

static void
_create_screenshot_save_cb(void *data, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;

   elm_object_text_set(zone->screenshot.save_bt, "Wait...");
   elm_object_disabled_set(zone->screenshot.save_bt, EINA_TRUE);
   elm_object_disabled_set(zone->screenshot.cancel_bt, EINA_TRUE);
   elm_object_disabled_set(zone->screenshot.file.entry, EINA_TRUE);
   elm_object_disabled_set(zone->screenshot.file.button, EINA_TRUE);

   zone->screenshot.timer = ecore_timer_add
     (0.01, _create_screenshot_save_timer_cb, zone);
}

static void
_create_screenshot_cancel_cb(void *data, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   _create_screenshot_cleanup(zone);
}

static void
find_path_next(char *buf, size_t buflen, const char *dname, const char *fmtname)
{
   char name[NAME_MAX], *bufp;
   size_t bufplen;
   unsigned int i;

   bufp = buf + eina_strlcpy(buf, dname, buflen);
   bufplen = buflen - (bufp - buf);

   if ((bufplen > 0) && (bufp[-1] != '/'))
     {
        bufp[0] = '/';
        bufp++;
        bufplen--;
     }

   for (i = 1; i < (unsigned)-1; i++)
     {
        snprintf(name, sizeof(name), fmtname, i);
        eina_strlcpy(bufp, name, bufplen);
        if (!ecore_file_exists(buf))
          return;
     }
}

static void
_create_screenshot_copy_cb(void *data, Eina_Bool success)
{
   Zone *zone = data;
   const Eina_List *l;
   const Evas_Object *orig_ruler, *last_ruler;
   Evas_Object *popup, *bx, *img, *o, *hbx;
   Edje_Message_Int_Set *msg;
   Evas *e;
   char path[PATH_MAX], buf[128], *bufp;
   int iw, ih;
   size_t bufplen;
   const void *pixels;

   if (!success)
     {
        ecore_evas_free(zone->screenshot.ee);
        zone->screenshot.ee = NULL;
        zone->screenshot.image = NULL;
        return;
     }

   evas_object_resize(zone->screenshot.image, zone->w, zone->h);
   evas_object_image_fill_set(zone->screenshot.image, 0, 0, zone->w, zone->h);
   evas_object_show(zone->screenshot.image);

   msg = alloca(sizeof(Edje_Message_Int_Set) + sizeof(int));
   msg->count = 2;

   bufp = buf + eina_strlcpy(buf, RULER_TYPE_PREFIX, sizeof(buf));
   bufplen = sizeof(buf) - (bufp - buf);

   e = ecore_evas_get(zone->screenshot.ee);
   last_ruler = zone_ruler_last_get(zone);
   EINA_LIST_FOREACH(zone->rulers, l, orig_ruler)
     {
        const Ruler_Data *orig_rd;
        Evas_Object *ro;
        int x, y, w, h, dx, dy;

        if ((!zone->last_ruler_used) && (orig_ruler == last_ruler)) continue;

        orig_rd = ruler_data_get(orig_ruler);
        ro =  edje_object_add(e);
        x = orig_rd->start.x;
        y = orig_rd->start.y;
        dx = w = orig_rd->stop.x - orig_rd->start.x;
        dy = h = orig_rd->stop.y - orig_rd->start.y;

        if (w < 0)
          {
             w = -w;
             x -= w;
          }

        if (h < 0)
          {
             h = -h;
             y -= h;
          }

        w++;
        h++;

        dx = dx < 0 ? -w : w;
        dy = dy < 0 ? -h : h;

        eina_strlcpy(bufp, ruler_type_names_strs[orig_rd->type], bufplen);
        theme_apply(ro, buf);

        evas_object_move(ro, x, y);
        evas_object_resize(ro, w, h);
        evas_object_show(ro);

        msg->val[0] = orig_rd->start.x;
        msg->val[1] = orig_rd->start.y;
        edje_object_message_send(ro, EDJE_MESSAGE_INT_SET, 0, msg);

        msg->val[0] = dx;
        msg->val[1] = dy;
        edje_object_message_send(ro, EDJE_MESSAGE_INT_SET, 1, msg);
     }

   /* dialog to ask for where to save, with preview */

   find_path_next(path, sizeof(path), getenv("HOME"), "eruler-%u.png");

   zone->screenshot.popup = popup = elm_popup_add(zone->win);
   elm_object_part_text_set(popup, "title,text", "Save screenshot");

   bx = elm_box_add(popup);
   elm_box_padding_set(bx, 0, 10);
   elm_box_horizontal_set(bx, EINA_FALSE);

   zone->screenshot.preview = img = evas_object_image_filled_add(zone->evas);
   evas_object_size_hint_weight_set(img, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(img, EVAS_HINT_FILL, EVAS_HINT_FILL);
#if (ELM_VERSION_MAJOR >= 1) && (ELM_VERSION_MINOR >= 8)
   evas_object_size_hint_min_set(img, zone->w * 0.5, zone->h * 0.5);
#else
   /* 1.7 popup theme is badly written and won't adapt base to contents,
    * instead the contents is "limited" to 400px in width. So use this :-(
    */
   evas_object_size_hint_min_set(img, 400, (zone->h * 400.0) / zone->w);
#endif

   ecore_evas_geometry_get(zone->screenshot.ee, NULL, NULL, &iw, &ih);
   pixels = ecore_evas_buffer_pixels_get(zone->screenshot.ee);

   evas_object_image_size_set(img, iw, ih);
   evas_object_image_colorspace_set(img, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(img, EINA_FALSE);
   evas_object_image_data_set(img, (void *)pixels);

   evas_object_show(img);
   elm_box_pack_end(bx, img);

   hbx = elm_box_add(bx);
   evas_object_size_hint_weight_set(hbx, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(hbx, EVAS_HINT_FILL, 0.5);
   elm_box_padding_set(hbx, 0, 0);
   elm_box_horizontal_set(hbx, EINA_TRUE);

   o = elm_label_add(hbx);
   elm_object_text_set(o, "Save as:");
   evas_object_show(o);
   elm_box_pack_end(hbx, o);

   zone->screenshot.file.entry = o = elm_entry_add(hbx);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.5);
   elm_entry_single_line_set(o, EINA_TRUE);
   elm_entry_scrollable_set(o, EINA_TRUE);
   elm_entry_entry_set(o, path);
   evas_object_show(o);
   elm_box_pack_end(hbx, o);

   zone->screenshot.file.button = o = elm_button_add(hbx);
   elm_object_text_set(o, "Select");
   evas_object_smart_callback_add(o, "clicked",
                                  _create_screenshot_select_cb, zone);
   evas_object_show(o);
   elm_box_pack_end(hbx, o);

   evas_object_show(hbx);
   elm_box_pack_end(bx, hbx);

   hbx = elm_box_add(bx);
   elm_box_padding_set(hbx, 10, 0);
   elm_box_horizontal_set(hbx, EINA_TRUE);

   zone->screenshot.save_bt = o = elm_button_add(hbx);
   elm_object_text_set(o, "Save");
   evas_object_smart_callback_add(o, "clicked",
                                  _create_screenshot_save_cb, zone);
   evas_object_show(o);
   elm_box_pack_end(hbx, o);

   zone->screenshot.cancel_bt = o = elm_button_add(hbx);
   elm_object_text_set(o, "Cancel");
   evas_object_smart_callback_add(o, "clicked",
                                  _create_screenshot_cancel_cb, zone);
   evas_object_show(o);
   elm_box_pack_end(hbx, o);

   evas_object_show(hbx);
   elm_box_pack_end(bx, hbx);

   elm_object_content_set(popup, bx);

   evas_object_show(popup);
}

static void
create_screenshot(Zone *zone)
{
   Evas *e;

   if (zone->screenshot.ee) return;

   zone->screenshot.ee = ecore_evas_buffer_new(zone->w, zone->h);
   ecore_evas_alpha_set(zone->screenshot.ee, EINA_FALSE);

   e = ecore_evas_get(zone->screenshot.ee);
   zone->screenshot.image = evas_object_image_add(e);

   platform_funcs->zone_screen_copy(zone, zone->screenshot.image,
                                    _create_screenshot_copy_cb, zone);
}

static void
_zone_screen_copy_cb(void *data, Eina_Bool success)
{
   Zone *zone = data;

   if (!success)
     {
        evas_object_del(zone->zoom.image);
        zone->zoom.image = NULL;
        evas_object_del(zone->zoom.frame);
        zone->zoom.frame = NULL;
        zone->zoom.ready = EINA_FALSE;
        elm_check_state_set(zone->gui.zoom, EINA_FALSE);
       return;
     }

   zone->zoom.ready = EINA_TRUE;
   zone->last_mouse.x = -1;
   zone->last_mouse.y = -1;
   _event_mouse_tracker(zone);
   evas_object_show(zone->zoom.frame);
   elm_check_state_set(zone->gui.zoom, EINA_TRUE);
}

static void
_toggle_visibility_cb(void *data EINA_UNUSED, const char *keyname EINA_UNUSED)
{
   visible = !visible;
   platform_funcs->windows_visibility_set(visible);
}

static void
_zone_win_key_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event_info)
{
   Zone *zone = data;
   const Evas_Event_Key_Down *ev = event_info;
   const char *keyname = ev->keyname;
   Eina_Bool control = evas_key_modifier_is_set(ev->modifiers, "Control");
   Eina_Bool shift = evas_key_modifier_is_set(ev->modifiers, "Shift");

   if (zone->screenshot.ee) return;

   if (strcmp(keyname, "Escape") == 0)
     {
        DBG("User requested exit!");
        elm_exit();
     }
   else if ((strcmp(keyname, "F1") == 0)  ||
            (strcmp(keyname, "h") == 0))
     show_gui_help(zone);
   else if (strcmp(keyname, "p") == 0)
     print_measurements();
   else if (strcmp(keyname, "c") == 0)
     {
        if (control)
          {
             zone_rulers_clear(zone);
             zone_ruler_create(zone);
          }
        else
          {
             if (zone->last_ruler_used)
               zone_ruler_create(zone);
             else
               DBG("Last ruler wasn't used, do not create a new one.");
          }
     }
   else if (strcmp(keyname, "g") == 0)
     {
        show_guides = !show_guides;
        show_guides_apply();
     }
   else if (strcmp(keyname, "d") == 0)
     {
        show_distances = !show_distances;
        show_distances_apply();
     }
   else if (strcmp(keyname, "t") == 0)
     {
        Evas_Object *ruler = zone_ruler_last_get(zone);
        Ruler_Data *rd = ruler_data_get(ruler);
        rd->type = (rd->type + 1) % N_RULER_TYPES;
        ruler_type_apply(ruler);
     }
   else if (strcmp(keyname, "z") == 0)
     {
        Eina_Bool state;
        if (zone->zoom.image)
          {
             evas_object_del(zone->zoom.image);
             zone->zoom.image = NULL;
             evas_object_del(zone->zoom.frame);
             zone->zoom.frame = NULL;
             zone->zoom.ready = EINA_FALSE;
             state = EINA_FALSE;
          }
        else
          {
             zone_zoom_pre_setup(zone);
             platform_funcs->zone_screen_copy(zone, zone->zoom.image,
                                              _zone_screen_copy_cb,
                                              zone);
             state = EINA_TRUE;
          }

        elm_check_state_set(zone->gui.zoom, state);
     }
   else if (strcmp(keyname, "x") == 0)
     hex_colors = !hex_colors;
   else if (strcmp(keyname, "space") == 0)
     {
        if (!zone->handling)
          {
             zone->keyboard_move = EINA_TRUE;
             _handling_start(zone);
          }
        else
          {
             zone->keyboard_move = EINA_FALSE;
             _handling_stop(zone);
          }
     }
   else if (strcmp(keyname, "Left") == 0)
     {
        int d = shift ? 10 : 1;
        if ((zone->last_ruler_used) && (!zone->keyboard_move))
          {
             Evas_Object *ruler = zone_ruler_last_get(zone);
             if (!control)
               ruler_move_relative(ruler, -d, 0);
             else
               ruler_resize_relative(ruler, -d, 0);
          }
        else
          platform_funcs->mouse_move_by(zone, -d, 0);
     }
   else if (strcmp(keyname, "Right") == 0)
     {
        int d = shift ? 10 : 1;
        if ((zone->last_ruler_used) && (!zone->keyboard_move))
          {
             Evas_Object *ruler = zone_ruler_last_get(zone);
             if (!control)
               ruler_move_relative(ruler, d, 0);
             else
               ruler_resize_relative(ruler, d, 0);
          }
        else
          platform_funcs->mouse_move_by(zone, d, 0);
     }
   else if (strcmp(keyname, "Up") == 0)
     {
        int d = shift ? 10 : 1;
        if ((zone->last_ruler_used) && (!zone->keyboard_move))
          {
             Evas_Object *ruler = zone_ruler_last_get(zone);
             if (!control)
               ruler_move_relative(ruler, 0, -d);
             else
               ruler_resize_relative(ruler, 0, -d);
          }
        else
          platform_funcs->mouse_move_by(zone, 0, -d);
     }
   else if (strcmp(keyname, "Down") == 0)
     {
        int d = shift ? 10 : 1;
        if ((zone->last_ruler_used) && (!zone->keyboard_move))
          {
             Evas_Object *ruler = zone_ruler_last_get(zone);
             if (!control)
               ruler_move_relative(ruler, 0, d);
             else
               ruler_resize_relative(ruler, 0, d);
          }
        else
          platform_funcs->mouse_move_by(zone, 0, d);
     }
   else if ((ev->string) && (strcmp(ev->string, "@") == 0))
     create_ruler_from_cmdbox(zone);
   else if (strcmp(keyname, "s") == 0)
     create_screenshot(zone);
   else
     DBG("Unhandled key %s, string: %s", keyname, ev->string);

}

typedef struct _Ruler_Spec Ruler_Spec;
struct _Ruler_Spec
{
   int x, y, w, h;
   enum ruler_type type;
};

static Eina_Bool
_parse_cmdline_ruler(const Ecore_Getopt *parser EINA_UNUSED, const Ecore_Getopt_Desc *desc EINA_UNUSED, const char *str, void *data EINA_UNUSED, Ecore_Getopt_Value *storage)
{
   int x, y, w, h, n;
   char *style = NULL;
   Eina_List **p_rulers_specs = (Eina_List **)storage->ptrp;
   Ruler_Spec *rs;

   n = sscanf(str, "%d:%d:%d:%d:%ms", &x, &y, &w, &h, &style);
   if (n < 4)
     {
        ERR("Invalid ruler format '%s', expected 'x:y:w:h[:style]'", str);
        return EINA_FALSE;
     }

   rs = malloc(sizeof(Ruler_Spec));
   EINA_SAFETY_ON_NULL_RETURN_VAL(rs, EINA_FALSE);

   rs->x = x;
   rs->y = y;
   rs->w = w;
   rs->h = h;
   rs->type = RULER_TYPE_SENTINEL;

   if (style)
     {
        int i;

        for (i = 0; ruler_type_names_strs[i] != NULL; i++)
          {
             if (strcasecmp(ruler_type_names_strs[i], style) == 0)
               rs->type = i;
          }
        free(style);
     }

   DBG("command line ruler: %dx%d at %d,%d type=%s",
       rs->w, rs->h, rs->x, rs->y, ruler_type_names_strs[rs->type]);

   *p_rulers_specs = eina_list_append(*p_rulers_specs, rs);

   return EINA_TRUE;
}

static void
_zone_win_resize_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   evas_object_geometry_get(zone->win, &zone->x, &zone->y, &zone->w, &zone->h);
   zone->last_mouse.x = -1;
   zone->last_mouse.y = -1;
   _event_mouse_tracker(zone);
}

static void
zone_del(Zone *zone)
{
   if (zone->tracker)
     ecore_animator_del(zone->tracker);

   /* all objects are deleted when canvas goes away.
    * when ruler and distance objects are deleted, they remove themselves from
    * their lists.
    */

   free(zone);
   zones = eina_list_remove(zones, zone);
}

static void
_zone_win_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Zone *zone = data;
   zone_del(zone);
}

static Eina_Bool
zone_create(int idx, int x, int y, int w, int h)
{
   Zone *zone = calloc(1, sizeof(Zone));
   EINA_SAFETY_ON_NULL_RETURN_VAL(zone, EINA_FALSE);

   zone->idx = idx;
   zone->x = x;
   zone->y = y;
   zone->w = w;
   zone->h = h;
   zone->last_mouse.x = -1;
   zone->last_mouse.y = -1;

   zone->win = elm_win_add(NULL, "eruler", ELM_WIN_UTILITY);
   EINA_SAFETY_ON_NULL_RETURN_VAL(zone->win, EINA_FALSE);

   DBG("created zone #%d (%d, %d, %d, %d) as win=%p",
       idx, x, y, w, h, zone->win);

   zone->evas = evas_object_evas_get(zone->win);

   elm_win_title_set(zone->win, "ERuler");
   elm_win_autodel_set(zone->win, EINA_TRUE);
   elm_win_layer_set(zone->win, 1000);
   elm_win_alpha_set(zone->win, EINA_TRUE);
   evas_object_move(zone->win, x, y);
   evas_object_resize(zone->win, w, h);

   evas_object_event_callback_add(zone->win,
                                  EVAS_CALLBACK_RESIZE,
                                  _zone_win_resize_cb, zone);
   evas_object_event_callback_add(zone->win,
                                  EVAS_CALLBACK_KEY_DOWN,
                                  _zone_win_key_down_cb, zone);
   evas_object_event_callback_add(zone->win,
                                  EVAS_CALLBACK_DEL,
                                  _zone_win_del_cb, zone);

   zone_hint_setup(zone);
   zone_ruler_setup(zone);
   zone_zoom_pre_setup(zone);
   zone_gui_setup(zone);

   zones = eina_list_append(zones, zone);

   return EINA_TRUE;
}

static Eina_Bool
_list_ruler_themes(const Ecore_Getopt *parser EINA_UNUSED, const Ecore_Getopt_Desc *desc EINA_UNUSED, const char *str EINA_UNUSED, void *data EINA_UNUSED, Ecore_Getopt_Value *storage)
{
   const char **itr;

   puts("Ruler types:");
   for (itr = ruler_type_names_strs; *itr != NULL; itr++)
     printf("\t%s\n", *itr);

   *storage->boolp = EINA_TRUE;

   return EINA_TRUE;
}

static const Ecore_Getopt options = {
   PACKAGE_NAME,
   "%prog [options]",
   PACKAGE_VERSION,
   "(C) 2013 Enlightenment Project",
   "GPL-2",
   "On-Screen Ruler and Measurement Tools.",
   EINA_TRUE,
   {
     ECORE_GETOPT_STORE_FALSE('D', "hide-distances",
                              "Start with distances hidden."),
     ECORE_GETOPT_STORE_FALSE('G', "hide-guides",
                              "Start with guides hidden."),
     ECORE_GETOPT_STORE_TRUE('x', "hexa-colors",
                             "Start with colors in hexadecimal format."),
     ECORE_GETOPT_CALLBACK_ARGS('r', "ruler",
                                "Define a ruler using format "
                                "'x:y:w:h[:style]'",
                                "x:y:w:h[:style]",
                                _parse_cmdline_ruler, NULL),
     ECORE_GETOPT_CALLBACK_NOARGS('T', "list-ruler-themes", "List ruler themes",
                                  _list_ruler_themes, NULL),
     ECORE_GETOPT_CHOICE('t', "ruler-theme",
                         "Specify initial theme for all new rulers",
                         ruler_type_names_strs),
     ECORE_GETOPT_VERSION('V', "version"),
     ECORE_GETOPT_COPYRIGHT('C', "copyright"),
     ECORE_GETOPT_LICENSE('L', "license"),
     ECORE_GETOPT_HELP('h', "help"),
     ECORE_GETOPT_SENTINEL
   }
};

int
elm_main(int argc, char **argv)
{
   int args;
   Eina_List *rulers_specs = NULL, *unused_rulers_specs;
   Ruler_Spec *rs = NULL;
   char *initial_ruler_type_str = NULL;
   Eina_Bool quit_option = EINA_FALSE;
   Ecore_Getopt_Value values[] = {
     ECORE_GETOPT_VALUE_BOOL(show_distances),
     ECORE_GETOPT_VALUE_BOOL(show_guides),
     ECORE_GETOPT_VALUE_BOOL(hex_colors),
     ECORE_GETOPT_VALUE_PTR_CAST(rulers_specs),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_STR(initial_ruler_type_str),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_NONE
   };
   const Eina_List *lm, *lz;
   Zone *zone;
   Evas_Object *tmp;
   int i, count;

   _log_dom = eina_log_domain_register("eruler", EINA_COLOR_CYAN);
   if (_log_dom < 0)
     {
        EINA_LOG_CRIT("Couldn't register log domain: 'eruler'");
        elm_shutdown();
        return EXIT_FAILURE;
     }

   args = ecore_getopt_parse(&options, values, argc, argv);
   if (args < 0)
     {
        ERR("Could not parse command line options.");
        retval = EXIT_FAILURE;
        goto end;
     }

   if (quit_option) goto end;

   if (initial_ruler_type_str)
     {
        for (i = 0; ruler_type_names_strs[i] != NULL; i++)
          if (strcmp(ruler_type_names_strs[i], initial_ruler_type_str) == 0)
            {
               initial_ruler_type = i;
               break;
            }
     }

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_info_set(elm_main, "eruler", "themes/default.edj");

   snprintf(theme_file, sizeof(theme_file), "%s/themes/default.edj",
            elm_app_data_dir_get());

   tmp = elm_win_add(NULL, "eruler", ELM_WIN_BASIC);
   if (0) {}
#ifdef HAVE_ECORE_X
   else if (elm_win_xwindow_get(tmp))
     platform_funcs = platform_funcs_x_get();
#endif
   else
     {
        CRI("Window platform not supported.");
        retval = EXIT_FAILURE;
        goto end;
     }

   if (!platform_funcs->pre_setup())
     {
        CRI("Could not pre-setup platform.");
        retval = EXIT_FAILURE;
        goto end;
     }

   count = platform_funcs->zones_count();
   for (i = 0; i < count; i++)
     {
        int x, y, w, h;
        if (!platform_funcs->zone_geometry_get(i, &x, &y, &w, &h))
          CRI("Could not get Zone #%d geometry.", i);
        else if (!zone_create(i, x, y, w, h))
          CRI("Could not create zone #%d (%d, %d, %d, %d).",
              i, x, y, w, h);
     }

   if (!zones)
     {
        show_gui_error("Couldn't create any zone!");
        retval = EXIT_FAILURE;
     }
   else
     {
        Eina_List *l;
        Zone *zone;
        EINA_LIST_FOREACH(zones, l, zone)
          {
             if (!platform_funcs->zone_setup(zone))
               CRI("Could not setup Zone #%d", zone->idx);
          }
     }


   if (!platform_funcs->post_setup())
     {
        show_gui_error("Platform couldn't do post_setup()");
        retval = EXIT_FAILURE;
     }

   unused_rulers_specs = eina_list_clone(rulers_specs);
   EINA_LIST_FOREACH(zones, lz, zone)
     {
        Eina_Bool did_ruler_spec = EINA_FALSE;
        int zx1, zy1, zx2, zy2;

        zx1 = zone->x;
        zy1 = zone->y;

        zx2 = zone->x + zone->w;
        zy2 = zone->y + zone->h;

        EINA_LIST_FOREACH(rulers_specs, lm, rs)
          {
             Evas_Object *ruler;
             Ruler_Data *rd;
             int mx, my;

             mx = rs->x - zx1;
             my = rs->y - zy1;

             if ((mx < 0) || (rs->x + rs->w > zx2) ||
                 (my < 0) || (rs->y + rs->h > zy2))
               continue;

             if (zone->last_ruler_used)
               zone_ruler_create(zone);
             ruler = zone_ruler_last_get(zone);
             rd = ruler_data_get(ruler);
             rd->type = rs->type == RULER_TYPE_SENTINEL ?
               initial_ruler_type : rs->type;
             rd->start.x = mx;
             rd->start.y = my;
             rd->stop.x = mx + rs->w - 1;
             rd->stop.y = my + rs->h - 1;
             ruler_type_apply(ruler);
             evas_object_show(ruler);
             zone_last_ruler_used_set(zone, EINA_TRUE);
             unused_rulers_specs = eina_list_remove(unused_rulers_specs, rs);
             did_ruler_spec = EINA_TRUE;
          }

        if (did_ruler_spec)
          zone_ruler_create(zone);
     }

   EINA_LIST_FREE(unused_rulers_specs, rs)
     ERR("Unused ruler spec %d,%d + %dx%d (type=%s) not fully inside a zone!",
         rs->x, rs->y, rs->w, rs->h, ruler_type_names_strs[rs->type]);

   EINA_LIST_FREE(rulers_specs, rs)
     free(rs);

   evas_object_del(tmp);

   if (retval != EXIT_SUCCESS)
     {
        while (zones)
          {
             Zone *zone = zones->data;
             evas_object_del(zone->win);
          }
     }
   else
     {
        platform_funcs->global_key_grab("F2", _toggle_visibility_cb, NULL);
     }

   elm_run();

   platform_funcs->pre_teardown();

 end:
   eina_log_domain_unregister(_log_dom);
   elm_shutdown();
   return retval;
}

static void
_error_quit_cb(void *data, Evas_Object *o EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Object *popup = data;
   evas_object_del(popup);
   DBG("Error dialog deleted, exit!");
   elm_exit();
}

void
show_gui_error(const char *message)
{
   Evas_Object *win, *bx, *help, *sep, *bt;

   CRI("show gui for error '%s'", message);

   win = elm_win_util_standard_add("eruler-error", "ERuler - Error!");

   bx = elm_box_add(win);
   elm_box_horizontal_set(bx, EINA_FALSE);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(bx);
   elm_win_resize_object_add(win, bx);

   help = elm_entry_add(bx);
   elm_entry_editable_set(help, EINA_FALSE);
#if (ELM_VERSION_MAJOR >= 1) && (ELM_VERSION_MINOR >= 8)
   elm_scroller_policy_set(help, ELM_SCROLLER_POLICY_OFF,
                           ELM_SCROLLER_POLICY_AUTO);
#else
   elm_entry_scrollbar_policy_set(help, ELM_SCROLLER_POLICY_OFF,
                                  ELM_SCROLLER_POLICY_AUTO);
#endif
   evas_object_size_hint_weight_set(help, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(help, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(help, message);
   evas_object_show(help);
   elm_box_pack_end(bx, help);

   sep = elm_separator_add(bx);
   evas_object_size_hint_weight_set(sep, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(sep, EVAS_HINT_FILL, 0.5);
   evas_object_show(sep);
   elm_box_pack_end(bx, sep);

   bt = elm_button_add(bx);
   elm_object_text_set(bt, "Quit");
   evas_object_smart_callback_add(bt, "clicked", _error_quit_cb, win);
   evas_object_show(bt);
   elm_box_pack_end(bx, bt);

   evas_object_resize(win, 320, 100);

   evas_object_show(win);
   elm_win_activate(win);

   retval = EXIT_FAILURE;
}

Evas_Object *
zone_win_get(const Zone *zone)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(zone, NULL);
   return zone->win;
}

Evas_Object *
zone_screen_copy_object_get(const Zone *zone)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(zone, NULL);
   return zone->zoom.image;
}

void
zone_screen_copy_ready_set(Zone *zone, Eina_Bool ready)
{
   EINA_SAFETY_ON_NULL_RETURN(zone);
   zone->zoom.ready = !!ready;

   if (!zone->zoom.frame) return;
   if (ready)
     evas_object_show(zone->zoom.frame);
   else
     evas_object_hide(zone->zoom.frame);
}

void
zone_geometry_get(const Zone *zone, int *x, int *y, int *w, int *h)
{
   if (x) *x = 0;
   if (y) *y = 0;
   if (w) *w = 0;
   if (h) *h = 0;
   EINA_SAFETY_ON_NULL_RETURN(zone);
   if (x) *x = zone->x;
   if (y) *y = zone->y;
   if (w) *w = zone->w;
   if (h) *h = zone->h;
}

int
zone_index_get(const Zone *zone)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(zone, -1);
   return zone->idx;
}

ELM_MAIN();
