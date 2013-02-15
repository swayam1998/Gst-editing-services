/* GStreamer Editing Services
 *
 * Copyright (C) <2012> Thibault Saunier <thibault.saunier@collabora.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "test-utils.h"
#include <ges/ges.h>
#include <gst/check/gstcheck.h>

static gboolean
my_fill_track_func (GESClip * clip,
    GESTrackElement * track_element, GstElement * gnlobj, gpointer user_data)
{
  GstElement *src;

  GST_DEBUG ("timelineobj:%p, trackelementec:%p, gnlobj:%p",
      clip, track_element, gnlobj);

  /* Let's just put a fakesource in for the time being */
  src = gst_element_factory_make ("fakesrc", NULL);
  /* If this fails... that means that there already was something
   * in it */
  fail_unless (gst_bin_add (GST_BIN (gnlobj), src));

  return TRUE;
}

static inline GESClip *
create_custom_clip (void)
{
  return GES_CLIP (ges_custom_source_clip_new (my_fill_track_func, NULL));
}

#define CHECK_OBJECT_PROPS(obj, start, inpoint, duration) {\
  assert_equals_uint64 (_START (obj), start);\
  assert_equals_uint64 (_INPOINT (obj), inpoint);\
  assert_equals_uint64 (_DURATION (obj), duration);\
}

