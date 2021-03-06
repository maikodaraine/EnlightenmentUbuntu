#ifdef E_TYPEDEFS
typedef enum _E_Screen_Limits
{
    E_SCREEN_LIMITS_PARTLY = 0,
    E_SCREEN_LIMITS_COMPLETELY = 1,
    E_SCREEN_LIMITS_WITHIN = 2
} E_Screen_Limits;

typedef enum _E_Icon_Preference
{
   E_ICON_PREF_E_DEFAULT,
   E_ICON_PREF_NETWM,
   E_ICON_PREF_USER
} E_Icon_Preference;

typedef enum _E_Direction
{
   E_DIRECTION_UP,
   E_DIRECTION_DOWN,
   E_DIRECTION_LEFT,
   E_DIRECTION_RIGHT
} E_Direction;

typedef enum _E_Transition
{
   E_TRANSITION_LINEAR = 0,
   E_TRANSITION_SINUSOIDAL = 1,
   E_TRANSITION_ACCELERATE = 2,
   E_TRANSITION_DECELERATE = 3,
   E_TRANSITION_ACCELERATE_LOTS = 4,
   E_TRANSITION_DECELERATE_LOTS = 5,
   E_TRANSITION_SINUSOIDAL_LOTS = 6,
   E_TRANSITION_BOUNCE = 7,
   E_TRANSITION_BOUNCE_LOTS = 8
} E_Transition;

typedef enum _E_Stacking
{
   E_STACKING_NONE,
   E_STACKING_ABOVE,
   E_STACKING_BELOW
} E_Stacking;

typedef enum _E_Focus_Policy
{
   E_FOCUS_CLICK,
   E_FOCUS_MOUSE,
   E_FOCUS_SLOPPY,
   E_FOCUS_LAST,
} E_Focus_Policy;

typedef enum
{
   /* same as ecore-x types */
   E_WINDOW_TYPE_UNKNOWN = 0,
   E_WINDOW_TYPE_DESKTOP,
   E_WINDOW_TYPE_DOCK,
   E_WINDOW_TYPE_TOOLBAR,
   E_WINDOW_TYPE_MENU,
   E_WINDOW_TYPE_UTILITY,
   E_WINDOW_TYPE_SPLASH,
   E_WINDOW_TYPE_DIALOG,
   E_WINDOW_TYPE_NORMAL,
   E_WINDOW_TYPE_DROPDOWN_MENU,
   E_WINDOW_TYPE_POPUP_MENU,
   E_WINDOW_TYPE_TOOLTIP,
   E_WINDOW_TYPE_NOTIFICATION,
   E_WINDOW_TYPE_COMBO,
   E_WINDOW_TYPE_DND
} E_Window_Type;

typedef enum _E_Urgency_Policy
{
   E_ACTIVEHINT_POLICY_IGNORE,
   E_ACTIVEHINT_POLICY_ANIMATE,
   E_ACTIVEHINT_POLICY_ACTIVATE,
   E_ACTIVEHINT_POLICY_ACTIVATE_EXCLUDE,
   E_ACTIVEHINT_POLICY_LAST,
} E_Urgency_Policy;

typedef enum _E_Focus_Setting
{
   E_FOCUS_NONE,
   E_FOCUS_NEW_WINDOW,
   E_FOCUS_NEW_DIALOG,
   E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED
} E_Focus_Setting;

typedef enum _E_Maximize
{
   E_MAXIMIZE_NONE = 0x00000000,
   E_MAXIMIZE_FULLSCREEN = 0x00000001,
   E_MAXIMIZE_SMART = 0x00000002,
   E_MAXIMIZE_EXPAND = 0x00000003,
   E_MAXIMIZE_FILL = 0x00000004,
   E_MAXIMIZE_TYPE = 0x0000000f,
   E_MAXIMIZE_VERTICAL = 0x00000010,
   E_MAXIMIZE_HORIZONTAL = 0x00000020,
   E_MAXIMIZE_BOTH = 0x00000030,
   E_MAXIMIZE_LEFT = 0x00000070,
   E_MAXIMIZE_RIGHT = 0x000000b0,
   E_MAXIMIZE_DIRECTION = 0x000000f0
} E_Maximize;

typedef enum _E_Fullscreen
{
   /* Resize window */
   E_FULLSCREEN_RESIZE,
   /* Change screen resoultion and resize window */
   E_FULLSCREEN_ZOOM
} E_Fullscreen;

