#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int   e_modapi_shutdown(E_Module *m);
EAPI int   e_modapi_save(E_Module *m);

typedef struct _Instance Instance;
typedef struct _Instance_Xembed Instance_Xembed;
typedef struct _Context_Notifier_Host Context_Notifier_Host;
typedef struct _Instance_Notifier_Host Instance_Notifier_Host;
typedef struct _Notifier_Item Notifier_Item;
typedef struct _Systray_Context Systray_Context;
typedef struct _E_Config_Dialog_Data Systray_Config;


struct _Instance
{
   E_Gadcon_Client *gcc;
   E_Comp     *comp;
   Evas            *evas;
   Instance_Xembed *xembed;
   Instance_Notifier_Host *notifier;
   struct
   {
      Evas_Object *gadget;
   } ui;
   struct
   {
      Ecore_Job *size_apply;
   } job;
};

E_Gadcon_Orient systray_orient_get(const Instance *inst);
const E_Gadcon *systray_gadcon_get(const Instance *inst);
E_Gadcon_Client *systray_gadcon_client_get(const Instance *inst);
const char *systray_style_get(const Instance *inst);
void systray_size_updated(Instance *inst);
Evas *systray_evas_get(const Instance *inst);
Evas_Object *systray_edje_get(const Instance *inst);
const Evas_Object *systray_box_get(const Instance *inst);
void systray_edje_box_append(const Instance *inst, Evas_Object *child);
void systray_edje_box_remove(const Instance *inst, Evas_Object *child);
void systray_edje_box_prepend(const Instance *inst, Evas_Object *child);

int systray_manager_number_get(const Instance *inst);
Ecore_X_Window systray_root_get(const Instance *inst);

void systray_xembed_init(void);
void systray_xembed_shutdown(void);

Instance_Xembed *systray_xembed_new(Instance *inst);
void systray_xembed_free(Instance_Xembed *xembed);
void systray_xembed_orient_set(Instance_Xembed *xembed, E_Gadcon_Orient orient);
void systray_xembed_size_updated(Instance_Xembed *xembed);

Instance_Notifier_Host *systray_notifier_host_new(Instance *inst, E_Gadcon *gadcon);
void systray_notifier_host_free(Instance_Notifier_Host *notifier);
void systray_notifier_host_init(void);
void systray_notifier_host_shutdown(void);

/**
 * @addtogroup Optional_Gadgets
 * @{
 *
 * @defgroup Module_Systray Systray (System Icons Tray)
 *
 * Shows system icons in a box.
 *
 * The icons come from the FreeDesktop.Org systray specification.
 *
 * @see http://standards.freedesktop.org/systemtray-spec/systemtray-spec-latest.html
 * @}
 */
#endif
