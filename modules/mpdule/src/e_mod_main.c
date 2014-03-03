#include <e.h>
#include "e_mod_main.h"
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <libmpd/libmpdclient.h>

#define MAX_SONG_LENGTH 255

/* Func Proto Requirements for Gadcon */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name,
                                 const char *id, const char *style);
static void             _gc_shutdown(E_Gadcon_Client *gcc);
static void             _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char      *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object     *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char      *_gc_id_new(const E_Gadcon_Client_Class *client_class);

/* Module Protos */
static void             _mpdule_cb_mouse_down(void *data, Evas *e, Evas_Object *obj,
                                              void *event_info);
static void             _mpdule_cb_mouse_in(void *data, Evas *e, Evas_Object *obj,
                                            void *event_info);
static void             _mpdule_cb_mouse_out(void *data, Evas *e, Evas_Object *obj,
                                             void *event_info);
static void             _mpdule_menu_cb_configure(void *data, E_Menu *m,
                                                  E_Menu_Item *mi);
static void             _mpdule_menu_cb_post(void *data, E_Menu *m);
static void             _mpdule_cb_play(void *data, Evas_Object *obj,
                                        const char *emission, const char *source);
static void             _mpdule_cb_stop(void *data, Evas_Object *obj,
                                        const char *emission, const char *source);
static void             _mpdule_cb_pause(void *data, Evas_Object *obj,
                                         const char *emission, const char *source);
static void             _mpdule_cb_next(void *data, Evas_Object *obj,
                                        const char *emission, const char *source);
static void             _mpdule_cb_previous(void *data, Evas_Object *obj,
                                            const char *emission, const char *source);
static Config_Item     *_mpdule_config_item_get(const char *id);

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;

Config *mpdule_config = NULL;

/* Define the class and gadcon functions this module provides */
static const E_Gadcon_Client_Class _gc_class = {
   GADCON_CLIENT_CLASS_VERSION,
   "mpdule", {_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon,
              _gc_id_new, NULL, e_gadcon_site_is_not_toolbar},
   E_GADCON_CLIENT_STYLE_PLAIN
};

typedef struct _Instance Instance;
struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *mpdule;
   Evas_Object     *o_popup;
   mpd_Connection  *mpd;
   Ecore_Timer     *update_timer;
   Config_Item     *ci;
   E_Gadcon_Popup  *popup;
   Eina_Bool popup_creation_in_progress;
   char *status;
   char *artist;
   char *title;
   char *album;
   char *track;
   char *date;
   char *genre;
   char *composer;
   char *time;
   char *file;
};