typedef enum _E_Window_Placement
{
   E_WINDOW_PLACEMENT_SMART,
   E_WINDOW_PLACEMENT_ANTIGADGET,
   E_WINDOW_PLACEMENT_CURSOR,
   E_WINDOW_PLACEMENT_MANUAL
} E_Window_Placement;

typedef enum E_Client_Property
{
   E_CLIENT_PROPERTY_NONE = 0,
   E_CLIENT_PROPERTY_SIZE = (1 << 0),
   E_CLIENT_PROPERTY_POS = (1 << 1),
   E_CLIENT_PROPERTY_TITLE = (1 << 2),
   E_CLIENT_PROPERTY_ICON = (1 << 3),
   E_CLIENT_PROPERTY_URGENCY = (1 << 4),
   E_CLIENT_PROPERTY_GRAVITY = (1 << 5),
   E_CLIENT_PROPERTY_NETWM_STATE = (1 << 6),
   E_CLIENT_PROPERTY_STICKY = (1 << 7),
} E_Client_Property;

typedef struct E_Client E_Client;

typedef struct E_Event_Client E_Event_Client;
typedef struct _E_Event_Client_Property E_Event_Client_Property;
typedef struct _E_Client_Pending_Resize E_Client_Pending_Resize;
typedef struct E_Event_Client_Zone_Set E_Event_Client_Zone_Set;
typedef struct E_Event_Client_Desk_Set E_Event_Client_Desk_Set;
typedef struct _E_Client_Hook E_Client_Hook;

typedef enum _E_Client_Hook_Point
{
   E_CLIENT_HOOK_EVAL_PRE_FETCH,
   E_CLIENT_HOOK_EVAL_FETCH,
   E_CLIENT_HOOK_EVAL_PRE_POST_FETCH,
   E_CLIENT_HOOK_EVAL_POST_FETCH,
   E_CLIENT_HOOK_EVAL_PRE_FRAME_ASSIGN,
   E_CLIENT_HOOK_EVAL_POST_FRAME_ASSIGN,
   E_CLIENT_HOOK_EVAL_PRE_NEW_CLIENT,
   E_CLIENT_HOOK_EVAL_POST_NEW_CLIENT,
   E_CLIENT_HOOK_EVAL_END,
   E_CLIENT_HOOK_FOCUS_SET,
   E_CLIENT_HOOK_FOCUS_UNSET,
   E_CLIENT_HOOK_NEW_CLIENT,
   E_CLIENT_HOOK_DESK_SET,
   E_CLIENT_HOOK_MOVE_BEGIN,
   E_CLIENT_HOOK_MOVE_UPDATE,
   E_CLIENT_HOOK_MOVE_END,
   E_CLIENT_HOOK_RESIZE_BEGIN,
   E_CLIENT_HOOK_RESIZE_UPDATE,
   E_CLIENT_HOOK_RESIZE_END,
   E_CLIENT_HOOK_DEL,
   E_CLIENT_HOOK_UNREDIRECT,
   E_CLIENT_HOOK_REDIRECT,
} E_Client_Hook_Point;

typedef void (*E_Client_Move_Intercept_Cb)(E_Client *, int x, int y);
typedef void (*E_Client_Hook_Cb)(void *data, E_Client *ec);
#else

#define E_CLIENT_TYPE (int)0xE0b01002

struct E_Event_Client
{
   E_Client *ec;
};

struct E_Event_Client_Desk_Set
{
   E_Client *ec;
   E_Desk *desk;
};

struct E_Event_Client_Zone_Set
{
   E_Client *ec;
   E_Zone *zone;
};

struct _E_Event_Client_Property
{
   E_Client *ec;
   unsigned int property;
};

struct _E_Client_Hook
{
   E_Client_Hook_Point hookpoint;
   E_Client_Hook_Cb func;
   void               *data;
   unsigned char       delete_me : 1;
};

struct _E_Client_Pending_Resize
{
   int           w, h;
   unsigned int  serial;
};

struct E_Client
{
   E_Object e_obj_inherit;
   EINA_INLIST;

   E_Pixmap *pixmap;
   E_Comp *comp;
   int depth;
   int x, y, w, h; //frame+client geom
   Eina_Rectangle client; //client geom
   Evas_Object *frame; //comp object
   E_Zone *zone;
   E_Desk *desk;

