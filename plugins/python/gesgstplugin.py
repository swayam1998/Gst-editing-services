#!/usr/bin/env python
# -*- Mode: Python -*-
# vi:si:et:sw=4:sts=4:ts=4
#
# Copyright (C) 2019 Igalia S.L
# Authors:
#   Thibault Saunier <tsaunier@igalia.com>
#

import sys

import gi
import tempfile
gi.require_version("GES", "1.0")
gi.require_version("Gst", "1.0")

from gi.repository import GObject
from gi.repository import Gst
Gst.init(None)
from gi.repository import GES

try:
    import opentimelineio as otio
    otio.adapters.from_name('xges')
except Exception as e:
    Gst.error("Could not load OpenTimelineIO: %s" % e)
    otio = None

class GESOtioFormatter(GES.Formatter):
    def do_can_load_uri(self, uri):
        if Gst.uri_get_protocol(uri) != "file":
            return False

        if uri.endswith(".xges"):
            return False

        try:
            return otio.adapters.from_filepath(Gst.uri_get_location(uri)) is not None
        except Exception:
            return False


    def do_load_from_uri(self, timeline, uri):
        location = Gst.uri_get_location(uri)
        in_adapter = otio.adapters.from_filepath(location)
        assert(in_adapter) # can_load_uri should have ensured it is loadable

        linker = otio.media_linker.MediaLinkingPolicy.ForceDefaultLinker
        otio_timeline = otio.adapters.read_from_file(
            location,
            in_adapter.name,
            media_linker_name=linker
        )

        with tempfile.NamedTemporaryFile(suffix=".xges") as tmpxges:
            otio.adapters.write_to_file(otio_timeline, tmpxges.name, "xges")
            formatter = GES.Formatter.get_default().extract()
            timeline.get_asset().add_formatter(formatter)
            return formatter.load_from_uri(timeline, "file://" + tmpxges.name)

if otio is not None:
    GObject.type_register(GESOtioFormatter)

    extensions = []
    for adapter in otio.plugins.ActiveManifest().adapters:
        if adapter.name != 'xges':
            extensions.extend(adapter.suffixes)
    GES.FormatterClass.register_metas(GESOtioFormatter, "otioformatter",
        "GES Formatter using OpenTimelineIO",
        ','.join(extensions), "application/otio", 0.1, Gst.Rank.SECONDARY)
