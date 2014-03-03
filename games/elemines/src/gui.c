/*
 * elemines: an EFL minesweeper
 * Copyright (C) 2012-2014 Jerome Pinot <ngc891@gmail.com> and various
 * contributors (see AUTHORS).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "elemines.h"

static double pause_time = 0;

static void
_quit(void *data __UNUSED__, Evas_Object *obj __UNUSED__,
      void *event_info __UNUSED__)
{
   etrophy_shutdown();
   elm_theme_extension_del(NULL, game.edje_file);
   elm_theme_flush(NULL);
   elm_exit();
}

static void
_popup_del(void *data __UNUSED__, Evas_Object *obj __UNUSED__,
           void *event_info __UNUSED__)
{
   evas_object_hide(game.ui.popup);
}

static void
_show_score(void *data __UNUSED__, Evas_Object *obj __UNUSED__,
            void *event_info __UNUSED__)
{
   Evas_Object *icon, *button, *leaderboard;

   /* set icon and title of the popup */
   game.ui.popup = elm_popup_add(game.ui.window);
   evas_object_size_hint_weight_set(game.ui.popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_part_text_set(game.ui.popup, "title,text", _("High Scores"));
   icon = elm_icon_add(game.ui.popup);
   elm_icon_standard_set(icon, "score");
   elm_object_part_content_set(game.ui.popup, "title,icon", icon);

   /* we use the default layout from etrophy library */
   leaderboard = etrophy_score_layout_add(game.ui.popup,
                                          game.trophy.gamescore);
   elm_object_content_set(game.ui.popup, leaderboard);

   button = elm_button_add(game.ui.popup);
   elm_object_text_set(button, "OK");
   elm_object_part_content_set(game.ui.popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked", _popup_del, NULL);
   evas_object_show(game.ui.popup);
}

static void
_config(void *data __UNUSED__, Evas_Object *obj __UNUSED__,
        void *event_info __UNUSED__)
{
   int number;
   Evas_Object *spin = data;

   /* we get back the mine number from user and init again the game */
   number = elm_spinner_value_get(spin);
   if ( (number < 2) || (number > (game.datas.x_theme * game.datas.y_theme)) )
       number = game.datas.mines_theme;

   game.datas.mines_total = number;
   evas_object_hide(game.ui.popup);
   init(NULL, NULL, NULL);
}

static void
_show_config(void *data __UNUSED__, Evas_Object *obj __UNUSED__,
             void *event_info __UNUSED__)
{
   Evas_Object *icon, *vbox, *spin, *label, *bt1, *bt2;
   char buffer[512];

   /* set icon and title of the popup */
   game.ui.popup = elm_popup_add(game.ui.window);
   evas_object_size_hint_weight_set(game.ui.popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   elm_object_part_text_set(game.ui.popup, "title,text", _("Configuration"));
   icon = elm_icon_add(game.ui.popup);
   elm_icon_standard_set(icon, "config");
   elm_object_part_content_set(game.ui.popup, "title,icon", icon);

   vbox = elm_box_add(game.ui.window);
   elm_box_homogeneous_set(vbox, EINA_FALSE);
   evas_object_size_hint_weight_set(vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(vbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(vbox);

   /* spinner to change mine number */
   spin = elm_spinner_add(game.ui.window);
   elm_spinner_label_format_set(spin, _("%.0f mines"));
   elm_spinner_min_max_set(spin, 2, game.datas.x_theme * game.datas.y_theme - 1);
   elm_spinner_value_set(spin, game.datas.mines_total);
   evas_object_size_hint_weight_set(spin, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(spin, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(spin);
   elm_box_pack_end(vbox, spin);

   /* Add some comments about scoring */
   label = elm_label_add(game.ui.window);
   elm_label_line_wrap_set(label, ELM_WRAP_WORD);
   snprintf(buffer, sizeof(buffer), _("<b>Note:</b> default mine number is "
            "<b>%d</b> with scoring in <b>%s</b> category. If you change "
            "the mine number to something else, your score will be put in the "
            "<b>%s</b> category."), game.datas.mines_theme, STANDARD, CUSTOM);
   elm_object_text_set(label, buffer);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(label);
   elm_box_pack_end(vbox, label);

   elm_object_content_set(game.ui.popup, vbox);

   /* button for validating */
   bt1 = elm_button_add(game.ui.popup);
   elm_object_text_set(bt1, _("OK"));
   elm_object_part_content_set(game.ui.popup, "button1", bt1);
   evas_object_smart_callback_add(bt1, "clicked", _config, spin);

   /* button for cancelling */
   bt2 = elm_button_add(game.ui.popup);
   elm_object_text_set(bt2, _("Cancel"));
   elm_object_part_content_set(game.ui.popup, "button2", bt2);
   evas_object_smart_callback_add(bt2, "clicked", _popup_del, NULL);

   evas_object_show(game.ui.popup);

}

static void
_show_about(void *data __UNUSED__, Evas_Object *obj __UNUSED__,
            void *event_info __UNUSED__)
{
   Evas_Object *icon, *label, *button;
   char buffer[256];

   /* set icon and title of the popup */
   game.ui.popup = elm_popup_add(game.ui.window);
   elm_object_part_text_set(game.ui.popup, "title,text", _("About"));
   icon = elm_icon_add(game.ui.popup);
   elm_icon_standard_set(icon, "about");
   elm_object_part_content_set(game.ui.popup, "title,icon", icon);

   /* Construct a formatted label for the about popup */
   label = elm_label_add(game.ui.window);
   snprintf(buffer, sizeof(buffer), _("<b>%s %s</b><br><br>"
            "%s<br><br>"
            "Pictures derived from Battle For Wesnoth:<br>"
            "http://www.wesnoth.org/<br>"),
            PACKAGE, VERSION, COPYRIGHT);
   elm_object_text_set(label, buffer);
   evas_object_show(label);

   elm_object_content_set(game.ui.popup, label);

   button = elm_button_add(game.ui.popup);
   elm_object_text_set(button, _("OK"));
   elm_object_part_content_set(game.ui.popup, "button1", button);
   evas_object_smart_callback_add(button, "clicked", _popup_del, NULL);
   evas_object_show(game.ui.popup);
}

static void
_pause_del(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj,
           void *event_info __UNUSED__)
{
   /* compute the pause delay to remove it from timer */
   game.clock.delay += ecore_time_get() - pause_time;
   if (game.clock.etimer)
     {
        ecore_timer_thaw(game.clock.etimer);
     }
   else
     {
        game.clock.delay = 0;
     }
   evas_object_hide(obj);
}

static void
_pause(void *data __UNUSED__, Evas_Object *obj __UNUSED__,
       void *event_info __UNUSED__)
{
   Evas_Object *popup, *layout;

   /* Show the pause window */
   popup = elm_win_inwin_add(game.ui.window);
   evas_object_show(popup);

   /* pause the timer */
   pause_time = ecore_time_get();
   if (game.clock.etimer) ecore_timer_freeze(game.clock.etimer);

   /* Construct a formatted label for the inwin */
   layout = elm_layout_add(game.ui.window);
   elm_layout_file_set(layout, game.edje_file, "pause");
   evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(layout);
   elm_win_inwin_content_set(popup, layout);

   /* Close the inwin when clicked */
   evas_object_event_callback_add(popup, EVAS_CALLBACK_MOUSE_DOWN,
                                  _pause_del, NULL);
}

Eina_Bool
gui(char *theme, Eina_Bool fullscreen)
{
   Evas_Object *background, *vbox, *toolbar, *hbox, *conform;
   int x, y;
   char *str1, *str2 = NULL;
   char name[PATH_MAX];

   /* get the edje theme file */
   if (eina_str_has_suffix(theme, ".edj"))
     {
        eina_strlcpy(name, theme, sizeof(name));
     }
   else
     {
        snprintf(name, sizeof(name), "%s.edj", theme);
     }
   if (strchr(name, '/'))
     {
        eina_strlcpy(game.edje_file, name, sizeof(game.edje_file));
     }
   else
     {
        snprintf(game.edje_file, sizeof(game.edje_file), "%s/themes/%s",
                 elm_app_data_dir_get(), name);
     }

   elm_theme_extension_add(NULL, game.edje_file);

   /* get board size from theme */
   if (((str1 = edje_file_data_get(game.edje_file, "SIZE_X")) != NULL)
       && ((str2 = edje_file_data_get(game.edje_file, "SIZE_Y")) != NULL))
     {
        game.datas.x_theme = atoi(str1);
        game.datas.y_theme = atoi(str2);
        free(str1);
        free(str2);
     }
   else
     {
        EINA_LOG_CRIT("Loading theme error: can not read the SIZE_? value in %s", game.edje_file);
        if (str1) free(str1);
        if (str2) free(str2);
        return EINA_FALSE;
     }

   /* get default mines count from theme */
   if ((str1 = edje_file_data_get(game.edje_file, "MINES")) != NULL)
     {
        game.datas.mines_theme = atoi(str1);
        free(str1);
     }
   else
     {
        EINA_LOG_CRIT("Loading theme error: can not read the MINES value in %s", game.edje_file);
        if (str1) free(str1);
        return EINA_FALSE;
     }

   /* Validate input values */
   if (game.datas.mines_total == 0)
     game.datas.mines_total =  game.datas.mines_theme;
   if (game.datas.mines_total < 0)
     game.datas.mines_total = 1;
   if (game.datas.mines_total > (game.datas.x_theme * game.datas.y_theme - 1))
     game.datas.mines_total = game.datas.x_theme * game.datas.y_theme - 1;

   /* set general properties */
   game.ui.window = elm_win_add(NULL, PACKAGE, ELM_WIN_BASIC);
   elm_win_title_set(game.ui.window, PACKAGE);
   elm_win_autodel_set(game.ui.window, EINA_TRUE);
   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   /* init score system */
   etrophy_init();
   game.trophy.gamescore = etrophy_gamescore_load(PACKAGE);
   if (!game.trophy.gamescore)
     {
        game.trophy.gamescore = etrophy_gamescore_new(PACKAGE);
        game.trophy.level =  etrophy_level_new(STANDARD);
        etrophy_gamescore_level_add(game.trophy.gamescore, game.trophy.level);
        game.trophy.level =  etrophy_level_new(CUSTOM);
        etrophy_gamescore_level_add(game.trophy.gamescore, game.trophy.level);
     }

   /* add a background */
   background = elm_bg_add(game.ui.window);
   elm_bg_file_set(background, game.edje_file, "bg");
   evas_object_size_hint_weight_set(background, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   elm_win_resize_object_add(game.ui.window, background);
   evas_object_show(background);

   /* main box */
   vbox = elm_box_add(game.ui.window);
   elm_box_homogeneous_set(vbox, EINA_FALSE);
   elm_win_resize_object_add(game.ui.window, vbox);
   evas_object_size_hint_weight_set(vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(vbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(vbox);

   /* the toolbar */
   toolbar = elm_toolbar_add(game.ui.window);
   elm_toolbar_shrink_mode_set(toolbar, ELM_TOOLBAR_SHRINK_SCROLL);
   elm_toolbar_select_mode_set(toolbar, ELM_OBJECT_SELECT_MODE_ALWAYS);
   evas_object_size_hint_weight_set(toolbar, 0.0, 0.0);
   evas_object_size_hint_align_set(toolbar, EVAS_HINT_FILL, 0.0);
   evas_object_show(toolbar);
   elm_box_pack_end(vbox, toolbar);

   /* box for timer and mine count */
   hbox = elm_box_add(game.ui.window);
   elm_box_homogeneous_set(hbox, EINA_FALSE);
   elm_box_horizontal_set(hbox, EINA_TRUE);
   evas_object_size_hint_weight_set(hbox, EVAS_HINT_EXPAND, 0.1);
   evas_object_size_hint_align_set(hbox, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(hbox);

   /* timer */
   game.ui.timer = elm_layout_add(game.ui.window);
   elm_layout_file_set(game.ui.timer, game.edje_file, "timer");
   evas_object_size_hint_weight_set(game.ui.timer, 0.5, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(game.ui.timer, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   evas_object_show(game.ui.timer);
   elm_box_pack_end(hbox, game.ui.timer);

   /* remaining mines */
   game.ui.mines = elm_layout_add(game.ui.window);
   elm_layout_file_set(game.ui.mines, game.edje_file, "mines");
   evas_object_size_hint_weight_set(game.ui.mines, 0.5, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(game.ui.mines, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   evas_object_show(game.ui.mines);
   elm_box_pack_end(hbox, game.ui.mines);
   elm_box_pack_end(vbox, hbox);

   /* add the main table for storing cells */
   game.ui.table = elm_layout_add(game.ui.window);
   elm_layout_file_set(game.ui.table, game.edje_file, "board");
   evas_object_size_hint_weight_set(game.ui.table, EVAS_HINT_EXPAND,
                                    EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(game.ui.table, EVAS_HINT_FILL,
                                   EVAS_HINT_FILL);
   elm_box_pack_end(vbox, game.ui.table);
   evas_object_show(game.ui.table);

   if (fullscreen == EINA_TRUE)
     {
        /* use conformant */
        elm_win_conformant_set(game.ui.window, EINA_TRUE);
        elm_win_fullscreen_set(game.ui.window, EINA_TRUE);
        evas_object_move(game.ui.window, 0, 0);
        conform = elm_conformant_add(game.ui.window);
        evas_object_size_hint_weight_set(conform, EVAS_HINT_EXPAND,
                                                  EVAS_HINT_EXPAND);
        evas_object_show(conform);
        elm_object_content_set(conform, vbox);
     }
   else
     {
        /* Get window's size from edje and resize it */
        x = atoi(str1 = edje_file_data_get(game.edje_file, "width"));
        y = atoi(str2 = edje_file_data_get(game.edje_file, "height"));
        free(str1);
        free(str2);
        evas_object_resize(game.ui.window, x, y);
     }

   /* Add item to the toolbar now so everything is initialized in the UI */
   elm_toolbar_item_append(toolbar, "refresh", _("Refresh"), init, NULL);
   elm_toolbar_item_append(toolbar, "pause", _("Pause"), _pause, NULL);
   elm_toolbar_item_append(toolbar, "config", _("Configuration"),
                           _show_config, NULL);
   elm_toolbar_item_append(toolbar, "score", _("Scores"), _show_score, NULL);
   elm_toolbar_item_append(toolbar, "about", _("About"), _show_about, NULL);
   elm_toolbar_item_append(toolbar, "quit", _("Quit"), _quit, NULL);

   elm_object_focus_set(game.ui.window, EINA_TRUE);
   evas_object_show(game.ui.window);

   return EINA_TRUE;
}

/* vim: set ts=8 sw=3 sts=3 expandtab cino=>5n-3f0^-2{2(0W1st0 : */
