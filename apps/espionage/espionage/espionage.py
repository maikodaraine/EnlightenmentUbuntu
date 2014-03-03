#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (C) 2013 Davide Andreoli <dave@gurumeditation.it>
#
# This file is part of Espionage.
#
# Espionage is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# Espionage is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Espionage.  If not, see <http://www.gnu.org/licenses/>.

import os
import sys
import dbus
import json
from xml.etree import ElementTree

from efl import elementary as elm
from efl.evas import EVAS_HINT_EXPAND, EVAS_HINT_FILL
from efl.elementary.window import StandardWindow
from efl.elementary.box import Box
from efl.elementary.button import Button
from efl.elementary.check import Check
from efl.elementary.entry import Entry, \
    Entry_markup_to_utf8 as markup_to_utf8, \
    Entry_utf8_to_markup as utf8_to_markup
from efl.elementary.flipselector import FlipSelector
from efl.elementary.icon import Icon
from efl.elementary.label import Label
from efl.elementary.panes import Panes
from efl.elementary.progressbar import Progressbar
from efl.elementary.popup import Popup
from efl.elementary.separator import Separator
from efl.elementary.table import Table
from efl.elementary.frame import Frame
from efl.elementary.genlist import Genlist, GenlistItem, GenlistItemClass, \
    ELM_GENLIST_ITEM_GROUP, ELM_GENLIST_ITEM_TREE, \
    ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY, ELM_LIST_SCROLL
from efl.dbus_mainloop import DBusEcoreMainLoop


class Options(object):
    """class to contain application options"""
    def __init__(self):
        self.show_introspect_stuff = False
        self.show_private_stuff = False
        self.pretty_output = True
        self.scroll_on_signal = True
        self.theme_name = 'default'

        self.stl_name = "font_weight=Bold color=#FFF  font_size=11"
        self.stl_value = "color=#FF99FF"
        self.stl_iface = "font_weight=Bold color=#999 font_size=11"
        self.stl_brackets = "color=#FFCC00"
        self.stl_arrow = "color=#AAAAAA"
        self.stl_ptype = "color=#6699FF"
        self.stl_pname = "color=#AAFFAA"


script_path = os.path.dirname(__file__)

def theme_resource_get(fname):
    return os.path.join(script_path, 'themes', options.theme_name, fname)

def prettify_if_needed(data):
    if options.pretty_output:
        return utf8_to_markup(json.dumps(data, indent=2))
    else:
        return utf8_to_markup(str(data))

def colored_params(plist, omit_braces=False):
    p = ', '.join(["<font %s>%s</>&nbsp;<font %s>%s</>" % \
                   (options.stl_ptype, ty, options.stl_pname, name)
                        for name, ty in plist])
    if omit_braces:
        return p
    return '<font %s>(</>%s<font %s>)</>' % \
           (options.stl_brackets, p, options.stl_brackets)


### connect to session and system buses, and set session as the current one
session_bus = dbus.SessionBus(mainloop=DBusEcoreMainLoop())
system_bus = dbus.SystemBus(mainloop=DBusEcoreMainLoop())
bus = session_bus
options = Options()


### Classes to describe various DBus nodes
class DBusNode(object):
    """base object for the others DBus nodes"""
    def __init__(self, name, parent):
        self._name = name
        self._parent = parent

    @property
    def name(self):
        return self._name
    
    @property
    def parent(self):
        return self._parent


class DBusObject(DBusNode):
    """object to represent a DBus Object """
    def __init__(self, name, parent_service):
        DBusNode.__init__(self, name, parent_service)
        self._interfaces = []

    @property
    def interfaces(self):
        return self._interfaces

    @property
    def icon(self):
        return 'object.png'

class DBusInterface(DBusNode):
    """object to represent a DBus Interface"""
    def __init__(self, name, parent_obj):
        DBusNode.__init__(self, name, parent_obj)
        self._properties = []
        self._methods = []
        self._signals = []
        
        parent_obj.interfaces.append(self)
    
    @property
    def properties(self):
        return self._properties
    
    @property
    def methods(self):
        return self._methods
    
    @property
    def signals(self):
        return self._signals

    @property
    def icon(self):
        return 'interface.png'

