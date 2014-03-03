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

double t0;
double dt = 0.1;

static int
_scoring(void)
{
   int score;
   double end_time;
   char *user;

   /* compute score using time, board size and mines count */
   end_time = ecore_loop_time_get() - t0 - game.clock.delay;
   score = (game.datas.x_theme * game.datas.y_theme * game.datas.mines_total)
            - (10 * end_time);
   /* Don't be rude with bad players */
   if (score < 0) score = 0;

   /* get system username for name */
   user = getenv("USER");

   /* add the score */
   game.trophy.escore = etrophy_score_new(user, score);
   /* Level is Standard if using default board values */
   if (game.datas.mines_total == game.datas.mines_theme)
     {
        game.trophy.game_type = STANDARD;
     }
   else
     {
        game.trophy.game_type = CUSTOM;
     }

   game.trophy.level = etrophy_gamescore_level_get(game.trophy.gamescore,
                                                   game.trophy.game_type);
   etrophy_level_score_add(game.trophy.level, game.trophy.escore);
   etrophy_gamescore_save(game.trophy.gamescore, NULL);

   return score;

}

static Eina_Bool
_timer(void *data __UNUSED__)
{
   char str[8] = { 0 };
   int min = 0;
   double t;

   if (game.clock.started == EINA_FALSE)
     return EINA_FALSE;

   t = ecore_loop_time_get() - t0 - game.clock.delay;
   while (t >= 60)
     {
        t -= 60;
        min++;
     }

   snprintf(str, sizeof(str), "%02d:%04.1f", min, t);
   if (game.datas.counter != 0)
     elm_object_part_text_set(game.ui.timer, "time", str);

   return EINA_TRUE;
}

static void
_finish(const char *target, Eina_Bool win)
{
   Elemines_Walker *walker;
   Eina_Iterator *it;
   Evas_Object *edje;
   int score;
   char str[255];

   game.clock.started = EINA_FALSE;

   /* disable click */           
   edje = elm_layout_edje_get(game.ui.table);
   edje_object_signal_callback_del_full(edje, "mouse,clicked,*", "board\\[*\\]:overlay", _click, NULL);

   /* show bombs */
   it = _walk(1, 1, game.datas.x_theme, game.datas.y_theme);
   EINA_ITERATOR_FOREACH(it, walker)
     {
        if (win == EINA_TRUE)
          {
             sprintf(str, "%s:win", walker->target);
          }
        else
          {
             if (walker->cell->mine == 1)
               {
                  sprintf(str, "%s:bomb", walker->target);
                  elm_object_signal_emit(game.ui.table, str, "");
               }
             sprintf(str, "%s:lose", walker->target);
          }
        elm_object_signal_emit(game.ui.table, str, "");
     }
   eina_iterator_free(it);

   /* highlight the fatal bomb */
   if (win == EINA_FALSE)
     {
        sprintf(str, "%s:boom", target);
        elm_object_signal_emit(game.ui.table, str, "");
     }
   else
     {
        /* prepare the congratulation message */
        edje_object_signal_emit(edje, "congrat", "");

        score = _scoring();
        snprintf(str, sizeof(str), _("Score: %d"), score);
        elm_object_part_text_set(game.ui.table, "congrat:score", str);

        if (score >= etrophy_gamescore_level_hi_score_get(game.trophy.gamescore,
                                                          game.trophy.game_type))
          {
             elm_object_part_text_set(game.ui.table, "congrat:best score",
                                      _("High Score!!"));
          }
        else
          {
             elm_object_part_text_set(game.ui.table, "congrat:best score", "");
          }

        elm_object_signal_emit(game.ui.table, "congrat:you win", "");
	elm_object_part_text_set(game.ui.table, "congrat:you win", _("You win!"));
     }

   if (game.clock.etimer)
     {
        ecore_timer_del(game.clock.etimer);
        game.clock.etimer = NULL;
     }
}

