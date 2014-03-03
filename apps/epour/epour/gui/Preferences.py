#
#  Epour - A bittorrent client using EFL and libtorrent
#
#  Copyright 2012-2013 Kai Huuhko <kai.huuhko@gmail.com>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
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
log = logging.getLogger("epour")

import libtorrent as lt

from efl.elementary.icon import Icon
from efl.elementary.box import Box
from efl.elementary.label import Label
from efl.elementary.button import Button
from efl.elementary.frame import Frame
from efl.elementary.entry import Entry
from efl.elementary.check import Check
from efl.elementary.spinner import Spinner
from efl.elementary.hoversel import Hoversel
from efl.elementary.popup import Popup
from efl.elementary.fileselector_button import FileselectorButton
from efl.elementary.scroller import Scroller, ELM_SCROLLER_POLICY_OFF, \
    ELM_SCROLLER_POLICY_AUTO
from efl.elementary.separator import Separator
from efl.elementary.slider import Slider
from efl.elementary.actionslider import Actionslider, \
    ELM_ACTIONSLIDER_LEFT, ELM_ACTIONSLIDER_CENTER, \
    ELM_ACTIONSLIDER_RIGHT, ELM_ACTIONSLIDER_ALL
from efl.elementary.naviframe import Naviframe
from efl.elementary.table import Table
from efl.elementary.configuration import Configuration
from efl.evas import Rectangle
from efl.ecore import Timer
from efl.elementary.window import Window, ELM_WIN_BASIC
from efl.elementary.background import Background

import Notify

EXPAND_BOTH = 1.0, 1.0
EXPAND_HORIZ = 1.0, 0.0
FILL_BOTH = -1.0, -1.0
FILL_HORIZ = -1.0, 0.5
SCROLL_BOTH = ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_AUTO

class PreferencesDialog(Window):
    """ Base class for all preferences dialogs """
    def __init__(self, title):

        elm_conf = Configuration()
        scale = elm_conf.scale

        Window.__init__(self, title, ELM_WIN_BASIC, title=title, autodel=True)

        self.size = scale * 480, scale * 320

        bg = Background(self, size_hint_weight=EXPAND_BOTH)
        self.resize_object_add(bg)
        bg.show()

        # bt = Button(self, text="Close")
        # bt.callback_clicked_add(lambda b: self.delete())

        self.scroller = Scroller(self, policy=SCROLL_BOTH,
            size_hint_weight=EXPAND_BOTH, size_hint_align=FILL_BOTH)
        self.resize_object_add(self.scroller)
        self.scroller.show()

        self.box = Box(self)
        self.box.size_hint_weight = EXPAND_BOTH
        self.scroller.content = self.box

        self.show()

    # def parent_resize_cb(self, parent):
    #     (pw, ph) = parent.size
    #     self.table.size_hint_min =  pw * 0.7, ph * 0.7

class PreferencesGeneral(PreferencesDialog):
    """ General preference dialog """
    def __init__(self, parent, session):
        self.session = session
        conf = session.conf
        PreferencesDialog.__init__(self, "General")

        limits = Limits(self, session)
        ports = ListenPorts(self, session)
        pe = EncryptionSettings(self, session)
        dlsel = DataStorageSelector(self, conf)

        pad = Rectangle(self.evas)
        pad.color = 0, 0, 0, 0
        pad.size_hint_min = 0, 10

        sep1 = Separator(self)
        sep1.horizontal = True

        chk1 = Check(self)
        chk1.size_hint_align = 0.0, 0.0
        chk1.text = "Delete original .torrent file when added"
        chk1.state = conf.getboolean("Settings", "delete_original")
        chk1.callback_changed_add(lambda x: conf.set("Settings",
            "delete_original", str(bool(chk1.state))))

        chk2 = Check(self)
        chk2.size_hint_align = 0.0, 0.0
        chk2.text = "Ask for confirmation on exit"
        chk2.state = conf.getboolean("Settings", "confirmations")
        chk2.callback_changed_add(lambda x: conf.set("Settings",
            "confirmations", str(bool(chk2.state))))

        sep2 = Separator(self)
        sep2.horizontal = True

        for w in ports, limits, dlsel, pe, pad, sep1, chk1, chk2, sep2:
            w.show()
            self.box.pack_end(w)

