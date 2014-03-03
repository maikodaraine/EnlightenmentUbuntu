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

try:
    from efl.elementary.label import Label
    from efl.elementary.notify import Notify
    from efl.elementary.popup import Popup
    from efl.elementary.button import Button
except ImportError:
    from elementary import Label, Notify, Popup, Button

class Information(object):
    def __init__(self, canvas, text):
        n = Notify(canvas)
        l = Label(canvas)
        l.text = text
        n.content = l
        n.timeout = 3
        n.show()

class Error(object):
    def __init__(self, canvas, title, text):
        n = Popup(canvas)
        n.part_text_set("title,text", title)
        n.text = text
        b = Button(canvas)
        b.text = "OK"
        b.callback_clicked_add(lambda x: n.delete())
        n.part_content_set("button1", b)
        n.show()

class ConfirmExit(object):
    def __init__(self, canvas, exit_func):
        n = Popup(canvas)
        n.part_text_set("title,text", "Confirm exit")
        n.text = "Are you sure you wish to exit Epour?"
        b = Button(canvas)
        b.text = "Yes"
        b.callback_clicked_add(lambda x: exit_func())
        n.part_content_set("button1", b)
        b = Button(canvas)
        b.text = "No"
        b.callback_clicked_add(lambda x: n.delete())
        n.part_content_set("button2", b)
        n.show()