   Ecore_Poller              *ping_poller;
   Ecore_Timer               *kill_timer;

   E_Client                  *modal;

   E_Client                  *leader;
   Eina_List                 *group;

   E_Client                  *parent;
   Eina_List                 *transients;

   E_Layer                    layer;

   Eina_Rectangle           *shape_rects;
   unsigned int              shape_rects_num;

   Eina_Rectangle           *shape_input_rects;
   unsigned int              shape_input_rects_num;

   Eina_Stringshare         *internal_icon;
   Eina_Stringshare         *internal_icon_key;

   E_Direction               shade_dir;

   E_Comp_Client_Data       *comp_data; //private for the compositor engine (X, Wayland) ONLY

   Evas_Object *input_object; //for running wayland clients in X

   E_Action                  *cur_mouse_action;

   int               border_size; //size of client's border

   struct
   {
      struct
      {
         int x, y, w, h;
         int mx, my;
      } current, last_down[3], last_up[3];
   } mouse;

   struct
   {
      struct
      {
         int x, y, w, h;
         int mx, my;
         int button;
      } down;
   } moveinfo;

   unsigned char      ignore_first_unmap;
   E_Pointer_Mode     resize_mode;

   struct
   {
      Eina_Bool mapping_change : 1;
      Eina_Bool iconic_shading : 1;
   } hacks;

   struct
   {
      unsigned char changed : 1;
      unsigned char user_selected : 1;
      Eina_Stringshare *name;
      } border;

   struct
   {
      int          x, y, w, h;
      E_Layer      layer;
      int          zone;
      E_Maximize   maximized;
   } saved;

   struct
   {
      unsigned char valid : 1;
      int           x, y, w, h;
      struct
      {
         int x, y, w, h;
      } saved;
   } pre_res_change;

   unsigned char  shaped : 1;
   unsigned char  argb : 1;

   /* ICCCM */
   struct
   {
      Eina_Stringshare        *title;
      Eina_Stringshare        *name;
      Eina_Stringshare        *class;
      Eina_Stringshare        *icon_name;
      Eina_Stringshare        *machine;
      int                       min_w, min_h;
      int                       max_w, max_h;
      int                       base_w, base_h;
      int                       step_w, step_h;
      int                       start_x, start_y;
      double                    min_aspect, max_aspect;
      Ecore_Window            icon_window;
      Ecore_Window            window_group;
      Ecore_Window            transient_for;
      Ecore_Window            client_leader;
#ifdef E_COMP_X_H
      Ecore_X_Window_State_Hint initial_state;
      Ecore_X_Window_State_Hint state;
      Ecore_X_Pixmap            icon_pixmap;
      Ecore_X_Pixmap            icon_mask;
      Ecore_X_Gravity           gravity;
#endif
      Eina_Stringshare         *window_role;
      unsigned char             take_focus : 1;
      unsigned char             accepts_focus : 1;
      unsigned char             urgent : 1;
      unsigned char             delete_request : 1;
      unsigned char             request_pos : 1;
      struct
      {
         int    argc;
         char **argv;
      } command;
      struct
      {
         unsigned char title : 1;
         unsigned char name_class : 1;
         unsigned char icon_name : 1;
         unsigned char machine : 1;
         unsigned char hints : 1;
         unsigned char size_pos_hints : 1;
         unsigned char protocol : 1;
         unsigned char transient_for : 1;
         unsigned char client_leader : 1;
         unsigned char window_role : 1;
         unsigned char state : 1;
         unsigned char command : 1;
      } fetch;
   } icccm;

   /* MWM */
   struct
   {
#ifdef E_COMP_X_H
      Ecore_X_MWM_Hint_Func  func;
      Ecore_X_MWM_Hint_Decor decor;
      Ecore_X_MWM_Hint_Input input;
#endif
      unsigned char          exists : 1;
      unsigned char          borderless : 1;
      struct
      {
         unsigned char hints : 1;
      } fetch;
   } mwm;

