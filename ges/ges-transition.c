/* GStreamer Editing Services
 * Copyright (C) 2010 Brandon Lewis <brandon@collabora.co.uk>
 *               2010 Nokia Corporation
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
 * SECTION:gestransition
 * @title: GESTransition
 * @short_description: base class for audio and video transitions
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ges/ges.h>
#include "ges-internal.h"

struct _GESTransitionPrivate
{
  /*  Dummy variable */
  void *nothing;
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (GESTransition, ges_transition,
    GES_TYPE_OPERATION);


static void
ges_transition_class_init (GESTransitionClass * klass)
{
}

static void
ges_transition_init (GESTransition * self)
{
  self->priv = ges_transition_get_instance_private (self);
}
