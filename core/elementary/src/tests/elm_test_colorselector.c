#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_suite.h"

START_TEST (elm_colorselector_palette)
{
   Evas_Object *win, *c;
   unsigned int palette_cnt;

   elm_init(1, NULL);
   win = elm_win_add(NULL, "check", ELM_WIN_BASIC);

   c = elm_colorselector_add(win);
   /* Get the count of default palettes */
   palette_cnt = eina_list_count(elm_colorselector_palette_items_get(c));
   evas_object_del(c);

   c = elm_colorselector_add(win);
   ck_assert(eina_list_count(elm_colorselector_palette_items_get(c)) == palette_cnt);
   elm_colorselector_palette_color_add(c, 255, 255, 255, 255);
   ck_assert(eina_list_count(elm_colorselector_palette_items_get(c)) == 1);
   evas_object_del(c);

   c = elm_colorselector_add(win);
   ck_assert(eina_list_count(elm_colorselector_palette_items_get(c)) == palette_cnt);
   evas_object_del(c);

   elm_shutdown();
}
END_TEST

void elm_test_colorselector(TCase *tc)
{
   tcase_add_test(tc, elm_colorselector_palette);
}
