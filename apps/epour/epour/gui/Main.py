#
#  Epour - A bittorrent client using EFL and libtorrent
#
#  Copyright 2012-2013 Kai Huuhko <kai.huuhko@gmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#

import os
import cgi
import logging
from datetime import timedelta

import libtorrent as lt

try:
    from efl.evas import EVAS_ASPECT_CONTROL_VERTICAL, Rectangle
    from efl.ecore import Timer
    from efl import elementary as elm
    from efl.elementary.genlist import Genlist, GenlistItemClass, \
        ELM_GENLIST_ITEM_FIELD_TEXT, ELM_GENLIST_ITEM_FIELD_CONTENT, \
        ELM_OBJECT_SELECT_MODE_NONE, ELM_LIST_COMPRESS
    from efl.elementary.window import StandardWindow
    from efl.elementary.icon import Icon
    from efl.elementary.box import Box
    from efl.elementary.label import Label
    from efl.elementary.button import Button
    from efl.elementary.innerwindow import InnerWindow
    from efl.elementary.frame import Frame
    from efl.elementary.fileselector import Fileselector
    from efl.elementary.entry import Entry
    from efl.elementary.object import ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT
    from efl.elementary.panel import Panel, ELM_PANEL_ORIENT_BOTTOM
    from efl.elementary.table import Table
    from efl.elementary.separator import Separator
    from efl.elementary.menu import Menu
    from efl.elementary.configuration import Configuration
    from efl.elementary.toolbar import Toolbar, ELM_TOOLBAR_SHRINK_NONE, \
        ELM_OBJECT_SELECT_MODE_NONE
except ImportError:
    from evas import EVAS_ASPECT_CONTROL_VERTICAL, Rectangle
    from ecore import Timer
    import elementary as elm
    from elementary import Genlist, GenlistItemClass, StandardWindow, Icon, \
        Box, Label, Button, ELM_GENLIST_ITEM_FIELD_TEXT, \
        ELM_GENLIST_ITEM_FIELD_CONTENT, InnerWindow, Frame, \
        Fileselector, Entry, Panel, ELM_PANEL_ORIENT_BOTTOM, \
        ELM_OBJECT_SELECT_MODE_NONE, Table, Separator, Menu, Configuration, \
        ELM_LIST_COMPRESS, Toolbar

from TorrentInfo import TorrentInfo
from Preferences import PreferencesGeneral, PreferencesProxy, PreferencesSession
from Notify import ConfirmExit, Error, Information

from intrepr import intrepr

