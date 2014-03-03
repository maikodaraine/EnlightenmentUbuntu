#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif

#include "elementaryxx/Radio.h"

using namespace std;

namespace Elmxx {

Radio::Radio (Evasxx::Object &parent)
{
  o = elm_radio_add (parent.obj ());
  
  elmInit ();
}

Radio::~Radio () {}

Radio *Radio::factory (Evasxx::Object &parent)
{
  return new Radio (parent);
}

void Radio::addGroup (const Evasxx::Object &group)
{
  elm_radio_group_add (o, group.obj ());
}

void Radio::setStateValue (int value)
{
  elm_radio_state_value_set (o, value);
}

void Radio::setValue (int value)
{
  elm_radio_value_set (o, value);
}

int Radio::getValue () const
{
  return elm_radio_value_get (o);
}

} // end namespace Elmxx
