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

#include "test-utils.h"
#include "../../../ges/ges-time-code.h"
#include <ges/ges.h>
#include <gst/check/gstcheck.h>

GST_START_TEST (test_frame_from_ts)
{
  fail_unless (ges_init ());

  assert_equals_uint64 (ges_timestamp_get_frame (2500333333, 30, 1, 0, FALSE),
      75);
  assert_equals_uint64 (ges_timestamp_get_frame (30, 30, 1, 0, FALSE), 0);
  assert_equals_uint64 (ges_timestamp_get_frame (GST_SECOND, 30, 1, 0, FALSE),
      30);

  /* We do not handle timestamp >= 24hrs */
  assert_equals_uint64 (ges_timestamp_get_frame (GST_SECOND * 60 * 60 * 24, 30,
          1, 0, FALSE), GES_FRAME_NONE);

  assert_equals_uint64 (ges_frames_to_ns (30, 30, 1, 0), GST_SECOND);

  ges_deinit ();
}

GST_END_TEST;


static Suite *
ges_suite (void)
{
  Suite *s = suite_create ("ges");
  TCase *tc_chain = tcase_create ("a");

  suite_add_tcase (s, tc_chain);

  tcase_add_test (tc_chain, test_frame_from_ts);

  return s;
}

GST_CHECK_MAIN (ges);
