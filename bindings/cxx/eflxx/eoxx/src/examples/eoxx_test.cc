#include <iostream>

#include <Elementary.h>

#include "Eo.hh"

static void
_win_del(void *data EINA_UNUSED, 
	 Evas_Object *obj EINA_UNUSED, 
	 void *event_info EINA_UNUSED) 
{
  elm_exit();
}

static Eina_Bool
_timer_run(void *data)
{
  std::cout << "Timer expired !" << std::endl;
  elm_exit();
  return EINA_FALSE;
}

int
main(int argc, char **argv)
{
  efl::weak_eo<efl::eo> timer;
  efl::weak_eo<efl::eo> window;
  efl::weak_eo<efl::eo> rect;

  elm_init(argc, argv);

  window = elm_win_add(NULL, "Eroxx", ELM_WIN_BASIC);
  if ((Eo*) window == NULL)
    return -1;
  elm_win_title_set(window, "Eroxx");
  evas_object_smart_callback_add(window, "delete,request", _win_del, NULL);

  rect = evas_object_rectangle_add(evas_object_evas_get(window));
  if ((Eo*) rect == NULL)
    return -1;
  evas_object_color_set(rect, 0, 128, 0, 255);
  evas_object_size_hint_weight_set(rect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  evas_object_size_hint_align_set(rect, EVAS_HINT_FILL, EVAS_HINT_FILL);
  evas_object_show(rect);

  elm_win_resize_object_add(window, rect);
  evas_object_resize(window, 256, 256);
  evas_object_show(window);

  timer = ecore_timer_add(1, _timer_run, NULL);

  elm_run();

  if ((Eo*) window)
    {
      std::cout << "Destroying window" << std::endl;
      evas_object_del(window);
    }
  if ((Eo*) timer)
    {
      std::cout << "Destroying timer" << std::endl;
      ecore_timer_del(timer);
    }

  elm_shutdown();

  return 0;
}
