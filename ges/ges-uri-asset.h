/* GStreamer Editing Services
 *
 * Copyright (C) 2012 Thibault Saunier <thibault.saunier@collabora.com>
 * Copyright (C) 2012 Volodymyr Rudyi <vladimir.rudoy@gmail.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#ifndef _GES_URI_CLIP_ASSET_
#define _GES_URI_CLIP_ASSET_

#include <glib-object.h>
#include <gio/gio.h>
#include <ges/ges-types.h>
#include <ges/ges-asset.h>
#include <ges/ges-asset-clip.h>
#include <ges/ges-asset-track-object.h>

G_BEGIN_DECLS
#define GES_TYPE_URI_CLIP_ASSET ges_uri_clip_asset_get_type()
#define GES_URI_CLIP_ASSET(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_URI_CLIP_ASSET, GESUriClipAsset))
#define GES_URI_CLIP_ASSET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_URI_CLIP_ASSET, GESUriClipAssetClass))
#define GES_IS_URI_CLIP_ASSET(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_URI_CLIP_ASSET))
#define GES_IS_URI_CLIP_ASSET_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_URI_CLIP_ASSET))
#define GES_URI_CLIP_ASSET_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_URI_CLIP_ASSET, GESUriClipAssetClass))

typedef struct _GESUriClipAssetPrivate GESUriClipAssetPrivate;

GType ges_uri_clip_asset_get_type (void);

struct _GESUriClipAsset
{
  GESAssetClip parent;

  /* <private> */
  GESUriClipAssetPrivate *priv;

  /* Padding for API extension */
  gpointer __ges_reserved[GES_PADDING];
};

struct _GESUriClipAssetClass
{
  GESAssetClipClass parent_class;

  /* <private> */
  GstDiscoverer *discoverer;

  gpointer _ges_reserved[GES_PADDING];
};

GstDiscovererInfo *ges_uri_clip_asset_get_info      (const GESUriClipAsset * self);
GstClockTime ges_uri_clip_asset_get_duration        (GESUriClipAsset *self);
gboolean ges_uri_clip_asset_is_image                (GESUriClipAsset *self);
void ges_uri_clip_asset_new                         (const gchar *uri,
                                                       GCancellable *cancellable,
                                                       GAsyncReadyCallback callback,
                                                       gpointer user_data);
void ges_uri_clip_asset_set_timeout                 (GESUriClipAssetClass *class,
                                                       GstClockTime timeout);
const GList * ges_uri_clip_asset_get_stream_assets  (GESUriClipAsset *self);

#define GES_TYPE_ASSET_TRACK_FILESOURCE ges_uri_source_asset_get_type()
#define GES_ASSET_TRACK_FILESOURCE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), GES_TYPE_ASSET_TRACK_FILESOURCE, GESUriSourceAsset))
#define GES_ASSET_TRACK_FILESOURCE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), GES_TYPE_ASSET_TRACK_FILESOURCE, GESUriSourceAssetClass))
#define GES_IS_ASSET_TRACK_FILESOURCE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GES_TYPE_ASSET_TRACK_FILESOURCE))
#define GES_IS_ASSET_TRACK_FILESOURCE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), GES_TYPE_ASSET_TRACK_FILESOURCE))
#define GES_ASSET_TRACK_FILESOURCE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), GES_TYPE_ASSET_TRACK_FILESOURCE, GESUriSourceAssetClass))

typedef struct _GESUriSourceAssetPrivate GESUriSourceAssetPrivate;

GType ges_uri_source_asset_get_type (void);

struct _GESUriSourceAsset
{
  GESAssetTrackElement parent;

  /* <private> */
  GESUriSourceAssetPrivate *priv;

  /* Padding for API extension */
  gpointer __ges_reserved[GES_PADDING];
};

struct _GESUriSourceAssetClass
{
  GESAssetTrackElementClass parent_class;

  gpointer _ges_reserved[GES_PADDING];
};
GstDiscovererStreamInfo * ges_uri_source_asset_get_stream_info     (GESUriSourceAsset *asset);
const gchar * ges_uri_source_asset_get_stream_uri                  (GESUriSourceAsset *asset);
const GESUriClipAsset *ges_uri_source_asset_get_filesource_asset (GESUriSourceAsset *asset);

G_END_DECLS
#endif /* _GES_URI_CLIP_ASSET */
