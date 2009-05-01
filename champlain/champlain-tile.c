/*
 * Copyright (C) 2008-2009 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
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

#include "champlain-tile.h"

#include "champlain-enum-types.h"
#include "champlain-map.h"
#include "champlain-private.h"

#include <math.h>
#include <errno.h>
#include <gdk/gdk.h>
#include <libsoup/soup.h>
#include <gio/gio.h>
#include <clutter/clutter.h>

G_DEFINE_TYPE (ChamplainTile, champlain_tile, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), CHAMPLAIN_TYPE_TILE, ChamplainTilePrivate))

enum
{
  PROP_0,
  PROP_X,
  PROP_Y,
  PROP_ZOOM_LEVEL,
  PROP_SIZE,
  PROP_URI,
  PROP_FILENAME,
  PROP_STATE,
  PROP_ACTOR,
  PROP_CONTENT,
  PROP_ETAG
};

typedef struct _ChamplainTilePrivate ChamplainTilePrivate;

struct _ChamplainTilePrivate {
  gint x;
  gint y;
  gint size;
  gint zoom_level;

  gchar * uri;
  gpointer data;
  ChamplainState state;
  gchar *filename;
  ClutterActor *actor;
  ClutterActor *content_actor;

  GTimeVal *modified_time;
  gchar* etag;
};

static void
champlain_tile_get_property (GObject *object,
                             guint property_id,
                             GValue *value,
                             GParamSpec *pspec)
{
  ChamplainTile *self = CHAMPLAIN_TILE (object);
  switch (property_id)
    {
      case PROP_X:
        g_value_set_int (value, champlain_tile_get_x (self));
        break;
      case PROP_Y:
        g_value_set_int (value, champlain_tile_get_y (self));
        break;
      case PROP_ZOOM_LEVEL:
        g_value_set_int (value, champlain_tile_get_zoom_level (self));
        break;
      case PROP_SIZE:
        g_value_set_uint (value, champlain_tile_get_size (self));
        break;
      case PROP_STATE:
        g_value_set_enum (value, champlain_tile_get_state (self));
        break;
      case PROP_URI:
        g_value_set_string (value, champlain_tile_get_uri (self));
        break;
      case PROP_FILENAME:
        g_value_set_string (value, champlain_tile_get_filename (self));
        break;
      case PROP_ACTOR:
        g_value_set_object (value, champlain_tile_get_actor (self));
        break;
      case PROP_CONTENT:
        g_value_set_object (value, champlain_tile_get_content (self));
        break;
      case PROP_ETAG:
        g_value_set_string (value, champlain_tile_get_etag (self));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
champlain_tile_set_property (GObject *object,
                             guint property_id,
                             const GValue *value,
                             GParamSpec *pspec)
{
  ChamplainTile *self = CHAMPLAIN_TILE (object);
  switch (property_id)
    {
      case PROP_X:
        champlain_tile_set_x (self, g_value_get_int (value));
        break;
      case PROP_Y:
        champlain_tile_set_y (self, g_value_get_int (value));
        break;
      case PROP_ZOOM_LEVEL:
        champlain_tile_set_zoom_level (self, g_value_get_int (value));
        break;
      case PROP_SIZE:
        champlain_tile_set_size (self, g_value_get_uint (value));
        break;
      case PROP_STATE:
        champlain_tile_set_state (self, g_value_get_enum (value));
        break;
      case PROP_URI:
        champlain_tile_set_uri (self, g_value_get_string (value));
        break;
      case PROP_FILENAME:
        champlain_tile_set_filename (self, g_value_get_string (value));
        break;
      case PROP_ACTOR:
        champlain_tile_set_actor (self, g_value_get_object (value));
        break;
      case PROP_CONTENT:
        champlain_tile_set_content (self, g_value_get_object (value), FALSE);
        break;
      case PROP_ETAG:
        champlain_tile_set_etag (self, g_value_get_string (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
champlain_tile_dispose (GObject *object)
{
  ChamplainTilePrivate *priv = GET_PRIVATE (object);

  g_free (priv->uri);
  g_free (priv->filename);
  if (priv->actor != NULL)
    {
      g_object_unref (G_OBJECT (priv->actor));
      priv->actor = NULL;
    }
  g_object_unref (priv->content_actor);

  G_OBJECT_CLASS (champlain_tile_parent_class)->dispose (object);
}

static void
champlain_tile_finalize (GObject *object)
{
  G_OBJECT_CLASS (champlain_tile_parent_class)->finalize (object);
}

static void
champlain_tile_class_init (ChamplainTileClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (ChamplainTilePrivate));

  object_class->get_property = champlain_tile_get_property;
  object_class->set_property = champlain_tile_set_property;
  object_class->dispose = champlain_tile_dispose;
  object_class->finalize = champlain_tile_finalize;


  g_object_class_install_property (object_class,
      PROP_X,
      g_param_spec_int ("x",
        "x",
        "The X position of the tile",
        G_MININT,
        G_MAXINT,
        0,
        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
      PROP_Y,
      g_param_spec_int ("y",
        "y",
        "The Y position of the tile",
        G_MININT,
        G_MAXINT,
        0,
        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
      PROP_ZOOM_LEVEL,
      g_param_spec_int ("zoom-level",
        "Zoom Level",
        "The zoom level of the tile",
        G_MININT,
        G_MAXINT,
        0,
        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
      PROP_SIZE,
      g_param_spec_uint ("size",
        "Size",
        "The size of the tile",
        0,
        G_MAXINT,
        256,
        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
      PROP_STATE,
      g_param_spec_enum ("state",
        "State",
        "The state of the tile",
        CHAMPLAIN_TYPE_STATE,
        CHAMPLAIN_STATE_NONE,
        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
      PROP_URI,
      g_param_spec_string ("uri",
        "URI",
        "The URI of the tile",
        "",
        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
      PROP_FILENAME,
      g_param_spec_string ("filename",
        "Filename",
        "The filename of the tile",
        "",
        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
      PROP_ACTOR,
      g_param_spec_object ("actor",
        "Actor",
        "The tile's actor",
        CLUTTER_TYPE_ACTOR,
        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
      PROP_CONTENT,
      g_param_spec_object ("content",
        "Content",
        "The tile's content",
        CLUTTER_TYPE_ACTOR,
        G_PARAM_READWRITE));

  g_object_class_install_property (object_class,
      PROP_ETAG,
      g_param_spec_string ("etag",
        "Entity Tag",
        "The entity tag of the tile",
        NULL,
        G_PARAM_READWRITE));

}

static void
champlain_tile_init (ChamplainTile *self)
{
  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  priv->state = CHAMPLAIN_STATE_INIT;
  priv->x = 0;
  priv->y = 0;
  priv->zoom_level = 0;
  priv->size = 0;
  priv->uri = g_strdup ("");
  priv->filename = g_strdup ("");
  priv->etag = NULL;

  priv->actor = g_object_ref (clutter_group_new ());
  priv->content_actor = NULL;
}

ChamplainTile*
champlain_tile_new (void)
{
  return g_object_new (CHAMPLAIN_TYPE_TILE, NULL);
}

gint
champlain_tile_get_x (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), 0);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->x;
}

gint
champlain_tile_get_y (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), 0);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->y;
}

gint
champlain_tile_get_zoom_level (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), 0);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->zoom_level;
}

guint
champlain_tile_get_size (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), 0);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->size;
}

ChamplainState
champlain_tile_get_state (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), CHAMPLAIN_STATE_NONE);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->state;
}

G_CONST_RETURN gchar *
champlain_tile_get_uri (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), NULL);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->uri;
}

G_CONST_RETURN gchar *
champlain_tile_get_filename (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), NULL);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->filename;
}

ClutterActor *
champlain_tile_get_actor (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), NULL);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->actor;
}

void
champlain_tile_set_x (ChamplainTile *self, gint x)
{
  g_return_if_fail(CHAMPLAIN_TILE(self));

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  priv->x = x;
  g_object_notify (G_OBJECT (self), "x");
}

void
champlain_tile_set_y (ChamplainTile *self, gint y)
{
  g_return_if_fail(CHAMPLAIN_TILE(self));

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  priv->y = y;
  g_object_notify (G_OBJECT (self), "y");
}

void
champlain_tile_set_zoom_level (ChamplainTile *self, gint zoom_level)
{
  g_return_if_fail(CHAMPLAIN_TILE(self));

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  priv->zoom_level = zoom_level;
  g_object_notify (G_OBJECT (self), "zoom-level");
}

void
champlain_tile_set_size (ChamplainTile *self, guint size)
{
  g_return_if_fail(CHAMPLAIN_TILE(self));

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  priv->size = size;
  g_object_notify (G_OBJECT (self), "size");
}

void
champlain_tile_set_state (ChamplainTile *self, ChamplainState state)
{
  g_return_if_fail(CHAMPLAIN_TILE(self));

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  priv->state = state;
  g_object_notify (G_OBJECT (self), "state");
}

ChamplainTile*
champlain_tile_new_full (gint x,
                         gint y,
                         guint size,
                         gint zoom_level)
{
  return g_object_new (CHAMPLAIN_TYPE_TILE, "x", x, "y", y, "zoom-level",
      zoom_level, "size", size, NULL);
}

void
champlain_tile_set_uri (ChamplainTile *self,
                        const gchar *uri)
{
  g_return_if_fail(CHAMPLAIN_TILE(self));
  g_return_if_fail(uri != NULL);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  g_free (priv->uri);
  priv->uri = g_strdup (uri);
  g_object_notify (G_OBJECT (self), "uri");
}

void
champlain_tile_set_filename (ChamplainTile *self,
                             const gchar *filename)
{
  g_return_if_fail(CHAMPLAIN_TILE(self));
  g_return_if_fail(filename != NULL);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  g_free (priv->filename);
  priv->filename = g_strdup (filename);
  g_object_notify (G_OBJECT (self), "filename");
}

void
champlain_tile_set_actor (ChamplainTile *self, ClutterActor *actor)
{
  g_return_if_fail(CHAMPLAIN_TILE(self));
  g_return_if_fail(actor != NULL);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  if (priv->actor != NULL)
    g_object_unref (priv->actor);

  priv->actor = g_object_ref (actor);
  g_object_notify (G_OBJECT (self), "actor");
}

const GTimeVal *
champlain_tile_get_modified_time (ChamplainTile *self)
{
  g_return_val_if_fail (CHAMPLAIN_TILE(self), NULL);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->modified_time;
}

void
champlain_tile_set_modified_time (ChamplainTile *self,
    GTimeVal *time)
{
  g_return_if_fail (CHAMPLAIN_TILE(self));
  g_return_if_fail (time != NULL);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  priv->modified_time = time;
}

char *
champlain_tile_get_modified_time_string (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), NULL);
  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  struct tm *other_time = gmtime (&priv->modified_time->tv_sec);
  char value [100];

  strftime (value, 100, "%a, %d %b %Y %T %Z", other_time);

  return g_strdup (value);
}

const char *
champlain_tile_get_etag (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), "");
  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->etag;
}

void
champlain_tile_set_etag (ChamplainTile *self,
    const gchar *etag)
{
  g_return_if_fail(CHAMPLAIN_TILE(self));

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  priv->etag = g_strdup (etag);
  g_object_notify (G_OBJECT (self), "etag");
}

typedef struct {
  ChamplainTile *tile;
  ClutterEffectTemplate *etemplate;
  ClutterTimeline *timeline;
  ClutterActor *old_actor;
} AnimationContext;

static void
fade_in_completed (ClutterTimeline *timeline,
    gpointer data)
{
  AnimationContext* ctx = (AnimationContext*) data;
  ChamplainTilePrivate *priv = GET_PRIVATE (ctx->tile);

  if (ctx->old_actor != NULL)
  {
    g_object_unref (ctx->old_actor);
    clutter_container_remove (CLUTTER_CONTAINER (priv->actor), ctx->old_actor, NULL);
  }

  g_object_unref (ctx->tile);
  g_object_unref (ctx->timeline);
  g_object_unref (ctx->etemplate);
  g_free (ctx);

}


void
champlain_tile_set_content (ChamplainTile *self, ClutterActor *actor, gboolean fade_in)
{
  g_return_if_fail(CHAMPLAIN_TILE(self));
  g_return_if_fail(actor != NULL);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);
  ClutterActor *old_actor = NULL;

  if (priv->content_actor != NULL)
    {
      if (fade_in == TRUE)
        old_actor = g_object_ref (priv->content_actor);
      else
        clutter_container_remove (CLUTTER_CONTAINER (priv->actor), priv->content_actor, NULL);
      g_object_unref (priv->content_actor);
    }

  clutter_container_add (CLUTTER_CONTAINER (priv->actor), actor, NULL);

  /* fixme: etemplate are leaked here */
  if (fade_in == TRUE)
    {
      AnimationContext *ctx = g_new0 (AnimationContext, 1);
      ctx->tile = g_object_ref (self);
      ctx->timeline = clutter_timeline_new_for_duration (750);
      ctx->etemplate = clutter_effect_template_new (ctx->timeline, CLUTTER_ALPHA_SINE_INC);
      ctx->old_actor = old_actor;

      g_signal_connect (ctx->timeline, "completed", G_CALLBACK (fade_in_completed), ctx);

      clutter_actor_set_opacity (actor, 0);
      clutter_effect_fade (ctx->etemplate, actor, 255, NULL, NULL);
    }

  priv->content_actor = g_object_ref (actor);
  g_object_notify (G_OBJECT (self), "content");
}

ClutterActor *
champlain_tile_get_content (ChamplainTile *self)
{
  g_return_val_if_fail(CHAMPLAIN_TILE(self), NULL);

  ChamplainTilePrivate *priv = GET_PRIVATE (self);

  return priv->content_actor;
}

