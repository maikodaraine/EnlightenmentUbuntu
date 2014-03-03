#include "test.h"



static void my_hover_bt (Evasxx::Object &obj, void *event_info, Hover *hv)
{
  hv->show ();
}

void test_hover (void *data, Evas_Object *obj, void *event_info)
{
  Button *bt = NULL;
  Box *bx = NULL;

  Window *win = Window::factory ("hover", ELM_WIN_BASIC);
  win->setTitle ("Hover");
  win->setAutoDel (true);

  Background *bg = Background::factory (*win);
  win->addResizeObject (*bg);
  bg->setSizeHintWeight (EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  bg->show ();

  bx = Box::factory (*win);
  bx->setSizeHintWeight (EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  win->addResizeObject (*bx);
  bx->show ();

  Hover *hv = Hover::factory (*win);

  bt = Button::factory (*win);
  bt->setText ("Button");
  bt->getEventSignal ("clicked")->connect (sigc::bind (sigc::ptr_fun (&my_hover_bt), hv));
  bx->packEnd (*bt);
  bt->show ();
  hv->setParent (*win);
  hv->setTarget (*bt);

  bt = Button::factory (*win);
  bt->setText ("Popup");
  hv->setContent ("middle", *bt);
  bt->show ();

  bx = Box::factory (*win);

  Icon *ic = Icon::factory (*win);
  ic->setFile (searchPixmapFile ("elementaryxx/logo_small.png"));
  ic->setNoScale (true);
  bx->packEnd (*ic);
  ic->show ();

  bt = Button::factory (*win);
  bt->setText ("Top 1");
  bx->packEnd (*bt);
  bt->show ();

  bt = Button::factory (*win);
  bt->setText ("Top 2");
  bx->packEnd (*bt);
  bt->show ();

  bt = Button::factory (*win);
  bt->setText ("Top 3");
  bx->packEnd (*bt);
  bt->show ();

  bx->show ();

  hv->setContent ("top", *bx);

  bt = Button::factory (*win);
  bt->setText ("Bottom");
  hv->setContent ("bottom", *bt);
  bt->show ();

  bt = Button::factory (*win);
  bt->setText ("Left");
  hv->setContent ("left", *bt);
  bt->show ();

  bt = Button::factory (*win);
  bt->setText ("Right");
  hv->setContent ("right", *bt);
  bt->show ();

  bg->setSizeHintMin (size160x160);
  bg->setSizeHintMax (size640x640);
  win->resize (size320x320);
  win->show ();
}

void test_hover2 (void *data, Evas_Object *obj, void *event_info)
{
  Box *bx = NULL;
  Button *bt = NULL;

  Window *win = Window::factory ("hover2", ELM_WIN_BASIC);
  win->setTitle ("Hover 2");
  win->setAutoDel (true);

  Background *bg = Background::factory (*win);
  win->addResizeObject (*bg);
  bg->setSizeHintWeight (EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  bg->show ();

  bx = Box::factory (*win);
  bx->setSizeHintWeight (EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
  win->addResizeObject (*bx);
  bx->show ();

  Hover *hv = Hover::factory (*win);
  hv->setStyle ("popout");

  bt = Button::factory (*win);
  bt->setText ("Button");
  bt->getEventSignal ("clicked")->connect (sigc::bind (sigc::ptr_fun (&my_hover_bt), hv));
  bx->packEnd (*bt);
  bt->show ();
  hv->setParent (*win);
  hv->setTarget (*bt);

  bt = Button::factory (*win);
  bt->setText ("Popup");
  hv->setContent ("middle", *bt);
  bt->show ();

  bx = Box::factory (*win);

  Icon *ic = Icon::factory (*win);
  ic->setFile (searchPixmapFile ("elementaryxx/logo_small.png"));
  ic->setNoScale (true);
  bx->packEnd (*ic);
  ic->show ();

  bt = Button::factory (*win);
  bt->setText ("Top 1");
  bx->packEnd (*bt);
  bt->show ();

  bt = Button::factory (*win);
  bt->setText ("Top 2");
  bx->packEnd (*bt);
  bt->show ();

  bt = Button::factory (*win);
  bt->setText ("Top 3");
  bx->packEnd (*bt);
  bt->show ();

  bx->show ();

  hv->setContent ("top", *bx);

  bt = Button::factory (*win);
  bt->setText ("Bot");
  hv->setContent ("bottom", *bt);
  bt->show ();

  bt = Button::factory (*win);
  bt->setText ("Left");
  hv->setContent ("left", *bt);
  bt->show ();

  bt = Button::factory (*win);
  bt->setText ("Right");
  hv->setContent ("right", *bt);
  bt->show ();

  bg->setSizeHintMin (size160x160);
  bg->setSizeHintMax (size640x640);
  win->resize (size320x320);
  win->show ();
}