class DBusProperty(DBusNode):
    """object to represent a DBus Property"""
    def __init__(self, name, parent_iface, typ = 'unknown', access = 'unknown'):
        DBusNode.__init__(self, name, parent_iface)
        parent_iface.properties.append(self)
        self._type = typ
        self._access = access
        self._value = None

    def fetch_value(self):
        named_service = self.parent.parent.parent
        object_path = self.parent.parent.name
        iface_name = self.parent.name

        obj = bus.get_object(named_service, object_path)
        iface = dbus.Interface(obj, "org.freedesktop.DBus.Properties")
        self._value = iface.Get(iface_name, self.name)

    @property
    def value(self):
        return self._value

    @property
    def type(self):
        return self._type

    @property
    def access(self):
        return self._access

    @property
    def icon(self):
        return 'property.png'

class DBusMethod(DBusNode):
    """object to represent a DBus Method"""
    def __init__(self, name, parent_iface):
        DBusNode.__init__(self, name, parent_iface)
        parent_iface.methods.append(self)
        self._params = []
        self._returns = []

    @property
    def params(self):
        return self._params

    @property
    def params_str(self):
        return ', '.join([(ty+' '+name).strip() for name, ty in self._params])

    @property
    def returns(self):
        return self._returns

    @property
    def returns_str(self):
        return ', '.join([(ty+' '+name).strip() for name, ty in self._returns])

    @property
    def icon(self):
        return 'method.png'

class DBusSignal(DBusNode):
    """object to represent a DBus Signal"""
    def __init__(self, name, parent_iface):
        DBusNode.__init__(self, name, parent_iface)
        parent_iface.signals.append(self)
        self._params = []

    @property
    def params(self):
        return self._params

    @property
    def params_str(self):
        return ', '.join([(ty+' '+name).strip() for name, ty in self._params])

    @property
    def icon(self):
        return 'signal.png'

### Introspect a named service and return a list of DBusObjects
def recursive_introspect(bus, named_service, object_path, ret_data=None):

    # first recursion, create an empty list
    if ret_data is None:
        ret_data = []

    # parse the xml string from the Introspectable interface
    obj = bus.get_object(named_service, object_path)
    iface = dbus.Interface(obj, 'org.freedesktop.DBus.Introspectable')
    xml_data = iface.Introspect()
    xml_root = ElementTree.fromstring(xml_data)

    # debug
    # print('=' * 80)
    # print("Introspecting path:'%s' on service:'%s'" % (object_path, named_service))
    # print('=' * 80)
    # print(xml_data)
    # print('=' * 80)

    # traverse the xml tree
    if xml_root.find('interface') is not None:
        # found a new object
        obj = DBusObject(object_path, named_service)
        ret_data.append(obj)
    
    for xml_node in xml_root:
        # found an interface
        if xml_node.tag == 'interface':
            iface = DBusInterface(xml_node.attrib['name'], obj)

            for child in xml_node:
                if child.tag == 'property':
                    typ = child.attrib['type']
                    access = child.attrib['access']
                    prop = DBusProperty(child.attrib['name'], iface, typ, access)

                if child.tag == 'method':
                    meth = DBusMethod(child.attrib['name'], iface)
                    for arg in child:
                        if arg.tag == 'arg':
                            if arg.attrib['direction'] == 'out':
                                L = meth.returns
                            else:
                                L = meth.params
                            L.append((
                                arg.attrib['name'] if 'name' in arg.attrib else '',
                                arg.attrib['type'] if 'type' in arg.attrib else ''))

                if child.tag == 'signal':
                    sig = DBusSignal(child.attrib['name'], iface)
                    for arg in child:
                        if arg.tag == 'arg':
                            sig.params.append((
                                arg.attrib['name'] if 'name' in arg.attrib else '',
                                arg.attrib['type'] if 'type' in arg.attrib else ''))

        # found another node, introspect it...
        if xml_node.tag == 'node':
            if object_path == '/':
                object_path = ''
            new_path = '/'.join((object_path, xml_node.attrib['name']))
            recursive_introspect(bus, named_service, new_path, ret_data)

    return ret_data


