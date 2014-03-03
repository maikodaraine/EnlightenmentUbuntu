#include "e.h"

/* TODO: Handle orientation in stored config */
/* TODO: Do we need e_randr_cfg->screen.{width,height} ? */
/* TODO: Check so we can rely on connected only, not connected && exists */
/* TODO: Ignore xrandr events triggered by changes in acpi cb */

/* local function prototypes */
static Eina_Bool _e_randr_config_load(void);
static void      _e_randr_config_new(void);
static void      _e_randr_config_free(void);
static void      _e_randr_free(void);
static Eina_Bool _e_randr_config_cb_timer(void *data);
static void      _e_randr_load(void);
static void      _e_randr_apply(void);
static void                   _e_randr_output_mode_update(E_Randr_Output *cfg);
static E_Config_Randr_Output *_e_randr_config_output_new(unsigned int id);
static E_Config_Randr_Output *_e_randr_config_output_find(Ecore_X_Randr_Output output);
static E_Randr_Crtc          *_e_randr_crtc_find(Ecore_X_Randr_Crtc xid);
static E_Randr_Output        *_e_randr_output_find(Ecore_X_Randr_Output xid);
static E_Randr_Crtc          *_e_randr_output_crtc_find(E_Randr_Output *output);

static void _e_randr_config_mode_geometry(Ecore_X_Randr_Orientation orient, Eina_Rectangle *rect);
static void _e_randr_config_primary_update(void);

static Eina_Bool _e_randr_event_cb_screen_change(void *data, int type, void *event);
static Eina_Bool _e_randr_event_cb_crtc_change(void *data, int type, void *event);
static Eina_Bool _e_randr_event_cb_output_change(void *data, int type, void *event);

static void      _e_randr_acpi_handler_add(void *data);
static int       _e_randr_is_lid(E_Randr_Output *cfg);
static void      _e_randr_outputs_from_crtc_set(E_Randr_Crtc *crtc);
static void      _e_randr_crtc_from_outputs_set(E_Randr_Crtc *crtc);
static Eina_Bool _e_randr_lid_update(void);
static Eina_Bool _e_randr_output_mode_valid(Ecore_X_Randr_Mode mode, Ecore_X_Randr_Mode *modes, int nmodes);
static void      _e_randr_output_active_set(E_Randr_Output *cfg, Eina_Bool connected);
static int       _e_randr_config_output_cmp(const void *a, const void *b);

/* local variables */
static Eina_List *_randr_event_handlers = NULL;
static E_Config_DD *_e_randr_edd = NULL;
static E_Config_DD *_e_randr_output_edd = NULL;

static int _e_randr_lid_is_closed = 0;

static E_Randr *e_randr = NULL;

/* external variables */
EAPI E_Config_Randr *e_randr_cfg = NULL;

/* private internal functions */
EINTERN Eina_Bool
e_randr_init(void)
{
   /* check if randr is available */
   if (!ecore_x_randr_query()) return EINA_FALSE;

   /* get initial lid status */
   _e_randr_lid_is_closed = (e_acpi_lid_status_get() == E_ACPI_LID_CLOSED);

   /* try to load config */
   if (!_e_randr_config_load())
     {
        /* NB: We should probably print an error here */
        return EINA_FALSE;
     }

   /* tell randr that we are interested in receiving events
    *
    * NB: Requires RandR >= 1.2 */
   if (ecore_x_randr_version_get() >= E_RANDR_VERSION_1_2)
     {
        Ecore_X_Window root = 0;

        if ((root = ecore_x_window_root_first_get()))
          ecore_x_randr_events_select(root, EINA_TRUE);

        /* setup randr event listeners */
        E_LIST_HANDLER_APPEND(_randr_event_handlers,
                              ECORE_X_EVENT_SCREEN_CHANGE,
                              _e_randr_event_cb_screen_change, NULL);
        E_LIST_HANDLER_APPEND(_randr_event_handlers,
                              ECORE_X_EVENT_RANDR_CRTC_CHANGE,
                              _e_randr_event_cb_crtc_change, NULL);
        E_LIST_HANDLER_APPEND(_randr_event_handlers,
                              ECORE_X_EVENT_RANDR_OUTPUT_CHANGE,
                              _e_randr_event_cb_output_change, NULL);
     }

   /* delay setting up acpi handler, as acpi is init'ed after randr */
   ecore_job_add(_e_randr_acpi_handler_add, NULL);

   return EINA_TRUE;
}

