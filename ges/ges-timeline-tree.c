/* GStreamer Editing Services
 * Copyright (C) 2019 Igalia S.L
 *                 Author: 2019 Thibault Saunier <tsaunier@igalia.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ges-timeline-tree.h"
#include "ges-internal.h"

GST_DEBUG_CATEGORY_STATIC (tree_debug);
#undef GST_CAT_DEFAULT
#define GST_CAT_DEFAULT tree_debug
void
timeline_tree_init_debug (void)
{
  GST_DEBUG_CATEGORY_INIT (tree_debug, "gestree",
      GST_DEBUG_FG_YELLOW, "timeline tree");
}

static gboolean
timeline_tree_can_move_element_internal (GNode * root,
    GESTimelineElement * element, guint32 priority, GstClockTime start,
    GstClockTime duration, GList * moving_track_elements);


static GNode *
find_node (GNode * root, gpointer element)
{
  return g_node_find (root, G_IN_ORDER, G_TRAVERSE_ALL, element);
}

static void
timeline_element_parent_cb (GObject * child, GParamSpec * arg
    G_GNUC_UNUSED, GNode * root)
{
}

/* FIXME: Provide a clean way to get layer prio generically */
static gint32
_priority (GESTimelineElement * element)
{
  if (GES_IS_CLIP (element))
    return ges_clip_get_layer_priority (GES_CLIP (element));

  if (GES_IS_TRACK_ELEMENT (element))
    return element->priority / LAYER_HEIGHT;

  return element->priority;
}

void
timeline_tree_track_element (GNode * root, GESTimelineElement * element)
{
  GNode *node;
  GNode *parent;
  GESTimelineElement *toplevel;

  if (find_node (root, element)) {
    return;
  }

  g_signal_connect (element, "notify::parent",
      G_CALLBACK (timeline_element_parent_cb), root);

  toplevel = ges_timeline_element_get_toplevel_parent (element);
  gst_object_unref (toplevel);  /* We own a ref */
  if (toplevel == element) {
    GST_INFO ("Tracking toplevel element %" GES_FORMAT, GES_ARGS (element));

    node = g_node_prepend_data (root, element);
  } else {
    parent = find_node (root, element->parent);
    GST_INFO ("%" GES_FORMAT "parent is %" GES_FORMAT, GES_ARGS (element),
        GES_ARGS (element->parent));
    g_assert (parent);
    node = g_node_prepend_data (parent, element);
  }

  if (GES_IS_CONTAINER (element)) {
    GList *tmp;

    for (tmp = GES_CONTAINER_CHILDREN (element); tmp; tmp = tmp->next) {
      GNode *child_node = find_node (root, tmp->data);

      if (child_node) {
        g_node_unlink (child_node);
        g_node_prepend (node, child_node);
      } else {
        timeline_tree_track_element (root, tmp->data);
      }
    }
  }
}

void
timeline_tree_stop_tracking_element (GNode * root, GESTimelineElement * element)
{
  GNode *node = find_node (root, element);

  node = find_node (root, element);

  /* Move children to the parent */
  while (node->children) {
    GNode *tmp = node->children;
    g_node_unlink (tmp);
    g_node_prepend (node->parent, tmp);
  }

  g_assert (node);
  GST_INFO ("Stop tracking %" GES_FORMAT, GES_ARGS (element));
  g_signal_handlers_disconnect_by_func (element, timeline_element_parent_cb,
      root);

  g_node_destroy (node);
}

typedef struct
{
  GstClockTime start;
  GstClockTime end;
} Overlap;

typedef struct
{
  GNode *root;
  gboolean res;
  GstClockTimeDiff start_diff;
  GstClockTimeDiff duration_diff;
  gint64 priority_diff;
  GESTimelineElement *element;
  GList *movings;

  gboolean start_is_overlaped;
  gboolean end_is_overlaped;
} CheckOverlapData;

