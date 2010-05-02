/*
 * Copyright (C) 2008 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:champlain-layer
 * @short_description: A container for #ChamplainMarker
 *
 * A ChamplainLayer is little more than a #ClutterContainer. It keeps the
 * markers ordered so that they display correctly.
 *
 * Use #clutter_container_add to add markers to the layer and
 * #clutter_container_remove to remove them.
 */

#include "config.h"

#include "champlain-layer.h"

#include "champlain-defines.h"
#include "champlain-base-marker.h"

#include <clutter/clutter.h>
#include <glib.h>

G_DEFINE_TYPE (ChamplainLayer, champlain_layer, CLUTTER_TYPE_GROUP)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHAMPLAIN_TYPE_LAYER, ChamplainLayerPrivate))

enum
{
  PROP_0
};

typedef struct _ChamplainLayerPrivate ChamplainLayerPrivate;

struct _ChamplainLayerPrivate {
  gpointer spacer;
};

static void layer_add_cb (ClutterGroup *layer,
    ClutterActor *marker,
    gpointer data);
static void layer_remove_cb (ClutterGroup *layer,
    ClutterActor *marker,
    gpointer data);

static void
champlain_layer_get_property (GObject *object,
    guint property_id,
    GValue *value,
    GParamSpec *pspec)
{
  //ChamplainLayer *self = CHAMPLAIN_LAYER (object);
  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
champlain_layer_set_property (GObject *object,
    guint property_id,
    const GValue *value,
    GParamSpec *pspec)
{
  //ChamplainLayer *self = CHAMPLAIN_LAYER (object);
  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
champlain_layer_class_init (ChamplainLayerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ChamplainLayerPrivate));

  object_class->get_property = champlain_layer_get_property;
  object_class->set_property = champlain_layer_set_property;
}

static void
champlain_layer_init (ChamplainLayer *self)
{
  g_signal_connect_after (G_OBJECT (self), "actor-added",
      G_CALLBACK (layer_add_cb), NULL);
  g_signal_connect_after (G_OBJECT (self), "actor-removed",
      G_CALLBACK (layer_remove_cb), NULL);
}

/* This callback serves to keep the markers ordered by their latitude.
 * Markers that are up north on the map should be lowered in the list so that
 * they are drawn the first. This is to make the illusion of a semi-3d plane
 * where the most north you are, the farther you are.
 */
static void
reorder_marker (ClutterGroup *layer,
    ChamplainBaseMarker *marker)
{
  guint i;
  gdouble y, tmp_y, low_y;
  ChamplainBaseMarker *lowest = NULL;

  g_object_get (G_OBJECT (marker), "latitude", &y, NULL);
  y = 90 - y;
  low_y = G_MAXDOUBLE;

  for (i = 0; i < clutter_group_get_n_children (layer); i++)
    {
      ChamplainBaseMarker *prev_marker = CHAMPLAIN_BASE_MARKER (clutter_group_get_nth_child (layer, i));

      if (prev_marker == (ChamplainBaseMarker*) marker)
        continue;

      g_object_get(G_OBJECT(prev_marker), "latitude", &tmp_y, NULL);
      tmp_y = 90 - tmp_y;

      if (y < tmp_y && tmp_y < low_y)
        {
          lowest = prev_marker;
          low_y = tmp_y;
        }
    }

  if (lowest)
    clutter_container_lower_child (CLUTTER_CONTAINER(layer),
        CLUTTER_ACTOR (marker), CLUTTER_ACTOR (lowest));
}

static void
marker_position_notify (GObject *gobject,
    GParamSpec *pspec,
    gpointer user_data)
{
  reorder_marker (CLUTTER_GROUP (user_data), CHAMPLAIN_BASE_MARKER (gobject));
}

static void
layer_add_cb (ClutterGroup *layer,
    ClutterActor *actor,
    gpointer data)
{
  ChamplainBaseMarker *marker = CHAMPLAIN_BASE_MARKER (actor);
  reorder_marker (layer, marker);

  g_signal_connect (G_OBJECT (marker), "notify::latitude",
      G_CALLBACK (marker_position_notify), layer);
}

static void
layer_remove_cb (ClutterGroup *layer,
    ClutterActor *actor,
    gpointer data)
{
  g_signal_handlers_disconnect_by_func (G_OBJECT (actor),
      G_CALLBACK (marker_position_notify), layer);
}

/**
 * champlain_layer_new:
 *
 * Returns: a new #ChamplainLayer ready to be used as a #ClutterContainer for the markers.
 *
 * Since: 0.2.2
 */
ChamplainLayer *
champlain_layer_new ()
{
  return g_object_new (CHAMPLAIN_TYPE_LAYER, NULL);
}

