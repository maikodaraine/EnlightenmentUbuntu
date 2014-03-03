#ifndef EVAS_ENGINE_H
# define EVAS_ENGINE_H

# include "config.h"

/* NB: This already includes wayland-client.h */
# include <wayland-egl.h>

# ifdef GL_GLES
#  include <EGL/egl.h>
/* NB: These are already included from gl_common */
/* #  include <GLES2/gl2.h> */
/* #  include <GLES2/gl2ext.h> */
# endif

# include "evas_common_private.h"
# include "evas_private.h"
# include "evas_gl_common.h"
# include "Evas.h"
# include "Evas_Engine_Wayland_Egl.h"

# define GL_GLEXT_PROTOTYPES

extern int _evas_engine_wl_egl_log_dom;

# ifdef ERR
#  undef ERR
# endif
# define ERR(...) EINA_LOG_DOM_ERR(_evas_engine_wl_egl_log_dom, __VA_ARGS__)

# ifdef DBG
#  undef DBG
# endif
# define DBG(...) EINA_LOG_DOM_DBG(_evas_engine_wl_egl_log_dom, __VA_ARGS__)

# ifdef INF
#  undef INF
# endif
# define INF(...) EINA_LOG_DOM_INFO(_evas_engine_wl_egl_log_dom, __VA_ARGS__)

# ifdef WRN
#  undef WRN
# endif
# define WRN(...) EINA_LOG_DOM_WARN(_evas_engine_wl_egl_log_dom, __VA_ARGS__)

# ifdef CRI
#  undef CRI
# endif
# define CRI(...) EINA_LOG_DOM_CRIT(_evas_engine_wl_egl_log_dom, __VA_ARGS__)

typedef struct _Evas_GL_Wl_Window Evas_GL_Wl_Window;

struct _Evas_GL_Wl_Window
{
   struct wl_display *disp;
   struct wl_egl_window *win;
   struct wl_surface *surface;
   int w, h;
   int depth, screen, rot, alpha;

   Evas_Engine_GL_Context *gl_context;

   struct 
     {
        Eina_Bool drew : 1;
     } draw;

#ifdef GL_GLES
   EGLContext egl_context[1];
   EGLSurface egl_surface[1];
   EGLConfig egl_config;
   EGLDisplay egl_disp;
#endif

   Eina_Bool surf : 1;
};

Evas_GL_Wl_Window *eng_window_new(struct wl_display *disp, struct wl_surface *surface, int screen, int depth, int w, int h, int indirect, int alpha, int rot);
void eng_window_free(Evas_GL_Wl_Window *gw);
void eng_window_use(Evas_GL_Wl_Window *gw);
void eng_window_unsurf(Evas_GL_Wl_Window *gw);
void eng_window_resurf(Evas_GL_Wl_Window *gw);
Eina_Bool eng_window_make_current(void *data, void *doit);

#endif