   /* NetWM */
   struct
   {
      pid_t         pid;
      unsigned int  desktop;
      Eina_Stringshare *name;
      Eina_Stringshare *icon_name;
#ifdef E_COMP_X_H
      Ecore_X_Icon *icons;
#endif
      int           num_icons;
      unsigned int  user_time;
      unsigned char opacity;
      Eina_Bool     opacity_changed : 1; // prevent fetching opacity next prop change
      struct
      {
         int left;
         int right;
         int top;
         int bottom;
         int left_start_y;
         int left_end_y;
         int right_start_y;
         int right_end_y;
         int top_start_x;
         int top_end_x;
         int bottom_start_x;
         int bottom_end_x;
      } strut;
      unsigned char ping : 1;
      struct
      {
         unsigned char        request : 1;
         unsigned char        alarm : 1;
         unsigned int         wait;
         unsigned int         serial;
         double               send_time;
      } sync;

      /* NetWM Window state */
      struct
      {
         unsigned char modal : 1;
         unsigned char sticky : 1;
         unsigned char maximized_v : 1;
         unsigned char maximized_h : 1;
         unsigned char shaded : 1;
         unsigned char skip_taskbar : 1;
         unsigned char skip_pager : 1;
         unsigned char hidden : 1;
         unsigned char fullscreen : 1;
         E_Stacking    stacking;
      } state;

      /* NetWM Window allowed actions */
      struct
      {
         unsigned char move : 1;
         unsigned char resize : 1;
         unsigned char minimize : 1;
         unsigned char shade : 1;
         unsigned char stick : 1;
         unsigned char maximized_h : 1;
         unsigned char maximized_v : 1;
         unsigned char fullscreen : 1;
         unsigned char change_desktop : 1;
         unsigned char close : 1;
      } action;
      E_Window_Type  type;
      E_Window_Type *extra_types;
      int                  extra_types_num;
      int                  startup_id;

      struct
      {
         unsigned char name : 1;
         unsigned char icon_name : 1;
         unsigned char icon : 1;
         unsigned char user_time : 1;
         unsigned char strut : 1;
         unsigned char type : 1;
         unsigned char state : 1;
         unsigned char opacity : 1;
         /* No, fetch on new_client, shouldn't be changed after map.
            unsigned char pid : 1;
          */
         /* No, ignore this
            unsigned char desktop : 1;
          */
      } fetch;

      struct
      {
         unsigned char state : 1;
      } update;
   } netwm;

   /* Extra e stuff */
   struct
   {
      struct
      {
         struct
         {
            int           x, y;

            unsigned char updated : 1;
         } video_position;
         Ecore_Window video_parent;
         E_Client      *video_parent_client;
         Eina_List     *video_child;
         struct
         {
            Eina_Stringshare *name;
            Eina_Stringshare **available_list;
            Eina_Stringshare *set;
            int             num;
            unsigned char   wait_for_done : 1;
            unsigned char   use : 1;
         } profile;
         unsigned char  centered : 1;
         unsigned char  video : 1;
      } state;

      struct
      {
         unsigned char state : 1;
         unsigned char video_parent : 1;
         unsigned char video_position : 1;
         unsigned char profile : 1;
      } fetch;
   } e;

   struct
   {
      struct
      {
         unsigned char soft_menu : 1;
         unsigned char soft_menus : 1;
      } fetch;

      unsigned char soft_menu : 1;
      unsigned char soft_menus : 1;
   } qtopia;

   struct
   {
      struct
      {
         unsigned char state : 1;
         unsigned char vkbd : 1;
      } fetch;
#ifdef E_COMP_X_H
      Ecore_X_Virtual_Keyboard_State state;
#endif
      unsigned char                  have_property : 1;
      unsigned char                  vkbd : 1;
   } vkbd;

   struct
   {
      unsigned char visible : 1;
      unsigned char pos : 1;
      unsigned char size : 1;
      unsigned char stack : 1;
      unsigned char prop : 1;
      unsigned char border : 1;
      unsigned char reset_gravity : 1;
      unsigned char shading : 1;
      unsigned char shaded : 1;
      unsigned char shape : 1;
      unsigned char shape_input : 1;
      unsigned char icon : 1;
      Eina_Bool internal_props : 1;
      Eina_Bool internal_state : 1;
      Eina_Bool need_maximize : 1;
      Eina_Bool need_unmaximize : 1;
   } changes;

