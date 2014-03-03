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

typedef struct _Elemines_Iterator Elemines_Iterator;
struct _Elemines_Iterator
{
   Eina_Iterator iterator;

   Elemines_Walker walker;

   unsigned char col;
   unsigned char row;
   unsigned char x;
   unsigned char y;
   unsigned char width;
   unsigned char height;
};

static Eina_Bool
_walk_iterator_next(Elemines_Iterator *it, void **data)
{
   if (!(it->col < it->x + it->width &&
         it->row < it->y + it->height))
     return EINA_FALSE;

   *data = &it->walker;

   it->walker.cell = &matrix[it->col][it->row];
   it->walker.x = it->col;
   it->walker.y = it->row;
   sprintf((char*) it->walker.target, "board[%i,%i]", it->walker.x, it->walker.y);

   it->col++;
   if (it->col == it->x + it->width)
     {
        it->col = it->x;
        it->row++;
     }

   return EINA_TRUE;
}

static void *
_walk_iterator_container(Elemines_Iterator *it EINA_UNUSED)
{
   return matrix;
}

static void
_walk_iterator_free(Elemines_Iterator *it)
{
   EINA_MAGIC_SET(&it->iterator, 0);
   free(it);
}

Eina_Iterator *
_walk(unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
   Elemines_Iterator *r;

   r = calloc(1, sizeof (Elemines_Iterator) + strlen("board[00,00]") + 1);
   if (!r) return NULL;

   r->iterator.version = EINA_ITERATOR_VERSION;
   r->iterator.next = FUNC_ITERATOR_NEXT(_walk_iterator_next);
   r->iterator.get_container = FUNC_ITERATOR_GET_CONTAINER(_walk_iterator_container);
   r->iterator.free = FUNC_ITERATOR_FREE(_walk_iterator_free);

   EINA_MAGIC_SET(&r->iterator, EINA_MAGIC_ITERATOR);

   r->col = x;
   r->row = y;
   r->x = x;
   r->y = y;
   r->width = w;
   r->height = h;
   r->walker.target = (char *) (r + 1);
   r->walker.x = x;
   r->walker.y = y;

   return &r->iterator;
}

static void
_debug(void)
{
   Eina_Iterator *it;
   Elemines_Walker *walker;
   unsigned char prev_y = 0;

   printf("== bomb positions =====\n");
   it = _walk(0, 0, game.datas.x_theme + 2, game.datas.y_theme + 2);
   EINA_ITERATOR_FOREACH(it, walker)
     {
        if (prev_y != walker->y) printf("\n");
        printf("%d ", walker->cell->mine);
        prev_y = walker->y;
     }
   eina_iterator_free(it);

   printf("\n\n== neighbours count ===\n");
   prev_y = 0;
   it = _walk(0, 0, game.datas.x_theme + 2, game.datas.y_theme + 2);
   EINA_ITERATOR_FOREACH(it, walker)
     {
        if (prev_y != walker->y) printf("\n");
        printf("%d ", walker->cell->neighbours);
        prev_y = walker->y;
     }
   eina_iterator_free(it);
   printf("\n");
}

static void
_shutdown(void)
{
   int i;

   if (matrix)
     {
        for (i = 0; i < game.datas.x_theme + 2; i++)
          free(matrix[i]);
        free(matrix);
     }
}

static const Ecore_Getopt optdesc = {
  "elemines",
  "%prog [options]",
  PACKAGE_VERSION,
  COPYRIGHT,
  "BSD with advertisement clause",
  "An EFL minesweeper clone",
  0,
  {
    ECORE_GETOPT_STORE_TRUE('d', "debug", "turn on debugging"),
    ECORE_GETOPT_STORE_TRUE('f', "fullscreen", "make the application fullscreen"),
    ECORE_GETOPT_STORE_STR('t', "theme", "change theme"),
    ECORE_GETOPT_STORE_INT('m', "mines", "define the number of mines on the grid"),
    ECORE_GETOPT_LICENSE('L', "license"),
    ECORE_GETOPT_COPYRIGHT('C', "copyright"),
    ECORE_GETOPT_VERSION('V', "version"),
    ECORE_GETOPT_HELP('h', "help"),
    ECORE_GETOPT_SENTINEL
  }
};

EAPI_MAIN int
elm_main(int argc __UNUSED__, char **argv __UNUSED__)
{
   char *theme = "default";
   int args;
   Eina_Bool debug = EINA_FALSE;
   Eina_Bool fullscreen = EINA_FALSE;
   Eina_Bool quit_option = EINA_FALSE;
   Ecore_Getopt_Value values[] = {
     ECORE_GETOPT_VALUE_BOOL(debug),
     ECORE_GETOPT_VALUE_BOOL(fullscreen),
     ECORE_GETOPT_VALUE_STR(theme),
     ECORE_GETOPT_VALUE_INT(game.datas.mines_total),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_NONE
   };

#if ENABLE_NLS
   setlocale(LC_ALL, "");
   bindtextdomain(PACKAGE, LOCALEDIR);
   bind_textdomain_codeset(PACKAGE, "UTF-8");
   textdomain(PACKAGE);
#endif

   /* will be used to check input */
   game.datas.mines_total = 0;

   /* Get user values */
   args = ecore_getopt_parse(&optdesc, values, argc, argv);
   if (args < 0)
     {
       EINA_LOG_CRIT("Could not parse arguments.");
       goto end;
     }
   else if (quit_option)
     {
       goto end;
     }

   elm_app_compile_bin_dir_set(PACKAGE_BIN_DIR);
   elm_app_compile_data_dir_set(PACKAGE_DATA_DIR);
   elm_app_info_set(elm_main, "elemines", "themes/default.edj");

   game.clock.started = EINA_FALSE;

   if (gui(theme, fullscreen) != EINA_TRUE)
     return -1;

   init(NULL, NULL, NULL);
   if (debug == EINA_TRUE) _debug();

   elm_run();

 end:
   _shutdown();
   elm_shutdown();

   return 0;
}
ELM_MAIN()

/* vim: set ts=8 sw=3 sts=3 expandtab cino=>5n-3f0^-2{2(0W1st0 : */