static gboolean
check_track_elements_overlap (GNode * node, CheckOverlapData * data)
{
  GESTimelineElement *e = (GESTimelineElement *) node->data;
  GstClockTime moving_start, moving_end, end, start;
  guint32 moving_priority;

  if (!GES_IS_SOURCE (e) || e == data->element
      || g_list_find (data->movings, e))
    return FALSE;

  /* Not in the same layer */
  if (ges_track_element_get_track ((GESTrackElement *) node->data) !=
      ges_track_element_get_track ((GESTrackElement *) data->element)) {
    GST_DEBUG ("%" GES_FORMAT " and %" GES_FORMAT
        " are not in the same track", GES_ARGS (node->data),
        GES_ARGS (data->element));
    return FALSE;
  }

  /* Not in the same layer */
  moving_priority = _priority (data->element) - (data->priority_diff);
  if (_priority (e) != moving_priority) {
    GST_LOG ("%" GST_PTR_FORMAT " and %" GST_PTR_FORMAT
        " are not on the same layer (%d != %d)", node->data, data->element,
        _priority (e), moving_priority);

    return FALSE;
  }

  start = e->start;
  end = e->start + e->duration;
  moving_start = data->element->start - data->start_diff;
  moving_end = (moving_start + data->element->duration - data->duration_diff);
  if (start > moving_end || moving_start > end) {
    /* They do not overlap at all */
    GST_LOG ("%" GST_PTR_FORMAT " and %" GST_PTR_FORMAT
        " do not overlap at all.", node->data, data->element);
    return FALSE;
  }

  if ((moving_start <= start && moving_end >= end) ||
      (moving_start >= start && moving_end <= end)) {
    GST_INFO ("Fully overlaped: %s<%p> [%ld-%ld] and %s<%p> [%ld-%ld (%ld)]",
        e->name, e, start, end,
        data->element->name, data->element, moving_start, moving_end,
        data->duration_diff);

    data->res = FALSE;
    return TRUE;
  }

  if (moving_start < end && moving_start > start) {
    GST_LOG ("Overlap start: %s<%p> [%ld-%ld] and %s<%p> [%ld-%ld (%ld)]",
        e->name, e, start, end,
        data->element->name, data->element, moving_start, moving_end,
        data->duration_diff);
    if (data->start_is_overlaped) {
      GST_LOG ("Clip is overlapped by 2 clips at its start");
      data->res = FALSE;

      return TRUE;
    }

    data->start_is_overlaped = TRUE;
  } else if (moving_end > end) {
    GST_LOG ("Overlap end: %s<%p> [%ld-%ld] and %s<%p> [%ld-%ld (%ld)]",
        e->name, e, start, end,
        data->element->name, data->element, moving_start, moving_end,
        data->duration_diff);

    if (data->end_is_overlaped) {
      GST_LOG ("Clip is overlapped by 2 clips at its end.");
      data->res = FALSE;

      return TRUE;
    }
    data->end_is_overlaped = TRUE;
  }

  return FALSE;
}

static gboolean
check_can_move_children (GNode * node, CheckOverlapData * data)
{
  GESTimelineElement *element = node->data;
  GstClockTime start = element->start - data->start_diff;
  GstClockTime duration = data->element->duration - data->duration_diff;
  guint32 priority = _priority (element) - data->priority_diff;

  if (element == data->element)
    return FALSE;

  data->res =
      timeline_tree_can_move_element_internal (data->root, node->data, priority,
      start, duration, data->movings);

  return !data->res;
}

static gboolean
timeline_tree_can_move_element_internal (GNode * root,
    GESTimelineElement * element, guint32 priority, GstClockTime start,
    GstClockTime duration, GList * moving_track_elements)
{
  GNode *node = find_node (root, element);
  CheckOverlapData data = {
    .root = root,
    .res = TRUE,
    .start_diff = element->start - start,
    .duration_diff = element->duration - duration,
    .priority_diff = _priority (element) - priority,
    .element = element,
    .movings = moving_track_elements,
    .start_is_overlaped = FALSE,
    .end_is_overlaped = FALSE
  };

  g_assert (node);
  if (G_NODE_IS_LEAF (node->data)) {
    if (GES_IS_SOURCE (node->data)) {
      data.priority_diff = _priority (element) - priority;
      g_node_traverse (root, G_IN_ORDER, G_TRAVERSE_LEAVES, -1,
          (GNodeTraverseFunc) check_track_elements_overlap, &data);

      return data.res;
    }
    return TRUE;
  }

  g_node_traverse (node, G_IN_ORDER, G_TRAVERSE_LEAFS, -1,
      (GNodeTraverseFunc) check_can_move_children, &data);

  return data.res;
}

gboolean
timeline_tree_can_move_element (GNode * root,
    GESTimelineElement * element, guint32 priority, GstClockTime start,
    GstClockTime duration, GList * moving_track_elements)
{
  GESTimelineElement *toplevel =
      ges_timeline_element_get_toplevel_parent (element);

  gboolean res =
      timeline_tree_can_move_element_internal (root, toplevel, priority, start,
      duration, moving_track_elements);

  g_object_unref (toplevel);
  return res;
}