EINTERN int
e_randr_shutdown(void)
{
   /* check if randr is available */
   if (!ecore_x_randr_query()) return 1;

   if (ecore_x_randr_version_get() >= E_RANDR_VERSION_1_2)
     {
        Ecore_X_Window root = 0;

        /* tell randr that we are not interested in receiving events anymore */
        if ((root = ecore_x_window_root_first_get()))
          ecore_x_randr_events_select(root, EINA_FALSE);
     }

   /* remove event listeners */
   E_FREE_LIST(_randr_event_handlers, ecore_event_handler_del);

   /* free config */
   _e_randr_free();
   _e_randr_config_free();

   /* free edd */
   E_CONFIG_DD_FREE(_e_randr_output_edd);
   E_CONFIG_DD_FREE(_e_randr_edd);

   return 1;
}

/* public API functions */
EAPI Eina_Bool
e_randr_config_save(void)
{
   /* save the new config */
   return e_config_domain_save("e_randr", _e_randr_edd, e_randr_cfg);
}

/* local functions */
static Eina_Bool
_e_randr_config_load(void)
{
   Eina_Bool do_restore = EINA_TRUE;

   /* define edd for output config */
   _e_randr_output_edd =
     E_CONFIG_DD_NEW("E_Config_Randr_Output", E_Config_Randr_Output);
#undef T
#undef D
#define T E_Config_Randr_Output
#define D _e_randr_output_edd
   E_CONFIG_VAL(D, T, xid, UINT);
   E_CONFIG_VAL(D, T, crtc, UINT);
   E_CONFIG_VAL(D, T, orient, UINT);
   E_CONFIG_VAL(D, T, geo.x, INT);
   E_CONFIG_VAL(D, T, geo.y, INT);
   E_CONFIG_VAL(D, T, geo.w, INT);
   E_CONFIG_VAL(D, T, geo.h, INT);
   E_CONFIG_VAL(D, T, connect, UCHAR);

   /* define edd for randr config */
   _e_randr_edd = E_CONFIG_DD_NEW("E_Config_Randr", E_Config_Randr);
#undef T
#undef D
#define T E_Config_Randr
#define D _e_randr_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_LIST(D, T, outputs, _e_randr_output_edd);
   E_CONFIG_VAL(D, T, restore, UCHAR);
   E_CONFIG_VAL(D, T, config_timestamp, ULL);
   E_CONFIG_VAL(D, T, primary, UINT);

   /* try to load the randr config */
   if ((e_randr_cfg = e_config_domain_load("e_randr", _e_randr_edd)))
     {
        /* check randr config version */
        if (e_randr_cfg->version < (E_RANDR_CONFIG_FILE_EPOCH * 1000000))
          {
             /* config is too old */
             do_restore = EINA_FALSE;
             _e_randr_config_free();
             ecore_timer_add(1.0, _e_randr_config_cb_timer,
                             _("Settings data needed upgrading. Your old settings have<br>"
                               "been wiped and a new set of defaults initialized. This<br>"
                               "will happen regularly during development, so don't report a<br>"
                               "bug. This simply means Enlightenment needs new settings<br>"
                               "data by default for usable functionality that your old<br>"
                               "settings simply lack. This new set of defaults will fix<br>"
                               "that by adding it in. You can re-configure things now to your<br>"
                               "liking. Sorry for the hiccup in your settings.<br>"));
          }
        else if (e_randr_cfg->version > E_RANDR_CONFIG_FILE_VERSION)
          {
             /* config is too new */
             do_restore = EINA_FALSE;
             _e_randr_config_free();
             ecore_timer_add(1.0, _e_randr_config_cb_timer,
                             _("Your settings are NEWER than Enlightenment. This is very<br>"
                               "strange. This should not happen unless you downgraded<br>"
                               "Enlightenment or copied the settings from a place where<br>"
                               "a newer version of Enlightenment was running. This is bad and<br>"
                               "as a precaution your settings have been now restored to<br>"
                               "defaults. Sorry for the inconvenience.<br>"));
          }
     }

   /* if config was too old or too new, then reload a fresh one */
   if (!e_randr_cfg)
     {
        _e_randr_config_new();
        do_restore = EINA_FALSE;
     }
   if (!e_randr_cfg) return EINA_FALSE;

   _e_randr_load();

   if ((do_restore) && (e_randr_cfg->restore))
     _e_randr_apply();

   return EINA_TRUE;
}

static void
_e_randr_config_new(void)
{
   Ecore_X_Window root = 0;

   /* create new randr cfg */
   if (!(e_randr_cfg = E_NEW(E_Config_Randr, 1)))
     return;

   /* grab the root window once */
   root = ecore_x_window_root_first_get();

   /* set version */
   e_randr_cfg->version = E_RANDR_CONFIG_FILE_VERSION;

   /* by default, restore config */
   e_randr_cfg->restore = EINA_TRUE;

   /* remember current primary */
   e_randr_cfg->primary = ecore_x_randr_primary_output_get(root);

   /* update recorded config timestamp */
   e_randr_cfg->config_timestamp = ecore_x_randr_config_timestamp_get(root);
}

