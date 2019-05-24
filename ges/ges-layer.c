/* GStreamer Editing Services
 * Copyright (C) 2009 Edward Hervey <edward.hervey@collabora.co.uk>
 *               2009 Nokia Corporation
 *               2011 Mathieu Duponchelle <mathieu.duponchelle@epitech.eu>
 *               2013 Thibault Saunier <thibault.saunier@collabora.com>
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
 * SECTION:geslayer
 * @title: GESLayer
 * @short_description: Non-overlapping sequence of GESClip
 *
 * Responsible for the ordering of the various contained Clip(s). A
 * timeline layer has a "priority" property, which is used to manage the
 * priorities of individual Clips. Two layers should not have the
 * same priority within a given timeline.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ges-internal.h"
#include "ges-layer.h"
#include "ges.h"
#include "ges-time-code.h"
#include "ges-source-clip.h"

static void ges_meta_container_interface_init
    (GESMetaContainerInterface * iface);

struct _GESLayerPrivate
{
  /*< private > */
  GList *clips_start;           /* The Clips sorted by start and
                                 * priority */

  guint32 priority;             /* The priority of the layer within the
                                 * containing timeline */
  gboolean auto_transition;
};

typedef struct
{
  GESClip *clip;
  GESLayer *layer;
} NewAssetUData;

enum
{
  PROP_0,
  PROP_PRIORITY,
  PROP_AUTO_TRANSITION,
  PROP_LAST
};

enum
{
  OBJECT_ADDED,
  OBJECT_REMOVED,
  LAST_SIGNAL
};

static guint ges_layer_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (GESLayer, ges_layer,
    G_TYPE_INITIALLY_UNOWNED, G_IMPLEMENT_INTERFACE (GES_TYPE_EXTRACTABLE, NULL)
    G_ADD_PRIVATE (GESLayer)
    G_IMPLEMENT_INTERFACE (GES_TYPE_META_CONTAINER,
        ges_meta_container_interface_init));

