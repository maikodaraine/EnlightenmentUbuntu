#!/usr/bin/env python

from distutils.core import setup


setup(
    name = 'espionage',
    version = '0.9',
    description = 'D-Bus inspector',
    long_description = 'Espionage is a complete D-Bus inspector that use the EFL',
    license = "GNU GPL",
    author = 'Dave Andreoli',
    author_email = 'dave@gurumeditation.it',
    packages = ['espionage'],
    requires = ['efl', 'dbus', 'json', 'xml.etree'],
    provides = ['espionage'],
    package_data = {
        'espionage': ['themes/*/*'],
    },
    data_files = [
        ('bin', ['bin/espionage']),
        ('share/applications', ['data/espionage.desktop']),
        ('share/icons', ['data/icons/256x256/espionage.png']),
        ('share/icons/hicolor/256x256/apps', ['data/icons/256x256/espionage.png']),
    ]
)