static void
_e_randr_config_free(void)
{
   E_Config_Randr_Output *output = NULL;

   /* safety check */
   if (!e_randr_cfg) return;

   /* loop the config outputs and free them */
   EINA_LIST_FREE(e_randr_cfg->outputs, output)
     {
        E_FREE(output);
     }

   /* free the config */
   E_FREE(e_randr_cfg);
}

static void
_e_randr_free(void)
{
   E_Randr_Crtc *crtc = NULL;
   E_Randr_Output *output = NULL;

   /* safety check */
   if (!e_randr) return;

   /* loop the crtcs and free them */
   EINA_LIST_FREE(e_randr->crtcs, crtc)
     {
        free(crtc);
     }

   /* loop the outputs and free them */
   EINA_LIST_FREE(e_randr->outputs, output)
     {
        free(output->name);
        free(output);
     }

   /* free the config */
   E_FREE(e_randr);
}

static Eina_Bool
_e_randr_config_cb_timer(void *data)
{
   e_util_dialog_show(_("Randr Settings Upgraded"), "%s", (char *)data);
   return EINA_FALSE;
}

/* function to map X's settings with E's settings */
static void
_e_randr_load(void)
{
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Output *outputs = NULL;
   int noutputs = 0;
   Ecore_X_Randr_Crtc *crtcs = NULL;
   int ncrtcs = 0, i = 0;

   e_randr = E_NEW(E_Randr, 1);
   if (!e_randr) return;

   /* grab the root window once */
   root = ecore_x_window_root_first_get();

   /* try to get the list of crtcs from x */
   if ((crtcs = ecore_x_randr_crtcs_get(root, &ncrtcs)))
     {
        /* loop the crtcs */
        for (i = 0; i < ncrtcs; i++)
          {
             E_Randr_Crtc *crtc;
             Ecore_X_Randr_Crtc_Info *cinfo;

             crtc = E_NEW(E_Randr_Crtc, 1);
             if (!crtc) continue;
             e_randr->crtcs = eina_list_append(e_randr->crtcs, crtc);
             crtc->xid = crtcs[i];

             /* get crtc info from X */
             if ((cinfo = ecore_x_randr_crtc_info_get(root, crtc->xid)))
               {
                  crtc->geo.x = cinfo->x;
                  crtc->geo.y = cinfo->y;
                  crtc->geo.w = cinfo->width;
                  crtc->geo.h = cinfo->height;
                  crtc->mode = cinfo->mode;
                  crtc->orient = cinfo->rotation;

                  ecore_x_randr_crtc_info_free(cinfo);
               }
          }

        free(crtcs);
     }

   /* try to get the list of outputs from x */
   if ((outputs = ecore_x_randr_outputs_get(root, &noutputs)))
     {
        int j = 0;

        for (j = 0; j < noutputs; j++)
          {
             E_Config_Randr_Output *output_cfg = NULL;
             E_Randr_Output *output = NULL;
             Ecore_X_Randr_Connection_Status status;

             output_cfg = _e_randr_config_output_find(outputs[j]);
             if (!output_cfg)
               output_cfg = _e_randr_config_output_new(outputs[j]);
             if (!output_cfg) continue;

             output = E_NEW(E_Randr_Output, 1);
             if (!output) continue;
             e_randr->outputs = eina_list_append(e_randr->outputs, output);
             output->cfg = output_cfg;

             output->name = ecore_x_randr_output_name_get(root, output->cfg->xid, NULL);
             output->is_lid = _e_randr_is_lid(output);

             status = ecore_x_randr_output_connection_status_get(root, output->cfg->xid);

             /* find a crtc if we want this output connected */
             if (output->cfg->connect && (status == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED))
               {
                  E_Randr_Crtc *crtc;

                  crtc = _e_randr_output_crtc_find(output);
                  if (crtc)
                    {
                       _e_randr_output_active_set(output, EINA_TRUE);

                       /* get orientation from crtc if not set */
                       if (!output->cfg->orient)
                         output->cfg->orient = crtc->orient;
                       /* find mode for output */
                       _e_randr_output_mode_update(output);
                    }
               }
          }

        free(outputs);
     }
   /* sort list by output id */
   e_randr_cfg->outputs = eina_list_sort(e_randr_cfg->outputs, -1, _e_randr_config_output_cmp);

   /* update lid status */
   _e_randr_lid_update();

   /* update primary output */
   _e_randr_config_primary_update();

   /* update recorded config timestamp */
   e_randr_cfg->config_timestamp = ecore_x_randr_config_timestamp_get(root);

   /* save the config */
   e_randr_config_save();
}