/* GObject standard vmethods */
static void
ges_layer_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESLayer *layer = GES_LAYER (object);

  switch (property_id) {
    case PROP_PRIORITY:
      g_value_set_uint (value, layer->priv->priority);
      break;
    case PROP_AUTO_TRANSITION:
      g_value_set_boolean (value, layer->priv->auto_transition);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_layer_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESLayer *layer = GES_LAYER (object);

  switch (property_id) {
    case PROP_PRIORITY:
      GST_FIXME ("Deprecated, use ges_timeline_move_layer instead");
      layer_set_priority (layer, g_value_get_uint (value), FALSE);
      break;
    case PROP_AUTO_TRANSITION:
      ges_layer_set_auto_transition (layer, g_value_get_boolean (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_layer_dispose (GObject * object)
{
  GESLayer *layer = GES_LAYER (object);
  GESLayerPrivate *priv = layer->priv;

  GST_DEBUG ("Disposing layer");

  while (priv->clips_start)
    ges_layer_remove_clip (layer, (GESClip *) priv->clips_start->data);

  G_OBJECT_CLASS (ges_layer_parent_class)->dispose (object);
}

static gboolean
_register_metas (GESLayer * layer)
{
  ges_meta_container_register_meta_float (GES_META_CONTAINER (layer),
      GES_META_READ_WRITE, GES_META_VOLUME, 1.0);

  return TRUE;
}

static void
ges_meta_container_interface_init (GESMetaContainerInterface * iface)
{

}

static void
ges_layer_class_init (GESLayerClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = ges_layer_get_property;
  object_class->set_property = ges_layer_set_property;
  object_class->dispose = ges_layer_dispose;

  /**
   * GESLayer:priority:
   *
   * The priority of the layer in the #GESTimeline. 0 is the highest
   * priority. Conceptually, a #GESTimeline is a stack of GESLayers,
   * and the priority of the layer represents its position in the stack. Two
   * layers should not have the same priority within a given GESTimeline.
   *
   * Note that the timeline needs to be commited (with #ges_timeline_commit)
   * for the change to be taken into account.
   *
   * Deprecated:1.16.0: use #ges_timeline_move_layer instead. This deprecation means
   * that you will not need to handle layer priorities at all yourself, GES
   * will make sure there is never 'gaps' between layer priorities.
   */
  g_object_class_install_property (object_class, PROP_PRIORITY,
      g_param_spec_uint ("priority", "Priority",
          "The priority of the layer", 0, G_MAXUINT, 0, G_PARAM_READWRITE));

  /**
   * GESLayer:auto-transition:
   *
   * Sets whether transitions are added automagically when clips overlap.
   */
  g_object_class_install_property (object_class, PROP_AUTO_TRANSITION,
      g_param_spec_boolean ("auto-transition", "Auto-Transition",
          "whether the transitions are added", FALSE, G_PARAM_READWRITE));

  /**
   * GESLayer::clip-added:
   * @layer: the #GESLayer
   * @clip: the #GESClip that was added.
   *
   * Will be emitted after the clip was added to the layer.
   */
  ges_layer_signals[OBJECT_ADDED] =
      g_signal_new ("clip-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GESLayerClass, object_added),
      NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE, 1, GES_TYPE_CLIP);

  /**
   * GESLayer::clip-removed:
   * @layer: the #GESLayer
   * @clip: the #GESClip that was removed
   *
   * Will be emitted after the clip was removed from the layer.
   */
  ges_layer_signals[OBJECT_REMOVED] =
      g_signal_new ("clip-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET (GESLayerClass,
          object_removed), NULL, NULL, g_cclosure_marshal_generic, G_TYPE_NONE,
      1, GES_TYPE_CLIP);
}

static void
ges_layer_init (GESLayer * self)
{
  self->priv = ges_layer_get_instance_private (self);

  self->priv->priority = 0;
  self->priv->auto_transition = FALSE;
  self->min_nle_priority = MIN_NLE_PRIO;
  self->max_nle_priority = LAYER_HEIGHT + MIN_NLE_PRIO;

  _register_metas (self);
}

static gint
ges_layer_resync_priorities_by_type (GESLayer * layer,
    gint starting_priority, GType type)
{
  GstClockTime next_reset = 0;
  gint priority = starting_priority, max_priority = priority;
  GList *tmp;
  GESTimelineElement *element;

  layer->priv->clips_start =
      g_list_sort (layer->priv->clips_start,
      (GCompareFunc) element_start_compare);
  for (tmp = layer->priv->clips_start; tmp; tmp = tmp->next) {

    element = GES_TIMELINE_ELEMENT (tmp->data);

    if (GES_IS_TRANSITION_CLIP (element)) {
      /* Blindly set transitions priorities to 0 */
      _set_priority0 (element, 0);
      continue;
    } else if (!g_type_is_a (G_OBJECT_TYPE (element), type))
      continue;

    if (element->start > next_reset) {
      priority = starting_priority;
      next_reset = 0;
    }

    if (element->start + element->duration > next_reset)
      next_reset = element->start + element->duration;

    _set_priority0 (element, priority);
    priority = priority + GES_CONTAINER_HEIGHT (element);

    if (priority > max_priority)
      max_priority = priority;
  }

  return max_priority;
}

/**
 * ges_layer_resync_priorities:
 * @layer: a #GESLayer
 *
 * Resyncs the priorities of the clips controlled by @layer.
 */
gboolean
ges_layer_resync_priorities (GESLayer * layer)
{
  gint min_source_prios;

  g_return_val_if_fail (GES_IS_LAYER (layer), FALSE);

  GST_INFO_OBJECT (layer, "Resync priorities (prio: %d)",
      layer->priv->priority);

  min_source_prios = ges_layer_resync_priorities_by_type (layer, 1,
      GES_TYPE_OPERATION_CLIP);

  ges_layer_resync_priorities_by_type (layer, min_source_prios,
      GES_TYPE_SOURCE_CLIP);

  return TRUE;
}

void
layer_set_priority (GESLayer * layer, guint priority, gboolean emit)
{
  GST_DEBUG ("layer:%p, priority:%d", layer, priority);

  if (priority != layer->priv->priority) {
    layer->priv->priority = priority;
    layer->min_nle_priority = (priority * LAYER_HEIGHT) + MIN_NLE_PRIO;
    layer->max_nle_priority = ((priority + 1) * LAYER_HEIGHT) + MIN_NLE_PRIO;

    ges_layer_resync_priorities (layer);
  }

  if (emit)
    g_object_notify (G_OBJECT (layer), "priority");
}

static void
new_asset_cb (GESAsset * source, GAsyncResult * res, NewAssetUData * udata)
{
  GError *error = NULL;

  GESAsset *asset = ges_asset_request_finish (res, &error);

  GST_DEBUG_OBJECT (udata->layer, "%" GST_PTR_FORMAT " Asset loaded, "
      "setting its asset", udata->clip);

  if (error) {
    GESProject *project = udata->layer->timeline ?
        GES_PROJECT (ges_extractable_get_asset (GES_EXTRACTABLE
            (udata->layer->timeline))) : NULL;
    if (project) {
      gchar *possible_id;

      possible_id = ges_project_try_updating_id (project, source, error);
      if (possible_id) {
        ges_asset_request_async (ges_asset_get_extractable_type (source),
            possible_id, NULL, (GAsyncReadyCallback) new_asset_cb, udata);
        g_free (possible_id);
        return;
      }
    }

    GST_ERROR ("Asset could not be created for uri %s, error: %s",
        ges_asset_get_id (asset), error->message);
  } else {
    GESProject *project = udata->layer->timeline ?
        GES_PROJECT (ges_extractable_get_asset (GES_EXTRACTABLE
            (udata->layer->timeline))) : NULL;
    ges_extractable_set_asset (GES_EXTRACTABLE (udata->clip), asset);

    ges_project_add_asset (project, asset);

    /* clip was already ref-sinked when creating udata,
     * gst_layer_add_clip() creates a new ref as such and
     * below we unref the ref from udata */
    ges_layer_add_clip (udata->layer, udata->clip);
  }

  gst_object_unref (asset);
  gst_object_unref (udata->clip);
  g_slice_free (NewAssetUData, udata);
}

/**
 * ges_layer_get_duration:
 * @layer: The #GESLayer to get the duration from
 *
 * Lets you retrieve the duration of the layer, which means
 * the end time of the last clip inside it
 *
 * Returns: The duration of a layer
 */
GstClockTime
ges_layer_get_duration (GESLayer * layer)
{
  GList *tmp;
  GstClockTime duration = 0;

  g_return_val_if_fail (GES_IS_LAYER (layer), 0);

  for (tmp = layer->priv->clips_start; tmp; tmp = tmp->next) {
    duration = MAX (duration, _END (tmp->data));
  }

  return duration;
}

/**
 * ges_layer_get_fduration:
 * @layer: The #GESLayer to get the duration from
 *
 * Lets you retrieve the duration of the layer in frames, which means
 * the number of the last frame to be displayed
 *
 * Returns: The duration of a layer in frames.
 *
 * Since: 1.18
 */
GstClockTime
ges_layer_get_fduration (GESLayer * layer)
{
  GList *tmp;
  guint64 duration = 0;
  gint framerate_n, framerate_d;
  GstVideoTimeCodeFlags flags;

  g_return_val_if_fail (GES_IS_LAYER (layer), 0);

  ges_timeline_get_timecodes_config (layer->timeline, &framerate_n,
      &framerate_d, &flags);
  for (tmp = layer->priv->clips_start; tmp; tmp = tmp->next) {
    guint fstart = ges_timeline_element_get_fstart (tmp->data),
        fduration = ges_timeline_element_get_fduration (tmp->data);

    if (!GES_FRAME_IS_VALID (fstart) || !GES_FRAME_IS_VALID (fstart)) {
      duration =
          MAX (duration, ges_timestamp_get_frame (_END (tmp->data), framerate_n,
              framerate_d, flags, FALSE));
    } else {
      duration = MAX (duration, fstart + fduration);
    }
  }

  return duration;
}

static gboolean
ges_layer_remove_clip_internal (GESLayer * layer, GESClip * clip,
    gboolean emit_removed)
{
  GESLayer *current_layer;

  GST_DEBUG ("layer:%p, clip:%p", layer, clip);

  current_layer = ges_clip_get_layer (clip);
  if (G_UNLIKELY (current_layer != layer)) {
    GST_WARNING ("Clip doesn't belong to this layer");

    if (current_layer != NULL)
      gst_object_unref (current_layer);

    return FALSE;
  }
  gst_object_unref (current_layer);

  /* Remove it from our list of controlled objects */
  layer->priv->clips_start = g_list_remove (layer->priv->clips_start, clip);

  if (emit_removed) {
    /* emit 'clip-removed' */
    g_signal_emit (layer, ges_layer_signals[OBJECT_REMOVED], 0, clip);
  }

  /* inform the clip it's no longer in a layer */
  ges_clip_set_layer (clip, NULL);
  /* so neither in a timeline */
  if (layer->timeline)
    ges_timeline_element_set_timeline (GES_TIMELINE_ELEMENT (clip), NULL);

  /* Remove our reference to the clip */
  gst_object_unref (clip);

  return TRUE;
}

/* Public methods */
/**
 * ges_layer_remove_clip:
 * @layer: a #GESLayer
 * @clip: the #GESClip to remove
 *
 * Removes the given @clip from the @layer and unparents it.
 * Unparenting it means the reference owned by @layer on the @clip will be
 * removed. If you wish to use the @clip after this function, make sure you
 * call gst_object_ref() before removing it from the @layer.
 *
 * Returns: %TRUE if the clip could be removed, %FALSE if the layer does
 * not want to remove the clip.
 */
gboolean
ges_layer_remove_clip (GESLayer * layer, GESClip * clip)
{
  g_return_val_if_fail (GES_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (GES_IS_CLIP (clip), FALSE);

  return ges_layer_remove_clip_internal (layer, clip, TRUE);
}

/**
 * ges_layer_set_priority:
 * @layer: a #GESLayer
 * @priority: the priority to set
 *
 * Sets the layer to the given @priority. See the documentation of the
 * priority property for more information.
 *
 * Deprecated:1.16.0: use #ges_timeline_move_layer instead. This deprecation means
 * that you will not need to handle layer priorities at all yourself, GES
 * will make sure there is never 'gaps' between layer priorities.
 */
void
ges_layer_set_priority (GESLayer * layer, guint priority)
{
  g_return_if_fail (GES_IS_LAYER (layer));

  GST_FIXME ("Deprecated, use ges_timeline_move_layer instead");

  layer_set_priority (layer, priority, TRUE);
}

/**
 * ges_layer_get_auto_transition:
 * @layer: a #GESLayer
 *
 * Gets whether transitions are automatically added when objects
 * overlap or not.
 *
 * Returns: %TRUE if transitions are automatically added, else %FALSE.
 */
gboolean
ges_layer_get_auto_transition (GESLayer * layer)
{
  g_return_val_if_fail (GES_IS_LAYER (layer), 0);

  return layer->priv->auto_transition;
}

/**
 * ges_layer_set_auto_transition:
 * @layer: a #GESLayer
 * @auto_transition: whether the auto_transition is active
 *
 * Sets the layer to the given @auto_transition. See the documentation of the
 * property auto_transition for more information.
 */
void
ges_layer_set_auto_transition (GESLayer * layer, gboolean auto_transition)
{

  g_return_if_fail (GES_IS_LAYER (layer));

  layer->priv->auto_transition = auto_transition;
  g_object_notify (G_OBJECT (layer), "auto-transition");
}

/**
 * ges_layer_get_priority:
 * @layer: a #GESLayer
 *
 * Get the priority of @layer within the timeline.
 *
 * Returns: The priority of the @layer within the timeline.
 */
guint
ges_layer_get_priority (GESLayer * layer)
{
  g_return_val_if_fail (GES_IS_LAYER (layer), 0);

  return layer->priv->priority;
}

/**
 * ges_layer_get_clips:
 * @layer: a #GESLayer
 *
 * Get the clips this layer contains.
 *
 * Returns: (transfer full) (element-type GESClip): a #GList of
 * clips. The user is responsible for
 * unreffing the contained objects and freeing the list.
 */

GList *
ges_layer_get_clips (GESLayer * layer)
{
  GESLayerClass *klass;

  g_return_val_if_fail (GES_IS_LAYER (layer), NULL);

  klass = GES_LAYER_GET_CLASS (layer);

  if (klass->get_objects) {
    return klass->get_objects (layer);
  }

  return g_list_sort (g_list_copy_deep (layer->priv->clips_start,
          (GCopyFunc) gst_object_ref, NULL),
      (GCompareFunc) element_start_compare);
}

/**
 * ges_layer_is_empty:
 * @layer: The #GESLayer to check
 *
 * Convenience method to check if @layer is empty (doesn't contain any clip),
 * or not.
 *
 * Returns: %TRUE if @layer is empty, %FALSE if it already contains at least
 * one #GESClip
 */
gboolean
ges_layer_is_empty (GESLayer * layer)
{
  g_return_val_if_fail (GES_IS_LAYER (layer), FALSE);

  return (layer->priv->clips_start == NULL);
}

/**
 * ges_layer_add_clip:
 * @layer: a #GESLayer
 * @clip: (transfer floating): the #GESClip to add.
 *
 * Adds the given clip to the layer. Sets the clip's parent, and thus
 * takes ownership of the clip.
 *
 * An clip can only be added to one layer.
 *
 * Calling this method will construct and properly set all the media related
 * elements on @clip. If you need to know when those objects (actually #GESTrackElement)
 * are constructed, you should connect to the container::child-added signal which
 * is emited right after those elements are ready to be used.
 *
 * Returns: %TRUE if the clip was properly added to the layer, or %FALSE
 * if the @layer refuses to add the clip.
 */
gboolean
ges_layer_add_clip (GESLayer * layer, GESClip * clip)
{
  GESAsset *asset;
  GESLayerPrivate *priv;
  GESLayer *current_layer;

  g_return_val_if_fail (GES_IS_LAYER (layer), FALSE);
  g_return_val_if_fail (GES_IS_CLIP (clip), FALSE);

  GST_DEBUG_OBJECT (layer, "adding clip:%p", clip);

  priv = layer->priv;
  current_layer = ges_clip_get_layer (clip);
  if (G_UNLIKELY (current_layer)) {
    GST_WARNING ("Clip %p already belongs to another layer", clip);
    gst_object_ref_sink (clip);
    gst_object_unref (current_layer);

    return FALSE;
  }

  asset = ges_extractable_get_asset (GES_EXTRACTABLE (clip));
  if (asset == NULL) {
    gchar *id;
    NewAssetUData *mudata = g_slice_new (NewAssetUData);

    mudata->clip = gst_object_ref_sink (clip);
    mudata->layer = layer;

    GST_DEBUG_OBJECT (layer, "%" GST_PTR_FORMAT " as no reference to any "
        "assets creating a asset... trying sync", clip);

    id = ges_extractable_get_id (GES_EXTRACTABLE (clip));
    asset = ges_asset_request (G_OBJECT_TYPE (clip), id, NULL);
    if (asset == NULL) {
      GESProject *project = layer->timeline ?
          GES_PROJECT (ges_extractable_get_asset (GES_EXTRACTABLE
              (layer->timeline))) : NULL;

      ges_asset_request_async (G_OBJECT_TYPE (clip),
          id, NULL, (GAsyncReadyCallback) new_asset_cb, mudata);

      if (project)
        ges_project_add_loading_asset (project, G_OBJECT_TYPE (clip), id);
      g_free (id);

      GST_LOG_OBJECT (layer, "Object added async");
      return TRUE;
    }
    g_free (id);

    ges_extractable_set_asset (GES_EXTRACTABLE (clip), asset);

    g_slice_free (NewAssetUData, mudata);
    gst_clear_object (&asset);
  } else {
    gst_object_ref_sink (clip);
  }

  /* Take a reference to the clip and store it stored by start/priority */
  priv->clips_start = g_list_insert_sorted (priv->clips_start, clip,
      (GCompareFunc) element_start_compare);

  /* Inform the clip it's now in this layer */
  ges_clip_set_layer (clip, layer);

  GST_DEBUG ("current clip priority : %d, Height: %d", _PRIORITY (clip),
      LAYER_HEIGHT);

  /* Set the priority. */
  if (_PRIORITY (clip) > LAYER_HEIGHT) {
    GST_WARNING_OBJECT (layer,
        "%p is out of the layer space, setting its priority to "
        "%d, setting it to the maximum priority of the layer: %d", clip,
        _PRIORITY (clip), LAYER_HEIGHT - 1);
    _set_priority0 (GES_TIMELINE_ELEMENT (clip), LAYER_HEIGHT - 1);
  }

  ges_layer_resync_priorities (layer);

  ges_timeline_element_set_timeline (GES_TIMELINE_ELEMENT (clip),
      layer->timeline);

  /* emit 'clip-added' */
  g_signal_emit (layer, ges_layer_signals[OBJECT_ADDED], 0, clip);

  if (!ELEMENT_FLAG_IS_SET (clip, GES_CLIP_IS_MOVING) && layer->timeline
      && !timeline_tree_can_move_element (timeline_get_tree (layer->timeline),
          GES_TIMELINE_ELEMENT (clip),
          GES_TIMELINE_ELEMENT_LAYER_PRIORITY (clip),
          GES_TIMELINE_ELEMENT_START (clip),
          GES_TIMELINE_ELEMENT_DURATION (clip), NULL)) {
    GST_INFO_OBJECT (layer, "Clip %" GES_FORMAT, GES_ARGS (clip));
    ges_layer_remove_clip_internal (layer, clip, TRUE);
    return FALSE;
  }

  return TRUE;
}

/**
 * ges_layer_add_asset:
 * @layer: a #GESLayer
 * @asset: The asset to add to
 * @start: The start value to set on the new #GESClip,
 * if @start == GST_CLOCK_TIME_NONE, it will be set to
 * the current duration of @layer
 * @inpoint: The inpoint value to set on the new #GESClip
 * @duration: The duration value to set on the new #GESClip
 * @track_types: The #GESTrackType to set on the the new #GESClip
 *
 * Creates Clip from asset, adds it to layer and returns its pointer.
 *
 * Returns: (transfer none): Created #GESClip
 */
GESClip *
ges_layer_add_asset (GESLayer * layer,
    GESAsset * asset, GstClockTime start, GstClockTime inpoint,
    GstClockTime duration, GESTrackType track_types)
{
  GESClip *clip;

  g_return_val_if_fail (GES_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GES_IS_ASSET (asset), NULL);
  g_return_val_if_fail (g_type_is_a (ges_asset_get_extractable_type
          (asset), GES_TYPE_CLIP), NULL);

  GST_DEBUG_OBJECT (layer, "Adding asset %s with: start: %" GST_TIME_FORMAT
      " inpoint: %" GST_TIME_FORMAT " duration: %" GST_TIME_FORMAT
      " track types: %d (%s)", ges_asset_get_id (asset), GST_TIME_ARGS (start),
      GST_TIME_ARGS (inpoint), GST_TIME_ARGS (duration), track_types,
      ges_track_type_name (track_types));

  clip = GES_CLIP (ges_asset_extract (asset, NULL));

  if (!GST_CLOCK_TIME_IS_VALID (start)) {
    start = ges_layer_get_duration (layer);

    GST_DEBUG_OBJECT (layer,
        "No start specified, setting it to %" GST_TIME_FORMAT,
        GST_TIME_ARGS (start));
  }

  _set_start0 (GES_TIMELINE_ELEMENT (clip), start);
  _set_inpoint0 (GES_TIMELINE_ELEMENT (clip), inpoint);
  if (track_types != GES_TRACK_TYPE_UNKNOWN)
    ges_clip_set_supported_formats (clip, track_types);

  if (GST_CLOCK_TIME_IS_VALID (duration)) {
    _set_duration0 (GES_TIMELINE_ELEMENT (clip), duration);
  }

  if (!ges_layer_add_clip (layer, clip)) {
    return NULL;
  }

  return clip;
}

/**
 * ges_layer_add_fasset:
 * @layer: a #GESLayer
 * @asset: The asset to add to
 * @start_frame: The start value in frame to set on the new #GESClip,
 * if @start_FRAME == %GES_FRAME_NONE, it will be set to the current duration of @layer
 * @inpoint_frame: The inpoint value in frame to set on the new #GESClip
 * @duration_frame: The duration value in frame to set on the new #GESClip
 * @track_types: The #GESTrackType to set on the the new #GESClip
 *
 * Creates Clip from asset, adds it to layer and returns its pointer.
 *
 * Returns: (transfer none): Created #GESClip
 *
 * Since: 1.18
 */
GESClip *
ges_layer_add_fasset (GESLayer * layer,
    GESAsset * asset, guint64 start_frame, guint64 inpoint_frame,
    guint64 duration_frame, GESTrackType track_types)
{
  GESClip *clip;
  gint fps_n, fps_d;
  GstVideoTimeCodeFlags flags;

  g_return_val_if_fail (GES_IS_LAYER (layer), NULL);
  g_return_val_if_fail (GES_IS_ASSET (asset), NULL);
  g_return_val_if_fail (g_type_is_a (ges_asset_get_extractable_type
          (asset), GES_TYPE_CLIP), NULL);
  g_return_val_if_fail (layer->timeline, NULL);
  g_return_val_if_fail (ges_timeline_get_timecodes_config (layer->timeline,
          &fps_n, &fps_d, &flags), NULL);

  GST_DEBUG_OBJECT (layer, "Adding asset %s with: start: %" G_GUINT64_FORMAT
      " inpoint: %" G_GUINT64_FORMAT " duration: %" G_GUINT64_FORMAT
      " at %d/%dfps - track types: %d (%s)", ges_asset_get_id (asset),
      start_frame, inpoint_frame, duration_frame, track_types,
      fps_n, fps_d, ges_track_type_name (track_types));

  clip = GES_CLIP (ges_asset_extract (asset, NULL));

  if (!GES_FRAME_IS_VALID (start_frame)) {
    start_frame = ges_layer_get_fduration (layer);

    GST_DEBUG_OBJECT (layer, "No start specified, setting it to %"
        G_GUINT64_FORMAT, start_frame);
  }

  ges_timeline_element_set_fstart (GES_TIMELINE_ELEMENT (clip), start_frame);
  ges_timeline_element_set_finpoint (GES_TIMELINE_ELEMENT (clip),
      inpoint_frame);
  if (GES_FRAME_IS_VALID (duration_frame))
    ges_timeline_element_set_fduration (GES_TIMELINE_ELEMENT (clip),
        duration_frame);

  if (track_types != GES_TRACK_TYPE_UNKNOWN)
    ges_clip_set_supported_formats (clip, track_types);

  if (!ges_layer_add_clip (layer, clip))
    return NULL;

  if (!ges_timeline_element_reset_framerate_on_edge (GES_TIMELINE_ELEMENT
          (clip), GES_EDGE_NONE, TRUE)) {
    GST_INFO_OBJECT (layer, "Could not set clip timing in frames");
    ges_layer_remove_clip (layer, clip);

    return NULL;
  }

  return clip;
}


/**
 * ges_layer_new:
 *
 * Creates a new #GESLayer.
 *
 * Returns: (transfer floating): A new #GESLayer
 */
GESLayer *
ges_layer_new (void)
{
  return g_object_new (GES_TYPE_LAYER, NULL);
}

/**
 * ges_layer_get_timeline:
 * @layer: The #GESLayer to get the parent #GESTimeline from
 *
 * Get the #GESTimeline in which #GESLayer currently is.
 *
 * Returns: (transfer none) (nullable): the #GESTimeline in which #GESLayer
 * currently is or %NULL if not in any timeline yet.
 */
GESTimeline *
ges_layer_get_timeline (GESLayer * layer)
{
  g_return_val_if_fail (GES_IS_LAYER (layer), NULL);

  return layer->timeline;
}

void
ges_layer_set_timeline (GESLayer * layer, GESTimeline * timeline)
{
  GList *tmp;

  g_return_if_fail (GES_IS_LAYER (layer));

  GST_DEBUG ("layer:%p, timeline:%p", layer, timeline);

  for (tmp = layer->priv->clips_start; tmp; tmp = tmp->next) {
    ges_timeline_element_set_timeline (tmp->data, timeline);
  }

  layer->timeline = timeline;
}

/**
 * ges_layer_get_clips_in_interval:
 * @layer: a #GESLayer
 * @start: start of the interval
 * @end: end of the interval
 *
 * Gets the clips which appear between @start and @end on @layer.
 *
 * Returns: (transfer full) (element-type GESClip): a #GList of clips intersecting [@start, @end) interval on @layer.
 */
GList *
ges_layer_get_clips_in_interval (GESLayer * layer, GstClockTime start,
    GstClockTime end)
{
  GList *tmp;
  GList *intersecting_clips = NULL;
  GstClockTime clip_start, clip_end;
  gboolean clip_intersects;

  g_return_val_if_fail (GES_IS_LAYER (layer), NULL);

  layer->priv->clips_start =
      g_list_sort (layer->priv->clips_start,
      (GCompareFunc) element_start_compare);
  for (tmp = layer->priv->clips_start; tmp; tmp = tmp->next) {
    clip_intersects = FALSE;
    clip_start = ges_timeline_element_get_start (tmp->data);
    clip_end = clip_start + ges_timeline_element_get_duration (tmp->data);
    if (start <= clip_start && clip_start < end)
      clip_intersects = TRUE;
    else if (start < clip_end && clip_end <= end)
      clip_intersects = TRUE;
    else if (clip_start < start && clip_end > end)
      clip_intersects = TRUE;

    if (clip_intersects)
      intersecting_clips =
          g_list_insert_sorted (intersecting_clips,
          gst_object_ref (tmp->data), (GCompareFunc) element_start_compare);
  }
  return intersecting_clips;
}