class DataStorageSelector(Frame):
    def __init__(self, parent, conf):
        Frame.__init__(self, parent)

        self.size_hint_align = -1.0, 0.0
        self.size_hint_weight = 1.0, 0.0
        self.text = "Data storage"

        self.conf = conf

        b = Box(parent)

        lbl = self.path_lbl = Label(parent)
        lbl.text = conf.get("Settings", "storage_path")

        self.dlsel = dlsel = FileselectorButton(self)
        dlsel.size_hint_align = -1.0, 0.0
        dlsel.inwin_mode = False
        dlsel.folder_only = True
        dlsel.expandable = False
        dlsel.text = "Change path"
        dlsel.path = conf.get("Settings", "storage_path")
        dlsel.callback_file_chosen_add(self.save_dlpath)

        for w in lbl, dlsel:
            w.show()
            b.pack_end(w)

        b.show()
        self.content = b

    def save_dlpath(self, fs, path):
        if not path:
            return

        if not os.path.exists(self.dlsel.path):
            p = Notify.Error(self, "Invalid storage path",
                "You have selected an invalid data storage path for torrents.")
            return

        self.path_lbl.text = path
        self.conf.set("Settings", "storage_path", self.dlsel.path)

class ListenPorts(Frame):
    def __init__(self, parent, session):
        Frame.__init__(self, parent)

        self.session = session

        self.size_hint_align = FILL_HORIZ
        self.text = "Listen port (range)"

        port = session.listen_port()

        b = Box(parent)
        b.size_hint_weight = EXPAND_HORIZ

        lp = self.lp = RangeSpinners(
            parent,
            low = session.conf.getint("Settings", "listen_low"),
            high = session.conf.getint("Settings", "listen_high"),
            minim = 0, maxim = 65535)
        lp.show()
        b.pack_end(lp)

        save = Button(parent)
        save.text = "Apply"
        save.callback_clicked_add(self.save_cb)
        save.show()
        b.pack_end(save)

        b.show()

        self.content = b

    def save_cb(self, btn):
        low = int(self.lp.listenlow.value)
        high = int(self.lp.listenhigh.value)
        self.session.listen_on(low, high)
        self.session.conf.set("Settings", "listen_low", str(low))
        self.session.conf.set("Settings", "listen_high", str(high))

class PreferencesProxy(PreferencesDialog):
    """ Proxy preference dialog """
    def __init__(self, parent, session):
        PreferencesDialog.__init__(self, "Proxy")

        proxies = [
            ["Proxy for torrent peer connections",
                session.peer_proxy, session.set_peer_proxy],
            ["Proxy for torrent web seed connections",
                session.web_seed_proxy, session.set_web_seed_proxy],
            ["Proxy for tracker connections",
                session.tracker_proxy, session.set_tracker_proxy],
            ["Proxy for DHT connections",
                session.dht_proxy, session.set_dht_proxy],
        ]

        for title, rfunc, wfunc in proxies:
            pg = ProxyGroup(self, title, rfunc, wfunc)
            pg.show()
            self.box.pack_end(pg)

