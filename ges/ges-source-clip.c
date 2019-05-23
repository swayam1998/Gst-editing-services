/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <edward.hervey@collabora.co.uk>
 *               2009 Nokia Corporation
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

/**
 * SECTION:gessourceclip
 * @title: GESSourceClip
 * @short_description: Base Class for sources of a GESLayer
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ges-internal.h"
#include "ges-clip.h"
#include "ges-source-clip.h"
#include "ges-source.h"


struct _GESSourceClipPrivate
{
  /*  dummy variable */
  void *nothing;
};

enum
{
  PROP_0,
};

G_DEFINE_TYPE_WITH_PRIVATE (GESSourceClip, ges_source_clip, GES_TYPE_CLIP);

static gboolean
_set_start_full (GESTimelineElement * element, GstClockTime start,
    GESFrameNumber fstart)
{
  GESTimelineElement *toplevel =
      ges_timeline_element_get_toplevel_parent (element);

  gst_object_unref (toplevel);
  if (element->timeline
      && !ELEMENT_FLAG_IS_SET (element, GES_TIMELINE_ELEMENT_SET_SIMPLE)
      && !ELEMENT_FLAG_IS_SET (toplevel, GES_TIMELINE_ELEMENT_SET_SIMPLE)) {
    GESFrameNumber fdiff = GES_FRAME_IS_VALID (fstart) ?
        _FSTART (element) - fstart : GES_FRAME_NONE;

    if (!timeline_tree_move (timeline_get_tree (element->timeline), element,
            0, GST_CLOCK_DIFF (start, element->start), GES_EDGE_START,
            ges_timeline_get_snapping_distance (element->timeline), fdiff))
      return FALSE;
    return -1;
  }

  return
      GES_TIMELINE_ELEMENT_CLASS (ges_source_clip_parent_class)->set_start_full
      (element, start, fstart);
}

static gboolean
_set_duration_full (GESTimelineElement * element, GstClockTime duration,
    GESFrameNumber fduration)
{
  GESTimelineElement *toplevel =
      ges_timeline_element_get_toplevel_parent (element);

  gst_object_unref (toplevel);
  if (element->timeline
      && !ELEMENT_FLAG_IS_SET (element, GES_TIMELINE_ELEMENT_SET_SIMPLE)
      && !ELEMENT_FLAG_IS_SET (toplevel, GES_TIMELINE_ELEMENT_SET_SIMPLE)) {
    GESFrameNumber fdiff = GES_FRAME_NONE;

    if (GES_FRAME_IS_VALID (fduration)) {
      ELEMENT_SET_FLAG (element, GES_TIMELINE_ELEMENT_SET_SIMPLE);
      if (!ges_timeline_element_reset_framerate_on_edge (element, GES_EDGE_NONE,
              TRUE)) {
        ELEMENT_UNSET_FLAG (element, GES_TIMELINE_ELEMENT_SET_SIMPLE);
        return FALSE;
      }
      ELEMENT_UNSET_FLAG (element, GES_TIMELINE_ELEMENT_SET_SIMPLE);

      fdiff = _FDURATION (element) - fduration;
    }

    if (!timeline_tree_trim (timeline_get_tree (element->timeline), element, 0,
            GST_CLOCK_DIFF (duration, element->duration), GES_EDGE_END,
            ges_timeline_get_snapping_distance (element->timeline), fdiff))
      return FALSE;
    else
      return -1;
  }

  return
      GES_TIMELINE_ELEMENT_CLASS
      (ges_source_clip_parent_class)->set_duration_full (element, duration,
      fduration);
}

static void
ges_source_clip_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_source_clip_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  switch (property_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_source_clip_finalize (GObject * object)
{
  G_OBJECT_CLASS (ges_source_clip_parent_class)->finalize (object);
}

static void
ges_source_clip_class_init (GESSourceClipClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GESTimelineElementClass *element_class = GES_TIMELINE_ELEMENT_CLASS (klass);

  object_class->get_property = ges_source_clip_get_property;
  object_class->set_property = ges_source_clip_set_property;
  object_class->finalize = ges_source_clip_finalize;

  element_class->set_start_full = _set_start_full;
  element_class->set_duration_full = _set_duration_full;
}

static void
ges_source_clip_init (GESSourceClip * self)
{
  self->priv = ges_source_clip_get_instance_private (self);
}
