#include "private.h"

#include <Elementary.h>
#include "config.h"
#include "termio.h"
#include "options.h"
#include "options_wallpaper.h"

void
options_keys(Evas_Object *opbox, Evas_Object *term EINA_UNUSED)
{
   Evas_Object *o, *fr, *li, *lbl;

   fr = o = elm_frame_add(opbox);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_text_set(o, "Key Bindings");
   elm_box_pack_end(opbox, o);
   evas_object_show(o);

   li = elm_list_add(o);
   elm_list_mode_set(li, ELM_LIST_LIMIT);
   evas_object_size_hint_weight_set(li, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(li, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(fr, li);

#define KB(_action, _keys) do {                               \
   lbl = elm_label_add(li);                                   \
   elm_object_text_set(lbl, _keys);                           \
   elm_list_item_append(li, _action, NULL, lbl, NULL, NULL);  \
} while (0)

   KB("Scroll one page up", "Shift + PgUp");
   KB("Scroll one page down", "Shift + PgDn");
   KB("Paste Clipboard (ctrl+v/c) selection", "Shift + Insert");
   KB("Paste Clipboard (ctrl+v/c) selection", "Ctrl + Shift + v");
   KB("Paste Primary (highlight) selection", "Shift + Ctrl + Insert");
   KB("Paste Primary (highlight) selection", "Alt + Return");
   KB("Copy current selection to clipboard", "Ctrl + Shift + c");
   KB("Copy current selection to clipboard", "Shift+Keypad-Divide");
   KB("Font size up 1", "Shift+Keypad-Plus");
   KB("Font size down 1", "Shift+Keypad-Minus");
   KB("Reset font size to 10", "Shift+Keypad-Multiply");
   KB("Split horizontally (new below)", "Ctrl + Shift + PgUp");
   KB("Split vertically (new on right)", "Ctrl + Shift + PgDn");
   KB("Focus to previous terminal", "Ctrl + PgUp");
   KB("Focus to next terminal", "Ctrl + PgDn");
   KB("Create new \"tab\"", "Ctrl + Shift + t");
   KB("Bring up \"tab\" switcher", "Ctrl + Shift + Home");
   KB("Switch to terminal tab 1", "Ctrl + 1");
   KB("Switch to terminal tab 2", "Ctrl + 2");
   KB("Switch to terminal tab 3", "Ctrl + 3");
   KB("Switch to terminal tab 4", "Ctrl + 4");
   KB("Switch to terminal tab 5", "Ctrl + 5");
   KB("Switch to terminal tab 6", "Ctrl + 6");
   KB("Switch to terminal tab 7", "Ctrl + 7");
   KB("Switch to terminal tab 8", "Ctrl + 8");
   KB("Switch to terminal tab 9", "Ctrl + 9");
   KB("Switch to terminal tab 10", "Ctrl + 0");
   KB("Enter command mode", "Alt + Home");
#undef KB
}
