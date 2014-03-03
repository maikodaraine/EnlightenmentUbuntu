#!/usr/bin/env python2
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

import sys
import os

from Globals import conf_dir, conf_path, data_dir
import logging

for d in conf_dir, data_dir:
    if not os.path.exists(d):
        os.mkdir(d, 0700)

def setup_log():
    log = logging.getLogger("epour")
    log.propagate = False
    log.setLevel(logging.INFO)

    ch = logging.StreamHandler()
    ch_formatter = logging.Formatter('%(name)s: [%(levelname)s] %(message)s')
    ch.setFormatter(ch_formatter)
    ch.setLevel(logging.DEBUG)
    log.addHandler(ch)

    fh = logging.FileHandler(os.path.join(data_dir, "epour.log"))
    fh_formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    fh.setFormatter(fh_formatter)
    fh.setLevel(logging.ERROR)
    log.addHandler(fh)

    return log

log = setup_log()

try:
    from e_dbus import DBusEcoreMainLoop
except ImportError:
    from efl.dbus_mainloop import DBusEcoreMainLoop

import dbus
ml = DBusEcoreMainLoop()
dbus.set_default_main_loop(ml)
import dbus.service
bus = dbus.SessionBus()

dbo = None
try:
    dbo = bus.get_object("net.launchpad.epour", "/net/launchpad/epour")
except dbus.exceptions.DBusException:
    pass

if dbo:
    if sys.argv[1:]:
        for f in sys.argv[1:]:
            log.info("Sending %s via dbus" % f)
            dbo.AddTorrent(f, dbus_interface="net.launchpad.epour")
    sys.exit()


from session import Session
from gui import MainInterface

class Epour(object):
    def __init__(self, torrents=None):
        session = self.session = Session(self)
        session.load_state()

        self.gui = MainInterface(self, session)

        session.load_torrents()

        # Add torrents from command line
        if torrents:
            for t in torrents:
                self.session.add_torrent(t)

        self.dbusname = dbus.service.BusName(
            "net.launchpad.epour", dbus.SessionBus()
        )
        self.dbo = EpourDBus(self)

        self.gui.run()

    def quit(self):
        session = self.session

        session.pause()

        try:
            session.save_torrents()
        except:
            log.exception("Saving torrents failed")

        try:
            session.save_state()
        except:
            log.exception("Saving session state failed")

        try:
            session.save_conf()
        except:
            log.exception("Saving conf failed")

class EpourDBus(dbus.service.Object):

    log = logging.getLogger("epour.dbus")

    def __init__(self, parent):
        self.parent = parent
        dbus.service.Object.__init__(self, dbus.SessionBus(),
            "/net/launchpad/epour", "net.launchpad.epour")

        self.props = {
        }

    @dbus.service.method(dbus_interface='net.launchpad.epour',
                         in_signature='s', out_signature='')
    def AddTorrent(self, f):
        self.log.info("Adding %s from dbus" % f)
        self.parent.session.add_torrent(str(f))

if __name__ == "__main__":
    log = logging.getLogger("epour")
    log.setLevel(logging.DEBUG)
    epour = Epour(sys.argv[1:])
    logging.shutdown()
