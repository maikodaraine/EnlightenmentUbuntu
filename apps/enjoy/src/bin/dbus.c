#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "private.h"
#include <Eldbus.h>

#define DBUS_NAME "org.enlightenment.enjoy"
#define DBUS_IFACE "org.enlightenment.enjoy.Control"
#define DBUS_PATH "/org/enlightenment/enjoy/Control"

static Eldbus_Connection *conn;
static Eldbus_Service_Interface *control;

static Eldbus_Message *
_cb_dbus_quit(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   enjoy_quit();
   return eldbus_message_method_return_new(msg);
}

static Eldbus_Message *
_cb_dbus_version(const Eldbus_Service_Interface *iface __UNUSED__, const Eldbus_Message *msg)
{
   Eldbus_Message *reply = eldbus_message_method_return_new(msg);
   uint16_t aj = VMAJ, in = VMIN, ic = VMIC;
   eldbus_message_arguments_append(reply, "qqq", aj, in, ic);
   return reply;
}

/* Avoid duplicating MPRIS -- see src/plugins/mpris */
static const Eldbus_Method control_methods[] = {
   { "Quit", NULL, NULL, _cb_dbus_quit, 0 },
   {
    "Version", NULL, ELDBUS_ARGS({"q", ""}, {"q", ""}, {"q", ""}),
    _cb_dbus_version, 0
   },
   /* TODO: DB management */
   { }
};

static const Eldbus_Service_Interface_Desc desc = {
   DBUS_IFACE, control_methods
};

static void
_cb_dbus_request_name(void *data __UNUSED__, const Eldbus_Message *msg, Eldbus_Pending *pending__UNUSED__)
{
   const char *error_name, *error_txt;
   unsigned flag;

   if (eldbus_message_error_get(msg, &error_name, &error_txt))
     {
        ERR("Error %s %s", error_name, error_txt);
        goto end;
     }

   if (!eldbus_message_arguments_get(msg, "u", &flag))
     {
        ERR("Error getting arguments.");
        goto end;
     }

   if (flag != ELDBUS_NAME_REQUEST_REPLY_PRIMARY_OWNER)
     {
        ERR("Bus name in use by another application.");
        goto end;
     }

   INF("Got DBus name - unique instance running.");
   control = eldbus_service_interface_register(conn, DBUS_PATH, &desc);

   /* will run after other events run, in the main loop */
   ecore_event_add(ENJOY_EVENT_STARTED, NULL, NULL, NULL);
   return;

end:
   ecore_main_loop_quit();
}

Eina_Bool
enjoy_dbus_init(void)
{
   eldbus_init();
   conn = eldbus_connection_get(ELDBUS_CONNECTION_TYPE_SESSION);
   if (!conn)
     {
        ERR("Could not get DBus session bus");
        return EINA_FALSE;
     }

   eldbus_name_request(conn, DBUS_NAME, ELDBUS_NAME_REQUEST_FLAG_DO_NOT_QUEUE,
                      _cb_dbus_request_name, NULL);
   return EINA_TRUE;
}

void
enjoy_dbus_shutdown(void)
{
   if (control)
     eldbus_service_interface_unregister(control);
   if (conn)
     eldbus_connection_unref(conn);
   eldbus_shutdown();
   conn = NULL;
   control = NULL;
}