class MainInterface(object):
    def __init__(self, parent, session):
        self.parent = parent
        self.session = session

        elm_conf = Configuration()
        scale = elm_conf.scale

        self.log = logging.getLogger("epour.gui")

        self.torrentitems = {}

        win = self.win = StandardWindow("epour", "Epour")
        win.callback_delete_request_add(lambda x: elm.exit())
        win.screen_constrain = True
        win.size = 480 * scale, 400 * scale

        mbox = Box(win)
        mbox.size_hint_weight = 1.0, 1.0
        win.resize_object_add(mbox)
        mbox.show()

        tb = Toolbar(win)
        tb.homogeneous = False
        tb.shrink_mode = ELM_TOOLBAR_SHRINK_NONE
        tb.select_mode = ELM_OBJECT_SELECT_MODE_NONE
        tb.size_hint_align = -1.0, 0.0
        tb.menu_parent = win

        item = tb.item_append("document-new", "Add torrent",
                              lambda t,i: self.select_torrent())

        def pause_session(it):
            self.session.pause()
            it.state_set(it.state_next())
        def resume_session(it):
            session.resume()
            del it.state
        item = tb.item_append("media-playback-pause", "Pause Session",
                              lambda tb, it: pause_session(it))
        item.state_add("media-playback-start", "Resume Session",
                              lambda tb, it: resume_session(it))

        item = tb.item_append("preferences-system", "Preferences")
        item.menu = True
        item.menu.item_add(None, "General", "preferences-system",
                           lambda o,i: PreferencesGeneral(self, self.session))
        item.menu.item_add(None, "Proxy", "preferences-system",
                           lambda o,i: PreferencesProxy(self, self.session))
        item.menu.item_add(None, "Session", "preferences-system",
                           lambda o,i: PreferencesSession(self, self.session))

        item = tb.item_append("application-exit", "Exit",
                              lambda tb, it: elm.exit())

        mbox.pack_start(tb)
        tb.show()

        self.tlist = tlist = Genlist(win)
        tlist.select_mode = ELM_OBJECT_SELECT_MODE_NONE
        tlist.mode = ELM_LIST_COMPRESS
        tlist.callback_activated_add(self.item_activated_cb)
        tlist.homogeneous = True
        tlist.size_hint_weight = 1.0, 1.0
        tlist.size_hint_align = -1.0, -1.0
        tlist.show()

        mbox.pack_end(tlist)

        pad = Rectangle(win.evas)
        pad.size_hint_weight = 1.0, 1.0

        p = Panel(win)
        p.color = 200,200,200,200
        p.size_hint_weight = 1.0, 1.0
        p.size_hint_align = -1.0, -1.0
        p.orient = ELM_PANEL_ORIENT_BOTTOM
        p.content = SessionStatus(win, session)
        p.hidden = True
        p.show()

        topbox = Box(win)
        topbox.horizontal = True
        topbox.size_hint_weight = 1.0, 1.0
        win.resize_object_add(topbox)

        topbox.pack_end(pad)
        topbox.pack_end(p)
        topbox.stack_above(mbox)
        topbox.show()

        session.alert_manager.callback_add(
            "torrent_added_alert", self.torrent_added_cb)
        session.alert_manager.callback_add(
            "torrent_removed_alert", self.torrent_removed_cb)

        for a_name in "torrent_paused_alert", "torrent_resumed_alert":
            session.alert_manager.callback_add(a_name, self.update_icon)

        session.alert_manager.callback_add(
            "state_changed_alert", self.state_changed_cb)

        Timer(15.0, lambda: session.alert_manager.callback_add(
            "torrent_finished_alert", self.torrent_finished_cb))

    def select_torrent(self):
        sel = Fileselector(self.win)
        sel.expandable = False
        sel.path_set(os.path.expanduser("~"))
        sel.size_hint_weight_set(1.0, 1.0)
        sel.size_hint_align_set(-1.0, -1.0)
        sel.show()

        sf = Frame(self.win)
        sf.size_hint_weight_set(1.0, 1.0)
        sf.size_hint_align_set(-1.0, -1.0)
        sf.text = "Select torrent file"
        sf.content = sel
        sf.show()

        magnet = Entry(self.win)
        magnet.single_line = True
        magnet.scrollable = True
        if hasattr(magnet, "cnp_selection_get"):
            magnet.cnp_selection_get(ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT)
        else:
            import pyperclip
            t = pyperclip.paste()
            if t is not None and t.startswith("magnet:"):
                    magnet.entry = t
        magnet.show()

        mf = Frame(self.win)
        mf.size_hint_weight_set(1.0, 0.0)
        mf.size_hint_align_set(-1.0, 0.0)
        mf.text = "Or enter magnet URI here"
        mf.content = magnet
        mf.show()

        mbtn = Button(self.win)
        mbtn.text = "Done"
        mbtn.show()
        mbox = Box(self.win)
        mbox.size_hint_weight_set(1.0, 0.0)
        mbox.size_hint_align_set(-1.0, 0.0)
        mbox.horizontal = True
        mbox.pack_end(mf)
        mbox.pack_end(mbtn)
        mbox.show()

        box = Box(self.win)
        box.size_hint_weight = (1.0, 1.0)
        box.size_hint_align = (-1.0, -1.0)
        box.pack_end(sf)
        box.pack_end(mbox)
        box.show()

        inwin = InnerWindow(self.win)
        inwin.content = box
        sel.callback_done_add(self.add_torrent_cb)
        sel.callback_done_add(lambda x, y: inwin.delete())
        mbtn.callback_clicked_add(self.add_magnet_uri_cb, magnet)
        mbtn.callback_clicked_add(lambda x: inwin.delete())
        inwin.activate()

    def add_torrent_cb(self, filesel, t):
        if t:
            self.session.add_torrent(t)

    def add_magnet_uri_cb(self, btn, magnet):
        self.add_torrent_cb(None, magnet.text)

    def state_changed_cb(self, a):
        h = a.handle
        ihash = str(h.info_hash())
        if not h.is_valid():
            self.log.debug("State changed for invalid handle.")
            return

        #elif not self.torrentitems.has_key(ihash):
            #self.add_torrent_item(h)

        self.update_icon(a)

    def run(self):
        self.win.show()

        self.timer = Timer(1.0, self.update)
        elm.run()
        self.quit()

    def update(self):
        for v in self.tlist.realized_items_get():
            v.fields_update("*", ELM_GENLIST_ITEM_FIELD_TEXT)
        return True

    def update_icon(self, a):
        h = a.handle
        if not h.is_valid(): return
        ihash = str(h.info_hash())
        if not self.torrentitems.has_key(ihash): return
        self.torrentitems[ihash].fields_update(
            "elm.swallow.icon", ELM_GENLIST_ITEM_FIELD_CONTENT
        )

    def torrent_added_cb(self, a):
        h = a.handle
        self.add_torrent_item(h)

    def add_torrent_item(self, h):
        ihash = str(h.info_hash())

        itc = TorrentClass(self.session, "double_label")
        item = self.tlist.item_append(itc, h)
        self.torrentitems[ihash] = item

    def torrent_removed_cb(self, a):
        self.remove_torrent_item(a.info_hash)

    def remove_torrent_item(self, info_hash):
        it = self.torrentitems.pop(str(info_hash), None)
        if it is not None:
            it.delete()

    def item_activated_cb(self, gl, item):
        h = item.data
        menu = Menu(self.win)

        menu.item_add(
            None,
            "Resume" if h.is_paused() else "Pause",
            None,
            self.resume_torrent_cb if h.is_paused() else self.pause_torrent_cb,
            h
        )
        q = menu.item_add(None, "Queue", None, None)
        menu.item_add(q, "Up", None, lambda x, y: h.queue_position_up())
        menu.item_add(q, "Down", None, lambda x, y: h.queue_position_down())
        menu.item_add(q, "Top", None, lambda x, y: h.queue_position_top())
        menu.item_add(q, "Bottom", None, lambda x, y: h.queue_position_bottom())
        rem = menu.item_add(None, "Remove torrent", None,
            self.remove_torrent_cb, item, h, False)
        menu.item_add(rem, "and data files", None,
            self.remove_torrent_cb, item, h, True)
        menu.item_add(None, "Force re-check", None,
            self.force_recheck, h)
        menu.item_separator_add(None)
        menu.item_add(None, "Torrent preferences", None,
            self.torrent_preferences_cb, h)

        menu.move(*self.win.evas.pointer_canvas_xy_get())
        menu.show()

    def resume_torrent_cb(self, menu, item, h):
        h.resume()
        h.auto_managed(True)

    def pause_torrent_cb(self, menu, item, h):
        h.auto_managed(False)
        h.pause()

    def force_recheck(self, menu, item, h):
        h.force_recheck()

    def remove_torrent_cb(self, menu, item, glitem, h, with_data=False):
        menu.close()
        ihash = self.parent.session.remove_torrent(h, with_data)

    def torrent_preferences_cb(self, menu, item, h):
        self.i = TorrentInfo(self, h)

    def show_error(self, title, text):
        Error(self.win, title, text)

    def torrent_finished_cb(self, a):
        msg = "Torrent {} has finished downloading.".format(
            cgi.escape(a.handle.name())
        )
        self.log.info(msg)

        Information(self.win, msg)

    def quit(self, *args):
        if self.session.conf.getboolean("Settings", "confirmations"):
            ConfirmExit(self.win, self.shutdown())
        else:
            self.shutdown()

    def shutdown(self):
        elm.shutdown()
        self.parent.quit()

