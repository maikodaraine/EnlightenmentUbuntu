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

import os
import mimetypes
from ConfigParser import SafeConfigParser
import cPickle
import urlparse
import urllib
import HTMLParser

import shutil
import cgi

import libtorrent as lt

try:
    from efl.ecore import Timer
except:
    from ecore import Timer

import logging

from Globals import conf_dir, conf_path, data_dir

class Session(lt.session):
    def __init__(self, parent):
        self.parent = parent

        self.log = logging.getLogger("epour.session")

        from Globals import version
        ver_ints = []
        for s in version.split("."):
            ver_ints.append(int(s))

        fp = lt.fingerprint("EP", *ver_ints)
        self.log.debug("peer-id: {}".format(fp))

        lt.session.__init__(
            self,
            fingerprint = fp,
            # flags = \
                #lt.session_flags_t.add_default_plugins | \
                #lt.session_flags_t.start_default_features
        )

        self.log.info("Session started")

        self.torrents = {}

                #sdpipsdtsppe
                #theprtertoer
                #atrboabaorer
                #t|flgtucrtro
                #s|oorugkam|r
                #||rces|ega||
                #||mks||rep||
        mask = 0b000001000001
        self.set_alert_mask(mask)

        conf = self.conf = self.setup_conf()

        self.listen_on(
            conf.getint("Settings", "listen_low"),
            conf.getint("Settings", "listen_high")
        )

        self.alert_manager = AlertManager(self)
        self.alert_manager.callback_add(
            "metadata_received_alert", self.metadata_received)

    def metadata_received(self, a):
        h = a.handle
        ihash = str(h.info_hash())
        self.log.debug("Metadata received.")
        t_path = self.write_torrent(h)
        self.torrents[ihash] = t_path

    def write_torrent(self, h):
        t_info = h.get_torrent_info()
        ihash = str(h.info_hash())

        self.log.debug("Writing torrent file {}".format(ihash))

        md = lt.bdecode(t_info.metadata())
        t = {}
        t["info"] = md
        t_path = os.path.join(
            data_dir, "{}.torrent".format(ihash)
        )
        with open(t_path, "wb") as f:
            f.write(lt.bencode(t))

        return t_path


    def setup_conf(self):
        conf = SafeConfigParser({
            "storage_path": os.path.expanduser(
                os.path.join("~", "Downloads")
            ),
            "confirmations": str(False),
            "delete_original": str(False),
            "listen_low": str(0),
            "listen_high": str(0),
        })

        conf.read(conf_path)

        if not conf.has_section("Settings"):
            conf.add_section("Settings")

        return conf

    def save_conf(self):
        with open(conf_path, 'wb') as configfile:
            self.conf.write(configfile)

    def load_state(self):
        try:
            with open(os.path.join(data_dir, "session"), 'rb') as f:
                state = lt.bdecode(f.read())
            lt.session.load_state(self, state)
        except:
            self.log.debug("Could not load previous session state.")

        settings = self.settings()
        from Globals import version
        settings.user_agent = "Epour/{} libtorrent/{}".format(version, lt.version)
        self.set_settings(settings)

    def save_state(self):
        state = lt.session.save_state(self)

        with open(os.path.join(data_dir, "session"), 'wb') as f:
            f.write(lt.bencode(state))

        self.log.debug("Session state saved.")

    def load_torrents(self):
        torrents_path = os.path.join(data_dir, "torrents")
        if not os.path.exists(torrents_path):
            self.log.debug("No list of torrents found.")
            return

        try:
            pkl_file = open(torrents_path, 'rb')
        except IOError:
            self.log.warning("Could not open the list of torrents.")
        else:
            try:
                paths = cPickle.load(pkl_file)
            except EOFError:
                self.log.exception("Opening the list of torrents failed.")
            else:
                self.log.debug(
                    "List of torrents opened, "
                    "restoring {} torrents.".format(len(paths))
                )
                for k, v in paths.iteritems():
                    try:
                        self.log.debug("Adding {}".format(v))
                        self.add_torrent(v)
                    except:
                        self.log.exception(
                            "Restoring torrent {0} failed".format(v)
                        )
            finally:
                pkl_file.close()

    def save_torrents(self):
        self.log.debug("Saving {} torrents.".format(len(self.torrents)))
        with open(os.path.join(data_dir, "torrents"), 'wb') as f:
            cPickle.dump(self.torrents, f)

        self.log.debug("List of torrents saved.")

        # Save fast resume data
        for h in self.get_torrents():
            if not h.is_valid() or not h.has_metadata():
                continue
            data = lt.bencode(h.write_resume_data())
            with open(os.path.join(
                    data_dir, '{}.fastresume'.format(h.info_hash())
                ), 'wb'
            ) as f:
                f.write(data)

        self.log.debug("Fast resume data saved.")

    def add_torrent(self, t_uri):
        if not t_uri:
            return

        storage_path = self.conf.get("Settings", "storage_path")

        if not t_uri.startswith("magnet"):
            mimetype = mimetypes.guess_type(t_uri)[0]
            if not mimetype == "application/x-bittorrent":
                self.log.error("Invalid file")
                return

            if t_uri.startswith("file://"):
                t_uri = urllib.unquote(urlparse.urlsplit(t_uri).path)

            with open(t_uri, 'rb') as t:
                t_raw = lt.bdecode(t.read())

            info = lt.torrent_info(t_raw)
            rd = None
            try:
                with open(os.path.join(
                    data_dir, "{}.fastresume".format(info.info_hash())
                    ), "rb"
                ) as f:
                    rd = lt.bdecode(f.read())
            except:
                try:
                    with open(os.path.join(
                        data_dir, "{}.fastresume".format(info.name())
                        ), "rb"
                    ) as f:
                        rd = lt.bdecode(f.read())
                except:
                    self.log.debug("Invalid resume data")

            h = lt.session.add_torrent(
                self, info, storage_path, resume_data=rd)

            ihash = str(h.info_hash())

            new_uri = os.path.join(
                data_dir, "{}.torrent".format(ihash)
            )

            if t_uri == new_uri:
                pass
            else:
                shutil.copy(t_uri, new_uri)

                if self.conf.getboolean("Settings", "delete_original"):
                    self.log.debug("Deleting original torrent file {}".format(t_uri))
                    os.remove(t_uri)

                t_uri = new_uri
        else:
            t_uri = urllib.unquote(t_uri)
            t_uri = str(HTMLParser.HTMLParser().unescape(t_uri))
            h = lt.add_magnet_uri(
                self, t_uri,
                { "save_path": str(storage_path) }
            )

        if not h.is_valid():
            self.log.error("Invalid torrent handle")
            return

        ihash = str(h.info_hash())

        self.torrents[ihash] = t_uri

        if not hasattr(lt, "torrent_added_alert"):
            class torrent_added_alert(object):
                def __init__(self, h):
                    self.handle = h

            a = torrent_added_alert(h)

            self.alert_manager.signal(a)

    def remove_torrent(self, h, with_data=False):
        ihash = str(h.info_hash())

        fr_path = os.path.join(
            data_dir, "{}.fastresume".format(ihash)
        )
        t_path = self.torrents[ihash]

        del self.torrents[ihash]
        lt.session.remove_torrent(self, h, option=with_data)

        try:
            with open(fr_path): pass
        except IOError:
            self.log.debug("Could not remove fast resume data.")
        else:
            os.remove(fr_path)

        try:
            with open(t_path): pass
        except IOError:
            self.log.debug("Could not remove torrent file.")
        else:
            os.remove(t_path)

        if not hasattr(lt, "torrent_removed_alert"):
            class torrent_removed_alert(object):
                def __init__(self, h, info_hash):
                    self.handle = h
                    self.info_hash = info_hash

            a = torrent_removed_alert(h, ihash)

            self.alert_manager.signal(a)

        return ihash


