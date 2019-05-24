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
 * SECTION:gesaudiourisource
 * @title: GESAudioUriSource
 * @short_description: outputs a single audio stream from a given file
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ges-utils.h"
#include "ges-internal.h"
#include "ges-track-element.h"
#include "ges-audio-uri-source.h"
#include "ges-uri-asset.h"
#include "ges-extractable.h"

struct _GESAudioUriSourcePrivate
{
  GstElement *decodebin;        /* Reference owned by parent class */
};

enum
{
  PROP_0,
  PROP_URI
};

static void
ges_audio_uri_source_track_set_cb (GESAudioUriSource * self,
    GParamSpec * arg G_GNUC_UNUSED, gpointer nothing)
{
  GESTrack *track;
  const GstCaps *caps = NULL;

  if (!self->priv->decodebin)
    return;

  track = ges_track_element_get_track (GES_TRACK_ELEMENT (self));
  if (!track)
    return;

  caps = ges_track_get_caps (track);

  GST_INFO_OBJECT (self, "Setting caps to: %" GST_PTR_FORMAT, caps);
  g_object_set (self->priv->decodebin, "caps", caps, NULL);
}

/* GESSource VMethod */
static GstElement *
ges_audio_uri_source_create_source (GESTrackElement * trksrc)
{
  GESAudioUriSource *self;
  GESTrack *track;
  GstElement *decodebin;
  const GstCaps *caps = NULL;

  self = (GESAudioUriSource *) trksrc;

  track = ges_track_element_get_track (trksrc);

  self->priv->decodebin = decodebin =
      gst_element_factory_make ("uridecodebin", NULL);

  if (track)
    caps = ges_track_get_caps (track);

  g_object_set (decodebin, "caps", caps,
      "expose-all-streams", FALSE, "uri", self->uri, NULL);

  return decodebin;
}

/* Extractable interface implementation */

static gchar *
ges_extractable_check_id (GType type, const gchar * id, GError ** error)
{
  return g_strdup (id);
}

static void
extractable_set_asset (GESExtractable * self, GESAsset * asset)
{
  /* FIXME That should go into #GESTrackElement, but
   * some work is needed to make sure it works properly */

  if (ges_track_element_get_track_type (GES_TRACK_ELEMENT (self)) ==
      GES_TRACK_TYPE_UNKNOWN) {
    ges_track_element_set_track_type (GES_TRACK_ELEMENT (self),
        ges_track_element_asset_get_track_type (GES_TRACK_ELEMENT_ASSET
            (asset)));
  }
}

static void
ges_extractable_interface_init (GESExtractableInterface * iface)
{
  iface->asset_type = GES_TYPE_URI_SOURCE_ASSET;
  iface->check_id = ges_extractable_check_id;
  iface->set_asset = extractable_set_asset;
}

G_DEFINE_TYPE_WITH_CODE (GESAudioUriSource, ges_audio_uri_source,
    GES_TYPE_AUDIO_SOURCE, G_ADD_PRIVATE (GESAudioUriSource)
    G_IMPLEMENT_INTERFACE (GES_TYPE_EXTRACTABLE,
        ges_extractable_interface_init));


/* GObject VMethods */

static gboolean
_get_natural_framerate (GESTimelineElement * self, gint * framerate_n,
    gint * framerate_d)
{
  if (self->parent)
    return ges_timeline_element_get_natural_framerate (self->parent,
        framerate_n, framerate_d);

  return FALSE;
}

static void
ges_audio_uri_source_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GESAudioUriSource *uriclip = GES_AUDIO_URI_SOURCE (object);

  switch (property_id) {
    case PROP_URI:
      g_value_set_string (value, uriclip->uri);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_audio_uri_source_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GESAudioUriSource *uriclip = GES_AUDIO_URI_SOURCE (object);

  switch (property_id) {
    case PROP_URI:
      if (uriclip->uri) {
        GST_WARNING_OBJECT (object, "Uri already set to %s", uriclip->uri);
        return;
      }
      uriclip->uri = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
ges_audio_uri_source_dispose (GObject * object)
{
  GESAudioUriSource *uriclip = GES_AUDIO_URI_SOURCE (object);

  if (uriclip->uri)
    g_free (uriclip->uri);

  G_OBJECT_CLASS (ges_audio_uri_source_parent_class)->dispose (object);
}

static void
ges_audio_uri_source_class_init (GESAudioUriSourceClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GESTimelineElementClass *element_class = GES_TIMELINE_ELEMENT_CLASS (klass);
  GESAudioSourceClass *source_class = GES_AUDIO_SOURCE_CLASS (klass);

  object_class->get_property = ges_audio_uri_source_get_property;
  object_class->set_property = ges_audio_uri_source_set_property;
  object_class->dispose = ges_audio_uri_source_dispose;

  /**
   * GESAudioUriSource:uri:
   *
   * The location of the file/resource to use.
   */
  g_object_class_install_property (object_class, PROP_URI,
      g_param_spec_string ("uri", "URI", "uri of the resource",
          NULL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  element_class->get_natural_framerate = _get_natural_framerate;

  source_class->create_source = ges_audio_uri_source_create_source;
}

static void
ges_audio_uri_source_init (GESAudioUriSource * self)
{
  self->priv = ges_audio_uri_source_get_instance_private (self);

  g_signal_connect (self, "notify::track",
      G_CALLBACK (ges_audio_uri_source_track_set_cb), NULL);
}

/**
 * ges_audio_uri_source_new:
 * @uri: the URI the source should control
 *
 * Creates a new #GESAudioUriSource for the provided @uri.
 *
 * Returns: (transfer floating) (nullable): The newly created
 * #GESAudioUriSource, or %NULL if there was an error.
 */
GESAudioUriSource *
ges_audio_uri_source_new (gchar * uri)
{
  return g_object_new (GES_TYPE_AUDIO_URI_SOURCE, "uri", uri, NULL);
}