class SessionStatus(Table):

    log = logging.getLogger("epour.gui.session")

    def __init__(self, parent, session):
        Table.__init__(self, parent)
        self.session = session

        s = session.status()

        self.padding = 5, 5

        ses_pause_ic = self.ses_pause_ic = Icon(parent)
        ses_pause_ic.size_hint_align = -1.0, -1.0
        try:
            if session.is_paused():
                ses_pause_ic.standard = "player_pause"
            else:
                ses_pause_ic.standard = "player_play"
        except RuntimeError:
            self.log.debug("Setting session ic failed")
        self.pack(ses_pause_ic, 1, 0, 1, 1)
        ses_pause_ic.show()

        title_l = Label(parent)
        title_l.text = "<b>Session</b>"
        self.pack(title_l, 0, 0, 1, 1)
        title_l.show()

        d_ic = Icon(parent)
        try:
            d_ic.standard = "down"
        except RuntimeError:
            self.log.debug("Setting d_ic failed")
        d_ic.size_hint_align = -1.0, -1.0
        self.pack(d_ic, 0, 2, 1, 1)
        d_ic.show()

        d_l = self.d_l = Label(parent)
        d_l.text = "{}/s".format(intrepr(s.payload_download_rate))
        self.pack(d_l, 1, 2, 1, 1)
        d_l.show()

        u_ic = Icon(self)
        try:
            u_ic.standard = "up"
        except RuntimeError:
            self.log.debug("Setting u_ic failed")
        u_ic.size_hint_align = -1.0, -1.0
        self.pack(u_ic, 0, 3, 1, 1)
        u_ic.show()

        u_l = self.u_l = Label(parent)
        u_l.text = "{}/s".format(intrepr(s.payload_upload_rate))
        self.pack(u_l, 1, 3, 1, 1)
        u_l.show()

        peer_t = Label(parent)
        peer_t.text = "Peers"
        self.pack(peer_t, 0, 4, 1, 1)
        peer_t.show()

        peer_l = self.peer_l = Label(parent)
        peer_l.text = str(s.num_peers)
        self.pack(peer_l, 1, 4, 1, 1)
        peer_l.show()

        self.show()

        self.update_timer = Timer(1.0, self.update)

    def update(self):
        s = self.session.status()
        self.d_l.text = "{}/s".format(intrepr(s.payload_download_rate))
        self.u_l.text = "{}/s".format(intrepr(s.payload_upload_rate))
        self.peer_l.text = str(s.num_peers)
        if self.session.is_paused():
            icon = "player_pause"
        else:
            icon = "player_play"
        try:
            self.ses_pause_ic.standard = icon
        except RuntimeError:
            self.log.debug("")

        return True