/**
 * champlain_layer_add_marker:
 * @layer: a #ChamplainLayer
 * @marker: a #ChamplainBaseMarker
 *
 * Adds the marker to the layer.
 *
 * Since: 0.4
 */
void
champlain_layer_add_marker (ChamplainLayer *layer,
    ChamplainBaseMarker *marker)
{
  g_return_if_fail (CHAMPLAIN_IS_LAYER (layer));
  g_return_if_fail (CHAMPLAIN_IS_BASE_MARKER (marker));

  clutter_container_add_actor (CLUTTER_CONTAINER (layer), CLUTTER_ACTOR (marker));
}

/**
 * champlain_layer_remove_marker:
 * @layer: a #ChamplainLayer
 * @marker: a #ChamplainBaseMarker
 *
 * Removes the marker from the layer.
 *
 * Since: 0.4
 */
void
champlain_layer_remove_marker (ChamplainLayer *layer,
    ChamplainBaseMarker *marker)
{
  g_return_if_fail (CHAMPLAIN_IS_LAYER (layer));
  g_return_if_fail (CHAMPLAIN_IS_BASE_MARKER (marker));

  clutter_container_remove_actor (CLUTTER_CONTAINER (layer), CLUTTER_ACTOR (marker));
}

/**
 * champlain_layer_show:
 * @layer: a #ChamplainLayer
 *
 * Makes the layer and its markers visible.
 *
 * Since: 0.4
 */
void
champlain_layer_show (ChamplainLayer *layer)
{
  g_return_if_fail (CHAMPLAIN_IS_LAYER (layer));

  clutter_actor_show (CLUTTER_ACTOR (layer));
}

/**
 * champlain_layer_hide:
 * @layer: a #ChamplainLayer
 *
 * Makes the layer and its markers invisible.
 *
 * Since: 0.4
 */
void
champlain_layer_hide (ChamplainLayer *layer)
{
  g_return_if_fail (CHAMPLAIN_IS_LAYER (layer));

  clutter_actor_hide (CLUTTER_ACTOR (layer));
}

/**
 * champlain_layer_animate_in_all_markers:
 * @layer: a #ChamplainLayer
 *
 * Fade in all markers with an animation
 *
 * Since: 0.4
 */
void
champlain_layer_animate_in_all_markers (ChamplainLayer *layer)
{
  guint i;
  guint delay = 0;

  g_return_if_fail (CHAMPLAIN_IS_LAYER (layer));

  for (i = 0; i < clutter_group_get_n_children (CLUTTER_GROUP (layer)); i++)
    {
      ChamplainBaseMarker *marker = CHAMPLAIN_BASE_MARKER (clutter_group_get_nth_child (CLUTTER_GROUP (layer), i));

      champlain_base_marker_animate_in_with_delay (marker, delay);
      delay += 50;
    }
}

/**
 * champlain_layer_animate_out_all_markers:
 * @layer: a #ChamplainLayer
 *
 * Fade out all markers with an animation
 *
 * Since: 0.4
 */
void
champlain_layer_animate_out_all_markers (ChamplainLayer *layer)
{
  guint i;
  guint delay = 0;

  g_return_if_fail (CHAMPLAIN_IS_LAYER (layer));

  for (i = 0; i < clutter_group_get_n_children (CLUTTER_GROUP (layer)); i++)
    {
      ChamplainBaseMarker *marker = CHAMPLAIN_BASE_MARKER (clutter_group_get_nth_child (CLUTTER_GROUP (layer), i));

      champlain_base_marker_animate_out_with_delay (marker, delay);
      delay += 50;
    }
}

/**
 * champlain_layer_show_all_markers:
 * @layer: a #ChamplainLayer
 *
 * Calls clutter_actor_show on all markers
 *
 * Since: 0.4
 */
void
champlain_layer_show_all_markers (ChamplainLayer *layer)
{
  guint i;

  g_return_if_fail (CHAMPLAIN_IS_LAYER (layer));

  for (i = 0; i < clutter_group_get_n_children (CLUTTER_GROUP (layer)); i++)
    {
      ClutterActor *marker = CLUTTER_ACTOR (clutter_group_get_nth_child (CLUTTER_GROUP (layer), i));

      clutter_actor_show (marker);
    }
}

/**
 * champlain_layer_hide_all_markers:
 * @layer: a #ChamplainLayer
 *
 * Calls clutter_actor_hide on all markers
 *
 * Since: 0.4
 */
void
champlain_layer_hide_all_markers (ChamplainLayer *layer)
{
  guint i;

  g_return_if_fail (CHAMPLAIN_IS_LAYER (layer));

  for (i = 0; i < clutter_group_get_n_children (CLUTTER_GROUP (layer)); i++)
    {
      ClutterActor *marker = CLUTTER_ACTOR (clutter_group_get_nth_child (CLUTTER_GROUP (layer), i));

      clutter_actor_hide (marker);
    }
}