class ProxyGroup(Frame):

    proxy_types = {
        lt.proxy_type.none.name: lt.proxy_type.none,
        lt.proxy_type.socks4.name: lt.proxy_type.socks4,
        lt.proxy_type.socks5.name: lt.proxy_type.socks5,
        lt.proxy_type.socks5_pw.name: lt.proxy_type.socks5_pw,
        lt.proxy_type.http.name: lt.proxy_type.http,
        lt.proxy_type.http_pw.name: lt.proxy_type.http_pw,
    }

    def __init__(self, parent, title, rfunc, wfunc):
        Frame.__init__(self, parent)
        self.size_hint_weight = EXPAND_HORIZ
        self.size_hint_align = FILL_HORIZ
        self.text = title

        t = Table(self, homogeneous=True, padding=(3,3))
        t.size_hint_weight = EXPAND_HORIZ
        t.size_hint_align = FILL_HORIZ
        t.show()

        l = Label(self, text="Proxy type")
        l.size_hint_align = 0.0, 0.5
        l.show()
        ptype = Hoversel(parent)
        ptype.size_hint_align = -1.0, 0.5
        ptype.text = rfunc().type.name
        for n in self.proxy_types.iterkeys():
            ptype.item_add(n, callback=lambda x, y, z=n: ptype.text_set(z))
        ptype.show()
        t.pack(l, 0, 0, 1, 1)
        t.pack(ptype, 1, 0, 1, 1)

        l = Label(self, text="Hostname")
        l.size_hint_align = 0.0, 0.5
        l.show()
        phost = Entry(parent)
        phost.size_hint_weight = EXPAND_HORIZ
        phost.size_hint_align = FILL_HORIZ
        phost.single_line = True
        phost.scrollable = True
        phost.entry = rfunc().hostname
        phost.show()
        t.pack(l, 0, 1, 1, 1)
        t.pack(phost, 1, 1, 1, 1)

        l = Label(self, text="Port")
        l.size_hint_align = 0.0, 0.5
        l.show()
        pport = Spinner(parent)
        pport.size_hint_align = -1.0, 0.5
        pport.min_max = 0, 65535
        pport.value = rfunc().port
        pport.show()
        t.pack(l, 0, 2, 1, 1)
        t.pack(pport, 1, 2, 1, 1)

        l = Label(self, text="Username")
        l.size_hint_align = 0.0, 0.5
        l.show()
        puser = Entry(parent)
        puser.size_hint_weight = EXPAND_HORIZ
        puser.size_hint_align = FILL_HORIZ
        puser.single_line = True
        puser.scrollable = True
        puser.entry = rfunc().username
        puser.show()
        t.pack(l, 0, 3, 1, 1)
        t.pack(puser, 1, 3, 1, 1)

        l = Label(self, text="Password")
        l.size_hint_align = 0.0, 0.5
        l.show()
        ppass = Entry(parent)
        ppass.size_hint_weight = EXPAND_HORIZ
        ppass.size_hint_align = FILL_HORIZ
        ppass.single_line = True
        ppass.scrollable = True
        ppass.password = True
        ppass.entry = rfunc().password
        ppass.show()
        t.pack(l, 0, 4, 1, 1)
        t.pack(ppass, 1, 4, 1, 1)

        entries = [ptype, phost, pport, puser, ppass]

        save = Button(parent, text="Apply")
        save.callback_clicked_add(self.save_conf, wfunc, entries)
        save.show()
        t.pack(save, 0, 5, 2, 1)

        self.content = t

    def save_conf(self, btn, wfunc, entries):
        ptype, phost, pport, puser, ppass = entries
        p = lt.proxy_settings()

        p.hostname = phost.entry.encode("utf-8")
        p.port = int(pport.value)
        p.username = puser.entry.encode("utf-8")
        p.password = ppass.entry.encode("utf-8")
        p.type = self.proxy_types[ptype.text]

        wfunc(p)

class EncryptionSettings(Frame):
    def __init__(self, parent, session):
        self.session = session

        Frame.__init__(self, parent)
        self.size_hint_align = -1.0, 0.0
        self.text = "Encryption settings"

        pes = self.pes = session.get_pe_settings()

        b = Box(parent)

        enc_values = lt.enc_policy.disabled, lt.enc_policy.enabled, lt.enc_policy.forced
        enc_levels = lt.enc_level.plaintext, lt.enc_level.rc4, lt.enc_level.both

        inc = self.inc = ActSWithLabel(parent,
            "Incoming encryption", enc_values, pes.in_enc_policy)
        b.pack_end(inc)
        inc.show()

        out = self.out = ActSWithLabel(parent,
            "Outgoing encryption", enc_values, pes.out_enc_policy)
        b.pack_end(out)
        out.show()

        lvl = self.lvl = ActSWithLabel(parent,
            "Allowed encryption level", enc_levels, pes.allowed_enc_level)
        b.pack_end(lvl)
        lvl.show()

        prf = self.prf = Check(parent)
        prf.style = "toggle"
        prf.text = "Prefer RC4 ecryption"
        prf.state = pes.prefer_rc4
        b.pack_end(prf)
        prf.show()

        a_btn = Button(parent)
        a_btn.text = "Apply"
        a_btn.callback_clicked_add(self.apply)
        b.pack_end(a_btn)
        a_btn.show()

        b.show()
        self.content = b

    def apply(self, btn):
        #TODO: Use callbacks to set these?
        self.pes.in_enc_policy = self.inc.get_value()
        self.pes.out_enc_policy = self.out.get_value()

        #FIXME: Find out why this isn't saved to the session.
        self.pes.allowed_enc_level = self.lvl.get_value()
        self.pes.prefer_rc4 = self.prf.state

        self.session.set_pe_settings(self.pes)