class TorrentClass(GenlistItemClass):

    state_str = ['Queued', 'Checking', 'Downloading metadata', \
        'Downloading', 'Finished', 'Seeding', 'Allocating', \
        'Checking resume data']

    log = logging.getLogger("epour.gui.torrent_list")

    def __init__(self, session, *args, **kwargs):
        GenlistItemClass.__init__(self, *args, **kwargs)

        self.session = session

    def text_get(self, obj, part, item_data):
        h = item_data
        name = h.name()

        if part == "elm.text":
            return '%s' % (
                name
            )
        elif part == "elm.text.sub":
            s = h.status()

            return "{:.0%} complete, ETA: {} " \
            "(Down: {}/s Up: {}/s Peers: {} Queue: {})".format(
                s.progress,
                timedelta(seconds=self.get_eta(h)),
                intrepr(s.download_payload_rate, precision=0),
                intrepr(s.upload_payload_rate, precision=0),
                s.num_peers,
                h.queue_position(),
            )

    def content_get(self, obj, part, item_data):
        if part == "elm.swallow.icon":
            h = item_data
            s = h.status()
            ic = Icon(obj)
            if h.is_paused():
                try:
                    ic.standard = "player_pause"
                except RuntimeError:
                    self.log.debug("Setting torrent ic failed")
            elif h.is_seed():
                try:
                    ic.standard = "up"
                except RuntimeError:
                    self.log.debug("Setting torrent ic failed")
            else:
                try:
                    ic.standard = "down"
                except RuntimeError:
                    self.log.debug("Setting torrent ic failed")
            ic.tooltip_text_set(self.state_str[s.state])
            ic.size_hint_aspect_set(EVAS_ASPECT_CONTROL_VERTICAL, 1, 1)
            return ic

    def get_eta(self, h):
        s = h.status()
        if False: #self.is_finished and self.options["stop_at_ratio"]:
            # We're a seed, so calculate the time to the 'stop_share_ratio'
            if not s.upload_payload_rate:
                return 0
            stop_ratio = self.session.settings().share_ratio_limit
            return ((s.all_time_download * stop_ratio) - \
                s.all_time_upload) / s.upload_payload_rate

        left = s.total_wanted - s.total_wanted_done

        if left <= 0 or s.download_payload_rate == 0:
            return 0

        try:
            eta = left / s.download_payload_rate
        except ZeroDivisionError:
            eta = 0

        return eta
