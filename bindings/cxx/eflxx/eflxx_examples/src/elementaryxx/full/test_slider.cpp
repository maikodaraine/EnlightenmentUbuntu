#include "test.h"

void test_slider (void *data, Evas_Object *obj, void *event_info)
{
  Icon *ic = NULL;
  Slider *sl = NULL;

  Window *win = Window::factory ("slider", ELM_WIN_BASIC);
  win->setTitle ("Slider");
  win->setAutoDel (true);

  Background *bg = Background::factory (*win);
  win->addResizeObject (*bg);
  bg->setSizeHintWeight (EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  bg->show ();

  Box *bx = Box::factory (*win);
  win->addResizeObject (*bx);
  bx->setSizeHintWeight (EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  bx->show ();

  ic = Icon::factory (*win);
  ic->setFile (searchPixmapFile ("elementaryxx/logo_small.png"));
  ic->setSizeHintAspect (EVAS_ASPECT_CONTROL_VERTICAL, Size (1, 1));
  ic->show ();

  sl = Slider::factory (*win);
  sl->setText ("Label");
  sl->setContent (*ic);
  sl->setUnitFormat ("%1.1f units");
  sl->setSpanSize (120);
  sl->setSizeHintAlign (EVAS_HINT_FILL, 0.5);
  sl->setSizeHintWeight (EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  bx->packEnd (*sl);
  sl->show ();

  sl = Slider::factory (*win);
  sl->setText ("Label 2");
  sl->setSpanSize (80);
  sl->setIndicatorFormat ("%3.0f");
  sl->setMinMax (50, 150);
  sl->setValue (80);
  sl->setInverted (true);
  sl->setSizeHintAlign (0.5, 0.5);
  sl->setSizeHintWeight (0.0, 0.0);
  bx->packEnd (*sl);
  sl->show ();

  sl = Slider::factory (*win);
  sl->setText ("Label 3");
  sl->setUnitFormat ("units");
  sl->setSpanSize (40);
  sl->setSizeHintAlign (EVAS_HINT_FILL, 0.5);
  sl->setSizeHintWeight (EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  sl->setIndicatorFormat ("%3.0f");
  sl->setMinMax (50, 150);
  sl->setValue (80);
  sl->setInverted (true);
  sl->setScale (2.0);
  bx->packEnd (*sl);
  sl->show ();

  ic = Icon::factory (*win);
  ic->setFile (searchPixmapFile ("elementaryxx/logo_small.png"));
  ic->setSizeHintAspect (EVAS_ASPECT_CONTROL_HORIZONTAL, Size (1, 1));
  ic->show ();

  sl = Slider::factory (*win);
  sl->setText ("Label 4");
  sl->setContent (*ic);
  sl->setUnitFormat ("units");
  sl->setSpanSize (60);
  sl->setSizeHintAlign (0.5L, EVAS_HINT_FILL);
  sl->setSizeHintWeight (0.0, EVAS_HINT_EXPAND);
  sl->setIndicatorFormat ("%1.1f");
  sl->setValue (0.2);
  sl->setScale (1.0);
  sl->setOrientation (Slider::Vertical);
  bx->packEnd (*sl);
  sl->show ();

  win->show ();
}
