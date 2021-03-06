/* tidy-viewport.c: Viewport actor
 *
 * Copyright (C) 2008 OpenedHand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Chris Lord <chris@openedhand.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <clutter/clutter.h>

#include "tidy-viewport.h"
#include "tidy-adjustment.h"
#include "tidy-scrollable.h"
#include "tidy-private.h"

static void scrollable_interface_init (TidyScrollableInterface *iface);

static void scrollable_set_adjustments (TidyScrollable *scrollable,
                                        TidyAdjustment *hadjustment,
                                        TidyAdjustment *vadjustment);

static void scrollable_get_adjustments (TidyScrollable  *scrollable,
                                        TidyAdjustment **hadjustment,
                                        TidyAdjustment **vadjustment);

G_DEFINE_TYPE_WITH_CODE (TidyViewport, tidy_viewport, CLUTTER_TYPE_GROUP,
                         G_IMPLEMENT_INTERFACE (TIDY_TYPE_SCROLLABLE,
                                                scrollable_interface_init))

#define VIEWPORT_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), TIDY_TYPE_VIEWPORT, \
  TidyViewportPrivate))

struct _TidyViewportPrivate
{
  gfloat x;
  gfloat y;
  gfloat z;

  TidyAdjustment *hadjustment;
  TidyAdjustment *vadjustment;

  gboolean sync_adjustments;
};

enum
{
  PROP_0,

  PROP_X_ORIGIN,
  PROP_Y_ORIGIN,
  PROP_Z_ORIGIN,
  PROP_HADJUST,
  PROP_VADJUST,
  PROP_SYNC_ADJUST
};

static void
tidy_viewport_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  TidyAdjustment *adjustment;

  TidyViewportPrivate *priv = TIDY_VIEWPORT (object)->priv;

  switch (prop_id)
    {
    case PROP_X_ORIGIN:
      g_value_set_int (value, priv->x);
      break;

    case PROP_Y_ORIGIN:
      g_value_set_int (value, priv->y);
      break;

    case PROP_Z_ORIGIN:
      g_value_set_int (value, priv->z);
      break;

    case PROP_HADJUST :
      scrollable_get_adjustments (TIDY_SCROLLABLE (object), &adjustment, NULL);
      g_value_set_object (value, adjustment);
      break;

    case PROP_VADJUST :
      scrollable_get_adjustments (TIDY_SCROLLABLE (object), NULL, &adjustment);
      g_value_set_object (value, adjustment);
      break;

    case PROP_SYNC_ADJUST :
      g_value_set_boolean (value, priv->sync_adjustments);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
tidy_viewport_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  TidyViewport *viewport = TIDY_VIEWPORT (object);
  TidyViewportPrivate *priv = viewport->priv;

  switch (prop_id)
    {
    case PROP_X_ORIGIN:
      tidy_viewport_set_origin (viewport,
                                 g_value_get_int (value),
                                 priv->y,
                                 priv->z);
      break;

    case PROP_Y_ORIGIN:
      tidy_viewport_set_origin (viewport,
                                 priv->x,
                                 g_value_get_int (value),
                                 priv->z);
      break;

    case PROP_Z_ORIGIN:
      tidy_viewport_set_origin (viewport,
                                 priv->x,
                                 priv->y,
                                 g_value_get_int (value));
      break;

    case PROP_HADJUST :
      scrollable_set_adjustments (TIDY_SCROLLABLE (object),
                                  g_value_get_object (value),
                                  priv->vadjustment);
      break;

    case PROP_VADJUST :
      scrollable_set_adjustments (TIDY_SCROLLABLE (object),
                                  priv->hadjustment,
                                  g_value_get_object (value));
      break;

    case PROP_SYNC_ADJUST :
      priv->sync_adjustments = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

void
tidy_viewport_stop (TidyViewport *viewport)
{
  TidyViewportPrivate *priv = TIDY_VIEWPORT (viewport)->priv;

  tidy_adjustment_interpolate_stop (priv->hadjustment);
  tidy_adjustment_interpolate_stop (priv->vadjustment);
}

static void
tidy_viewport_dispose (GObject *gobject)
{
  TidyViewportPrivate *priv = TIDY_VIEWPORT (gobject)->priv;

  if (priv->hadjustment)
    {
      tidy_adjustment_interpolate_stop (priv->hadjustment);
      g_object_unref (priv->hadjustment);
      priv->hadjustment = NULL;
    }

  if (priv->vadjustment)
    {
      tidy_adjustment_interpolate_stop (priv->vadjustment);
      g_object_unref (priv->vadjustment);
      priv->vadjustment = NULL;
    }

  G_OBJECT_CLASS (tidy_viewport_parent_class)->dispose (gobject);
}

static void
tidy_viewport_paint (ClutterActor *self)
{
  TidyViewportPrivate *priv = TIDY_VIEWPORT (self)->priv;

  cogl_push_matrix ();

  cogl_translate ((priv->x) * -1.0,
                  (priv->y) * -1.0,
                  (priv->z) * -1.0);

  CLUTTER_ACTOR_CLASS (tidy_viewport_parent_class)->paint (self);

  cogl_pop_matrix ();
}

static void
tidy_viewport_pick (ClutterActor       *self,
                    const ClutterColor *color)
{
  tidy_viewport_paint (self);
}

static void
tidy_viewport_allocate (ClutterActor          *self,
                        const ClutterActorBox *box,
                        ClutterAllocationFlags flags)
{
  CoglFixed prev_value;

  TidyViewportPrivate *priv = TIDY_VIEWPORT (self)->priv;

  /* Chain up */
  CLUTTER_ACTOR_CLASS (tidy_viewport_parent_class)->
    allocate (self, box, flags);

  /* Refresh adjustments */
  if (priv->sync_adjustments)
    {
      if (priv->hadjustment)
        {
          g_object_set (G_OBJECT (priv->hadjustment),
                       "lower", 0.0,
                       "upper", (box->x2 - box->x1),
                       NULL);

          /* Make sure value is clamped */
          prev_value = tidy_adjustment_get_value (priv->hadjustment);
          tidy_adjustment_set_value (priv->hadjustment, prev_value);
        }

      if (priv->vadjustment)
        {
          g_object_set (G_OBJECT (priv->vadjustment),
                       "lower", 0.0,
                       "upper", (box->y2 - box->y1),
                       NULL);

          prev_value = tidy_adjustment_get_value (priv->vadjustment);
          tidy_adjustment_set_value (priv->vadjustment, prev_value);
        }
    }
}

