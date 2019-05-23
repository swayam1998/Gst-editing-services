# -*- coding: utf-8 -*-
#
# Copyright (c) 2019 Thibault Saunier <tsaunier@igalia.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this program; if not, write to the
# Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
# Boston, MA 02110-1301, USA.

from . import overrides_hack

import os
import tempfile

import gi

gi.require_version("GLib", "2.0")
gi.require_version("Gst", "1.0")
gi.require_version("GES", "1.0")
gi.require_version("GstVideo", "1.0")

from gi.repository import GLib  # noqa
from gi.repository import Gst  # noqa
from gi.repository import GstVideo  # noqa
from gi.repository import GES  # noqa
import unittest  # noqa
from unittest import mock

from .common import create_main_loop
from .common import create_project
from .common import GESSimpleTimelineTest  # noqa

Gst.init(None)
GES.init()


class TestFrameAccurateAPI(GESSimpleTimelineTest):

    def test_set_fstart(self):
        self.assertEqual(self.timeline.get_timecodes_config(), (True, 30, 1, GstVideo.VideoTimeCodeFlags.NONE))
        self.assertTrue(self.timeline.set_timecodes_config(30000, 1001, GstVideo.VideoTimeCodeFlags.NONE))

        clip = GES.TestClip.new()
        self.assertEqual(clip.props.start, 0)
        self.assertEqual(clip.props.in_point, 0)
        self.assertEqual(clip.props.duration, 0)

        self.assertTrue(clip.set_fstart(25))
        # Nothing changed as the clip is not in the timeline yet!
        self.assertEqual(clip.props.start, 0)
        self.assertEqual(clip.props.in_point, 0)
        self.assertEqual(clip.props.duration, 0)

        self.assertTrue(self.layer.add_clip(clip))
        self.assertEqual(clip.props.start, 834166666)
        self.assertEqual(clip.props.duration, 0)

        clip.props.fstart = 29
        self.assertTimelineTopology([
             [
                 (GES.TestClip, 29, GES.FRAME_NONE, GES.FRAME_NONE)
             ]
        ], in_frames=True)
        self.assertEqual(clip.props.start, 967633333)
        self.assertEqual(clip.get_fstart(), 29)

    def test_set_finpoint(self):
        self.assertTrue(self.timeline.set_timecodes_config(60, 1, GstVideo.VideoTimeCodeFlags.NONE))
        clip = GES.UriClip.new(Gst.filename_to_uri(os.path.join(__file__, "../../assets/30frames.ogv")))
        self.assertEqual(clip.get_natural_framerate(), (True, 30, 1))

        self.assertTrue(clip.set_finpoint(15))
        self.assertEqual(clip.get_finpoint(), 15)
        self.check_clip_values(clip, 0, 0, Gst.SECOND)

        self.assertFalse(self.layer.add_clip(clip))
        # FIXME Value was set... not ideal as the clip was not actually added!
        self.assertEqual(clip.props.in_point, Gst.SECOND / 2)

        self.assertTrue(clip.set_finpoint(30))
        self.assertEqual(clip.props.in_point, Gst.SECOND / 2)

        testclip = GES.TestClip.new()
        self.layer.add_clip(testclip)
        self.assertTrue(testclip.set_finpoint(30))
        self.assertEqual(testclip.get_finpoint(), 30)
        self.assertEqual(testclip.props.in_point, Gst.SECOND / 2)
        testclip.props.in_point = Gst.SECOND * 2
        self.assertEqual(testclip.get_finpoint(), 30)

    def test_set_fduration(self):
        self.assertTrue(self.timeline.set_timecodes_config(30, 1, GstVideo.VideoTimeCodeFlags.NONE))

        testclip = GES.TestClip.new()
        self.layer.add_clip(testclip)

        # Do not snap to next when setting to 0
        testclip.props.duration = 0
        self.assertEqual(testclip.props.duration, 0)

        # Snapping to the next frame when setting the duration
        testclip.props.fduration = 1
        self.assertEqual(testclip.props.fduration, 1)
        self.assertEqual(testclip.props.duration, 33333333)

    def check_clip_property(self, clip, prop, value):
        self.assertEqual(clip.get_property(prop), value, "for %s" % prop)
        for child in clip.get_children(True):
            self.assertEqual(child.get_property(prop), value,
                msg="%s inside %s" % (clip.props.name, child.props.name))

    def test_change_timeline_framerate(self):
        self.assertTrue(self.timeline.set_timecodes_config(30, 1, GstVideo.VideoTimeCodeFlags.NONE))

        testclip = GES.TestClip.new()
        self.assertTrue(self.layer.add_clip(testclip))

        testclip.props.duration = 0
        self.assertEqual(testclip.props.duration, 0)

        # Snapping to the next frame when setting the duration
        testclip.props.fduration = 1
        self.assertEqual(testclip.props.duration, int(Gst.SECOND / 30))
        self.assertEqual(testclip.get_fduration(), 1)
        self.assertTrue(self.timeline.set_timecodes_config(100, 1, GstVideo.VideoTimeCodeFlags.NONE))
        self.check_clip_property(testclip, "duration", Gst.SECOND / 100 * 3)
        self.assertEqual(testclip.get_fduration(), 3)

    def test_adding_layer_add_fasset(self):
        self.assertTrue(self.timeline.set_timecodes_config(30, 1, GstVideo.VideoTimeCodeFlags.DROP_FRAME))
        asset = GES.UriClipAsset.request_sync(Gst.filename_to_uri(os.path.join(__file__, "../../assets/30frames.ogv")))

        clip = self.layer.add_fasset(asset, 0, 10, 15, GES.TrackType.UNKNOWN)
        self.assertTimelineTopology([
             [
                 (GES.UriClip, 0, 10, 15)
             ]
        ], in_frames=True)
        self.assertTimelineTopology([
             [
                 (GES.UriClip, 0, int(Gst.SECOND / 3), int(Gst.SECOND) / 2)
             ]
        ])

    def test_ftrim(self):
        self.assertTrue(self.timeline.set_timecodes_config(30, 1, GstVideo.VideoTimeCodeFlags.DROP_FRAME))
        asset = GES.UriClipAsset.request_sync(Gst.filename_to_uri(os.path.join(__file__, "../../assets/30frames.ogv")))

        clip = self.layer.add_fasset(asset, 0, 10, 15, GES.TrackType.UNKNOWN)
        self.assertTimelineTopology([
             [
                 (GES.UriClip, 0, 10, 15)
             ]
        ], in_frames=True)
        self.assertTrue(clip.ftrim(5))
        self.assertTimelineTopology([
             [
                 (GES.UriClip, 5, 15, 10)
             ]
        ], in_frames=True)

    def test_edit_trim(self):
        self.assertTrue(self.timeline.set_timecodes_config(30, 1, GstVideo.VideoTimeCodeFlags.DROP_FRAME))
        asset = GES.UriClipAsset.request_sync(Gst.filename_to_uri(os.path.join(__file__, "../../assets/30frames.ogv")))

        clip = self.layer.add_fasset(asset, 5, 15, 10, GES.TrackType.UNKNOWN)
        self.assertTrue(clip.fedit([], -1, GES.EditMode.EDIT_TRIM, GES.Edge.EDGE_START, 0))
        self.assertTimelineTopology([
             [
                 (GES.UriClip, 0, 10, 15)
             ]
        ], in_frames=True, check_track_elements=True)

        self.assertTrue(clip.fedit([], 1, GES.EditMode.EDIT_TRIM, GES.Edge.EDGE_START, 5))
        self.assertTimelineTopology([
            [
                (GES.UriClip, 5, 15, 10)
            ]
        ], in_frames=True, check_track_elements=True)

        self.assertTimelineTopology([
            [
                (GES.UriClip, 166666666, 500000000, 333333333)
            ]
        ], check_track_elements=True)

    def test_simple_edit_trim_natural_framerate(self):
        self.assertTrue(self.timeline.set_timecodes_config(25, 1, GstVideo.VideoTimeCodeFlags.NONE))
        asset = GES.UriClipAsset.request_sync(Gst.filename_to_uri(os.path.join(__file__, "../../assets/30frames.ogv")))

        clip = self.layer.add_fasset(asset, 0, 0, 20, GES.TrackType.UNKNOWN)
        self.assertTrue(clip.fedit([], -1, GES.EditMode.EDIT_TRIM, GES.Edge.EDGE_START, 10))
        self.assertTimelineTopology([
             [
                 (GES.UriClip, 10, 10, 10)
             ]
        ], in_frames=True)

    def test_simple_serialize(self):
        self.test_simple_edit_trim_natural_framerate()

        with tempfile.NamedTemporaryFile() as tmpxges:
            uri = Gst.filename_to_uri(tmpxges.name)
            self.timeline.save_to_uri(uri, None, True)

            Gst.error("Load....")
            self.timeline = GES.Timeline.new_from_uri(uri)
            self.print_timeline()
            self.assertTimelineTopology([
                [
                    (GES.UriClip, 10, 10, 10)
                ]
            ], in_frames=True)