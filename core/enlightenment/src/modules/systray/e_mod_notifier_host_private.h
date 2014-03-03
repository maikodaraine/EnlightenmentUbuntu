#include "e_mod_main.h"

typedef enum {
   CATEGORY_UNKNOWN = 0,
   CATEGORY_SYSTEM_SERVICES,
   CATEGORY_LAST
} Category;

typedef enum {
   STATUS_UNKNOWN = 0,
   STATUS_ACTIVE,
   STATUS_PASSIVE,
   STATUS_ATTENTION,
   STATUS_LAST
} Status;

typedef struct _Notifier_Data
{
   EINA_INLIST;
   Notifier_Item *item;
   Evas_Object *icon;
} Notifier_Item_Icon;

struct _Instance_Notifier_Host
{
   EINA_INLIST;
   Instance *inst;
   const Evas_Object *box;
   const Evas_Object *edje;
   Eina_Inlist *ii_list;
   E_Gadcon *gadcon;
};

struct _Context_Notifier_Host
{
   Eldbus_Connection *conn;
   Eldbus_Proxy *watcher;
   Eina_Inlist *item_list;
   Eina_Inlist *instances;
   Eina_List *pending;
};

struct _Notifier_Item
{
   EINA_INLIST;
   const char *bus_id;
   const char *path;
   Eldbus_Proxy *proxy;
   Category category;
   Status status;
   E_DBusMenu_Item *dbus_item;
   const char *id;
   const char *title;
   const char *icon_name;
   const char *attention_icon_name;
   const char *icon_path;
   const char *menu_path;
   E_DBusMenu_Ctx *menu_data;
   Eina_List *signals;
};

typedef void (*E_Notifier_Watcher_Item_Registered_Cb)(void *data, const char *service);
typedef void (*E_Notifier_Watcher_Item_Unregistered_Cb)(void *data, const char *service);

void systray_notifier_update_menu(void *data, E_DBusMenu_Item *new_root_item);
void systray_notifier_item_update(Notifier_Item *item);
void systray_notifier_item_free(Notifier_Item *item);

void systray_notifier_dbus_init(Context_Notifier_Host *ctx);
void systray_notifier_dbus_shutdown(Context_Notifier_Host *ctx);

void systray_notifier_dbus_watcher_start(Eldbus_Connection *connection, E_Notifier_Watcher_Item_Registered_Cb registered, E_Notifier_Watcher_Item_Unregistered_Cb unregistered, const void *data);
void systray_notifier_dbus_watcher_stop(void);