GST_START_TEST (test_basic_timeline_edition)
{
  GESTrack *track;
  GESTimeline *timeline;
  GESTrackElement *trackelement, *trackelement1, *trackelement2;
  GESClip *clip, *clip1, *clip2;

  ges_init ();

  track = ges_track_new (GES_TRACK_TYPE_CUSTOM, gst_caps_ref (GST_CAPS_ANY));
  fail_unless (track != NULL);

  timeline = ges_timeline_new ();
  fail_unless (timeline != NULL);

  fail_unless (ges_timeline_add_track (timeline, track));

  clip = create_custom_clip ();
  clip1 = create_custom_clip ();
  clip2 = create_custom_clip ();


  fail_unless (clip && clip1 && clip2);

  /**
   * Our timeline
   *
   * inpoints 0-------   0--------      0-----------
   *          |  clip  |  |  clip1  |     |     clip2  |
   * time     0------- 10 --------20    50---------60
   */
  g_object_set (clip, "start", (guint64) 0, "duration", (guint64) 10,
      "in-point", (guint64) 0, NULL);
  g_object_set (clip1, "start", (guint64) 10, "duration", (guint64) 10,
      "in-point", (guint64) 0, NULL);
  g_object_set (clip2, "start", (guint64) 50, "duration", (guint64) 60,
      "in-point", (guint64) 0, NULL);

  trackelement = ges_clip_create_track_element (clip, track->type);
  fail_unless (trackelement != NULL);
  fail_unless (ges_clip_add_track_element (clip, trackelement));
  fail_unless (ges_track_add_element (track, trackelement));
  assert_equals_uint64 (_DURATION (trackelement), 10);

  trackelement1 = ges_clip_create_track_element (clip1, track->type);
  fail_unless (trackelement1 != NULL);
  fail_unless (ges_clip_add_track_element (clip1, trackelement1));
  fail_unless (ges_track_add_element (track, trackelement1));
  assert_equals_uint64 (_DURATION (trackelement1), 10);

  trackelement2 = ges_clip_create_track_element (clip2, track->type);
  fail_unless (ges_clip_add_track_element (clip2, trackelement2));
  fail_unless (trackelement2 != NULL);
  fail_unless (ges_track_add_element (track, trackelement2));
  assert_equals_uint64 (_DURATION (trackelement2), 60);

  /**
   * Simple rippling clip to: 10
   *
   * New timeline:
   * ------------
   *
   * inpoints 0-------   0--------      0-----------
   *          |  clip  |  |  clip1  |     |   clip2    |
   * time    10------- 20 --------30    60---------120
   */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_NONE, 10) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 10, 0, 10);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 60, 0, 60);


  /* FIXME find a way to check that we are using the same MovingContext
   * inside the GESTrack */
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_NONE, 40) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 10, 0, 10);
  CHECK_OBJECT_PROPS (trackelement1, 40, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 80, 0, 60);

  /**
   * Rippling clip1 back to: 20 (getting to the exact same timeline as before
   */
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_NONE, 20) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 10, 0, 10);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 60, 0, 60);

  /**
   * Simple move clip to: 27 and clip2 to 35
   *
   * New timeline:
   * ------------
   *                    0------------
   * inpoints   0-------|---  clip 0--|----------
   *            |  clip1 27 -|-----|-37   clip2   |
   * time      20-----------30   35-------------120
   */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_NORMAL,
          GES_EDGE_NONE, 27) == TRUE);
  fail_unless (ges_clip_edit (clip2, NULL, -1, GES_EDIT_MODE_NORMAL,
          GES_EDGE_NONE, 35) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 27, 0, 10);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 35, 0, 60);

  /**
   * Trim start clip to: 32 and clip2 to 35
   *
   * New timeline:
   * ------------
   *                           5--------
   * inpoints   0-----------   | clip 0--|----------
   *            |  clip1     |  32----|-37   clip2   |
   * time      20-----------30      35-------------120
   */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_TRIM,
          GES_EDGE_START, 32) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 32, 5, 5);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 35, 0, 60);

  /* Ripple end clip to 42
   * New timeline:
   * ------------
   *                           5--------
   * inpoints   0-----------   | clip 0--|----------
   *            |  clip1     |  32----|-42   clip2   |
   * time      20-----------30      35-------------120
   */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_END, 42) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 32, 5, 10);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 35, 0, 60);

  /**
   * New timeline:
   * ------------
   * inpoints 0-------     5-------- 0-----------
   *          |  clip1 |    |  clip1  ||  clip2    |
   * time    20-------30  32--------52 ---------112
   */
  fail_unless (ges_clip_edit (clip2, NULL, -1, GES_EDIT_MODE_NORMAL,
          GES_EDGE_NONE, 42) == TRUE);
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_END, 52) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 32, 5, 20);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 52, 0, 60);

  /**
   * New timeline:
   * ------------
   * inpoints 0-------     5-------- 0------------
   *          |  clip1 |    |  clip   ||    clip2    |
   * time    20-------40  42--------62 ---------122
   */
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_END, 40) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 42, 5, 20);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 20);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /**
   * New timeline:
   * ------------
   * inpoints 0------- 0-------- 0-----------
   *          |  clip1 ||   clip  ||  clip2     |
   * time    20------ 25 ------ 62 ---------122
   */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_TRIM,
          GES_EDGE_START, 40) == TRUE);
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_ROLL,
          GES_EDGE_START, 25) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 5);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /* Make sure that not doing anything when not able to roll */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_ROLL,
          GES_EDGE_START, 65) == TRUE);
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_ROLL,
          GES_EDGE_END, 65) == TRUE, 0);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 5);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  g_object_unref (timeline);
  g_object_unref (clip);
  g_object_unref (clip1);
  g_object_unref (clip2);
}

GST_END_TEST;

