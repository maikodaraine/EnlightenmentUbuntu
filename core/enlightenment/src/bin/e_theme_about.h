#ifdef E_TYPEDEFS

typedef struct _E_Obj_Dialog E_Theme_About;

#else
#ifndef E_THEME_ABOUT_H
#define E_THEME_ABOUT_H

EAPI E_Theme_About  *e_theme_about_new  (E_Comp *c);
EAPI void            e_theme_about_show (E_Theme_About *about);

#endif
#endif