   unsigned int       visible : 1;
   Eina_Bool          hidden : 1; // set when window has been hidden by api and should not be shown
   unsigned int       await_hide_event;
   unsigned int       moving : 1;
   unsigned int       focused : 1;
   unsigned int       new_client : 1;
   unsigned int       re_manage : 1;
   unsigned int       placed : 1;
   unsigned int       shading : 1;
   unsigned int       shaded : 1;
   unsigned int       iconic : 1;
   unsigned int       deskshow : 1;
   unsigned int       sticky : 1;
   unsigned int       shaped_input : 1;
   unsigned int       need_shape_merge : 1;
   unsigned int       need_shape_export : 1;
   unsigned int       fullscreen : 1;
   unsigned int       need_fullscreen : 1;
   unsigned int       already_unparented : 1;
   unsigned int       need_reparent : 1;
   unsigned int       button_grabbed : 1;
   unsigned int       delete_requested : 1;
   unsigned int       ping_ok : 1;
   unsigned int       hung : 1;
   unsigned int       take_focus : 1;
   unsigned int       want_focus : 1;
   unsigned int       user_skip_winlist : 1;
   E_Maximize         maximized;
   E_Fullscreen       fullscreen_policy;
   unsigned int       borderless : 1;
   unsigned char      offer_resistance : 1;
   Eina_Stringshare  *bordername;

   unsigned int       lock_user_location : 1; /*DONE*/
   unsigned int       lock_client_location : 1; /*DONE*/
   unsigned int       lock_user_size : 1; /*DONE*/
   unsigned int       lock_client_size : 1; /*DONE*/
   unsigned int       lock_user_stacking : 1; /*DONE*/
   unsigned int       lock_client_stacking : 1; /*DONE*/
   unsigned int       lock_user_iconify : 1; /*DONE*/
   unsigned int       lock_client_iconify : 1; /*DONE*/
   unsigned int       lock_user_desk : 1;
   unsigned int       lock_client_desk : 1;
   unsigned int       lock_user_sticky : 1; /*DONE*/
   unsigned int       lock_client_sticky : 1; /*DONE*/
   unsigned int       lock_user_shade : 1; /*DONE*/
   unsigned int       lock_client_shade : 1; /*DONE*/
   unsigned int       lock_user_maximize : 1; /*DONE*/
   unsigned int       lock_client_maximize : 1; /*DONE*/
   unsigned int       lock_user_fullscreen : 1; /*DONE*/
   unsigned int       lock_client_fullscreen : 1; /*DONE*/
   unsigned int       lock_border : 1; /*DONE*/
   unsigned int       lock_close : 1; /*DONE*/
   unsigned int       lock_focus_in : 1; /*DONE*/
   unsigned int       lock_focus_out : 1; /*DONE*/
   unsigned int       lock_life : 1; /*DONE*/

   unsigned int       stolen : 1;

   unsigned int       internal : 1;
   unsigned int       internal_no_remember : 1;
   unsigned int       internal_no_reopen : 1;
   Eina_Bool          theme_shadow : 1;

   Ecore_Evas        *internal_ecore_evas;

   double             ping;

   unsigned char      changed : 1;

   unsigned char      icon_preference;

   struct
   {
      int x, y;
      int modified;
   } shelf_fix;

   Eina_List       *stick_desks;
   E_Menu          *border_menu;
   E_Config_Dialog *border_locks_dialog;
   E_Config_Dialog *border_remember_dialog;
   E_Config_Dialog *border_border_dialog;
   E_Dialog        *border_prop_dialog;
   Eina_List       *pending_resize;

   struct
   {
      unsigned char start : 1;
      int           x, y;
   } drag;

   Ecore_Timer               *raise_timer;
   E_Client_Move_Intercept_Cb move_intercept_cb;
   E_Remember                *remember;

   Efreet_Desktop            *desktop;
   E_Exec_Instance           *exe_inst;

   unsigned char              comp_hidden   : 1;

   unsigned char              post_move   : 1;
   unsigned char              post_resize : 1;
   unsigned char              post_show : 1;
   unsigned char              during_lost : 1;

   Ecore_Idle_Enterer        *post_job;

   E_Focus_Policy             focus_policy_override;

   Eina_Bool override : 1;
   Eina_Bool input_only : 1;
   Eina_Bool dialog : 1;
   Eina_Bool tooltip : 1;
   Eina_Bool redirected : 1;
   Eina_Bool unredirected_single : 1; //window has been selectively unredirected
   Eina_Bool shape_changed : 1;
   Eina_Bool layer_block : 1; // client is doing crazy stuff and should not be relayered in protocol
   Eina_Bool ignored : 1; // client is comp-ignored
   Eina_Bool no_shape_cut : 1; // client shape should not be cut
   Eina_Bool maximize_override : 1; // client is doing crazy stuff and should "just do it" when moving/resizing
};

