#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Elementary.h>

#include "../mess_header.h"

static void *done_data;
static void (*done_cb)(void *data);

static void
_discard(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Ecrire_Entry *ent = done_data;

   evas_object_del(data);
   done_cb(ent);
}

static void
_save(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Ecrire_Entry *ent = done_data;

   evas_object_del(data);
   editor_save(ent);
   done_cb(done_data);
}

static void
_cancel(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   evas_object_del(data);
}

void
ui_alert_need_saving(Evas_Object *entry, void (*done)(void *data), void *data)
{
   Ecrire_Entry *ent = data;
   Evas_Object *popup, *bx, *hbx, *btn, *lbl;
   popup = elm_popup_add(elm_object_top_widget_get(entry));
   evas_object_show(popup);

   done_cb = done;
   done_data = data;

   bx = elm_box_add(popup);
   elm_object_content_set(popup, bx);
   evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(bx, EVAS_HINT_FILL, 0.0);
   evas_object_show(bx);

   lbl = elm_label_add(popup);
   elm_object_text_set(lbl,
         _("<align=center>Would you like to save changes to document?<br>"
         "Any unsaved changes will be lost."));
   evas_object_size_hint_weight_set(lbl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(lbl, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_pack_end(bx, lbl);
   evas_object_show(lbl);

   hbx = elm_box_add(popup);
   elm_box_horizontal_set(hbx, EINA_TRUE);
   evas_object_size_hint_weight_set(hbx, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(hbx, EVAS_HINT_FILL, 0.0);
   elm_box_pack_end(bx, hbx);
   evas_object_show(hbx);

   btn = elm_button_add(popup);
   elm_object_text_set(btn, _("Save"));
   elm_box_pack_end(hbx, btn);
   evas_object_show(btn);
   evas_object_smart_callback_add(btn, "clicked", _save, popup);

   if (elm_object_item_disabled_get(ent->save_item) || !ent->filename)
     {
        elm_object_disabled_set(btn, EINA_TRUE);
     }

   btn = elm_button_add(popup);
   elm_object_text_set(btn, _("Discard"));
   elm_box_pack_end(hbx, btn);
   evas_object_show(btn);
   evas_object_smart_callback_add(btn, "clicked", _discard, popup);

   btn = elm_button_add(popup);
   elm_object_text_set(btn, _("Cancel"));
   elm_box_pack_end(hbx, btn);
   evas_object_show(btn);
   evas_object_smart_callback_add(btn, "clicked", _cancel, popup);
}
