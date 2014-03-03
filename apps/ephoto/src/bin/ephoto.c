#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <Elementary.h>
#ifndef ELM_LIB_QUICKLAUNCH

#include "ephoto.h"

static void _ephoto_display_usage(void);

/* Global log domain pointer */
int __log_domain = -1;

EAPI int
elm_main(int argc, char **argv)
{
//   Ethumb_Client *client;
   int r = 0;

#if ENABLE_NLS
   setlocale(LC_ALL, "");
   bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
   bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
   textdomain(GETTEXT_PACKAGE);
#endif

   eio_init();
   elm_need_efreet();
   elm_need_ethumb();
   elm_init(argc, argv);

   __log_domain = eina_log_domain_register("ephoto", EINA_COLOR_ORANGE);
   if (!__log_domain)
     {
        EINA_LOG_ERR("Could not register log domain: Ephoto");
        r = 1;
        goto end_log_domain;
     }

   if (!efreet_mime_init())
     ERR("Could not init efreet_mime!");

   elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

   DBG("Logging initialized");
   if (argc > 2)
     {
        printf("Too Many Arguments!\n");
        _ephoto_display_usage();
        r = 1;
        goto end;
     }
   else if (argc < 2)
     {
        Evas_Object *win = ephoto_window_add(NULL);
        if (!win)
          {
             r = 1;
             goto end;
          }
     }
   else if (!strncmp(argv[1], "--help", 6))
     {
        _ephoto_display_usage();
        r = 0;
        goto end;
     }
   else
     {
        char *real = ecore_file_realpath(argv[1]);
        if (!real)
          {
             printf("invalid file or directory: '%s'\n", argv[1]);
             r = 1;
             goto end;
          }
        Evas_Object *win = ephoto_window_add(real);
        free(real);
        if (!win)
          {
             r = 1;
             goto end;
          }
     }

   elm_run();

 end:
   eina_log_domain_unregister(__log_domain);
   efreet_mime_shutdown();
 end_log_domain:
   elm_shutdown();
   eio_shutdown();

   return r;
}

/*Display useage commands for ephoto*/
static void
_ephoto_display_usage(void)
{
   printf("Ephoto Usage: \n"
          "ephoto --help   : This page\n"
          "ephoto filename : Specifies a file to open\n"
          "ephoto dirname  : Specifies a directory to open\n");
}

#endif
ELM_MAIN()