#define e_client_focus_policy_click(ec) \
  ((ec->focus_policy_override == E_FOCUS_CLICK) || (e_config->focus_policy == E_FOCUS_CLICK))

/* macro for finding misuse of changed flag */
#if 0
# define EC_CHANGED(EC) \
  do { \
     if (e_object_is_del(E_OBJECT(EC))) \
       EINA_LOG_CRIT("CHANGED SET ON DELETED CLIENT!"); \
     EC->changed = 1; \
     INF("%s:%d - EC CHANGED: %p", __FILE__, __LINE__, EC); \
  } while (0)
#else
# define EC_CHANGED(EC) EC->changed = 1
#endif

#define E_CLIENT_FOREACH(COMP, EC) \
  for (EC = e_client_bottom_get(COMP); EC; EC = e_client_above_get(EC))

#define E_CLIENT_REVERSE_FOREACH(COMP, EC) \
  for (EC = e_client_top_get(COMP); EC; EC = e_client_below_get(EC))


EAPI extern int E_EVENT_CLIENT_ADD;
EAPI extern int E_EVENT_CLIENT_REMOVE;
EAPI extern int E_EVENT_CLIENT_ZONE_SET;
EAPI extern int E_EVENT_CLIENT_DESK_SET;
EAPI extern int E_EVENT_CLIENT_RESIZE;
EAPI extern int E_EVENT_CLIENT_MOVE;
EAPI extern int E_EVENT_CLIENT_SHOW;
EAPI extern int E_EVENT_CLIENT_HIDE;
EAPI extern int E_EVENT_CLIENT_ICONIFY;
EAPI extern int E_EVENT_CLIENT_UNICONIFY;
EAPI extern int E_EVENT_CLIENT_STACK;
EAPI extern int E_EVENT_CLIENT_FOCUS_IN;
EAPI extern int E_EVENT_CLIENT_FOCUS_OUT;
EAPI extern int E_EVENT_CLIENT_PROPERTY;
EAPI extern int E_EVENT_CLIENT_FULLSCREEN;
EAPI extern int E_EVENT_CLIENT_UNFULLSCREEN;


