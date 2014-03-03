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

static Eina_Bool
_generate(void)
{
   Elemines_Walker *walker;
   Eina_Iterator *it;
   int i, x, y;

   /* allocate  the matrix */
   matrix = (Elemines_Cell**)malloc((game.datas.x_theme + 2) * sizeof(Elemines_Cell*));
   if (!matrix) return EINA_FALSE;

   for(i = 0; i < game.datas.x_theme + 2; i++)
     {
        matrix[i] = (Elemines_Cell*)malloc((game.datas.y_theme + 2) * sizeof(Elemines_Cell));
        if (!matrix[i]) return EINA_FALSE;
     }

   /* fill the matrix with 0 */
   it = _walk(0, 0, game.datas.x_theme + 2, game.datas.y_theme + 2);
   EINA_ITERATOR_FOREACH(it, walker)
     {
        walker->cell->neighbours = 0;
        walker->cell->mine = 0;
        walker->cell->flag = 0;
        walker->cell->uncover = 0;
     }
   eina_iterator_free(it);

   /* 1st table: the mines */
   srand(time(NULL));
   for (i = 0; i < game.datas.mines_total; i++)
     {
        /* random coordinates */
        x = (int)((double)game.datas.x_theme * rand() / RAND_MAX + 1);
        y = (int)((double)game.datas.y_theme * rand() / RAND_MAX + 1);

        if (matrix[x][y].mine == 0 )
          matrix[x][y].mine = 1;
        else /* if there is already a bomb here, try again */
          i--;
     }

   /* 2nd table: neighbours */
   it = _walk(1, 1, game.datas.x_theme, game.datas.y_theme);
   EINA_ITERATOR_FOREACH(it, walker)
     {
        int neighbours = 9;

        /* mark a mine place with a 9 */
        if (!walker->cell->mine)
          {
             Elemines_Walker *walkerc;
             Eina_Iterator *itc;

             neighbours = 0;
             itc = _walk(walker->x - 1, walker->y - 1, 3, 3);
             EINA_ITERATOR_FOREACH(itc, walkerc)
               {
                  if (walkerc->cell->mine)
                    neighbours++;
               }
             eina_iterator_free(itc);
          }

        walker->cell->neighbours = neighbours;
     }
   eina_iterator_free(it);

   return EINA_TRUE;
}

static Eina_Bool
_board(void)
{
   Elemines_Walker *walker;
   Eina_Iterator *it;
   Evas_Object *edje;

   edje = elm_layout_edje_get(game.ui.table);
   elm_object_signal_emit(game.ui.table, "reset", "");

   edje_object_signal_callback_del_full(edje, "mouse,clicked,*", "board\\[*\\]:overlay", _click, NULL);
   edje_object_signal_callback_add(edje, "mouse,clicked,*", "board\\[*\\]:overlay", _click, NULL);

   /* prepare the board */
   it = _walk(1, 1, game.datas.x_theme, game.datas.y_theme);
   EINA_ITERATOR_FOREACH(it, walker)
     {
        char tmp[128];
        int scenery;

        sprintf(tmp, "%s:reset", walker->target);
        elm_object_signal_emit(game.ui.table, tmp, "");

        /* add some random scenery */
        scenery = (int)((double)100 * rand() / RAND_MAX + 1);
        if (scenery < 15)
          {
             sprintf(tmp, "%s:flowers", walker->target);
             elm_object_signal_emit(game.ui.table, tmp, "");
          }
        if ((scenery > 12) && (scenery < 18))
          {
             sprintf(tmp, "%s:mushrooms", walker->target);
             elm_object_signal_emit(game.ui.table, tmp, "");
          }
     }
   eina_iterator_free(it);

   return EINA_TRUE;
}

void
init(void *data __UNUSED__, Evas_Object *obj __UNUSED__,
     void *event_info __UNUSED__)
{
   int i;
   char str[8];

   /* init variables */
   game.clock.started = EINA_FALSE;
   game.clock.delay = 0;
   game.datas.remain = game.datas.mines_total;
   game.datas.counter = game.datas.x_theme * game.datas.y_theme - game.datas.mines_total;

   if (matrix)
     {
        for(i = 0; i < game.datas.y_theme + 2; i++)
          free((void *)matrix[i]);
        free((void *)matrix);
     }

   if (_generate() == EINA_FALSE) EINA_LOG_CRIT("Can not generate the data matrix");
   _board();

   /* reinit widgets if needed */
   if (game.ui.timer)
     elm_object_part_text_set(game.ui.timer, "time", "00:00.0");
   snprintf(str, sizeof(str), "%d/%d", game.datas.mines_total,
            game.datas.mines_total);
   if (game.ui.mines)
     elm_object_part_text_set(game.ui.mines, "mines", str);

}

/* vim: set ts=8 sw=3 sts=3 expandtab cino=>5n-3f0^-2{2(0W1st0 : */