### Names genlist (the one on the left)
class NamesListGroupItemClass(GenlistItemClass):
    def __init__(self):
        GenlistItemClass.__init__(self, item_style="group_index")
    def text_get(self, gl, part, name):
        return name

class NamesListItemClass(GenlistItemClass):
    def __init__(self):
        GenlistItemClass.__init__(self, item_style="default")
    def text_get(self, gl, part, name):
        return name

class NamesList(Genlist):
    def __init__(self, parent):

        self.win = parent
        self.sig1 = None
        self.waiting_activation = None
        self.waiting_popup = None

        # create the genlist
        Genlist.__init__(self, parent)
        self.itc = NamesListItemClass()
        self.itc_g = NamesListGroupItemClass()
        self.callback_selected_add(self.item_selected_cb)

        # add public group item
        self.public_group = self.item_append(self.itc_g, "Public Services",
                               flags=ELM_GENLIST_ITEM_GROUP)
        self.public_group.select_mode_set(ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)

        # add activatables group item
        self.activatable_group = self.item_append(self.itc_g, "Activatable Services",
                               flags=ELM_GENLIST_ITEM_GROUP)
        self.activatable_group.select_mode_set(ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
        
        # add private group item
        if options.show_private_stuff:
            self.private_group = self.item_append(self.itc_g, "Private Services",
                                 flags=ELM_GENLIST_ITEM_GROUP)
            self.private_group.select_mode_set(ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)

        # populate the genlist
        self.populate()

    def populate(self):
        active = set(bus.list_names())
        activatables = set(bus.list_activatable_names()) - active

        for name in active:
            self.service_add(name)

        for name in activatables:
            self.service_activatable_add(name)

        # keep the list updated when a name changes
        if self.sig1: self.sig1.remove()
        self.sig1 = bus.add_signal_receiver(self.name_owner_changed_cb, 
                                            "NameOwnerChanged")
        # bus.add_signal_receiver(self.name_acquired_cb, "NameAcquired")
        # bus.add_signal_receiver(self.name_lost_cb, "NameLost")
    
    def clear(self):
        self.public_group.subitems_clear()
        if options.show_private_stuff:
            self.private_group.subitems_clear()

    def item_selected_cb(self, gl, item):
        name = item.data

        if item.parent is self.activatable_group:
            # activate the service, async with a cool popup
            bus.call_async("org.freedesktop.DBus", "/", "org.freedesktop.DBus",
                           "StartServiceByName", "su", (name, 0), None, None)
            spinner = Progressbar(self.win, style="wheel", pulse_mode=True)
            spinner.pulse(True)
            def stop_waiting_cb(btn):
                self.waiting_popup.delete()
                self.waiting_activation = None
            button = Button(self.win, text="Stop waiting")
            button.callback_clicked_add(stop_waiting_cb)
            popup = Popup(self.win, content=spinner)
            popup.part_text_set('title,text', 'Activating service...')
            popup.part_content_set('button1', button)
            popup.show()
            self.waiting_activation = name
            self.waiting_popup = popup
        else:
            self.win.detail_list.populate(name)
    
    def sort_cb(self, it1, it2):
        return 1 if it1.data.lower() < it2.data.lower() else -1

    def service_activatable_add(self, name):
        self.item_sorted_insert(self.itc, name, self.sort_cb,
                                self.activatable_group, 0, None)

    def service_add(self, name):
        if name.startswith(":"):
            if options.show_private_stuff:
                item = self.item_sorted_insert(self.itc, name, self.sort_cb,
                                               self.private_group, 0, None)
        else:
            item = self.item_sorted_insert(self.itc, name, self.sort_cb,
                                           self.public_group, 0, None)

        if self.waiting_activation is not None and name == self.waiting_activation:
            self.waiting_popup.delete()
            self.waiting_activation = None
            item.selected = True
            item.show()

    def service_del(self, name):
        item = self.first_item
        while item:
            if item.data == name:
                item.delete()
                return
            item = item.next
        
    def name_owner_changed_cb(self, name, old_owner, new_owner):
        if old_owner == '':
            self.service_add(name)
        elif new_owner == '':
            self.service_del(name)


### Detail genlist (the one on the right)
class ObjectItemClass(GenlistItemClass):
    def __init__(self):
        GenlistItemClass.__init__(self, item_style="group_index")
    def text_get(self, gl, part, obj):
        return obj.name
    def content_get(self, gl, part, obj):
        if part == 'elm.swallow.icon':
            return Icon(gl, file=theme_resource_get(obj.icon))

class NodeItemClass(GenlistItemClass):

    def __init__(self):
        GenlistItemClass.__init__(self, item_style="default_style")

    def text_get(self, gl, part, obj):
        if isinstance(obj, DBusInterface):
            return '<font %s>%s</>' % (options.stl_iface, obj.name)
        if isinstance(obj, DBusProperty):
            if obj.value is not None:
                return '<font %s>%s</> <font %s>%s</> %s <font %s>→</> <font %s>%s</>' % \
                    (options.stl_name, obj.name, options.stl_ptype, obj.type,
                     obj.access, options.stl_arrow, options.stl_value, str(obj.value))
            else:
                return '<font %s>%s</> <font %s>%s</> %s <font %s>→</>' % \
                        (options.stl_name, obj.name, options.stl_ptype, obj.type,
                         obj.access, options.stl_arrow)
        if isinstance(obj, DBusMethod):
            params = colored_params(obj.params)
            if obj.returns:
                rets = colored_params(obj.returns)
                return '<font %s>%s</> %s <font %s>→</> %s' % \
                       (options.stl_name, obj.name, params, options.stl_arrow, rets)
            else:
                return '<font %s>%s</> %s' % (options.stl_name, obj.name, params)
        if isinstance(obj, DBusSignal):
            params = colored_params(obj.params)
            return '<font %s>%s</> %s' % (options.stl_name, obj.name, params)

    def content_get(self, gl, part, obj):
        if part == 'elm.swallow.icon':
            return Icon(gl, file=theme_resource_get(obj.icon))
    
class DetailList(Genlist):
    def __init__(self, parent):
        Genlist.__init__(self, parent)
        self._parent = parent
        self.service_name = None
        self.itc = NodeItemClass()
        self.itc_g = ObjectItemClass()
        self.callback_expand_request_add(self.expand_request_cb)
        self.callback_expanded_add(self.expanded_cb)
        self.callback_contract_request_add(self.contract_request_cb)
        self.callback_contracted_add(self.contracted_cb)
        self.callback_clicked_double_add(self.double_click_cb)

    def populate(self, name):
        self.service_name = name
        self.clear()

        # objects
        for obj in recursive_introspect(bus, name, '/'):
            obj_item = self.item_append(self.itc_g, obj,
                                        flags=ELM_GENLIST_ITEM_GROUP)
            obj_item.select_mode_set(ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY)
            
            # interfaces
            for iface in obj.interfaces:
                if not options.show_introspect_stuff and \
                   iface.name.startswith("org.freedesktop.DBus"):
                  continue
                iface_item = self.item_append(self.itc, iface,
                                              parent_item=obj_item,
                                              flags=ELM_GENLIST_ITEM_TREE)

    def sort_cb(self, it1, it2):
        pri1 = pri2 = 0
        if isinstance(it1.data, DBusProperty): pri1 = 3
        elif isinstance(it1.data, DBusMethod): pri1 = 2
        elif isinstance(it1.data, DBusSignal): pri1 = 1
        if isinstance(it2.data, DBusProperty): pri2 = 3
        elif isinstance(it2.data, DBusMethod): pri2 = 2
        elif isinstance(it2.data, DBusSignal): pri2 = 1
        if pri1 > pri2: return 1
        elif pri1 < pri2: return -1
        return 1 if it1.data.name.lower() < it2.data.name.lower() else -1

    def expand_request_cb(self, genlist, item):
        item.expanded = True

    def expanded_cb(self, genlist, item):
        iface = item.data
        for obj in iface.properties + iface.methods + iface.signals:
            self.item_sorted_insert(self.itc, obj, self.sort_cb, parent_item=item)
    
    def contract_request_cb(self, genlist, item):
        item.expanded = False

    def contracted_cb(self, genlist, item):
        item.subitems_clear()

    def double_click_cb(self, genlist, item):
        if isinstance(item.data, DBusMethod):
            MethodRunner(self._parent, item.data)
        elif isinstance(item.data, DBusProperty):
            item.data.fetch_value()
            item.update()


### Methods runner
class MethodRunner(StandardWindow):
    def __init__(self, parent, method):
        StandardWindow.__init__(self, "espionage", "Method", autodel = True)
        self._method = method
        self._param_entry = None
        self._return_entry = None

        # content is vbox (with surrounding pad frame)
        pad = Frame(self, style='pad_medium')
        pad.size_hint_weight = EVAS_HINT_EXPAND, EVAS_HINT_EXPAND
        pad.size_hint_align = EVAS_HINT_FILL, EVAS_HINT_FILL
        self.resize_object_add(pad)
        pad.show()
        
        vbox = Box(self)
        vbox.size_hint_weight = EVAS_HINT_EXPAND, EVAS_HINT_EXPAND
        vbox.size_hint_align = EVAS_HINT_FILL, EVAS_HINT_FILL
        pad.content = vbox
        vbox.show()

        # title
        f = "font_size=16 align=0.5 font_weight=Bold"
        en = Entry(self, text='<font %s>%s()</>' % (f, method.name))
        en.size_hint_weight = EVAS_HINT_EXPAND, 0.0
        en.size_hint_align = EVAS_HINT_FILL, 0.0
        en.editable = False
        vbox.pack_end(en)
        en.show()

        # params label + entry
        if len(method.params) > 0:
            label = Entry(self, editable=False)
            label.size_hint_weight = EVAS_HINT_EXPAND, 0.0
            label.size_hint_align = EVAS_HINT_FILL, 0.0
            pars = colored_params(method.params, omit_braces=True)
            label.text = 'Params: %s' % (pars if method.params else 'None')
            vbox.pack_end(label)
            label.show()

            en = Entry(self, editable=True, scrollable=True, single_line=True)
            en.size_hint_weight = EVAS_HINT_EXPAND, 0.0
            en.size_hint_align = EVAS_HINT_FILL, 0.0
            self._param_entry = en
            vbox.pack_end(en)
            en.show()

        # returns label + entry
        label = Entry(self, editable=False)
        label.size_hint_weight = EVAS_HINT_EXPAND, 0.0
        label.size_hint_align = EVAS_HINT_FILL, 0.0
        rets = colored_params(method.returns, omit_braces=True)
        label.text = 'Returns: %s' % (rets if method.returns else 'None')
        vbox.pack_end(label)
        label.show()

        en = Entry(self, editable=False, scrollable=True)
        en.size_hint_weight = EVAS_HINT_EXPAND, EVAS_HINT_EXPAND
        en.size_hint_align = EVAS_HINT_FILL, EVAS_HINT_FILL
        self._return_entry = en
        vbox.pack_end(en)
        en.show()

        # pretty print check button
        def pretty_output_clicked_cb(chk):
            options.pretty_output = chk.state
        ch = Check(self)
        ch.size_hint_align = 0.0, 0.5
        ch.text = "Prettify output (loosing type infos)"
        ch.state = options.pretty_output
        ch.callback_changed_add(pretty_output_clicked_cb)
        ch.show()
        vbox.pack_end(ch)

        sep = Separator(self, horizontal=True)
        vbox.pack_end(sep)
        sep.show()

        # buttons
        hbox = Box(self, horizontal=True)
        hbox.size_hint_weight = EVAS_HINT_EXPAND, 0.0
        hbox.size_hint_align = EVAS_HINT_FILL, 0.5
        vbox.pack_end(hbox)
        hbox.show()

        btn = Button(self)
        btn.text = 'Close'
        btn.callback_clicked_add(lambda b: self.delete())
        hbox.pack_end(btn)
        btn.show()

        btn = Button(self)
        btn.text = 'Clear output'
        btn.callback_clicked_add(lambda b: self._return_entry.entry_set(''))
        hbox.pack_end(btn)
        btn.show()

        btn = Button(self)
        btn.text = 'Run method'
        btn.callback_clicked_add(self.run_clicked_cb)
        hbox.pack_end(btn)
        btn.show()

        # show the window
        self.resize(300, 300)
        self.show()

    def run_clicked_cb(self, btn):
        # collect method infos
        named_service = self._method.parent.parent.parent
        object_path = self._method.parent.parent.name
        iface_name = self._method.parent.name
        method_name = self._method.name
        if self._param_entry:
            user_params = markup_to_utf8(self._param_entry.entry)
        else:
            user_params = None

        # create the dbus proxy
        obj = bus.get_object(named_service, object_path)
        iface = dbus.Interface(obj, iface_name)
        meth = iface.get_dbus_method(method_name)

        # async method call
        try:
            if user_params:
                meth(eval(user_params),
                     reply_handler = self.reply_handler,
                     error_handler = self.error_handler)
            else:
                meth(reply_handler = self.reply_handler,
                     error_handler = self.error_handler)
        except Exception as e:
            s = "Error running method<br>Exception: "
            self._return_entry.entry = s + utf8_to_markup(str(e))

        # TODO find a way to catch errors after this point
        #      wrong params for example are raised later :/

    def reply_handler(self, *rets):
        self._return_entry.entry = prettify_if_needed(rets) if rets \
                else "Method executed successfully.<br>Nothing returned."

    def error_handler(self, *rets):
        self._return_entry.entry = 'Error executing method'


### Signals receiver
class SignalItemClass(GenlistItemClass):
    def __init__(self):
        GenlistItemClass.__init__(self, item_style='default_style')
    def text_get(self, gl, part, data):
        return '<font %s>%s</>  <font %s>iface: %s</> <font %s>path: %s</> <font %s>sender: %s</>' % \
               (options.stl_name, data['signal'], options.stl_ptype, data['iface'],
                options.stl_pname, data['path'], options.stl_value, data['sender'])
    def content_get(self, gl, part, data):
        if part == 'elm.swallow.icon':
            return Icon(gl, file=theme_resource_get('signal.png'))
    
class SignalReceiver(Frame):
    def __init__(self, parent):
        Frame.__init__(self, parent, text="Signals")
        self._parent = parent

        vbox = Box(self)
        vbox.show()
        self.content = vbox

        self.siglist = Genlist(self, homogeneous=True, mode=ELM_LIST_SCROLL)
        self.siglist.size_hint_weight = EVAS_HINT_EXPAND, EVAS_HINT_EXPAND
        self.siglist.size_hint_align = EVAS_HINT_FILL, EVAS_HINT_FILL
        self.siglist.callback_clicked_double_add(self.signal_clicked_cb)
        self.siglist.show()
        vbox.pack_end(self.siglist)
        self.itc = SignalItemClass()

        hbox = Box(self, horizontal=True)
        hbox.size_hint_align = 0.0, 0.5
        hbox.show()
        vbox.pack_end(hbox)

        bt = Button(self, text='Clear')
        bt.callback_clicked_add(lambda b: self.siglist.clear())
        hbox.pack_end(bt)
        bt.show()

        def scroll_on_signal_clicked_cb(chk):
            options.scroll_on_signal = chk.state
        ck = Check(self, text='Scroll on signal')
        ck.state = options.scroll_on_signal
        ck.callback_changed_add(scroll_on_signal_clicked_cb)
        hbox.pack_end(ck)
        ck.show()

        for b in session_bus, system_bus:
            b.add_signal_receiver(self.signal_cb, sender_keyword='sender',
                destination_keyword='dest', interface_keyword='iface',
                member_keyword='signal',path_keyword='path')

    def signal_cb(self, *args, **kargs):
        # print('*** SIGNAL RECEIVED ***')
        # print(json.dumps(args, indent=2))
        # print(json.dumps(kargs, indent=2))

        kargs['args'] = args
        item = self.siglist.item_append(self.itc, kargs)
        if options.scroll_on_signal is True:
            item.bring_in()

        if self.siglist.items_count > 200:
            self.siglist.first_item.delete()

    def signal_clicked_cb(self, gl, item):
        pp = Popup(self._parent)
        pp.part_text_set('title,text', 'Signal content')

        en = Entry(self, text = prettify_if_needed(item.data['args']))
        en.size_hint_weight = EVAS_HINT_EXPAND, EVAS_HINT_EXPAND
        en.size_hint_align = EVAS_HINT_FILL, EVAS_HINT_FILL
        en.size_hint_min = 800, 800 # TODO: this should be respected :/
        en.editable = False
        en.scrollable = True
        pp.content = en

        bt = Button(pp, text="Close")
        bt.callback_clicked_add(lambda b: pp.delete())
        pp.part_content_set('button2', bt)

        def prettify_clicked_cb(chk):
            options.pretty_output = chk.state
            en.text = prettify_if_needed(item.data['args'])
        ck = Check(pp, text="Prettify")
        ck.state = options.pretty_output
        ck.callback_changed_add(prettify_clicked_cb)
        pp.part_content_set('button1', ck)

        pp.show()


### The main window
class EspionageWin(StandardWindow):
    def __init__(self):
        StandardWindow.__init__(self, "espionage", "EFL DBus Spy - Espionage")

        self.autodel_set(True)
        self.callback_delete_request_add(lambda o: elm.exit())

        box = Box(self)
        self.resize_object_add(box)
        box.size_hint_weight = EVAS_HINT_EXPAND, EVAS_HINT_EXPAND
        box.show()

        tb = Table(self)
        box.pack_end(tb)
        tb.show()

        lb = Label(self, text="Connect to:", scale=1.3)
        tb.pack(lb, 0, 0, 1, 2)
        lb.show()

        flip = FlipSelector(self, scale=1.3)
        flip.item_append("Session Bus", self.flip_selected_cb, session_bus)
        flip.item_append("System Bus", self.flip_selected_cb, system_bus)
        tb.pack(flip, 1, 0, 1, 2)
        flip.show()

        chk = Check(self, text="Show private services")
        chk.size_hint_align = 0.0, 1.0
        chk.state = options.show_private_stuff
        chk.callback_changed_add(self.show_private_cb)
        tb.pack(chk, 2, 0, 1, 1)
        chk.show()

        chk = Check(self, text="Show DBus introspectables")
        chk.size_hint_align = 0.0, 0.0
        chk.state = options.show_introspect_stuff
        chk.callback_changed_add(self.show_introspectables_cb)
        tb.pack(chk, 2, 1, 1, 1)
        chk.show()

        vpanes = Panes(self, horizontal=True)
        vpanes.size_hint_weight = EVAS_HINT_EXPAND, EVAS_HINT_EXPAND
        vpanes.size_hint_align = EVAS_HINT_FILL, EVAS_HINT_FILL
        vpanes.content_left_size = 2.0 / 3
        box.pack_end(vpanes)
        vpanes.show()
        
        hpanes = Panes(self)
        hpanes.size_hint_weight = EVAS_HINT_EXPAND, EVAS_HINT_EXPAND
        hpanes.size_hint_align = EVAS_HINT_FILL, EVAS_HINT_FILL
        hpanes.content_left_size = 1.0 / 3
        vpanes.part_content_set("left", hpanes)
        self.panes = hpanes
        hpanes.show()

        self.names_list = NamesList(self)
        hpanes.part_content_set("left", self.names_list)

        self.detail_list = DetailList(self)
        hpanes.part_content_set("right", self.detail_list)

        self.sigs_receiver = SignalReceiver(self)
        vpanes.part_content_set("right", self.sigs_receiver)

        self.resize(700, 500)
        self.show()

    def flip_selected_cb(self, flipselector, item, selected_bus):
        global bus

        bus = selected_bus
        self.detail_list.clear()
        self.names_list.clear()
        self.names_list.populate()

    def show_private_cb(self, chk):
        options.show_private_stuff = chk.state
        self.names_list.delete()
        self.names_list = NamesList(self)
        self.panes.part_content_set("left", self.names_list)

    def show_introspectables_cb(self, chk):
        options.show_introspect_stuff = chk.state
        self.detail_list.populate(self.detail_list.service_name)

def main():
    elm.init()
    win = EspionageWin()
    elm.run()
    elm.shutdown()
    return 0


if __name__ == "__main__":
    sys.exit(main())