EINTERN void e_client_idler_before(void);
EINTERN Eina_Bool e_client_init(void);
EINTERN void e_client_shutdown(void);
EAPI E_Client *e_client_new(E_Comp *c, E_Pixmap *cp, int first_map, int internal);
EAPI void e_client_desk_set(E_Client *ec, E_Desk *desk);
EAPI Eina_Bool e_client_comp_grabbed_get(void);
EAPI E_Client *e_client_action_get(void);
EAPI E_Client *e_client_warping_get(void);
EAPI Eina_List *e_clients_immortal_list(const E_Comp *c);
EAPI void e_client_mouse_in(E_Client *ec, int x, int y);
EAPI void e_client_mouse_out(E_Client *ec, int x, int y);
EAPI void e_client_mouse_wheel(E_Client *ec, Evas_Point *output, E_Binding_Event_Wheel *ev);
EAPI void e_client_mouse_down(E_Client *ec, int button, Evas_Point *output, E_Binding_Event_Mouse_Button *ev);
EAPI void e_client_mouse_up(E_Client *ec, int button, Evas_Point *output, E_Binding_Event_Mouse_Button* ev);
EAPI void e_client_mouse_move(E_Client *ec, Evas_Point *output);
EAPI void e_client_res_change_geometry_save(E_Client *bd);
EAPI void e_client_res_change_geometry_restore(E_Client *ec);
EAPI void e_client_zone_set(E_Client *ec, E_Zone *zone);
EAPI void e_client_geometry_get(E_Client *ec, int *x, int *y, int *w, int *h);
EAPI E_Client *e_client_above_get(const E_Client *ec);
EAPI E_Client *e_client_below_get(const E_Client *ec);
EAPI E_Client *e_client_bottom_get(const E_Comp *c);
EAPI E_Client *e_client_top_get(const E_Comp *c);
EAPI unsigned int e_clients_count(E_Comp *c);
EAPI void e_client_move_intercept_cb_set(E_Client *ec, E_Client_Move_Intercept_Cb cb);
EAPI E_Client_Hook *e_client_hook_add(E_Client_Hook_Point hookpoint, E_Client_Hook_Cb func, const void *data);
EAPI void e_client_hook_del(E_Client_Hook *ch);
EAPI void e_client_focus_latest_set(E_Client *ec);
EAPI void e_client_raise_latest_set(E_Client *ec);
EAPI Eina_Bool e_client_focus_track_enabled(void);
EAPI void e_client_focus_track_freeze(void);
EAPI void e_client_focus_track_thaw(void);
EAPI void e_client_refocus(void);
EAPI void e_client_focus_set_with_pointer(E_Client *ec);
EAPI void e_client_activate(E_Client *ec, Eina_Bool just_do_it);
EAPI E_Client *e_client_focused_get(void);
EAPI Eina_List *e_client_focus_stack_get(void);
EAPI Eina_List *e_client_raise_stack_get(void);
EAPI Eina_List *e_client_lost_windows_get(E_Zone *zone);
EAPI void e_client_shade(E_Client *ec, E_Direction dir);
EAPI void e_client_unshade(E_Client *ec, E_Direction dir);
EAPI void e_client_maximize(E_Client *ec, E_Maximize max);
EAPI void e_client_unmaximize(E_Client *ec, E_Maximize max);
EAPI void e_client_fullscreen(E_Client *ec, E_Fullscreen policy);
EAPI void e_client_unfullscreen(E_Client *ec);
EAPI void e_client_iconify(E_Client *ec);
EAPI void e_client_uniconify(E_Client *ec);
EAPI void e_client_urgent_set(E_Client *ec, Eina_Bool urgent);
EAPI void e_client_stick(E_Client *ec);
EAPI void e_client_unstick(E_Client *ec);
EAPI void e_client_pinned_set(E_Client *ec, Eina_Bool set);
EAPI void e_client_comp_hidden_set(E_Client *ec, Eina_Bool hidden);
EAPI void e_client_act_move_keyboard(E_Client *ec);
EAPI void e_client_act_resize_keyboard(E_Client *ec);
EAPI void e_client_act_move_begin(E_Client *ec, E_Binding_Event_Mouse_Button *ev);
EAPI void e_client_act_move_end(E_Client *ec, E_Binding_Event_Mouse_Button *ev EINA_UNUSED);
EAPI void e_client_act_resize_begin(E_Client *ec, E_Binding_Event_Mouse_Button *ev);
EAPI void e_client_act_resize_end(E_Client *ec, E_Binding_Event_Mouse_Button *ev EINA_UNUSED);
EAPI void e_client_act_menu_begin(E_Client *ec, E_Binding_Event_Mouse_Button *ev, int key);
EAPI void e_client_act_close_begin(E_Client *ec);
EAPI void e_client_act_kill_begin(E_Client *ec);
EAPI Evas_Object *e_client_icon_add(E_Client *ec, Evas *evas);
EAPI void e_client_ping(E_Client *cw);
EAPI void e_client_move_cancel(void);
EAPI void e_client_resize_cancel(void);
EAPI Eina_Bool e_client_resize_begin(E_Client *ec);
EAPI void e_client_frame_recalc(E_Client *ec);
EAPI void e_client_signal_move_begin(E_Client *ec, const char *sig, const char *src EINA_UNUSED);
EAPI void e_client_signal_move_end(E_Client *ec, const char *sig EINA_UNUSED, const char *src EINA_UNUSED);
EAPI void e_client_signal_resize_begin(E_Client *ec, const char *dir, const char *sig, const char *src EINA_UNUSED);
EAPI void e_client_signal_resize_end(E_Client *ec, const char *dir EINA_UNUSED, const char *sig EINA_UNUSED, const char *src EINA_UNUSED);
EAPI void e_client_resize_limit(E_Client *ec, int *w, int *h);
EAPI E_Client *e_client_under_pointer_get(E_Desk *desk, E_Client *exclude);
EAPI int e_client_pointer_warp_to_center_now(E_Client *ec);
EAPI int e_client_pointer_warp_to_center(E_Client *ec);
EAPI void e_client_redirected_set(E_Client *ec, Eina_Bool set);
EAPI Eina_Bool e_client_is_stacking(const E_Client *ec);
#include "e_client.x"
#endif
