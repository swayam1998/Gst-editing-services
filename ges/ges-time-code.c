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
#include "ges-time-code.h"

#include <gst/gst.h>

gboolean
ges_time_code_init (GESTimeCode * gestc, GESFrameNumber frame, gint framerate_n,
    gint framerate_d, GstVideoTimeCodeFlags flags)
{
  GstVideoTimeCode *tc = TIMECODE (gestc);

  g_assert (frame >= 0);

  gestc->n_frames = frame;
  if (!GES_FRAME_IS_VALID (frame))
    return FALSE;

  gst_video_time_code_init (tc, framerate_n, framerate_d, NULL,
      GST_VIDEO_TIME_CODE_FLAGS_NONE, 0, 0, 0, 0, 0);

  if (!gst_video_time_code_is_valid (tc))
    return FALSE;

  gst_video_time_code_add_frames (tc, frame);
  return gst_video_time_code_is_valid (tc);
}

void
ges_time_code_set_flags (GESTimeCode * gestc, GstVideoTimeCodeFlags flags)
{
  GESTimeCode gestc1;
  GstVideoTimeCode *tc = TIMECODE (gestc), *tc1 = TIMECODE (&gestc1);

  g_assert (gst_video_time_code_is_valid (tc));

  gst_video_time_code_init (tc1, tc->config.fps_n, tc->config.fps_d,
      NULL, flags, 0, 0, 0, 0, 0);
  gst_video_time_code_add_frames (tc1, gestc->n_frames);

  *gestc = gestc1;
}

GstClockTime
ges_time_code_get_ts (GESTimeCode * tc)
{
  if (!GES_FRAME_IS_VALID (tc->n_frames))
    return GST_CLOCK_TIME_NONE;

  return gst_video_time_code_nsec_since_daily_jam (TIMECODE (tc));
}

GstClockTime
ges_frames_diff_to_ns (GESFrameNumber frame, gint framerate_n, gint framerate_d,
    GstVideoTimeCodeFlags flags)
{
  if (frame < 0)
    return -ges_frames_to_ns (ABS (frame), framerate_n, framerate_d, flags);

  return ges_frames_to_ns (frame, framerate_n, framerate_d, flags);
}

GstClockTime
ges_frames_to_ns (GESFrameNumber frame, gint framerate_n, gint framerate_d,
    GstVideoTimeCodeFlags flags)
{
  GESTimeCode tc;

  if (!ges_time_code_init (&tc, frame, framerate_n, framerate_d, flags)) {
    GST_ERROR ("Could not convert %" G_GINT64_FORMAT "f@%d/%dfps", frame,
        framerate_n, framerate_d);

    return GST_CLOCK_TIME_NONE;
  }

  return gst_video_time_code_nsec_since_daily_jam (TIMECODE (&tc));
}

GESFrameNumber
ges_timestamp_get_frame (GstClockTime ts, gint framerate_n, gint framerate_d,
    GstVideoTimeCodeFlags flags, gboolean strict)
{
  GESFrameNumber frame;

  if (!GST_CLOCK_TIME_IS_VALID (ts))
    return GES_FRAME_NONE;

  if (ts >= GST_SECOND * 24 * 60 * 60) {
    GST_ERROR ("Can\t handle timestamp > 24hrs");

    return GES_FRAME_NONE;
  }

  frame = gst_util_uint64_scale (ts, framerate_n, framerate_d * GST_SECOND);

  /* Take rounding errors into account and always snap on previous frame. */
  while (ges_frames_to_ns (frame + 1, framerate_n, framerate_d, flags) <= ts)
    frame++;

  while (frame > 0
      && ges_frames_to_ns (frame - 1, framerate_n, framerate_d, flags) >= ts)
    frame--;

  if (strict && ges_frames_to_ns (frame, framerate_n, framerate_d, flags) != ts) {
    GST_DEBUG ("Strictly trying to get timestamp but %" G_GUINT64_FORMAT
        " is not a frame", ts);

    return GES_FRAME_NONE;
  }

  GST_DEBUG ("Snapping of the %" G_GINT64_FORMAT " nth frame (ts: %"
      GST_TIME_FORMAT ")", frame,
      GST_TIME_ARGS (ges_frames_to_ns (frame, framerate_n, framerate_d,
              flags)));

  GST_FIXME ("Detect daily jam!");

  return frame;
}
