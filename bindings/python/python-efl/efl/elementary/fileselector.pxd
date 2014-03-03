from efl.eina cimport Eina_Bool, const_Eina_List
from efl.evas cimport Evas_Object, const_Evas_Object, Evas_Coord
from enums cimport Elm_Fileselector_Mode, Elm_Fileselector_Sort
from libc.string cimport const_char

cdef extern from "Elementary.h":
    ctypedef Eina_Bool (*Elm_Fileselector_Filter_Func)(const_char *path, Eina_Bool dir, void *data)
    
    Evas_Object *           elm_fileselector_add(Evas_Object *parent)
    void                    elm_fileselector_is_save_set(Evas_Object *obj, Eina_Bool is_save)
    Eina_Bool               elm_fileselector_is_save_get(const_Evas_Object *obj)
    void                    elm_fileselector_folder_only_set(Evas_Object *obj, Eina_Bool value)
    Eina_Bool               elm_fileselector_folder_only_get(const_Evas_Object *obj)
    void                    elm_fileselector_buttons_ok_cancel_set(Evas_Object *obj, Eina_Bool value)
    Eina_Bool               elm_fileselector_buttons_ok_cancel_get(const_Evas_Object *obj)
    void                    elm_fileselector_expandable_set(Evas_Object *obj, Eina_Bool value)
    Eina_Bool               elm_fileselector_expandable_get(const_Evas_Object *obj)
    void                    elm_fileselector_path_set(Evas_Object *obj, const_char *path)
    const_char *            elm_fileselector_path_get(const_Evas_Object *obj)
    void                    elm_fileselector_mode_set(Evas_Object *obj, Elm_Fileselector_Mode mode)
    Elm_Fileselector_Mode   elm_fileselector_mode_get(const_Evas_Object *obj)
    void                    elm_fileselector_multi_select_set(Evas_Object *obj, Eina_Bool multi)
    Eina_Bool               elm_fileselector_multi_select_get(const_Evas_Object *obj)
    Eina_Bool               elm_fileselector_selected_set(Evas_Object *obj, const_char *path)
    const_char *            elm_fileselector_selected_get(const_Evas_Object *obj)
    const_Eina_List *       elm_fileselector_selected_paths_get(const_Evas_Object *obj)
    Eina_Bool               elm_fileselector_mime_types_filter_append(Evas_Object *obj, const_char *mime_types, const_char *filter_name)
    Eina_Bool               elm_fileselector_custom_filter_append(Evas_Object *obj, Elm_Fileselector_Filter_Func func, void *data, const_char *filter_name)
    void                    elm_fileselector_filters_clear(Evas_Object *obj)
    void                    elm_fileselector_hidden_visible_set(Evas_Object *obj, Eina_Bool visible)
    Eina_Bool               elm_fileselector_hidden_visible_get(const_Evas_Object *obj)
    Elm_Fileselector_Sort   elm_fileselector_sort_method_get(const_Evas_Object *obj)
    void                    elm_fileselector_sort_method_set(Evas_Object *obj, Elm_Fileselector_Sort method)
    void                    elm_fileselector_thumbnail_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
    void                    elm_fileselector_thumbnail_size_get(const_Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
