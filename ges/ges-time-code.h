/* gst-editing-services
 * Copyright (C) 2019 Igalia S.L
 *     Author: 2019 Thibault Saunier <tsaunier@igalia.com>
 *
 * gst-editing-services is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * gst-editing-services is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <gst/video/video.h>
#include "ges-types.h"

typedef struct
{
  GstVideoTimeCode parent;
  /* Total number of frames, GES_FRAME_NONE means none provided
   * which is the default, and this is reset when the parent
   * timecode structure is initialized */
  GESFrameNumber n_frames;
  gboolean is_used;
  const gchar *propname;

  /* The offset of the time field in the GESTimelineElement structure */
  goffset offset;

} GESTimeCode;

#define TC_IS_VALIDE(gestc) (GES_FRAME_IS_VALID(gestc->n_frames))
#define TIMECODE(tc) ((GstVideoTimeCode*) tc)

gboolean ges_time_code_init             (GESTimeCode * gestc, GESFrameNumber frames,
                                         gint framerate_n, gint framerate_d,
                                         GstVideoTimeCodeFlags flags);
GESFrameNumber ges_timestamp_get_frame  (GstClockTime ts, gint framerate_n, gint framerate_d, GstVideoTimeCodeFlags flags, gboolean strict);
GstClockTime ges_frames_to_ns           (GESFrameNumber frames, gint framerate_n, gint framerate_d, GstVideoTimeCodeFlags flags);
GstClockTime ges_frames_diff_to_ns      (GESFrameNumber frames, gint framerate_n, gint framerate_d, GstVideoTimeCodeFlags flags);
GstClockTime ges_time_code_get_ts       (GESTimeCode *tc);
void ges_time_code_set_flags            (GESTimeCode * gestc, GstVideoTimeCodeFlags flags);