static void
tidy_viewport_class_init (TidyViewportClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

  g_type_class_add_private (klass, sizeof (TidyViewportPrivate));

  gobject_class->get_property = tidy_viewport_get_property;
  gobject_class->set_property = tidy_viewport_set_property;
  gobject_class->dispose = tidy_viewport_dispose;

  actor_class->paint = tidy_viewport_paint;
  actor_class->pick = tidy_viewport_pick;
  actor_class->allocate = tidy_viewport_allocate;

  g_object_class_install_property (gobject_class,
                                   PROP_X_ORIGIN,
                                   g_param_spec_int ("x-origin",
                                                     "X Origin",
                                                     "Origin's X coordinate in pixels",
                                                     -G_MAXINT, G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_Y_ORIGIN,
                                   g_param_spec_int ("y-origin",
                                                     "Y Origin",
                                                     "Origin's Y coordinate in pixels",
                                                     -G_MAXINT, G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_Z_ORIGIN,
                                   g_param_spec_int ("z-origin",
                                                     "Z Origin",
                                                     "Origin's Z coordinate in pixels",
                                                     -G_MAXINT, G_MAXINT,
                                                     0,
                                                     G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_SYNC_ADJUST,
                                   g_param_spec_boolean ("sync-adjustments",
                                                         "Synchronise "
                                                         "adjustments",
                                                         "Whether to "
                                                         "synchronise "
                                                         "adjustments with "
                                                         "viewport size",
                                                         TRUE,
                                                         G_PARAM_READWRITE));

  g_object_class_override_property (gobject_class,
                                    PROP_HADJUST,
                                    "hadjustment");

  g_object_class_override_property (gobject_class,
                                    PROP_VADJUST,
                                    "vadjustment");
}

static void
hadjustment_value_notify_cb (TidyAdjustment *adjustment,
                             GParamSpec     *pspec,
                             TidyViewport   *viewport)
{
  TidyViewportPrivate *priv = viewport->priv;
  gdouble value;

  value = tidy_adjustment_get_value (adjustment);

  tidy_viewport_set_origin (viewport,
                             value,
                             priv->y,
                             priv->z);
}

static void
vadjustment_value_notify_cb (TidyAdjustment *adjustment, GParamSpec *arg1,
                             TidyViewport *viewport)
{
  TidyViewportPrivate *priv = viewport->priv;
  gdouble value;

  value = tidy_adjustment_get_value (adjustment);

  tidy_viewport_set_origin (viewport,
                             priv->x,
                             value,
                             priv->z);
}

static void
scrollable_set_adjustments (TidyScrollable *scrollable,
                            TidyAdjustment *hadjustment,
                            TidyAdjustment *vadjustment)
{
  TidyViewportPrivate *priv = TIDY_VIEWPORT (scrollable)->priv;

  if (hadjustment != priv->hadjustment)
    {
      if (priv->hadjustment)
        {
          g_signal_handlers_disconnect_by_func (priv->hadjustment,
                                                hadjustment_value_notify_cb,
                                                scrollable);
          g_object_unref (priv->hadjustment);
        }

      if (hadjustment)
        {
          g_object_ref (hadjustment);
          g_signal_connect (hadjustment, "notify::value",
                            G_CALLBACK (hadjustment_value_notify_cb),
                            scrollable);
        }

      priv->hadjustment = hadjustment;
    }

  if (vadjustment != priv->vadjustment)
    {
      if (priv->vadjustment)
        {
          g_signal_handlers_disconnect_by_func (priv->vadjustment,
                                                vadjustment_value_notify_cb,
                                                scrollable);
          g_object_unref (priv->vadjustment);
        }

      if (vadjustment)
        {
          g_object_ref (vadjustment);
          g_signal_connect (vadjustment, "notify::value",
                            G_CALLBACK (vadjustment_value_notify_cb),
                            scrollable);
        }

      priv->vadjustment = vadjustment;
    }
}

static void
scrollable_get_adjustments (TidyScrollable *scrollable,
                            TidyAdjustment **hadjustment,
                            TidyAdjustment **vadjustment)
{
  TidyViewportPrivate *priv;

  g_return_if_fail (TIDY_IS_VIEWPORT (scrollable));

  priv = ((TidyViewport *)scrollable)->priv;

  if (hadjustment)
    {
      if (priv->hadjustment)
        *hadjustment = priv->hadjustment;
      else
        {
          TidyAdjustment *adjustment;
          guint width, stage_width, increment;

          width = clutter_actor_get_width (CLUTTER_ACTOR(scrollable));
          stage_width = clutter_actor_get_width (clutter_stage_get_default ());
          increment = MAX (1, MIN(stage_width, width));

          adjustment = tidy_adjustment_new (priv->x,
                                            0,
                                            width,
                                            1,
                                            increment,
                                            increment);
          scrollable_set_adjustments (scrollable,
                                      adjustment,
                                      priv->vadjustment);
          *hadjustment = adjustment;
        }
    }

  if (vadjustment)
    {
      if (priv->vadjustment)
        *vadjustment = priv->vadjustment;
      else
        {
          TidyAdjustment *adjustment;
          guint height, stage_height, increment;

          height = clutter_actor_get_height (CLUTTER_ACTOR(scrollable));
          stage_height = clutter_actor_get_height (clutter_stage_get_default ());
          increment = MAX (1, MIN(stage_height, height));

          adjustment = tidy_adjustment_new (priv->y,
                                            0,
                                            height,
                                            1,
                                            increment,
                                            increment);
          scrollable_set_adjustments (scrollable,
                                      priv->hadjustment,
                                      adjustment);
          *vadjustment = adjustment;
        }
    }
}

static void
scrollable_interface_init (TidyScrollableInterface *iface)
{
  iface->set_adjustments = scrollable_set_adjustments;
  iface->get_adjustments = scrollable_get_adjustments;
}

static void
clip_notify_cb (ClutterActor *actor,
                GParamSpec   *pspec,
                TidyViewport *self)
{
  gfloat width, height;
  TidyViewportPrivate *priv = self->priv;

  if (!priv->sync_adjustments)
    return;

  if (!clutter_actor_has_clip (actor))
    {
      if (priv->hadjustment)
        g_object_set (priv->hadjustment, "page-size", (gdouble)1.0, NULL);
      if (priv->vadjustment)
        g_object_set (priv->vadjustment, "page-size", (gdouble)1.0, NULL);
      return;
    }

  clutter_actor_get_clip (actor, NULL, NULL, &width, &height);

  if (priv->hadjustment)
    g_object_set (priv->hadjustment, "page-size", (gdouble)width, NULL);

  if (priv->vadjustment)
    g_object_set (priv->vadjustment, "page-size", (gdouble)height, NULL);
}

static void
tidy_viewport_init (TidyViewport *self)
{
  self->priv = VIEWPORT_PRIVATE (self);

  self->priv->sync_adjustments = TRUE;

  g_signal_connect (self, "notify::clip",
                    G_CALLBACK (clip_notify_cb), self);
}

ClutterActor *
tidy_viewport_new (void)
{
  return g_object_new (TIDY_TYPE_VIEWPORT, NULL);
}

void
tidy_viewport_set_origin (TidyViewport *viewport,
                          float x,
                          float y,
                          float z)
{
  TidyViewportPrivate *priv;

  g_return_if_fail (TIDY_IS_VIEWPORT (viewport));

  priv = viewport->priv;

  g_object_freeze_notify (G_OBJECT (viewport));

  if (x != priv->x)
    {
      priv->x = x;
      g_object_notify (G_OBJECT (viewport), "x-origin");

      if (priv->hadjustment)
        tidy_adjustment_set_value (priv->hadjustment,
                                    x);
    }

  if (y != priv->y)
    {
      priv->y = y;
      g_object_notify (G_OBJECT (viewport), "y-origin");

      if (priv->vadjustment)
        tidy_adjustment_set_value (priv->vadjustment,
                                    y);
    }

  if (z != priv->z)
    {
      priv->z = z;
      g_object_notify (G_OBJECT (viewport), "z-origin");
    }

  g_object_thaw_notify (G_OBJECT (viewport));

  clutter_actor_queue_redraw (CLUTTER_ACTOR (viewport));
}

void
tidy_viewport_get_origin (TidyViewport *viewport,
                          float *x,
                          float *y,
                          float *z)
{
  TidyViewportPrivate *priv;

  g_return_if_fail (TIDY_IS_VIEWPORT (viewport));

  priv = viewport->priv;

  if (x)
    *x = priv->x;

  if (y)
    *y = priv->y;

  if (z)
    *z = priv->z;
}