class ActSWithLabel(Box):
    def __init__(self, parent, label_text, values, initial_value):
        Box.__init__(self, parent)

        self.vd = {
            ELM_ACTIONSLIDER_LEFT: values[0],
            ELM_ACTIONSLIDER_CENTER: values[1],
            ELM_ACTIONSLIDER_RIGHT: values[2],
        }

        self.horizontal = True
        self.size_hint_align = -1.0, 0.0
        self.size_hint_weight = 1.0, 0.0

        l = Label(parent)
        l.text = label_text
        l.show()
        w = self.w = Actionslider(parent)
        w.magnet_pos = ELM_ACTIONSLIDER_ALL
        w.size_hint_align = -1.0, 0.0
        w.size_hint_weight = 1.0, 0.0
        w.show()

        parts = "left", "center", "right"

        for i, v in enumerate(values):
            w.part_text_set(parts[i], str(v))
        w.indicator_pos = values.index(initial_value) + 1

        self.pack_end(l)
        self.pack_end(w)

    def get_value(self):
        return self.vd[self.w.indicator_pos]

class PreferencesSession(PreferencesDialog):
    """ Session preference dialog """
    def __init__(self, parent, session):
        PreferencesDialog.__init__(self, "Session")

        # TODO: Construct and populate this with an Idler

        self.session = session

        widgets = {}

        elm_conf = Configuration()

        s = session.settings()

        t = Table(self, padding=(5,5), homogeneous=True,
            size_hint_align=FILL_BOTH)
        self.box.pack_end(t)
        t.show()

        i = 0

        INT_MIN = -2147483648
        INT_MAX =  2147483647

        scale = elm_conf.scale

        for k in dir(s):
            if k.startswith("__"): continue
            try:
                a = getattr(s, k)
                if isinstance(a, lt.disk_cache_algo_t):
                    w = Spinner(t)
                    w.size_hint_align = FILL_HORIZ
                    # XXX: lt-rb python bindings don't have all values.
                    w.min_max = 0, 2 #len(lt.disk_cache_algo_t.values.keys())
                    for name, val in lt.disk_cache_algo_t.names.items():
                        w.special_value_add(val, name)
                    w.value = a
                elif isinstance(a, bool):
                    w = Check(t)
                    w.size_hint_align = 1.0, 0.0
                    w.style = "toggle"
                    w.state = a
                elif isinstance(a, int):
                    w = Spinner(t)
                    w.size_hint_align = FILL_HORIZ
                    w.min_max = INT_MIN, INT_MAX
                    w.value = a
                elif isinstance(a, float):
                    w = Slider(t)
                    w.size_hint_align = FILL_HORIZ
                    w.size_hint_weight = EXPAND_HORIZ
                    w.unit_format = "%1.2f"
                    if k.startswith("peer_turnover"):
                        w.min_max = 0.0, 1.0
                    else:
                        w.min_max = 0.0, 20.0
                    w.value = a
                elif k == "peer_tos":
                    # XXX: This is an int pair in libtorrent,
                    #      which doesn't have a python equivalent.
                    continue
                elif k == "user_agent":
                    w = Entry(t)
                    w.size_hint_align = 1.0, 0.0
                    w.size_hint_weight = EXPAND_HORIZ
                    w.single_line = True
                    w.editable = False
                    w.entry = cgi.escape(a)
                else:
                    w = Entry(t)
                    w.part_text_set("guide", "Enter here")
                    w.size_hint_align = FILL_HORIZ
                    w.size_hint_weight = EXPAND_HORIZ
                    w.single_line = True
                    w.entry = cgi.escape(a)
                l = Label(t)
                l.text = k.replace("_", " ").capitalize()
                l.size_hint_align = 0.0, 0.0
                l.size_hint_weight = EXPAND_HORIZ
                l.show()
                t.pack(l, 0, i, 1, 1)
                #w.size_hint_min = scale * 150, scale * 25
                t.pack(w, 1, i, 1, 1)
                w.show()
                widgets[k] = w
                i += 1
            except TypeError:
                pass #print("Error {}".format(k))

        save_btn = Button(self)
        save_btn.text = "Apply session settings"
        save_btn.callback_clicked_add(self.apply_settings, widgets, session)
        save_btn.show()
        self.box.pack_end(save_btn)

    def apply_settings(self, btn, widgets, session):
        s = lt.session_settings()

        for k, w in widgets.iteritems():

            if k == "disk_cache_algorithm":
                v = lt.disk_cache_algo_t(w.value)
            elif isinstance(w, Spinner):
                v = int(w.value)
            elif isinstance(w, Slider):
                v = w.value
            elif isinstance(w, Entry):
                v = w.entry.encode("utf-8")
            elif isinstance(w, Check):
                v = bool(w.state)
            else:
                v = None

            setattr(s, k, v)

        session.set_settings(s)
        Notify.Information(self, "Session settings saved.")