GST_START_TEST (test_snapping)
{
  GESTrack *track;
  GESTimeline *timeline;
  GESTrackElement *trackelement, *trackelement1, *trackelement2;
  GESClip *clip, *clip1, *clip2;
  GESTimelineLayer *layer;
  GList *trackelements;

  ges_init ();

  track = ges_track_new (GES_TRACK_TYPE_CUSTOM, gst_caps_ref (GST_CAPS_ANY));
  fail_unless (track != NULL);

  timeline = ges_timeline_new ();
  fail_unless (timeline != NULL);

  fail_unless (ges_timeline_add_track (timeline, track));

  clip = create_custom_clip ();
  clip1 = create_custom_clip ();
  clip2 = create_custom_clip ();

  fail_unless (clip && clip1 && clip2);

  /**
   * Our timeline
   * ------------
   * inpoints 0------- 0-------- 0-----------
   *          |  clip1 ||   clip  ||  clip2     |
   * time    20------ 25 ------ 62 ---------122
   */
  g_object_set (clip, "start", (guint64) 25, "duration", (guint64) 37,
      "in-point", (guint64) 0, NULL);
  g_object_set (clip1, "start", (guint64) 20, "duration", (guint64) 15,
      "in-point", (guint64) 0, NULL);
  g_object_set (clip2, "start", (guint64) 62, "duration", (guint64) 60,
      "in-point", (guint64) 0, NULL);

  fail_unless ((layer = ges_timeline_append_layer (timeline)) != NULL);
  assert_equals_int (ges_timeline_layer_get_priority (layer), 0);


  fail_unless (ges_timeline_layer_add_clip (layer, clip));
  fail_unless ((trackelements = ges_clip_get_track_elements (clip)) != NULL);
  fail_unless ((trackelement =
          GES_TRACK_ELEMENT (trackelements->data)) != NULL);
  fail_unless (ges_track_element_get_track (trackelement) == track);
  assert_equals_uint64 (_DURATION (trackelement), 37);
  g_list_free_full (trackelements, g_object_unref);

  /* We have 3 references to trackelement from:
   *  track + timeline + clip */
  ASSERT_OBJECT_REFCOUNT (trackelement, "First trackelement", 3);
  /* We have 1 ref to clip1:
   * + layer */
  ASSERT_OBJECT_REFCOUNT (clip, "First clip", 1);

  fail_unless (ges_timeline_layer_add_clip (layer, clip1));
  fail_unless ((trackelements = ges_clip_get_track_elements (clip1)) != NULL);
  fail_unless ((trackelement1 =
          GES_TRACK_ELEMENT (trackelements->data)) != NULL);
  fail_unless (ges_track_element_get_track (trackelement1) == track);
  assert_equals_uint64 (_DURATION (trackelement1), 15);
  g_list_free_full (trackelements, g_object_unref);

  /* Same ref logic */
  ASSERT_OBJECT_REFCOUNT (trackelement1, "First trackelement", 3);
  ASSERT_OBJECT_REFCOUNT (clip1, "First clip", 1);

  fail_unless (ges_timeline_layer_add_clip (layer, clip2));
  fail_unless ((trackelements = ges_clip_get_track_elements (clip2)) != NULL);
  fail_unless ((trackelement2 =
          GES_TRACK_ELEMENT (trackelements->data)) != NULL);
  fail_unless (ges_track_element_get_track (trackelement2) == track);
  assert_equals_uint64 (_DURATION (trackelement2), 60);
  g_list_free_full (trackelements, g_object_unref);

  /* Same ref logic */
  ASSERT_OBJECT_REFCOUNT (trackelement2, "First trackelement", 3);
  ASSERT_OBJECT_REFCOUNT (clip2, "First clip", 1);

  /* Snaping to edge, so no move */
  g_object_set (timeline, "snapping-distance", (guint64) 3, NULL);
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_TRIM,
          GES_EDGE_END, 27) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 5);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /* Snaping to edge, so no move */
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_TRIM,
          GES_EDGE_END, 27) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 5);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /**
   * New timeline:
   * ------------
   *                    0----------- 0-------------
   * inpoints   0-------|--   clip   ||   clip2      |
   *            |  clip1 25-|------- 62 -----------122
   * time      20----------30
   */
  g_object_set (timeline, "snapping-distance", (guint64) 0, NULL);
  ges_timeline_element_set_duration (GES_TIMELINE_ELEMENT (clip1), 10);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /**
   * New timeline(the "layers" are just to help reading diagram, nothing else):
   * ------------
   *                    0----------
   *                    |   clip    |
   *                    25---------62
   * inpoints   0----------------------- 10--------
   *            |       clip1            ||  clip2   |
   * time      20---------------------- 72 --------122
   */
  /* Rolling involves only neighbour that are currently snapping */
  fail_unless (ges_timeline_element_roll_end (GES_TIMELINE_ELEMENT (clip1),
          62));
  fail_unless (ges_timeline_element_roll_end (GES_TIMELINE_ELEMENT (clip1),
          72) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 52);
  CHECK_OBJECT_PROPS (trackelement2, 72, 10, 50);

  /**
   *                    0----------
   *                    |   clip    |
   *                    25---------62
   * inpoints           5--------------- 10--------
   *                    |     clip1      ||  clip2   |
   * time               25------------- 72 --------122
   */
  g_object_set (timeline, "snapping-distance", (guint64) 4, NULL);
  fail_unless (ges_timeline_element_trim (GES_TIMELINE_ELEMENT (clip1),
          28) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 25, 5, 47);
  CHECK_OBJECT_PROPS (trackelement2, 72, 10, 50);

  /**
   *                    0----------
   *                    |   clip    |
   *                    25---------62
   * inpoints           5---------- 0---------
   *                    |  clip1    ||  clip2   |
   * time               25-------- 62 --------122
   */
  fail_unless (ges_timeline_element_roll_start (GES_TIMELINE_ELEMENT (clip2),
          59) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 25, 5, 37);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

   /**
   * inpoints           0----------5---------- 0----------
   *                    |   clip    ||  clip1    ||  clip2   |
   * time               25---------62-------- 99 --------170
   */
  fail_unless (ges_timeline_element_ripple (GES_TIMELINE_ELEMENT (clip1),
          58) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 62, 5, 37);
  CHECK_OBJECT_PROPS (trackelement2, 99, 0, 60);

  /**
   * inpoints     0----------5----------     0----------
   *              |   clip    ||  clip1    |   |  clip2    |
   * time         25---------62-------- 99  110--------170
   */
  ges_timeline_element_set_start (GES_TIMELINE_ELEMENT (clip2), 110);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 62, 5, 37);
  CHECK_OBJECT_PROPS (trackelement2, 110, 0, 60);

  /**
   * inpoints     0----------5    5 --------- 0----------
   *              |   clip    |    |  clip1    ||  clip2    |
   * time         25---------62   73---------110--------170
   */
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_NORMAL,
          GES_EDGE_NONE, 72) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 73, 5, 37);
  CHECK_OBJECT_PROPS (trackelement2, 110, 0, 60);

  /**
   * inpoints     0----------5----------     0----------
   *              |   clip    ||  clip1    |   |  clip2    |
   * time         25---------62-------- 99  110--------170
   */
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_NORMAL,
          GES_EDGE_NONE, 58) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 62, 5, 37);
  CHECK_OBJECT_PROPS (trackelement2, 110, 0, 60);


  /**
   * inpoints     0----------5---------- 0----------
   *              |   clip    ||  clip1   ||  clip2    |
   * time         25---------62--------110--------170
   */
  g_object_set (clip1, "duration", 46, NULL);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 62, 5, 48);
  CHECK_OBJECT_PROPS (trackelement2, 110, 0, 60);

  /**
   * inpoints     5----------- 0--------- 0----------
   *              |   clip1    ||  clip2   ||  clip     |
   * time         62---------110--------170--------207
   */
  g_object_set (clip, "start", 168, NULL);
  CHECK_OBJECT_PROPS (trackelement, 170, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 62, 5, 48);
  CHECK_OBJECT_PROPS (trackelement2, 110, 0, 60);

  /* Check we didn't lose/screwed any references */
  ASSERT_OBJECT_REFCOUNT (trackelement, "First trackelement", 3);
  ASSERT_OBJECT_REFCOUNT (trackelement1, "Second trackelement", 3);
  ASSERT_OBJECT_REFCOUNT (trackelement2, "Third trackelement", 3);
  ASSERT_OBJECT_REFCOUNT (clip, "First clip", 1);
  ASSERT_OBJECT_REFCOUNT (clip1, "Second clip", 1);
  ASSERT_OBJECT_REFCOUNT (clip2, "Third clip", 1);

  g_object_unref (timeline);

  /* Check we destroyed everything */
  fail_if (G_IS_OBJECT (trackelement));
  fail_if (G_IS_OBJECT (trackelement1));
  fail_if (G_IS_OBJECT (trackelement2));
  fail_if (G_IS_OBJECT (clip));
  fail_if (G_IS_OBJECT (clip1));
  fail_if (G_IS_OBJECT (clip2));
  fail_if (G_IS_OBJECT (layer));
}