static void      _mpdule_connect(Instance *inst);
static void      _mpdule_disconnect(Instance *inst);
static void      _mpdule_update_song(Instance *inst);
static Eina_Bool _mpdule_update_song_cb(void *data);
static void      _mpdule_popup_destroy(Instance *inst);
static void      _mpdule_popup_create(Instance *inst);

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Evas *evas;
   Instance *inst;
   char buf[4096];

   inst = E_NEW(Instance, 1);

   inst->ci = _mpdule_config_item_get(id);
   if (!inst->ci->id)
     inst->ci->id = eina_stringshare_add(id);

   o = edje_object_add(gc->evas);
   snprintf(buf, sizeof(buf), "%s/mpdule.edj",
            e_module_dir_get(mpdule_config->module));
   if (!e_theme_edje_object_set
         (o, "base/theme/modules/mpdule", "modules/mpdule/main"))
     edje_object_file_set(o, buf, "modules/mpdule/main");
   evas_object_show(o);

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   inst->gcc = gcc;
   inst->mpdule = o;

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mpdule_cb_mouse_down, inst);
   evas_object_event_callback_add(inst->mpdule, EVAS_CALLBACK_MOUSE_IN,
                                  _mpdule_cb_mouse_in, inst);
   evas_object_event_callback_add(inst->mpdule, EVAS_CALLBACK_MOUSE_OUT,
                                  _mpdule_cb_mouse_out, inst);
   edje_object_signal_callback_add(o, "mpdule,play", "", _mpdule_cb_play,
                                   inst);
   edje_object_signal_callback_add(o, "mpdule,stop", "", _mpdule_cb_stop,
                                   inst);
   edje_object_signal_callback_add(o, "mpdule,pause", "", _mpdule_cb_pause,
                                   inst);
   edje_object_signal_callback_add(o, "mpdule,next", "", _mpdule_cb_next,
                                   inst);
   edje_object_signal_callback_add(o, "mpdule,previous", "",
                                   _mpdule_cb_previous, inst);
   _mpdule_connect(inst);
   _mpdule_update_song(inst);
   inst->update_timer = ecore_timer_add(inst->ci->poll_time,
                                        _mpdule_update_song_cb, inst);

   mpdule_config->instances =
     eina_list_append(mpdule_config->instances, inst);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;
   if (inst->update_timer)
     ecore_timer_del(inst->update_timer);
   _mpdule_disconnect(inst);
   mpdule_config->instances =
     eina_list_remove(mpdule_config->instances, inst);

   evas_object_event_callback_del(inst->mpdule, EVAS_CALLBACK_MOUSE_DOWN,
                                  _mpdule_cb_mouse_down);
   evas_object_event_callback_del(inst->mpdule, EVAS_CALLBACK_MOUSE_IN,
                                  _mpdule_cb_mouse_in);
   evas_object_event_callback_del(inst->mpdule, EVAS_CALLBACK_MOUSE_OUT,
                                  _mpdule_cb_mouse_out);
   _mpdule_popup_destroy(inst);
   evas_object_del(inst->mpdule);
   free(inst->status);
   free(inst->artist);
   free(inst->title);
   free(inst->album);
   free(inst->track);
   free(inst->date);
   free(inst->genre);
   free(inst->composer);
   free(inst->time);
   free(inst->file);
   free(inst);
   gcc->data = NULL;
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;
   Evas_Coord mw, mh;

   inst = gcc->data;
   edje_object_size_min_calc(inst->mpdule, &mw, &mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class)
{
   return D_("MPDule");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-mpdule.edj",
            e_module_dir_get(mpdule_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class)
{
   Config_Item *ci = _mpdule_config_item_get(NULL);
   return ci->id;
}

static void
_mpdule_cb_mouse_down(void *data, Evas *e, Evas_Object *obj,
                      void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Instance *inst = data;

   if ((ev->button == 3) && (!mpdule_config->menu))
     {
        E_Menu *m;
        E_Menu_Item *mi;
        int x, y, w, h;

        m = e_menu_new();
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, D_("Settings"));
        e_util_menu_item_theme_icon_set(mi, "preferences-system");
        e_menu_item_callback_set(mi, _mpdule_menu_cb_configure, inst);

        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
        e_menu_post_deactivate_callback_set(m, _mpdule_menu_cb_post, inst);
        mpdule_config->menu = m;

        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, &w, &h);
        e_menu_activate_mouse(m,
                              e_util_zone_current_get(e_manager_current_get()),
                              x + ev->output.x,
                              y + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
   else if (ev->button == 1)
     {
        if (inst->popup) _mpdule_popup_destroy(inst);
        else _mpdule_popup_create(inst);
     }
}

static void
_mpdule_cb_mouse_in(void *data, Evas *e, Evas_Object *obj,
                    void *event_info)
{
   Instance *inst = data;

   if ((!inst) || (!inst->ci->show_popup))
     return;
   if (!inst->popup)
     {
        _mpdule_popup_create(inst);
     }
}

static void
_mpdule_cb_mouse_out(void *data, Evas *e, Evas_Object *obj,
                     void *event_info)
{
   Instance *inst = data;

   if ((!inst) || ((!inst->ci->show_popup) && (!inst->popup)))
     return;
   if (inst->popup)
     {
        _mpdule_popup_destroy(inst);
     }
}

static void
_mpdule_menu_cb_post(void *data, E_Menu *m)
{
   if (!mpdule_config->menu)
     return;
   e_object_del(E_OBJECT(mpdule_config->menu));
   mpdule_config->menu = NULL;
}

static void
_mpdule_menu_cb_configure(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Instance *inst = data;
   _config_mpdule_module(inst->ci);
}

void
_mpdule_config_updated(Config_Item *ci)
{
   Eina_List *l;

   if (!mpdule_config)
     return;
   for (l = mpdule_config->instances; l; l = l->next)
     {
        Instance *inst;

        inst = l->data;
        if (inst->ci != ci)
          continue;
        _mpdule_disconnect(inst);
        _mpdule_connect(inst);
        _mpdule_update_song(inst);
        if (inst->update_timer)
          {
             ecore_timer_interval_set(inst->update_timer, ci->poll_time);
          }
        else
          {
             inst->update_timer =
                 ecore_timer_add(ci->poll_time, _mpdule_update_song_cb,
                                 inst);
          }
        _mpdule_popup_destroy(inst);
        break;
     }
}

static void
_mpdule_cb_play(void *data, Evas_Object *obj, const char *emission,
                const char *source)
{
   Instance *inst = data;
   mpd_Connection *mpd;

   mpd = inst->mpd;
   mpd_sendPlayCommand(mpd, -1);
}

static void
_mpdule_cb_previous(void *data, Evas_Object *obj, const char *emission,
                    const char *source)
{
   Instance *inst = data;
   mpd_Connection *mpd;

   mpd = inst->mpd;
   mpd_sendPrevCommand(mpd);
}

static void
_mpdule_cb_next(void *data, Evas_Object *obj, const char *emission,
                const char *source)
{
   Instance *inst = data;
   mpd_Connection *mpd;

   mpd = inst->mpd;
   mpd_sendNextCommand(mpd);
}

static void
_mpdule_cb_stop(void *data, Evas_Object *obj, const char *emission,
                const char *source)
{
   Instance *inst = data;
   mpd_Connection *mpd;

   mpd = inst->mpd;
   mpd_sendStopCommand(mpd);
}

static void
_mpdule_cb_pause(void *data, Evas_Object *obj, const char *emission,
                 const char *source)
{
   Instance *inst = data;
   mpd_Connection *mpd;

   mpd = inst->mpd;
   mpd_sendPauseCommand(mpd, 1);
}

static Config_Item *
_mpdule_config_item_get(const char *id)
{
   Eina_List *l;
   Config_Item *ci;
   char buf[128];

   if (!id)
     {
        int num = 0;

        /* Create id */
        if (mpdule_config->items)
          {
             const char *p;
             ci = eina_list_last(mpdule_config->items)->data;
             p = strrchr(ci->id, '.');
             if (p)
               num = atoi(p + 1) + 1;
          }
        snprintf(buf, sizeof(buf), "%s.%d", _gc_class.name, num);
        id = buf;
     }
   else
     {
        for (l = mpdule_config->items; l; l = l->next)
          {
             ci = l->data;
             if (!ci->id)
               continue;
             if (!strcmp(ci->id, id))
               return ci;
          }
     }

   ci = E_NEW(Config_Item, 1);
   ci->id = eina_stringshare_add(id);
   ci->poll_time = 1.0;
   ci->hostname = eina_stringshare_add("localhost");
   ci->port = 6600;
   ci->show_popup = 1;

   mpdule_config->items = eina_list_append(mpdule_config->items, ci);
   return ci;
}

EAPI E_Module_Api e_modapi = {
   E_MODULE_API_VERSION,
   "MPDule"
};

static void
_mpdule_connect(Instance *inst)
{
   mpd_Connection *mpd;
   Config_Item *ci;

   ci = inst->ci;
   mpd = mpd_newConnection(ci->hostname, ci->port, 3.0);
   inst->mpd = mpd;
}

static void
_mpdule_disconnect(Instance *inst)
{
   if (inst->mpd)
     {
        mpd_closeConnection(inst->mpd);
        inst->mpd = NULL;
     }
}

static Eina_Bool
_mpdule_update_song_cb(void *data)
{
   Instance *inst = data;

   _mpdule_update_song(inst);
   return EINA_TRUE;
}

static void
_mpdule_update_song(Instance *inst)
{
   mpd_Connection *mpd;
   Evas_Object *mpdule;
   Evas_Object *o_popup;

   if (!inst->mpd)
     return;
   mpd = inst->mpd;
   mpdule = inst->mpdule;
   o_popup = inst->o_popup;
   mpd_sendStatusCommand(mpd);
   if (mpd->error == 0)
     {
        mpd_Status *status = mpd_getStatus(mpd);

        free(inst->status); inst->status = NULL;
        free(inst->artist); inst->artist = NULL;
        free(inst->title); inst->title = NULL;
        free(inst->album); inst->album = NULL;
        free(inst->track); inst->track = NULL;
        free(inst->date); inst->date = NULL;
        free(inst->genre); inst->genre = NULL;
        free(inst->composer); inst->composer = NULL;
        free(inst->time); inst->time = NULL;
        free(inst->file); inst->file = NULL;

        if (status)
          {
             if (status->state == MPD_STATUS_STATE_UNKNOWN)
               {
                  inst->status = strdup(D_("Unknown"));
                  edje_object_part_text_set(mpdule, "mpdule.status",
                                            inst->status);
                  edje_object_part_text_set(o_popup, "mpdule.status",
                                            inst->status);
               }
             else if (status->state == MPD_STATUS_STATE_STOP)
               {
                  inst->status = strdup(D_("Stopped"));
                  edje_object_part_text_set(mpdule, "mpdule.status",
                                            inst->status);
                  edje_object_part_text_set(o_popup, "mpdule.status",
                                            inst->status);
               }
             else if (status->state == MPD_STATUS_STATE_PLAY)
               {
                  inst->status = strdup(D_("Playing"));
                  edje_object_part_text_set(mpdule, "mpdule.status",
                                            inst->status);
                  edje_object_part_text_set(o_popup, "mpdule.status",
                                            inst->status);
               }
             else if (status->state == MPD_STATUS_STATE_PAUSE)
               {
                  inst->status = strdup(D_("Paused"));
                  edje_object_part_text_set(mpdule, "mpdule.status",
                                            inst->status);
                  edje_object_part_text_set(o_popup, "mpdule.status",
                                            inst->status);
               }

             if (status->state > MPD_STATUS_STATE_STOP)
               {
                  mpd_sendCurrentSongCommand(mpd);
                  mpd_InfoEntity *entity = NULL;

                  while ((entity = mpd_getNextInfoEntity(mpd)))
                    {
                       if (entity->type == MPD_INFO_ENTITY_TYPE_SONG &&
                           entity->info.song->id == status->songid)
                         {
                            mpd_Song *song = entity->info.song;

#define FILL_IN(_var)                                                        \
                     if (song->_var)                                         \
                       {                                                     \
                          inst->_var = strdup(song->_var);                   \
                          edje_object_part_text_set(mpdule, "mpdule."#_var,  \
                                                    song->_var);             \
                          edje_object_part_text_set(o_popup, "mpdule."#_var, \
                                                    song->_var);             \
                       }                                                     \
                     else                                                    \
                       {                                                     \
                          edje_object_part_text_set(mpdule, "mpdule."#_var,  \
                                                    "");                     \
                          edje_object_part_text_set(o_popup, "mpdule."#_var, \
                                                    "");                     \
                       }

                            FILL_IN(artist);
                            FILL_IN(title);
                            FILL_IN(album);
                            FILL_IN(track);
                            FILL_IN(date);
                            FILL_IN(genre);
                            FILL_IN(composer);
                            //FILL_IN(time);
                            FILL_IN(file);
#undef FILL_IN
                         }

                       mpd_freeInfoEntity(entity);
                    }
               }

             mpd_freeStatus(status);
          }
     }
   else
     {
        _mpdule_disconnect(inst);
        _mpdule_connect(inst);
     }
}

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/locale", e_module_dir_get(m));
   bindtextdomain(PACKAGE, buf);
   bind_textdomain_codeset(PACKAGE, "UTF-8");

   conf_item_edd = E_CONFIG_DD_NEW("MPDule_Config_Item", Config_Item);
#undef T
#undef D
#define T Config_Item
#define D conf_item_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, poll_time, DOUBLE);
   E_CONFIG_VAL(D, T, hostname, STR);
   E_CONFIG_VAL(D, T, port, INT);
   E_CONFIG_VAL(D, T, show_popup, UCHAR);

   conf_edd = E_CONFIG_DD_NEW("MPDule_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_LIST(D, T, items, conf_item_edd);

   mpdule_config = e_config_domain_load("module.mpdule", conf_edd);
   if (!mpdule_config)
     {
        Config_Item *ci;

        mpdule_config = E_NEW(Config, 1);

        ci = E_NEW(Config_Item, 1);
        ci->id = eina_stringshare_add("0");
        ci->poll_time = 1.0;
        ci->hostname = eina_stringshare_add("localhost");
        ci->port = 6600;
        ci->show_popup = 1;

        mpdule_config->items = eina_list_append(mpdule_config->items, ci);
     }
   mpdule_config->module = m;

   e_gadcon_provider_register(&_gc_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   mpdule_config->module = NULL;
   e_gadcon_provider_unregister(&_gc_class);

   if (mpdule_config->config_dialog)
     e_object_del(E_OBJECT(mpdule_config->config_dialog));
   if (mpdule_config->menu)
     {
        e_menu_post_deactivate_callback_set(mpdule_config->menu, NULL, NULL);
        e_object_del(E_OBJECT(mpdule_config->menu));
        mpdule_config->menu = NULL;
     }

   while (mpdule_config->items)
     {
        Config_Item *ci;

        ci = mpdule_config->items->data;
        mpdule_config->items =
          eina_list_remove_list(mpdule_config->items, mpdule_config->items);
        if (ci->id)
          eina_stringshare_del(ci->id);
        free(ci);
     }

   free(mpdule_config);
   mpdule_config = NULL;
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.mpdule", conf_edd, mpdule_config);
   return 1;
}

static void
_mpdule_popup_destroy(Instance *inst)
{
   if (inst->popup)
     {
        E_Gadcon_Popup *popup = inst->popup;

        inst->popup = NULL;
        inst->o_popup = NULL;
        e_object_del(E_OBJECT(popup));
     }
}

static void
_mpdule_popup_create(Instance *inst)
{
   if (inst->popup)
     return;

   if (inst->ci->show_popup)
     {
        Evas *evas;
        Evas_Object *o_popup;

        if (inst->popup_creation_in_progress)
            return;
        inst->popup_creation_in_progress = EINA_TRUE;

        inst->popup = e_gadcon_popup_new(inst->gcc);
        evas = inst->popup->win->evas;
        o_popup = edje_object_add(evas);
        if (!e_theme_edje_object_set
              (o_popup, "base/theme/modules/mpdule", "modules/mpdule/popup"))
          {
             char buf[4096];

             snprintf(buf, sizeof(buf), "%s/mpdule.edj",
                      e_module_dir_get(mpdule_config->module));
             edje_object_file_set(o_popup, buf, "modules/mpdule/popup");
          }
        edje_object_part_text_set(o_popup, "mpdule.artist_label", D_("Artist:"));
        edje_object_part_text_set(o_popup, "mpdule.title_label", D_("Title:"));
        edje_object_part_text_set(o_popup, "mpdule.album_label", D_("Album:"));
        edje_object_part_text_set(o_popup, "mpdule.genre_label", D_("Genre:"));
        e_gadcon_popup_content_set(inst->popup, o_popup);
        e_gadcon_popup_show(inst->popup);
        edje_object_size_min_calc(o_popup, NULL, NULL);
        inst->o_popup = o_popup;
        edje_object_signal_callback_add(o_popup, "mpdule,play", "",
                                        _mpdule_cb_play, inst);
        edje_object_signal_callback_add(o_popup, "mpdule,stop", "",
                                        _mpdule_cb_stop, inst);
        edje_object_signal_callback_add(o_popup, "mpdule,pause", "",
                                        _mpdule_cb_pause, inst);
        edje_object_signal_callback_add(o_popup, "mpdule,next", "",
                                        _mpdule_cb_next, inst);
        edje_object_signal_callback_add(o_popup, "mpdule,previous", "",
                                        _mpdule_cb_previous, inst);

#define DISPLAY_INFO(_var) \
        if (inst->_var)                                         \
          {                                                     \
             edje_object_part_text_set(o_popup, "mpdule."#_var, \
                                       inst->_var);             \
          }
        DISPLAY_INFO(artist);
        DISPLAY_INFO(title);
        DISPLAY_INFO(album);
        DISPLAY_INFO(track);
        DISPLAY_INFO(date);
        DISPLAY_INFO(genre);
        DISPLAY_INFO(composer);
        //DISPLAY_INFO(time);
        DISPLAY_INFO(file);
#undef DISPLAY_INFO

        inst->popup_creation_in_progress = EINA_FALSE;
     }
}
