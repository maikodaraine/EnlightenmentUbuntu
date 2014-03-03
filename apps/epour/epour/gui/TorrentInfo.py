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

import cgi
import sys
import os
import logging
log = logging.getLogger("epour")

import libtorrent as lt

try:
    from efl import ecore
    from efl.elementary.genlist import Genlist, GenlistItemClass
    from efl.elementary.innerwindow import InnerWindow
    from efl.elementary.button import Button
    from efl.elementary.box import Box
    from efl.elementary.hoversel import Hoversel
    from efl.elementary.check import Check
    from efl.elementary.label import Label, ELM_WRAP_CHAR, ELM_WRAP_WORD
    from efl.elementary.entry import Entry
    from efl.elementary.naviframe import Naviframe
    from efl.elementary.frame import Frame
    from efl.elementary.object import ELM_SEL_FORMAT_TEXT, ELM_SEL_TYPE_CLIPBOARD
except ImportError:
    import ecore
    from elementary import Genlist, GenlistItemClass, InnerWindow, Button, \
        Box, Hoversel, Check, Label, ELM_WRAP_CHAR, ELM_WRAP_WORD, Entry, \
        Naviframe, Frame

from intrepr import intrepr
from Notify import Information

class TorrentInfo(InnerWindow):
    def __init__(self, parent, h):
        if not h.is_valid():
            Information(parent.win, "Invalid torrent handle.")
            return

        if not h.has_metadata():
            Information(parent.win, "Torrent contains no metadata.")
            return

        i = h.get_torrent_info()

        InnerWindow.__init__(self, parent.win)

        box = Box(self)
        box.size_hint_align = -1.0, -1.0
        box.size_hint_weight = 1.0, 1.0

        tname = Label(self)
        tname.size_hint_align = -1.0, 0.5
        tname.line_wrap = ELM_WRAP_CHAR
        tname.ellipsis = True
        tname.text = "{}".format(cgi.escape(i.name()))
        tname.show()
        box.pack_end(tname)

        for func in i.comment, i.creation_date, i.creator:
            try:
                w = func()
            except Exception as e:
                log.debug(e)
            else:
                if w:
                    f = Frame(self)
                    f.size_hint_align = -1.0, 0.0
                    f.text = func.__name__.replace("_", " ").capitalize()
                    l = Label(self)
                    l.ellipsis = True
                    l.text = cgi.escape(str(w))
                    l.show()
                    f.content = l
                    f.show()
                    box.pack_end(f)

        tpriv = Check(self)
        tpriv.size_hint_align = 0.0, 0.0
        tpriv.text = "Private"
        tpriv.tooltip_text_set(
            "Whether this torrent is private.<br> \
            i.e., it should not be distributed on the trackerless network<br> \
            (the kademlia DHT)."
            )
        tpriv.disabled = True
        tpriv.state = i.priv()

        magnet_uri = lt.make_magnet_uri(h)

        f = Frame(self)
        f.size_hint_align = -1.0, 0.0
        f.text = "Magnet URI"
        me_box = Box(self)
        me_box.horizontal = True
        me = Entry(self)
        me.size_hint_align = -1.0, 0.0
        me.size_hint_weight = 1.0, 0.0
        #me.editable = False
        me.entry = magnet_uri
        me_box.pack_end(me)
        me.show()
        me_btn = Button(self)
        me_btn.text = "Copy"
        if hasattr(me, "cnp_selection_set"):
            me_btn.callback_clicked_add(
                lambda x: me.top_widget.cnp_selection_set(
                    ELM_SEL_TYPE_CLIPBOARD, ELM_SEL_FORMAT_TEXT, me.text
                    )
                )
        else:
            import pyperclip
            me_btn.callback_clicked_add(lambda x: pyperclip.copy(magnet_uri))
        me_btn.show()
        me_box.pack_end(me_btn)
        me_box.show()
        f.content = me_box
        f.show()
        box.pack_end(f)


        fl_btn = Button(self)
        fl_btn.text = "Files ->"
        fl_btn.callback_clicked_add(self.file_list_cb, h)

        xbtn = Button(self)
        xbtn.text_set("Close")
        xbtn.callback_clicked_add(lambda x: self.delete())


        for w in tpriv, fl_btn, xbtn:
            w.show()
            box.pack_end(w)

        box.show()

        nf = self.nf = Naviframe(self)
        nf.item_simple_push(box)

        self.content_set(nf)
        self.activate()

    def file_list_cb(self, btn, h):
        self.nf.item_simple_push(TorrentFiles(self.nf, h))

class TorrentFiles(Box):
    def __init__(self, parent, h):
        Box.__init__(self, parent)

        filelist = Genlist(self)
        filelist.size_hint_align = -1.0, -1.0
        filelist.size_hint_weight = 1.0, 1.0

        self.populate(filelist, h)

        filelist.callback_activated_add(self.item_activated_cb)
        filelist.show()

        sel_all = Button(self)
        sel_all.text = "Select all"
        sel_all.callback_clicked_add(self.select_all_cb, filelist, h, True)
        sel_all.show()

        sel_none = Button(self)
        sel_none.text ="Select none"
        sel_none.callback_clicked_add(self.select_all_cb, filelist, h, False)
        sel_none.show()

        xbtn = Button(self)
        xbtn.text = "Close"
        xbtn.callback_clicked_add(lambda x: parent.item_pop())
        xbtn.show()

        btn_box = Box(self)
        btn_box.horizontal = True
        btn_box.pack_end(sel_all)
        btn_box.pack_end(sel_none)
        btn_box.pack_end(xbtn)
        btn_box.show()

        self.pack_end(filelist)
        self.pack_end(btn_box)
        self.show()

    def select_all_cb(self, btn, filelist, h, all_selected=True):
        priorities = h.file_priorities()
        for n, p in enumerate(priorities):
            priorities[n] = all_selected
        h.prioritize_files(priorities)
        filelist.realized_items_update()

    def populate(self, filelist, h):
        filelist.clear()
        progress = h.file_progress()

        i = h.get_torrent_info()
        files = i.files()
        i_cls = FileSelectionClass()
        for n, file_entry in enumerate(files):
            filelist.item_append(
                i_cls,
                (file_entry, progress[n], n, h),
                None, 0)

    def item_activated_cb(self, gl, item):
        file_entry, progress, n, h = item.data
        if progress != file_entry.size:
            return

        path = os.path.join(h.save_path(), file_entry.path)

        if sys.platform == 'linux2':
            ecore.Exe("xdg-open '{0}'".format(path))
        else:
            os.startfile(path)

class FileSelectionClass(GenlistItemClass):
    def text_get(self, obj, part, data):
        file_entry, progress, n, h = data
        return "{} - {}/{} ({:.0%})".format(
            file_entry.path,
            intrepr(progress),
            intrepr(file_entry.size),
            float(progress)/float(file_entry.size)
        )

    def content_get(self, obj, part, data):
        file_entry, progress, n, h = data
        if part == "elm.swallow.icon":
            check = Check(obj)
            check.tooltip_text_set("Enable/disable file download")
            check.state = h.file_priorities()[n]
            check.callback_changed_add(self.toggle_file_enabled, n, h)
            return check

    def toggle_file_enabled(self, ck, n, h):
        priorities = h.file_priorities()
        priorities[n] = int(ck.state)
        h.prioritize_files(priorities)

