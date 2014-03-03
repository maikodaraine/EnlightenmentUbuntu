#ifndef _ERULER_PRIVATE_H_
#define _ERULER_PRIVATE_H_ 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Elementary.h>

extern int _log_dom;

#define CRI(...) EINA_LOG_DOM_CRIT(_log_dom, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_log_dom, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(_log_dom, __VA_ARGS__)

void show_gui_error(const char *message);

typedef struct _Zone Zone;

typedef struct _Platform_Funcs Platform_Funcs;
struct _Platform_Funcs
{
    Eina_Bool (*pre_setup)(void);
    Eina_Bool (*post_setup)(void);
    int (*zones_count)(void);
    Eina_Bool (*zone_geometry_get)(int zone, int *x, int *y, int *w, int *h);
    Eina_Bool (*zone_setup)(Zone *zone);
    void (*zone_screen_copy)(Zone *zone, Evas_Object *img, void (*cb)(void *data, Eina_Bool success), const void *data);
    void (*pre_teardown)(void);
    void (*mouse_move_by)(Zone *zone, int dx, int dy);
    void (*windows_visibility_set)(Eina_Bool visible);
    void (*global_key_grab)(const char *keyname, void (*cb)(void *data, const char *keyname), const void *data);
};

Evas_Object *zone_win_get(const Zone *zone);
Evas_Object *zone_screen_copy_object_get(const Zone *zone);
void zone_screen_copy_ready_set(Zone *zone, Eina_Bool ready);

void zone_geometry_get(const Zone *zone, int *x, int *y, int *w, int *h);
int zone_index_get(const Zone *zone);

#ifdef HAVE_ECORE_X
const Platform_Funcs *platform_funcs_x_get(void);
#endif

#endif