class UnitSpinner(Box):
    def __init__(self, parent, base, units):
        self.base = base # the divisor/multiplier for units
        self.units = units # a list of strings with the base unit description at index 0

        super(UnitSpinner, self).__init__(parent)
        self.horizontal = True

        self.save_timer = None

        s = self.spinner = Spinner(parent)
        s.size_hint_weight = EXPAND_HORIZ
        s.size_hint_align = FILL_HORIZ
        s.min_max = 0, base
        s.show()
        self.pack_end(s)

        hs = self.hoversel = Hoversel(parent)
        for u in units:
            hs.item_add(u, None, 0, lambda x=hs, y=None, u=u: x.text_set(u))
        hs.show()
        self.pack_end(hs)

    def callback_changed_add(self, func, delay=None):
        self.spinner.callback_changed_add(self.changed_cb, func, delay)
        self.hoversel.callback_selected_add(self.changed_cb, func, delay)

    def changed_cb(self, widget, *args):
        func, delay = args[-2:]

        if delay:
            if self.save_timer is not None:
                self.save_timer.delete()

            self.save_timer = Timer(2.0, self.save_cb, func)
        else:
            self.save_cb(func)

    def save_cb(self, func):
        v = int(self.get_value())
        log.debug("Saving value {}.".format(v))
        func(v)
        return False

    def get_value(self):
        return self.spinner.value * ( self.base ** self.units.index(self.hoversel.text) )

    def set_value(self, v):
        i = 0

        while v // self.base > 0:
            i += 1
            v = float(v) / float(self.base)

        if i > len(self.units):
            i = len(self.units) - 1

        self.spinner.value = v
        self.hoversel.text = self.units[i]

class RangeSpinners(Box):
    def __init__(self, parent, low, high, minim, maxim):

        Box.__init__(self, parent)
        self.size_hint_weight = EXPAND_BOTH
        self.size_hint_align = FILL_BOTH
        self.horizontal = True

        l = self.listenlow = Spinner(parent)
        l.size_hint_weight = EXPAND_BOTH
        l.size_hint_align = FILL_BOTH
        l.min_max = minim, maxim
        l.value = low
        self.pack_end(l)
        l.show()

        h = self.listenhigh = Spinner(parent)
        h.size_hint_weight = EXPAND_BOTH
        h.size_hint_align = FILL_BOTH
        h.min_max = minim, maxim
        h.value = high
        self.pack_end(h)
        h.show()

class Limits(Frame):
    def __init__(self, parent, session):
        Frame.__init__(self, parent)

        self.text = "Limits"
        self.size_hint_align = FILL_HORIZ

        base = 1024
        units = ( "bytes/s", "KiB/s", "MiB/s", "GiB/s", "TiB/s" )

        t = Table(parent)
        for r, values in enumerate((
            ("Upload limit", session.upload_rate_limit, session.set_upload_rate_limit),
            ("Download limit", session.download_rate_limit, session.set_download_rate_limit),
            ("Upload limit for local connections", session.local_upload_rate_limit, session.set_local_upload_rate_limit),
            ("Download limit for local connections", session.local_download_rate_limit, session.set_local_download_rate_limit),
        )):
            title, rfunc, wfunc = values

            l = Label(parent)
            l.text = title
            l.size_hint_align = FILL_HORIZ
            t.pack(l, 0, r, 1, 1)
            l.show()

            usw = UnitSpinner(parent, base, units)
            usw.size_hint_weight = EXPAND_HORIZ
            usw.size_hint_align = FILL_HORIZ
            usw.set_value(rfunc())
            usw.callback_changed_add(wfunc, delay=2.0)
            t.pack(usw, 1, r, 1, 1)
            usw.show()

        self.content = t

# TODO:
# max uploads?, max conns?, max half open conns?

# ip filter