class TorrentDict(dict):

    # Required keys are save_path and either ti or info_hash

    # torrent_info ti
    # string tracker_url
    # string info_hash
    # string name
    # string save_path
    # string resume_data
    # storage_mode_t storage_mode
    # bool paused
    # bool auto_managed
    # bool duplicate_is_error
    # storage?
    # object userdata?
    # bool seed_mode
    # bool override_resume_data
    # bool upload_mode

    def __init__(self, **kwargs):
        dict.__init__(self, **kwargs)

class AlertManager(object):

    log = logging.getLogger("epour.alert")
    update_interval = 0.2
    alerts = {}

    def __init__(self, session):
        self.session = session

        self.timer = Timer(self.update_interval, self.update)

    def callback_add(self, alert_type, cb, *args, **kwargs):
        if not self.alerts.has_key(alert_type):
            self.alerts[alert_type] = []
        self.alerts[alert_type].append((cb, args, kwargs))

    def callback_del(self, alert_type, cb, *args, **kwargs):
        for i, a in enumerate(self.alerts):
            if a == (cb, args, kwargs):
                del(self.alerts[alert_type][i])

    def signal(self, a):
        a_name = type(a).__name__

        if not self.alerts.has_key(a_name):
            self.log.debug("No handler: {} | {}".format(a_name, a))
            return

        for cb, args, kwargs in self.alerts[a_name]:
            try:
                cb(a, *args, **kwargs)
            except:
                self.log.exception("Exception while handling alerts")

    def update(self):
        while 1:
            a = self.session.pop_alert()
            if not a: break

            self.signal(a)

        return True