static void
_e_randr_apply(void)
{
   E_Randr_Crtc *crtc;
   E_Randr_Output *output;
   Eina_List *l, *ll;
   Ecore_X_Window root = 0;
   int nw = 0, nh = 0;
   int minw = 0, minh = 0;
   int maxw = 0, maxh = 0;
   int nx = INT_MAX, ny = INT_MAX;

   /* don't try to restore if we have fake screens */
   if (e_xinerama_fake_screens_exist()) return;

   /* grab the X server so that we can apply settings without triggering
    * any randr event updates until we are done */
   ecore_x_grab();

   /* get the root window */
   root = ecore_x_window_root_first_get();

   /* get the min and max screen size */
   ecore_x_randr_screen_size_range_get(root, &minw, &minh, &maxw, &maxh);

   /* loop our lists of crtcs */
   EINA_LIST_FOREACH(e_randr->crtcs, l, crtc)
     {
        int x = 0, y = 0, w = 0, h = 0;
        Ecore_X_Randr_Mode mode = 0;
        Ecore_X_Randr_Orientation orient = ECORE_X_RANDR_ORIENTATION_ROT_0;
        Eina_Rectangle rect;
        int count = 0;
        Ecore_X_Randr_Output *coutputs;

        /* disable crtc if no outputs */
        if (!crtc->outputs)
          {
             fprintf(stderr, "disable crtc: %d\n", crtc->xid);
             fprintf(stderr, "\t%d > %d\n", (x + w), maxw);
             fprintf(stderr, "\t%d > %d\n", (y + h), maxh);
             fprintf(stderr, "\t%d\n", mode);
             fprintf(stderr, "\t%p\n", crtc->outputs);
             ecore_x_randr_crtc_settings_set(root, crtc->xid, NULL, 0, 0, 0, 0,
                                             ECORE_X_RANDR_ORIENTATION_ROT_0);
             continue;
          }

        /* set config from connected outputs */
        _e_randr_crtc_from_outputs_set(crtc);

        x = crtc->geo.x;
        y = crtc->geo.y;
        mode = crtc->mode;
        orient = crtc->orient;

        /* at this point, we should have geometry, mode and orientation.
         * we can now proceed to calculate crtc size */
        rect = crtc->geo;
        _e_randr_config_mode_geometry(orient, &rect);
        w = rect.w;
        h = rect.h;

        /* if the crtc does not fit, disable it */
        if (((x + w) > maxw) || ((y + h) > maxh) || (mode == 0))
          {
             fprintf(stderr, "disable crtc: %d\n", crtc->xid);
             fprintf(stderr, "\t%d > %d\n", (x + w), maxw);
             fprintf(stderr, "\t%d > %d\n", (y + h), maxh);
             fprintf(stderr, "\t%d\n", mode);
             fprintf(stderr, "\t%p\n", crtc->outputs);
             ecore_x_randr_crtc_settings_set(root, crtc->xid, NULL, 0, 0, 0, 0,
                                             ECORE_X_RANDR_ORIENTATION_ROT_0);
             continue;
          }

        /* find outputs to enable on crtc */
        coutputs = calloc(eina_list_count(crtc->outputs), sizeof(Ecore_X_Randr_Output));
        if (!coutputs)
          {
             /* TODO: ERROR! */
             continue;
          }
        EINA_LIST_FOREACH(crtc->outputs, ll, output)
          {
             coutputs[count] = output->cfg->xid;
             count++;
          }

        /* get new size */
        if ((x + w) > nw) nw = x + w;
        if ((y + h) > nh) nh = y + h;
        if (x < nx) nx = x;
        if (y < ny) ny = y;

        /* apply our stored crtc settings */
        fprintf(stderr, "enable crtc: %d %d\n", crtc->xid, count);
        fprintf(stderr, "\t%dx%d+%d+%d\n", crtc->geo.w, crtc->geo.h, crtc->geo.x, crtc->geo.y);
        fprintf(stderr, "\t%d %d\n", crtc->mode, crtc->orient);
        ecore_x_randr_crtc_settings_set(root, crtc->xid, coutputs,
                                        count, crtc->geo.x, crtc->geo.y,
                                        crtc->mode, crtc->orient);

        /* cleanup */
        free(coutputs);
     }
   /* Adjust size according to origo */
   nw -= nx;
   nh -= ny;

   /* apply the new screen size */
   ecore_x_randr_screen_current_size_set(root, nw, nh, -1, -1);
   /* this moves screen to (0, 0) */
   ecore_x_randr_screen_reset(root);

   /* release the server grab */
   ecore_x_ungrab();
}

