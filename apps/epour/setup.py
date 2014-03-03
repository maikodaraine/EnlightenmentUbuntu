#!/usr/bin/env python

from DistUtilsExtra.auto import setup

from epour.Globals import version

setup(name='epour',
   version=version,
   author='Kai Huuhko',
   author_email='kai.huuhko@gmail.com',
   maintainer='Kai Huuhko',
   maintainer_email='kai.huuhko@gmail.com',
   description='Simple torrent client',
   long_description='Epour is a simple torrent client using EFL and libtorrent.',
   #url='',
   #download_url='',
   license='GNU GPL',
   platforms='linux',
    requires=[
        'libtorrent',
        'evas',
        'ecore',
        'elementary',
        'e_dbus',
    ],
    provides=[
        'epour',
    ],
)