GST_END_TEST;

GST_START_TEST (test_timeline_edition_mode)
{
  guint i;
  GESTrack *track;
  GESTimeline *timeline;
  GESTrackElement *trackelement, *trackelement1, *trackelement2;
  GESClip *clip, *clip1, *clip2;
  GESTimelineLayer *layer, *layer1, *layer2;
  GList *trackelements, *layers, *tmp;

  ges_init ();

  track = ges_track_new (GES_TRACK_TYPE_CUSTOM, gst_caps_ref (GST_CAPS_ANY));
  fail_unless (track != NULL);

  timeline = ges_timeline_new ();
  fail_unless (timeline != NULL);

  fail_unless (ges_timeline_add_track (timeline, track));

  clip = create_custom_clip ();
  clip1 = create_custom_clip ();
  clip2 = create_custom_clip ();

  fail_unless (clip && clip1 && clip2);

  /**
   * Our timeline
   *
   *          0-------
   * layer:   |  clip  |
   *          0-------10
   *
   *                   0--------     0-----------
   * layer1:           |  clip1  |    |     clip2  |
   *                  10--------20   50---------60
   */
  g_object_set (clip, "start", (guint64) 0, "duration", (guint64) 10,
      "in-point", (guint64) 0, NULL);
  g_object_set (clip1, "start", (guint64) 10, "duration", (guint64) 10,
      "in-point", (guint64) 0, NULL);
  g_object_set (clip2, "start", (guint64) 50, "duration", (guint64) 60,
      "in-point", (guint64) 0, NULL);

  fail_unless ((layer = ges_timeline_append_layer (timeline)) != NULL);
  assert_equals_int (ges_timeline_layer_get_priority (layer), 0);


  fail_unless (ges_timeline_layer_add_clip (layer, clip));
  fail_unless ((trackelements = ges_clip_get_track_elements (clip)) != NULL);
  fail_unless ((trackelement =
          GES_TRACK_ELEMENT (trackelements->data)) != NULL);
  fail_unless (ges_track_element_get_track (trackelement) == track);
  assert_equals_uint64 (_DURATION (trackelement), 10);
  g_list_free_full (trackelements, g_object_unref);

  /* Add a new layer and add clipects to it */
  fail_unless ((layer1 = ges_timeline_append_layer (timeline)) != NULL);
  fail_unless (layer != layer1);
  assert_equals_int (ges_timeline_layer_get_priority (layer1), 1);

  fail_unless (ges_timeline_layer_add_clip (layer1, clip1));
  fail_unless ((trackelements = ges_clip_get_track_elements (clip1)) != NULL);
  fail_unless ((trackelement1 =
          GES_TRACK_ELEMENT (trackelements->data)) != NULL);
  fail_unless (ges_track_element_get_track (trackelement1) == track);
  assert_equals_uint64 (_DURATION (trackelement1), 10);
  g_list_free_full (trackelements, g_object_unref);

  fail_unless (ges_timeline_layer_add_clip (layer1, clip2));
  fail_unless ((trackelements = ges_clip_get_track_elements (clip2)) != NULL);
  fail_unless ((trackelement2 =
          GES_TRACK_ELEMENT (trackelements->data)) != NULL);
  fail_unless (ges_track_element_get_track (trackelement2) == track);
  assert_equals_uint64 (_DURATION (trackelement2), 60);
  g_list_free_full (trackelements, g_object_unref);

  /**
   * Simple rippling clip to: 10
   *
   * New timeline:
   * ------------
   *
   * inpoints 0-------
   *          |  clip  |
   * time    10-------20
   *
   *                   0--------      0-----------
   *                   |  clip1  |     |   clip2    |
   *                  20--------30    60--------120
   */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_NONE, 10) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 10, 0, 10);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 60, 0, 60);


  /* FIXME find a way to check that we are using the same MovingContext
   * inside the GESTimeline */
  fail_unless (ges_clip_edit (clip1, NULL, 3, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_NONE, 40) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 10, 0, 10);
  CHECK_OBJECT_PROPS (trackelement1, 40, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 80, 0, 60);
  layer2 = ges_clip_get_layer (clip1);
  assert_equals_int (ges_timeline_layer_get_priority (layer2), 3);
  /* clip2 should have moved layer too */
  fail_unless (ges_clip_get_layer (clip2) == layer2);
  /* We got 2 reference to the same clipect, unref them */
  g_object_unref (layer2);
  g_object_unref (layer2);

  /**
   * Rippling clip1 back to: 20 (getting to the exact same timeline as before
   */
  fail_unless (ges_clip_edit (clip1, NULL, 1, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_NONE, 20) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 10, 0, 10);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 60, 0, 60);
  layer2 = ges_clip_get_layer (clip1);
  assert_equals_int (ges_timeline_layer_get_priority (layer2), 1);
  /* clip2 should have moved layer too */
  fail_unless (ges_clip_get_layer (clip2) == layer2);
  /* We got 2 reference to the same clipect, unref them */
  g_object_unref (layer2);
  g_object_unref (layer2);

  /**
   * Simple move clip to 27 and clip2 to 35
   *
   * New timeline:
   * ------------
   *
   * inpoints 0-------
   *          |  clip  |
   * time    27-------37
   *
   *                   0--------   0-----------
   *                   |  clip1  |  |   clip2    |
   *                  20--------30 35---------95
   */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_NORMAL,
          GES_EDGE_NONE, 27) == TRUE);
  fail_unless (ges_clip_edit (clip2, NULL, -1, GES_EDIT_MODE_NORMAL,
          GES_EDGE_NONE, 35) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 27, 0, 10);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 35, 0, 60);

  /**
   * Simple trimming start clip to: 32
   *
   * New timeline:
   * ------------
   *
   *                      5-------
   * layer 0:             |  clip  |
   *                     32-------37
   *
   *               0--------      0-----------
   * layer 1       |  clip1  |     |   clip2    |
   *              20--------30    35---------95
   */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_TRIM,
          GES_EDGE_START, 32) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 32, 5, 5);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 35, 0, 60);

  /* Ripple end clip to 35 and move to layer 2
   * New timeline:
   * ------------
   *
   *            0--------          0-----------
   * layer 1:   |  clip1  |         |   clip2    |
   *            20--------30       35---------95
   *
   *                        5------
   * layer 2:               |  clip |
   *                       32------35
   */
  fail_unless (ges_clip_edit (clip, NULL, 2, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_END, 35) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 32, 5, 3);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 35, 0, 60);
  layer = ges_clip_get_layer (clip);
  assert_equals_int (ges_timeline_layer_get_priority (layer), 2);
  g_object_unref (layer);

  /* Roll end clip to 50
   * New timeline:
   * ------------
   *
   *            0--------          0-----------
   * layer 1:   |  clip1  |         |   clip2    |
   *            20--------30       50---------95
   *
   *                        5------
   * layer 2:               |  clip |
   *                       32------50
   */
  fail_unless (ges_clip_edit (clip, NULL, 2, GES_EDIT_MODE_ROLL,
          GES_EDGE_END, 50) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 32, 5, 18);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 50, 15, 45);
  layer = ges_clip_get_layer (clip);
  assert_equals_int (ges_timeline_layer_get_priority (layer), 2);
  g_object_unref (layer);

  /* Some more intensive roll testing */
  for (i = 0; i < 20; i++) {
    gint32 random = g_random_int_range (35, 94);
    guint64 tck3_inpoint = random - 35;

    fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_ROLL,
            GES_EDGE_END, random) == TRUE);
    CHECK_OBJECT_PROPS (trackelement, 32, 5, random - 32);
    CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
    CHECK_OBJECT_PROPS (trackelement2, random, tck3_inpoint, 95 - random);
  }

  /* Roll end clip back to 35
   * New timeline:
   * ------------
   *
   *            0--------          0-----------
   * layer 1:   |  clip1  |         |   clip2    |
   *            20--------30       35---------95
   *
   *                        5------
   * layer 2:               |  clip |
   *                       32------35
   */
  fail_unless (ges_clip_edit (clip, NULL, 2, GES_EDIT_MODE_ROLL,
          GES_EDGE_END, 35) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 32, 5, 3);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 35, 0, 60);
  layer = ges_clip_get_layer (clip);
  assert_equals_int (ges_timeline_layer_get_priority (layer), 2);
  g_object_unref (layer);

  /* Ripple clip end to 52
   * New timeline:
   * ------------
   *
   *            0--------          0----------
   * layer 1:   |  clip1  |         |   clip2   |
   *            20-------30       52---------112
   *
   *                        5------
   * layer 2:               |  clip |
   *                       32------52
   *
   */
  /* Can not move to the first layer as clip2 should move to a layer with priority < 0 */
  fail_unless (ges_clip_edit (clip, NULL, 0, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_END, 52) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 32, 5, 20);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 52, 0, 60)
      layer = ges_clip_get_layer (clip);
  assert_equals_int (ges_timeline_layer_get_priority (layer), 2);
  g_object_unref (layer);


  /* Little check that we have 4 layers in the timeline */
  layers = ges_timeline_get_layers (timeline);
  assert_equals_int (g_list_length (layers), 4);

  /* Some refcount checkings */
  /*  We have a reference to each layer in layers */
  for (tmp = layers; tmp; tmp = tmp->next)
    ASSERT_OBJECT_REFCOUNT (layer, "Layer", 2);
  g_list_free_full (layers, g_object_unref);

  /* We have 3 references:
   *  track  + timeline  + clip
   */
  ASSERT_OBJECT_REFCOUNT (trackelement, "First trackelement", 3);
  ASSERT_OBJECT_REFCOUNT (trackelement1, "Second trackelement", 3);
  ASSERT_OBJECT_REFCOUNT (trackelement2, "Third trackelement", 3);
  /* We have 1 ref:
   * + layer */
  ASSERT_OBJECT_REFCOUNT (clip, "First clip", 1);
  ASSERT_OBJECT_REFCOUNT (clip1, "Second clip", 1);
  ASSERT_OBJECT_REFCOUNT (clip2, "Third clip", 1);

  /* Ripple clip end to 52
   * New timeline:
   * ------------
   *
   *            0--------          0-----------
   * layer 0:   |  clip1  |         |   clip2    |
   *            20-------40       62----------112
   *
   *                        5------
   * layer 1:               |  clip |
   *                       42------60
   *
   */
  fail_unless (ges_clip_edit (clip1, NULL, 0, GES_EDIT_MODE_RIPPLE,
          GES_EDGE_END, 40) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 42, 5, 20);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 20);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /* Check that movement between layer has been done properly */
  layer1 = ges_clip_get_layer (clip);
  layer = ges_clip_get_layer (clip1);
  assert_equals_int (ges_timeline_layer_get_priority (layer1), 1);
  assert_equals_int (ges_timeline_layer_get_priority (layer), 0);
  fail_unless (ges_clip_get_layer (clip2) == layer);
  g_object_unref (layer1);
  /* We have 2 references to @layer that we do not need anymore */ ;
  g_object_unref (layer);
  g_object_unref (layer);

  /* Trim clip start to 40
   * New timeline:
   * ------------
   *
   *            0--------          0-----------
   * layer 0:   |  clip1  |         |   clip2    |
   *            20-------40       62---------112
   *
   *                      0------
   * layer 1:             |  clip |
   *                     40------62
   *
   */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_TRIM,
          GES_EDGE_START, 40) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 40, 3, 22);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 20);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /* Roll clip end to 25
   * New timeline:
   * ------------
   *
   *            0--------          0-----------
   * layer 0:   |  clip1  |         |   clip2    |
   *            20-------25       62---------112
   *
   *                      0------
   * layer 1:             |  clip |
   *                     25------62
   *
   */
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_ROLL,
          GES_EDGE_END, 25) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 5);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /* Make sure that not doing anything when not able to roll */
  fail_unless (ges_clip_edit (clip, NULL, -1, GES_EDIT_MODE_ROLL,
          GES_EDGE_START, 65) == TRUE);
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_ROLL,
          GES_EDGE_END, 65) == TRUE, 0);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 5);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /* Snaping to edge, so no move */
  g_object_set (timeline, "snapping-distance", (guint64) 3, NULL);
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_TRIM,
          GES_EDGE_END, 27) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 5);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /* Snaping to edge, so no move */
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_TRIM,
          GES_EDGE_END, 27) == TRUE);

  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 5);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /**
   * New timeline:
   * ------------
   *                    0----------- 0-------------
   * inpoints   0-------|--   clip  ||   clip2      |
   *            |  clip1 25-|------- 62 -----------122
   * time      20----------30
   */
  g_object_set (timeline, "snapping-distance", (guint64) 0, NULL);
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_TRIM,
          GES_EDGE_END, 30) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 10);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

  /**
   * New timeline
   * ------------
   *                    0----------
   *                    |   clip   |
   *                    25---------62
   * inpoints   0----------------------- 10--------
   *            |       clip1           ||  clip2  |
   * time      20---------------------- 72 --------122
   */
  /* Rolling involves only neighbours that are currently snapping */
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_ROLL,
          GES_EDGE_END, 62) == TRUE);
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_ROLL,
          GES_EDGE_END, 72) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 20, 0, 52);
  CHECK_OBJECT_PROPS (trackelement2, 72, 10, 50);

  /* Test Snapping */
  /**
   *                    0----------
   *                    |   clip   |
   *                    25---------62
   * inpoints           5--------------- 10--------
   *                    |     clip1     ||  clip2  |
   * time               25------------- 72 --------122
   */
  g_object_set (timeline, "snapping-distance", (guint64) 4, NULL);
  fail_unless (ges_clip_edit (clip1, NULL, -1, GES_EDIT_MODE_TRIM,
          GES_EDGE_START, 28) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 25, 5, 47);
  CHECK_OBJECT_PROPS (trackelement2, 72, 10, 50);

  /**
   *                    0----------
   *                    |   clip   |
   *                    25---------62
   * inpoints           5---------- 0---------
   *                    |  clip1   ||  clip2  |
   * time               25-------- 62 --------122
   */
  fail_unless (ges_clip_edit (clip2, NULL, -1, GES_EDIT_MODE_ROLL,
          GES_EDGE_START, 59) == TRUE);
  CHECK_OBJECT_PROPS (trackelement, 25, 0, 37);
  CHECK_OBJECT_PROPS (trackelement1, 25, 5, 37);
  CHECK_OBJECT_PROPS (trackelement2, 62, 0, 60);

}

GST_END_TEST;

static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("ges-timeline-edition");
  TCase *tc_chain = tcase_create ("timeline-edition");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_basic_timeline_edition);
  tcase_add_test (tc_chain, test_snapping);
  tcase_add_test (tc_chain, test_timeline_edition_mode);

  return s;
}

int
main (int argc, char **argv)
{
  int nf;

  Suite *s = ges_suite ();
  SRunner *sr = srunner_create (s);

  gst_check_init (&argc, &argv);

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  return nf;
}