static Eina_Bool
_e_randr_event_cb_screen_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Screen_Change *ev;
   Eina_Bool changed = EINA_FALSE;
   Ecore_X_Randr_Output primary = 0;

   fprintf(stderr, "E_RANDR: _e_randr_event_cb_screen_change\n");
   ev = event;

   /* check if this event's root window is Our root window */
   if (ev->root != e_manager_current_get()->root)
     return ECORE_CALLBACK_RENEW;

   primary = ecore_x_randr_primary_output_get(ev->root);

   if (e_randr_cfg->primary != primary)
     {
        e_randr_cfg->primary = primary;
        changed = EINA_TRUE;
     }

   if (e_randr_cfg->config_timestamp != ev->config_time)
     {
        e_randr_cfg->config_timestamp = ev->config_time;
        changed = EINA_TRUE;
     }

   fprintf(stderr, "E_RANDR: changed: %d\n", changed);

   if (changed) e_randr_config_save();

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_randr_event_cb_crtc_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Randr_Crtc_Change *ev;
   E_Randr_Crtc *crtc;

   ev = event;
   fprintf(stderr, "E_RANDR: _e_randr_event_cb_crtc_change\n");
   fprintf(stderr, "  %d %d %d\n", ev->crtc, ev->mode, ev->orientation);
   fprintf(stderr, "  %dx%d+%d+%d\n", ev->geo.w, ev->geo.h, ev->geo.x, ev->geo.y);

   crtc = _e_randr_crtc_find(ev->crtc);

   if (!crtc)
     {
        fprintf(stderr, "E_RANDR: Weird, a new crtc?\n");
     }
   else
     {
        /* check (and update if needed) our crtc config */
        if ((ev->mode != crtc->mode) ||
            (ev->orientation != crtc->orient) ||
            (ev->geo.x != crtc->geo.x) || (ev->geo.y != crtc->geo.y) ||
            (ev->geo.w != crtc->geo.w) || (ev->geo.h != crtc->geo.h))
          {
             crtc->mode = ev->mode;
             crtc->orient = ev->orientation;
             crtc->geo = ev->geo;

             /* propagate changes to stored outputs */
             _e_randr_outputs_from_crtc_set(crtc);

             e_randr_config_save();
          }
     }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_randr_event_cb_output_change(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   Ecore_X_Event_Randr_Output_Change *ev;
   E_Randr_Output *output;
   Eina_Bool changed = EINA_FALSE;

   ev = event;
   fprintf(stderr, "E_RANDR: _e_randr_event_cb_output_change\n");
   fprintf(stderr, "  %d %d %d\n", ev->output, ev->crtc, ev->mode);
   fprintf(stderr, "  %d %d %d\n", ev->orientation, ev->connection, ev->subpixel_order);

   /* check if this event's root window is Our root window */
   if (ev->win != e_manager_current_get()->root)
     return ECORE_CALLBACK_RENEW;

   /* loop our crtcs and try to find this output */
   output = _e_randr_output_find(ev->output);
   if (!output)
     {
        fprintf(stderr, "E_RANDR: Weird, a new output?\n");
     }
   else
     {
        /* we know this output */
        if (output->is_lid && _e_randr_lid_is_closed)
          {
             /* ignore event from disconnected lid */
             fprintf(stderr, "E_RANDR: ignore event from closed lid\n");
          }
        else if (ev->connection == ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
          {
             E_Randr_Crtc *crtc = NULL;

             /* connected */
             if ((ev->crtc != 0) && (output->cfg->crtc != ev->crtc))
               {
                  fprintf(stderr, "E_RANDR: output changed crtc\n");
                  /* remove from old crtc */
                  _e_randr_output_active_set(output, EINA_FALSE);
                  /* set new crtc on output */
                  output->cfg->crtc = ev->crtc;
               }
             if (!output->active)
               {
                  fprintf(stderr, "E_RANDR: output connected to crtc\n");
                  crtc = _e_randr_output_crtc_find(output);
                  if (crtc)
                    {
                       /* connect to crtc */
                       _e_randr_output_active_set(output, EINA_TRUE);
                       /* get orientation from crtc if not set */
                       if (!output->cfg->orient)
                         output->cfg->orient = crtc->orient;
                       /* validate output mode */
                       _e_randr_output_mode_update(output);
                    }
                  changed = EINA_TRUE;
               }
          }
        else if (ev->connection == ECORE_X_RANDR_CONNECTION_STATUS_DISCONNECTED)
          {
             /* disconnected */
             if (output->active)
               {
                  fprintf(stderr, "E_RANDR: output disconnected: %s\n", output->name);
                  _e_randr_output_active_set(output, EINA_FALSE);
                  changed = EINA_TRUE;
               }
          }
     }

   /* save the config if anything changed or we added a new one */
   if (changed)
     {
        _e_randr_config_primary_update();
        _e_randr_apply();

        e_randr_config_save();
     }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_randr_event_cb_acpi(void *data EINA_UNUSED, int type EINA_UNUSED, void *event)
{
   E_Event_Acpi *ev;

   fprintf(stderr, "E_RANDR: _e_randr_event_cb_acpi\n");
   ev = event;

   if (ev->type == E_ACPI_TYPE_LID)
     {
        Eina_Bool changed;

        _e_randr_lid_is_closed = (ev->status == E_ACPI_LID_CLOSED);
        changed = _e_randr_lid_update();
        fprintf(stderr, "ch: %d\n", changed);
        if (changed)
          {
             /* force new evaluation of primary */
             e_randr_cfg->primary = 0;
             _e_randr_config_primary_update();
             _e_randr_apply();
          }
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_e_randr_output_mode_update(E_Randr_Output *output)
{
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Mode *modes = NULL;
   int nmodes = 0, pref = 0;
   Ecore_X_Randr_Mode_Info **mode_infos = NULL;
   int nmode_infos = 0;
   int i = 0;

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* get mode infos */
   mode_infos = ecore_x_randr_modes_info_get(root, &nmode_infos);
   if (nmode_infos == 0)
     goto error;
   /* get the list of modes for this output */
   modes = ecore_x_randr_output_modes_get(root, output->cfg->xid, &nmodes, &pref);
   if (nmodes == 0)
     goto error;

   /* try to find the matching mode */
   output->mode = 0;
   if ((output->cfg->geo.w != 0) && (output->cfg->geo.h != 0))
     {
        for (i = 0; i < nmode_infos; i++)
          {
             if ((mode_infos[i]->width == (unsigned int)output->cfg->geo.w) &&
                 (mode_infos[i]->height == (unsigned int)output->cfg->geo.h))
               {
                  output->mode = mode_infos[i]->xid;
                  break;
               }
          }
        /* check if mode is available */
        if (!_e_randr_output_mode_valid(output->mode, modes, nmodes))
          output->mode = 0;
     }

   /* see if we can use the mode of the crtc */
   if (!output->mode)
     {
        E_Randr_Crtc *crtc;

        crtc = _e_randr_crtc_find(output->cfg->crtc);
        if (crtc && crtc->mode)
          {
             if (_e_randr_output_mode_valid(crtc->mode, modes, nmodes))
               output->mode = crtc->mode;
             /* TODO: See if we have a mode of the same size with another mode id */
          }
     }

   /* set preferred mode */
   if (!output->mode)
     {
        if (pref > 0)
          output->mode = modes[pref - 1];
        else
          output->mode = modes[0];
     }

   /* set width and height from mode */
   for (i = 0; i < nmode_infos; i++)
     {
        if (mode_infos[i]->xid == output->mode)
          {
             output->cfg->geo.w = mode_infos[i]->width;
             output->cfg->geo.h = mode_infos[i]->height;
             break;
          }
     }

   if (modes) free(modes);
   if (mode_infos)
     {
        for (i = 0; i < nmode_infos; i++)
          free(mode_infos[i]);
        free(mode_infos);
     }
   return;
error:
   if (modes) free(modes);
   if (mode_infos)
     {
        for (i = 0; i < nmode_infos; i++)
          free(mode_infos[i]);
        free(mode_infos);
     }
   fprintf(stderr, "E_RANDR: reset %s = %dx%d+%d+%d\n",
           output->name,
           output->cfg->geo.w, output->cfg->geo.h,
           output->cfg->geo.x, output->cfg->geo.y);
   output->cfg->geo.x = output->cfg->geo.y = output->cfg->geo.w = output->cfg->geo.h = 0;
   output->mode = 0;
   return;
}

static E_Config_Randr_Output *
_e_randr_config_output_new(unsigned int id)
{
   E_Config_Randr_Output *cfg = NULL;

   if ((cfg = E_NEW(E_Config_Randr_Output, 1)))
     {
        Ecore_X_Window root = 0;

        /* assign output xid */
        cfg->xid = id;

        /* grab the root window */
        root = ecore_x_window_root_first_get();

        /* get the crtc for this output */
        cfg->crtc = ecore_x_randr_output_crtc_get(root, cfg->xid);

        /* all new outputs should connect automatically */
        /* TODO: config option */
        cfg->connect = EINA_TRUE;

        /* append this output config to randr config */
        e_randr_cfg->outputs =
           eina_list_append(e_randr_cfg->outputs, cfg);
     }

   return cfg;
}

static E_Config_Randr_Output *
_e_randr_config_output_find(Ecore_X_Randr_Output output)
{
   Eina_List *l;
   E_Config_Randr_Output *output_cfg;

   EINA_LIST_FOREACH(e_randr_cfg->outputs, l, output_cfg)
     {
        if (output_cfg->xid == output)
          return output_cfg;
     }

   return NULL;
}

static E_Randr_Crtc *
_e_randr_crtc_find(Ecore_X_Randr_Crtc xid)
{
   Eina_List *l;
   E_Randr_Crtc *crtc;

   EINA_LIST_FOREACH(e_randr->crtcs, l, crtc)
     {
        if (crtc->xid == xid)
          return crtc;
     }

   return NULL;
}

static E_Randr_Output *
_e_randr_output_find(Ecore_X_Randr_Output xid)
{
   Eina_List *l;
   E_Randr_Output *output;

   EINA_LIST_FOREACH(e_randr->outputs, l, output)
     {
        if (output->cfg->xid == xid)
          return output;
     }

   return NULL;
}

static E_Randr_Crtc *
_e_randr_output_crtc_find(E_Randr_Output *output)
{
   Ecore_X_Window root = 0;
   E_Randr_Crtc *crtc = NULL;
   Ecore_X_Randr_Crtc *possible = NULL;
   Ecore_X_Randr_Mode *modes = NULL;
   int num = 0, i = 0;
   int nmodes, pref;

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* check if last is available */
   if ((crtc = _e_randr_crtc_find(output->cfg->crtc)))
     {
        if (!crtc->outputs)
          goto done;
     }
   crtc = NULL;

   /* get a list of possible crtcs for this output */
   possible = ecore_x_randr_output_possible_crtcs_get(root, output->cfg->xid, &num);
   if (num == 0) goto error;

   /* loop the possible crtcs */
   for (i = 0; i < num; i++)
     {
	if ((crtc = _e_randr_crtc_find(possible[i])))
	  {
             if (!crtc->outputs)
               goto done;
          }
     }
   crtc = NULL;

   /* get the list of modes for this output */
   modes = ecore_x_randr_output_modes_get(root, output->cfg->xid, &nmodes, &pref);
   if (nmodes == 0)
     goto error;

   /* can we clone this output */
   for (i = 0; i < num; i++)
     {
        if ((crtc = _e_randr_crtc_find(possible[i])))
          {
             /* check if mode is valid for this output */
             if (_e_randr_output_mode_valid(crtc->mode, modes, nmodes))
               goto done;
          }
     }
   crtc = NULL;

done:
   free(possible);
   free(modes);

   if (crtc)
     output->cfg->crtc = crtc->xid;
   return crtc;

error:
   free(possible);
   free(modes);

   output->cfg->crtc = 0;
   return NULL;
}

static void
_e_randr_config_mode_geometry(Ecore_X_Randr_Orientation orient, Eina_Rectangle *rect)
{
   int mw = 0, mh = 0;

   /* based on orientation, calculate mode sizes */
   switch (orient)
     {
      case ECORE_X_RANDR_ORIENTATION_ROT_0:
      case ECORE_X_RANDR_ORIENTATION_ROT_180:
        mw = rect->w;
        mh = rect->h;
        break;
      case ECORE_X_RANDR_ORIENTATION_ROT_90:
      case ECORE_X_RANDR_ORIENTATION_ROT_270:
        mw = rect->h;
        mh = rect->w;
        break;
      default:
        break;
     }

   rect->x = 0;
   rect->y = 0;
   rect->w = mw;
   rect->h = mh;
}

static void
_e_randr_config_primary_update(void)
{
   Ecore_X_Window root = 0;
   Eina_List *l;
   E_Randr_Output *output;
   Ecore_X_Randr_Output primary = 0;

   /* check if we agree with system setup */
   root = ecore_x_window_root_first_get();
   primary = ecore_x_randr_primary_output_get(root);
   /* see if our selected primary exists and is connected */
   output = _e_randr_output_find(e_randr_cfg->primary);
   if ((!output) || (!output->active))
     e_randr_cfg->primary = 0;

   /* check if system primary is the same as ours */
   if ((primary > 0) && (primary == e_randr_cfg->primary)) return;

   if ((e_randr_cfg->primary > 0) &&
       output && output->active)
     {
        /* prefer our primary */
        ecore_x_randr_primary_output_set(root, e_randr_cfg->primary);
     }
   else
     {
        /* find lid */
        e_randr_cfg->primary = 0;
        EINA_LIST_FOREACH(e_randr->outputs, l, output)
          {
             if (output->is_lid)
               {
                  if (output->active)
                    e_randr_cfg->primary = output->cfg->xid;
                  break;
               }
          }
        if (!e_randr_cfg->primary)
          {
             /* TODO: prefer top-left active monitor */
             /* no lid, use first existing */
             EINA_LIST_FOREACH(e_randr->outputs, l, output)
               {
                  if (output->active)
                    {
                       e_randr_cfg->primary = output->cfg->xid;
                       break;
                    }
               }
          }
        /* set primary */
        ecore_x_randr_primary_output_set(root, e_randr_cfg->primary);
     }
}

static void
_e_randr_acpi_handler_add(void *data EINA_UNUSED)
{
   E_LIST_HANDLER_APPEND(_randr_event_handlers,
                         E_EVENT_ACPI,
                         _e_randr_event_cb_acpi, NULL);
}

static int
_e_randr_is_lid(E_Randr_Output *cfg)
{
   /* TODO: ecore_x_randr_output_connector_type_get */
   int ret = 0;

   if (!cfg->name) return 0;
   if (strstr(cfg->name, "LVDS")) ret = 1;
   else if (strstr(cfg->name, "lvds")) ret = 1;
   else if (strstr(cfg->name, "Lvds")) ret = 1;
   else if (strstr(cfg->name, "LCD")) ret = 1;
   return ret;
}

static void
_e_randr_outputs_from_crtc_set(E_Randr_Crtc *crtc)
{
   E_Randr_Output *output;
   Eina_List *l;

   EINA_LIST_FOREACH(crtc->outputs, l, output)
     {
        output->mode = crtc->mode;
        output->cfg->orient = crtc->orient;
        output->cfg->geo = crtc->geo;
        fprintf(stderr, "set from crtc: %d = %dx%d+%d+%d\n",
                crtc->xid,
                output->cfg->geo.w, output->cfg->geo.h,
                output->cfg->geo.x, output->cfg->geo.y);
     }
}

static void
_e_randr_crtc_from_outputs_set(E_Randr_Crtc *crtc)
{
   E_Randr_Output *output;
   Eina_List *l;

   EINA_LIST_FOREACH(crtc->outputs, l, output)
     {
        /* TODO: Match all connected outputs, not only the first */
        crtc->mode = output->mode;
        crtc->orient = output->cfg->orient;
        crtc->geo = output->cfg->geo;
        break;
     }
}

static Eina_Bool
_e_randr_lid_update(void)
{
   E_Randr_Output *output;
   Eina_List *l;
   Eina_Bool changed = EINA_FALSE;

   /* loop through connections to find lid */
   changed = EINA_FALSE;
   EINA_LIST_FOREACH(e_randr->outputs, l, output)
     {
        if (!output->is_lid) continue;
        /* only disable lid if we got more than 1 connected output */
        if ((_e_randr_lid_is_closed) && (output->active) && (e_randr->active > 1))
          {
             _e_randr_output_active_set(output, EINA_FALSE);
             changed = EINA_TRUE;
          }
        else if (!output->active)
          {
             _e_randr_output_active_set(output, EINA_TRUE);
             changed = EINA_TRUE;
          }
     }

   return changed;
}

static Eina_Bool
_e_randr_output_mode_valid(Ecore_X_Randr_Mode mode, Ecore_X_Randr_Mode *modes, int nmodes)
{
   Eina_Bool valid = EINA_FALSE;
   int i;

   for (i = 0; i < nmodes; i++)
     {
        if (modes[i] == mode)
          {
             valid = EINA_TRUE;
             break;
          }
     }
   return valid;
}

static void
_e_randr_output_active_set(E_Randr_Output *output, Eina_Bool active)
{
   E_Randr_Crtc *crtc;

   if (output->active == active) return;
   output->active = active;

   crtc = _e_randr_crtc_find(output->cfg->crtc);
   if (crtc)
     {
        if (active)
          {
             crtc->outputs =
                eina_list_append(crtc->outputs, output);
             e_randr->active++;
          }
        else
          {
             crtc->outputs =
                eina_list_remove(crtc->outputs, output);
             e_randr->active--;
          }
     }
}

static int
_e_randr_config_output_cmp(const void *a, const void *b)
{
   const E_Config_Randr_Output *cfg1 = a;
   const E_Config_Randr_Output *cfg2 = b;

   if (cfg1->xid < cfg2->xid) return -1;
   if (cfg1->xid > cfg2->xid) return 1;
   return 0;
}