static void
_clean_walk(const char *target, unsigned char x, unsigned char y)
{
   /* we are out of board */
   if (x == 0 || x == game.datas.x_theme + 1
       || y == 0 || y == game.datas.y_theme + 1)
     return;

   /* do nothing if the square is already uncovered */
   if (matrix[x][y].uncover == 1)
      return;

   /* flagged square can not be opened */
   if (matrix[x][y].flag == 1)
      return;

   /* no mine */
   if (matrix[x][y].mine == 0)
     {
        int scenery;
        char str[128];

        /* clean scenery */
        matrix[x][y].uncover = 1;

        /* add some stones */
        scenery = (int)((double)100 * rand() / RAND_MAX + 1);
        if (scenery < 15)
          {
             sprintf(str, "%s:stones", target);
             elm_object_signal_emit(game.ui.table, str, "");
          }

        sprintf(str, "%s:digg", target);
	elm_object_signal_emit(game.ui.table, str, "");

        /* at least 1 neighbour */
        if (matrix[x][y].neighbours != 0)
          {
             sprintf(str, "%s:%d", target, matrix[x][y].neighbours);
             elm_object_signal_emit(game.ui.table, str, "");
          }
        /* no neighbour */
        else
          {
             Elemines_Walker *walker;
             Eina_Iterator *it;

             it =  _walk(x - 1, y - 1, 3, 3);
             EINA_ITERATOR_FOREACH(it, walker)
               _clean_walk(walker->target, walker->x, walker->y);
             eina_iterator_free(it);
          }
        /* keep track of this empty spot */
        game.datas.counter--;
        if (game.datas.counter == 0)
          _finish(target, EINA_TRUE);
     }
   else /* BOOM! */
     {
        _finish(target, EINA_FALSE);
     }
   return;   
}

void
_click(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
       const char *emission, const char *source)
{
   int x, y;
   char str[128];

   /* get back the coordinates of the cell */
   sscanf(source, "board[%i,%i]:overlay", &x, &y);

   /* if we push 1st mouse button and there is no flag */
   if (!strcmp(emission, "mouse,clicked,1") && matrix[x][y].flag == 0)
     {
        if (game.clock.started == EINA_FALSE)
          {
             game.clock.started = EINA_TRUE;
             t0 = ecore_time_get();
             game.clock.etimer = ecore_timer_add(dt, _timer, NULL);
          }

        sprintf(str, "board[%i,%i]", x, y);
        _clean_walk(str, x, y);
     }

   /* second button: put a flag */
   if (!strcmp(emission, "mouse,clicked,3"))
     {
        if (matrix[x][y].uncover != 1)
          {
             if (!matrix[x][y].flag) /* set flag */
               {
                  sprintf(str, "board[%i,%i]:flag", x, y);
                  game.datas.remain--;
               }
             else /* already a flag, remove it */
               {
                  sprintf(str, "board[%i,%i]:default", x, y);
                  game.datas.remain++;
               }
             matrix[x][y].flag = !matrix[x][y].flag;
             elm_object_signal_emit(game.ui.table, str, "");
          }

        /* show the remaining mines */
        snprintf(str, sizeof(str), "%d/%d", game.datas.remain,
                 game.datas.mines_total);

        elm_object_part_text_set(game.ui.mines, "mines", str);
     }

   /* middle button: open rest if we have enough mines */
   if (!strcmp(emission, "mouse,clicked,2") && (matrix[x][y].uncover == 1))
     {
        Elemines_Walker *walker;
        Eina_Iterator *it;
        int flags = 0;

        /* count surrounding flags */
        it = _walk(x - 1, y - 1, 3, 3);
        EINA_ITERATOR_FOREACH(it, walker)
          {
             if (walker->x == x && walker->y == y)
               continue ;
             if (walker->cell->flag == 1)
               flags++;
          }
        eina_iterator_free(it);

        /* open surrounding squares if correct number of flags is set */
        if (flags == matrix[x][y].neighbours)
          {
             it = _walk(x - 1, y - 1, 3, 3);
             EINA_ITERATOR_FOREACH(it, walker)
               _clean_walk(walker->target, walker->x, walker->y);
             eina_iterator_free(it);
          }
     }
}

/* vim: set ts=8 sw=3 sts=3 expandtab cino=>5n-3f0^-2{2(0W1st0 : */
