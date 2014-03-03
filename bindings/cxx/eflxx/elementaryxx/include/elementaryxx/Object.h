#ifndef ELMXX_OBJECT_H
#define ELMXX_OBJECT_H

/* STL */
#include <string>

/* EFL */
#include <Elementary.h>

/* ELFxx */
#include <evasxx/Smart.h>

namespace Elmxx {

/* forward declarations */ 
class Theme;
  
class Object : public Evasxx::Smart
{
public:
  void setScale (double scale);
  double getScale ();
  void setStyle (const std::string &style);
  const std::string getStyle ();
  void setDisabled (bool disabled);
  bool getDisabled ();
  
  void focus ();

  /*!
   * Check if the given Evas Object is an Elementary widget.
   */
  bool checkWidget ();
 
  /*!
   * Get the first parent of the given object that is an Elementary widget.
   */
 	Eflxx::CountedPtr <Evasxx::Object> getParentWidget ();
    
  /*!
   * Get the top level parent of an Elementary widget.
   */
 	Eflxx::CountedPtr <Evasxx::Object> getTopWidget ();

  /*!
   * Get the string that represents this Elementary widget. 
   */
  const std::string getWidgetType ();	

  /**
   * Set a label of an object
   *
   * @param obj The Elementary object
   * @param part The text part name to set (NULL for the default label)
   * @param label The new text of the label
   *
   * @note Elementary objects may have many labels (e.g. Action Slider)
   *
   * @ingroup General
   */
  virtual void setText (const std::string &label);
  virtual void setText (const std::string &part, const std::string &label);

  /**
   * Get a label of an object
   *
   * @param obj The Elementary object
   * @param part The text part name to get (NULL for the default label)
   * @return text of the label or NULL for any error
   *
   * @note Elementary objects may have many labels (e.g. Action Slider)
   *
   * @ingroup General
   */
  virtual const std::string getText ();
  virtual const std::string getText (const std::string &part);

  /**
   * Set a content of an object
   *
   * @param obj The Elementary object
   * @param part The content part name to set (NULL for the default content)
   * @param content The new content of the object
   *
   * @note Elementary objects may have many contents
   *
   * @ingroup General
   */
  void setContent (const Evasxx::Object &content);
  void setContent (const std::string &part, const Evasxx::Object &content);

  /**
   * Get a content of an object
   *
   * @param obj The Elementary object
   * @param part The content part name to get (NULL for the default content)
   * @return content of the object or NULL for any error
   *
   * @note Elementary objects may have many contents
   *
   * @ingroup General
   */
  Eflxx::CountedPtr <Evasxx::Object> getContent ();
  Eflxx::CountedPtr <Evasxx::Object> getContent (const std::string &part);

  /**
   * Set a specific theme to be used for this object and its children
   *
   * @param th The theme to set
   *
   * This sets a specific theme that will be used for the given object and any
   * child objects it has. If @p th is NULL then the theme to be used is
   * cleared and the object will inherit its theme from its parent (which
   * ultimately will use the default theme if no specific themes are set).
   *
   * Use special themes with great care as this will annoy users and make
   * configuration difficult. Avoid any custom themes at all if it can be
   * helped.
   *
   * @ingroup Theme
   */
  void setTheme(const Theme *th);

  /**
   * Get the specific theme to be used
   *
   * @return The specific theme set.
   *
   * This will return a specific theme set, or NULL if no specific theme is
   * set on that object. It will not return inherited themes from parents, only
   * the specific theme set for that specific object. See setTheme()
   * for more information.
   *
   * @ingroup Theme
   */
  Eflxx::CountedPtr <Theme> getTheme();

  /**
   * Unset a content of an object
   *
   * @param obj The Elementary object
   * @param part The content part name to unset (NULL for the default content)
   *
   * @note Elementary objects may have many contents
   *
   * @ingroup General
   */
  // TODO: if return value is used to report success the implement exception -> need to check in code!
  void unsetContent();
  void unsetContent(const std::string &part);

  /**
   * Get a named object from the children
   *
   * @param obj The parent object whose children to look at
   * @param name The name of the child to find
   * @param recurse Set to the maximum number of levels to recurse (0 == none, 1 is only look at 1 level of children etc.)
   * @return The found object of that name, or NULL if none is found
   *
   * This function searches the children (or recursively children of
   * children and so on) of the given @p obj object looking for a child with
   * the name of @p name. If the child is found the object is returned, or
   * NULL is returned. You can set the name of an object with
   * evas_object_name_set(). If the name is not unique within the child
   * objects (or the tree is @p recurse is greater than 0) then it is
   * undefined as to which child of that name is returned, so ensure the name
   * is unique amongst children. If recurse is set to -1 it will recurse
   * without limit.
   *
   * @ingroup General
   */
  virtual Eflxx::CountedPtr <Evasxx::Object> findName(const std::string &name, int recurse);

  /**
   * Set the text to read out when in accessibility mode
   *
   * @param obj The object which is to be described
   * @param txt The text that describes the widget to people with poor or no vision
   *
   * @ingroup General
   */
  virtual void setInfoAccess(const std::string &txt);

 
  virtual void destroy ();
  
protected:
  Object (); // allow only construction for child classes
  virtual ~Object (); // forbid direct delete -> use destroy()
  
  virtual void elmInit ();
  
private:
  Object (const Object&); // forbid copy constructor

  void freeSignalHandler ();
};

} // end namespace Elmxx

#endif // ELMXX_OBJECT_H

