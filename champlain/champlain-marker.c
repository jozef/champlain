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
 * SECTION:champlain-marker
 * @short_description: A marker to identify points of interest on a map
 *
 * Markers reprensent points of interest on a map. Markers need to be placed on
 * a layer (a #ChamplainLayer).  Layers have to be added to a #ChamplainView for
 * the markers to show on the map.
 *
 * A marker is nothing more than a regular #ClutterActor.  You can draw on it
 * what ever you want. Set the markers position on the map
 * using #champlain_marker_set_position.
 *
 * Champlain has a default type of markers with text. To create one,
 * use #champlain_marker_new_with_text.
 */

#include "config.h"

#include "champlain-marker.h"

#include "champlain.h"
#include "champlain-base-marker.h"
#include "champlain-defines.h"
#include "champlain-marshal.h"
#include "champlain-private.h"
#include "champlain-tile.h"

#include <clutter/clutter.h>
#include <glib.h>
#include <glib-object.h>
#include <cairo.h>
#include <math.h>
#include <string.h>

#define DEFAULT_FONT_NAME "Sans 11"

static ClutterColor SELECTED_COLOR = {0x00, 0x33, 0xcc, 0xff};
static ClutterColor SELECTED_TEXT_COLOR = {0xff, 0xff, 0xff, 0xff};

static ClutterColor DEFAULT_COLOR = {0x33, 0x33, 0x33, 0xff};
static ClutterColor DEFAULT_TEXT_COLOR = {0xee, 0xee, 0xee, 0xff};

enum
{
  /* normal signals */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_TEXT,
  PROP_USE_MARKUP,
  PROP_ALIGNMENT,
  PROP_ATTRIBUTES,
  PROP_ELLIPSIZE,
  PROP_COLOR,
  PROP_TEXT_COLOR,
  PROP_FONT_NAME,
  PROP_WRAP,
  PROP_WRAP_MODE,
  PROP_SINGLE_LINE_MODE,
  PROP_DRAW_BACKGROUND
};

//static guint champlain_marker_signals[LAST_SIGNAL] = { 0, };

struct _ChamplainMarkerPrivate
{
  gchar *text;
  ClutterActor *image;
  gboolean use_markup;
  PangoAlignment alignment;
  PangoAttrList *attributes;
  ClutterColor *color;
  ClutterColor *text_color;
  gchar *font_name;
  gboolean wrap;
  PangoWrapMode wrap_mode;
  gboolean single_line_mode;
  PangoEllipsizeMode ellipsize;
  gboolean draw_background;

  ClutterActor *text_actor;
  ClutterActor *shadow;
  ClutterActor *background;
  guint redraw_id;
};

G_DEFINE_TYPE (ChamplainMarker, champlain_marker, CHAMPLAIN_TYPE_BASE_MARKER);

#define GET_PRIVATE(obj)    (G_TYPE_INSTANCE_GET_PRIVATE((obj), CHAMPLAIN_TYPE_MARKER, ChamplainMarkerPrivate))

static void draw_marker (ChamplainMarker *marker);

/**
 * champlain_marker_set_highlight_color:
 * @color: a #ClutterColor
 *
 * Changes the highlight color, this is to ensure a better integration with
 * the desktop, this is automatically done by GtkChamplainEmbed.
 *
 * Since: 0.4
 */
void
champlain_marker_set_highlight_color (ClutterColor *color)
{
  SELECTED_COLOR.red = color->red;
  SELECTED_COLOR.green = color->green;
  SELECTED_COLOR.blue = color->blue;
  SELECTED_COLOR.alpha = color->alpha;
}

/**
 * champlain_marker_get_highlight_color:
 *
 * Gets the highlight color.
 *
 * Returns: the highlight color. Should not be freed.
 *
 * Since: 0.4.1
 */
const ClutterColor *
champlain_marker_get_highlight_color ()
{
  return &SELECTED_COLOR;
}

/**
 * champlain_marker_set_highlight_text_color:
 * @color: a #ClutterColor
 *
 * Changes the highlight text color, this is to ensure a better integration with
 * the desktop, this is automatically done by GtkChamplainEmbed.
 *
 * Since: 0.4
 */
void
champlain_marker_set_highlight_text_color (ClutterColor *color)
{
  SELECTED_TEXT_COLOR.red = color->red;
  SELECTED_TEXT_COLOR.green = color->green;
  SELECTED_TEXT_COLOR.blue = color->blue;
  SELECTED_TEXT_COLOR.alpha = color->alpha;
}

/**
 * champlain_marker_get_highlight_text_color:
 *
 * Gets the highlight text color.
 *
 * Returns: the highlight text color. Should not be freed.
 *
 * Since: 0.4.1
 */
const ClutterColor *
champlain_marker_get_highlight_text_color ()
{
  return &SELECTED_TEXT_COLOR;
}

static void
champlain_marker_get_property (GObject *object,
    guint prop_id,
    GValue *value,
    GParamSpec *pspec)
{
    ChamplainMarkerPrivate *priv = CHAMPLAIN_MARKER (object)->priv;

    switch (prop_id)
      {
        case PROP_TEXT:
          g_value_set_string (value, priv->text);
          break;
        case PROP_IMAGE:
          g_value_set_object (value, priv->image);
          break;
        case PROP_USE_MARKUP:
          g_value_set_boolean (value, priv->use_markup);
          break;
        case PROP_ALIGNMENT:
          g_value_set_enum (value, priv->alignment);
          break;
        case PROP_COLOR:
          clutter_value_set_color (value, priv->color);
          break;
        case PROP_TEXT_COLOR:
          clutter_value_set_color (value, priv->text_color);
          break;
        case PROP_FONT_NAME:
          g_value_set_string (value, priv->font_name);
          break;
        case PROP_WRAP:
          g_value_set_boolean (value, priv->wrap);
          break;
        case PROP_WRAP_MODE:
          g_value_set_enum (value, priv->wrap_mode);
          break;
        case PROP_DRAW_BACKGROUND:
          g_value_set_boolean (value, priv->draw_background);
          break;
        case PROP_ELLIPSIZE:
          g_value_set_enum (value, priv->ellipsize);
          break;
        case PROP_SINGLE_LINE_MODE:
          g_value_set_enum (value, priv->single_line_mode);
          break;
        default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      }
}

static void
champlain_marker_set_property (GObject *object,
    guint prop_id,
    const GValue *value,
    GParamSpec *pspec)
{
    ChamplainMarker *marker = CHAMPLAIN_MARKER (object);

    switch (prop_id)
    {
      case PROP_TEXT:
        champlain_marker_set_text (marker, g_value_get_string (value));
        break;
      case PROP_IMAGE:
        champlain_marker_set_image (marker, g_value_get_object (value));
        break;
      case PROP_USE_MARKUP:
        champlain_marker_set_use_markup (marker, g_value_get_boolean (value));
        break;
      case PROP_ALIGNMENT:
        champlain_marker_set_alignment (marker, g_value_get_enum (value));
        break;
      case PROP_COLOR:
        champlain_marker_set_color (marker, clutter_value_get_color (value));
        break;
      case PROP_TEXT_COLOR:
        champlain_marker_set_text_color (marker, clutter_value_get_color (value));
        break;
      case PROP_FONT_NAME:
        champlain_marker_set_font_name (marker, g_value_get_string (value));
        break;
      case PROP_WRAP:
        champlain_marker_set_wrap (marker, g_value_get_boolean (value));
        break;
      case PROP_WRAP_MODE:
        champlain_marker_set_wrap_mode (marker, g_value_get_enum (value));
        break;
      case PROP_ELLIPSIZE:
        champlain_marker_set_ellipsize (marker, g_value_get_enum (value));
        break;
      case PROP_DRAW_BACKGROUND:
        champlain_marker_set_draw_background (marker, g_value_get_boolean (value));
        break;
      case PROP_SINGLE_LINE_MODE:
        champlain_marker_set_single_line_mode (marker, g_value_get_boolean (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
champlain_marker_dispose (GObject *object)
{
  ChamplainMarkerPrivate *priv = CHAMPLAIN_MARKER (object)->priv;

  if (priv->background)
    {
      g_object_unref (priv->background);
      priv->background = NULL;
    }

  if (priv->shadow)
    {
      g_object_unref (priv->shadow);
      priv->shadow = NULL;
    }

  if (priv->text_actor)
    {
      g_object_unref (priv->text_actor);
      priv->text_actor = NULL;
    }

  if (priv->image)
    {
      g_object_unref (priv->image);
      priv->image = NULL;
    }

  if (priv->attributes)
    {
      pango_attr_list_unref (priv->attributes);
      priv->attributes = NULL;
    }

  G_OBJECT_CLASS (champlain_marker_parent_class)->dispose (object);
}

static void
champlain_marker_finalize (GObject *object)
{
  ChamplainMarkerPrivate *priv = CHAMPLAIN_MARKER (object)->priv;

  if (priv->text)
    {
      g_free (priv->text);
      priv->text = NULL;
    }

  if (priv->font_name)
    {
      g_free (priv->font_name);
      priv->font_name = NULL;
    }

  if (priv->color)
    {
      clutter_color_free (priv->color);
      priv->color = NULL;
    }

  if (priv->text_color)
    {
      clutter_color_free (priv->text_color);
      priv->text_color = NULL;
    }

  if (priv->redraw_id)
    {
      g_source_remove (priv->redraw_id);
      priv->redraw_id = 0;
    }

  G_OBJECT_CLASS (champlain_marker_parent_class)->finalize (object);
}

static void
champlain_marker_class_init (ChamplainMarkerClass *markerClass)
{
  g_type_class_add_private (markerClass, sizeof (ChamplainMarkerPrivate));

  GObjectClass *object_class = G_OBJECT_CLASS (markerClass);
  object_class->finalize = champlain_marker_finalize;
  object_class->dispose = champlain_marker_dispose;
  object_class->get_property = champlain_marker_get_property;
  object_class->set_property = champlain_marker_set_property;

  markerClass->draw_marker = draw_marker;
  /**
  * ChamplainMarker:text:
  *
  * The text of the marker
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_TEXT,
      g_param_spec_string ("text", "Text", "The text of the marker",
          "", CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:image:
  *
  * The image of the marker
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_IMAGE,
      g_param_spec_object ("image", "Image", "The image of the marker",
          CLUTTER_TYPE_ACTOR, CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:use-markup:
  *
  * If the marker's text uses markup
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_USE_MARKUP,
      g_param_spec_boolean ("use-markup", "Use Markup", "The text uses markup",
          FALSE, CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:alignment:
  *
  * The marker's alignment
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_ALIGNMENT,
      g_param_spec_enum ("alignment", "Alignment", "The marker's alignment",
          PANGO_TYPE_ALIGNMENT, PANGO_ALIGN_LEFT, CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:color:
  *
  * The marker's color
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_COLOR,
      clutter_param_spec_color ("color", "Color", "The marker's color",
          &DEFAULT_COLOR, CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:text-color:
  *
  * The marker's text color
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_TEXT_COLOR,
      clutter_param_spec_color ("text-color", "Text Color", "The marker's text color",
          &DEFAULT_TEXT_COLOR, CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:font-name:
  *
  * The marker's text font name
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_FONT_NAME,
      g_param_spec_string ("font-name", "Font Name", "The marker's text font name",
          "Sans 11", CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:wrap:
  *
  * If the marker's text wrap is set
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_WRAP,
      g_param_spec_boolean ("wrap", "Wrap", "The marker's text wrap",
          FALSE, CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:wrap-mode:
  *
  * The marker's text wrap mode
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_WRAP_MODE,
      g_param_spec_enum ("wrap-mode", "Wrap Mode", "The marker's text wrap mode",
          PANGO_TYPE_WRAP_MODE, PANGO_WRAP_WORD, CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:ellipsize:
  *
  * The marker's ellipsize mode
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_ELLIPSIZE,
      g_param_spec_enum ("ellipsize", "Ellipsize Mode", "The marker's text ellipsize mode",
          PANGO_TYPE_ELLIPSIZE_MODE, PANGO_ELLIPSIZE_NONE, CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:draw-background:
  *
  * If the marker has a background
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_DRAW_BACKGROUND,
      g_param_spec_boolean ("draw-background", "Draw Background", "The marker has a background",
          TRUE, CHAMPLAIN_PARAM_READWRITE));

  /**
  * ChamplainMarker:single-line-mode:
  *
  * If the marker is in single line mode
  *
  * Since: 0.4
  */
  g_object_class_install_property (object_class, PROP_SINGLE_LINE_MODE,
      g_param_spec_boolean ("single-line-mode", "Single Line Mode", "The marker's single line mode",
          TRUE, CHAMPLAIN_PARAM_READWRITE));

}

#define RADIUS 10
#define PADDING (RADIUS / 2)

static void
draw_box (cairo_t *cr,
   gint width,
   gint height,
   gint point,
   gboolean mirror)
{
  if (mirror)
    {
      cairo_move_to (cr, RADIUS, 0);
      cairo_line_to (cr, width - RADIUS, 0);
      cairo_arc (cr, width - RADIUS, RADIUS, RADIUS - 1, 3 * M_PI / 2.0, 0);
      cairo_line_to (cr, width, height - RADIUS);
      cairo_arc (cr, width - RADIUS, height - RADIUS, RADIUS - 1, 0, M_PI / 2.0);
      cairo_line_to (cr, point, height);
      cairo_line_to (cr, 0, height + point);
      cairo_arc (cr, RADIUS, RADIUS, RADIUS - 1, M_PI, 3 * M_PI / 2.0);
      cairo_close_path (cr);
    }
  else
    {
      cairo_move_to (cr, RADIUS, 0);
      cairo_line_to (cr, width - RADIUS, 0);
      cairo_arc (cr, width - RADIUS, RADIUS, RADIUS - 1, 3 * M_PI / 2.0, 0);
      cairo_line_to (cr, width, height + point);
      cairo_line_to (cr, width - point, height);
      cairo_line_to (cr, RADIUS, height);
      cairo_arc (cr, RADIUS, height - RADIUS, RADIUS - 1, M_PI / 2.0, M_PI);
      cairo_line_to (cr, 0, RADIUS);
      cairo_arc (cr, RADIUS, RADIUS, RADIUS - 1, M_PI, 3 * M_PI / 2.0);
      cairo_close_path (cr);
    }

}

static void
draw_shadow (ChamplainMarker *marker,
   gint width,
   gint height,
   gint point)
{
  ChamplainMarkerPrivate *priv = marker->priv;
  ClutterActor *shadow = NULL;
  cairo_t *cr;
  gdouble slope;
  gdouble scaling;
  gint x;
  cairo_matrix_t matrix;

  slope = -0.3;
  scaling = 0.65;
  if (priv->alignment == PANGO_ALIGN_LEFT)
    x = -40 * slope;
  else
    x = -58 * slope;

  shadow = clutter_cairo_texture_new (width + x, (height + point));
  cr = clutter_cairo_texture_create (CLUTTER_CAIRO_TEXTURE (shadow));

  cairo_matrix_init (&matrix,
      1, 0,
      slope, scaling,
      x, 0);
  cairo_set_matrix (cr, &matrix);

  draw_box (cr, width, height, point, priv->alignment == PANGO_ALIGN_LEFT);

  cairo_set_source_rgba (cr, 0, 0, 0, 0.15);
  cairo_fill (cr);

  cairo_destroy (cr);

  clutter_actor_set_position (shadow, 0, height / 2.0 );

  clutter_container_add_actor (CLUTTER_CONTAINER (marker), shadow);

  if (priv->shadow != NULL)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (marker),
          priv->shadow);
      g_object_unref (priv->shadow);
    }

  priv->shadow = g_object_ref (shadow);
}


static void
draw_background (ChamplainMarker *marker,
    gint width,
    gint height,
    gint point)
{
  ChamplainMarkerPrivate *priv = marker->priv;
  ChamplainBaseMarker *base_marker = CHAMPLAIN_BASE_MARKER (marker);
  ClutterActor *bg = NULL;
  ClutterColor *color;
  ClutterColor darker_color;
  cairo_t *cr;

  bg = clutter_cairo_texture_new (width, height + point);
  cr = clutter_cairo_texture_create (CLUTTER_CAIRO_TEXTURE (bg));

  /* If selected, add the selection color to the marker's color */
  if (champlain_base_marker_get_highlighted (base_marker))
    color = &SELECTED_COLOR;
  else
    color = priv->color;


  draw_box (cr, width, height, point, priv->alignment == PANGO_ALIGN_LEFT);

  clutter_color_darken (color, &darker_color);

  cairo_set_source_rgba (cr,
      color->red / 255.0,
      color->green / 255.0,
      color->blue / 255.0,
      color->alpha / 255.0);
  cairo_fill_preserve (cr);

  cairo_set_line_width (cr, 1.0);
  cairo_set_source_rgba (cr,
      darker_color.red / 255.0,
      darker_color.green / 255.0,
      darker_color.blue / 255.0,
      darker_color.alpha / 255.0);
  cairo_stroke (cr);
  cairo_destroy (cr);

  clutter_container_add_actor (CLUTTER_CONTAINER (marker), bg);

  if (priv->background != NULL)
    {
      clutter_container_remove_actor (CLUTTER_CONTAINER (marker),
          priv->background);
      g_object_unref (priv->background);
    }

  priv->background = g_object_ref (bg);
}

static void
draw_marker (ChamplainMarker *marker)
{
  ChamplainMarkerPrivate *priv = marker->priv;
  ChamplainBaseMarker *base_marker = CHAMPLAIN_BASE_MARKER (marker);
  guint height = 0, point = 0;
  guint total_width = 0, total_height = 0;

  if (priv->image != NULL)
    {
      clutter_actor_set_position (priv->image, PADDING, PADDING);
      total_width = clutter_actor_get_width (priv->image) + 2 * PADDING;
      total_height = clutter_actor_get_height (priv->image) + 2 * PADDING;
      if (clutter_actor_get_parent (priv->image) == NULL)
        clutter_container_add_actor (CLUTTER_CONTAINER (marker), priv->image);
    }

  if (priv->text != NULL && strlen (priv->text) > 0)
    {
      ClutterText *label;
      if (priv->text_actor == NULL)
        {
          priv->text_actor = clutter_text_new_with_text (priv->font_name, priv->text);
          g_object_ref (priv->text_actor);
        }

      label = CLUTTER_TEXT (priv->text_actor);
      clutter_text_set_font_name (label, priv->font_name);
      clutter_text_set_text (label, priv->text);
      clutter_text_set_line_alignment (label, priv->alignment);
      clutter_text_set_line_wrap (label, priv->wrap);
      clutter_text_set_line_wrap_mode (label, priv->wrap_mode);
      clutter_text_set_ellipsize (label, priv->ellipsize);
      clutter_text_set_attributes (label, priv->attributes);
      clutter_text_set_use_markup (label, priv->use_markup);

      height = clutter_actor_get_height (priv->text_actor);
      if (priv->image != NULL)
        {
          clutter_actor_set_position (priv->text_actor, total_width, (total_height - height) / 2.0);
          total_width += clutter_actor_get_width (priv->text_actor) + 2 * PADDING;
        }
      else
        {
          clutter_actor_set_position (priv->text_actor, 2 * PADDING, PADDING);
          total_width += clutter_actor_get_width (priv->text_actor) + 4 * PADDING;
        }

      height += 2 * PADDING;
      if (height > total_height)
        total_height = height;

      clutter_text_set_color (CLUTTER_TEXT (priv->text_actor),
          (champlain_base_marker_get_highlighted (base_marker) ? &SELECTED_TEXT_COLOR : priv->text_color));
      if (clutter_actor_get_parent (priv->text_actor) == NULL)
        clutter_container_add_actor (CLUTTER_CONTAINER (marker), priv->text_actor);
    }

  if (priv->text_actor == NULL && priv->image == NULL)
    {
      total_width = 6 * PADDING;
      total_height = 6 * PADDING;
    }

  point = (total_height + 2 * PADDING) / 4.0;

  if (priv->draw_background)
    {
      draw_shadow (marker, total_width, total_height, point);
      draw_background (marker, total_width, total_height, point);
    }
  else
  {
    if (priv->background != NULL)
      {
        clutter_container_remove_actor (CLUTTER_CONTAINER (marker), priv->background);
        g_object_unref (G_OBJECT (priv->background));
        priv->background = NULL;
      }

    if (priv->shadow != NULL)
      {
        clutter_container_remove_actor (CLUTTER_CONTAINER (marker), priv->shadow);
        g_object_unref (G_OBJECT (priv->shadow));
        priv->shadow = NULL;
      }
  }

  if (priv->text_actor != NULL && priv->background != NULL)
    clutter_actor_raise (priv->text_actor, priv->background);
  if (priv->image != NULL && priv->background != NULL)
    clutter_actor_raise (priv->image, priv->background);

  if (priv->draw_background)
  {
    if (priv->alignment == PANGO_ALIGN_RIGHT)
      clutter_actor_set_anchor_point (CLUTTER_ACTOR (marker), total_width, total_height + point);
    else
      clutter_actor_set_anchor_point (CLUTTER_ACTOR (marker), 0, total_height + point);
  }
  else if (priv->image != NULL)
    clutter_actor_set_anchor_point (CLUTTER_ACTOR (marker),
        clutter_actor_get_width (priv->image) / 2.0 + PADDING,
        clutter_actor_get_height (priv->image) / 2.0 + PADDING);
  else if (priv->text_actor != NULL)
    clutter_actor_set_anchor_point (CLUTTER_ACTOR (marker),
        0,
        clutter_actor_get_height (priv->text_actor) / 2.0);
}

static gboolean
redraw_on_idle (gpointer gobject)
{
  ChamplainMarker *marker = CHAMPLAIN_MARKER (gobject);
  CHAMPLAIN_MARKER_GET_CLASS (gobject)->draw_marker (marker);
  marker->priv->redraw_id = 0;
  return FALSE;
}

/**
 * champlain_marker_queue_redraw:
 * @marker: a #ChamplainMarker
 *
 * Queue a redraw of the marker as soon as possible. This function should not
 * be used unless you are subclassing ChamplainMarker and adding new properties
 * that affect the aspect of the marker.  When they change, call this function
 * to update the marker.
 *
 * Since: 0.4.3
 */
void
champlain_marker_queue_redraw (ChamplainMarker *marker)
{
  ChamplainMarkerPrivate *priv = marker->priv;

  if (!priv->redraw_id)
    priv->redraw_id = g_idle_add (redraw_on_idle, marker);
}

static void
notify_highlighted (GObject *gobject,
    GParamSpec *pspec,
    gpointer user_data)
{
  champlain_marker_queue_redraw (CHAMPLAIN_MARKER (gobject));
}

static void
champlain_marker_init (ChamplainMarker *marker)
{
  ChamplainMarkerPrivate *priv = GET_PRIVATE (marker);

  marker->priv = priv;

  priv->text = NULL;
  priv->image = NULL;
  priv->background = NULL;
  priv->use_markup = FALSE;
  priv->alignment = PANGO_ALIGN_LEFT;
  priv->attributes = NULL;
  priv->color = clutter_color_copy (&DEFAULT_COLOR);
  priv->text_color = clutter_color_copy (&DEFAULT_TEXT_COLOR);
  priv->font_name = g_strdup (DEFAULT_FONT_NAME);
  priv->wrap = FALSE;
  priv->wrap_mode = PANGO_WRAP_WORD;
  priv->single_line_mode = TRUE;
  priv->ellipsize = PANGO_ELLIPSIZE_NONE;
  priv->draw_background = TRUE;
  priv->redraw_id = 0;

  g_signal_connect (marker, "notify::highlighted", G_CALLBACK (notify_highlighted), NULL);
}

/**
 * champlain_marker_new:
 *
 * Creates a new instance of #ChamplainMarker.
 *
 * Returns: a new #ChamplainMarker ready to be used as a #ClutterActor.
 *
 * Since: 0.2
 */
ClutterActor *
champlain_marker_new (void)
{
  return CLUTTER_ACTOR (g_object_new (CHAMPLAIN_TYPE_MARKER, NULL));
}

/**
 * champlain_marker_new_with_text:
 * @text: the text of the text
 * @font: the font to use to draw the text, for example "Courrier Bold 11", can be NULL
 * @text_color: a #ClutterColor, the color of the text, can be NULL
 * @marker_color: a #ClutterColor, the color of the marker, can be NULL
 *
 * Creates a new instance of #ChamplainMarker with text value.
 *
 * Returns: a new #ChamplainMarker with a drawn marker containing the given text.
 *
 * Since: 0.2
 */
ClutterActor *
champlain_marker_new_with_text (const gchar *text,
    const gchar *font,
    ClutterColor *text_color,
    ClutterColor *marker_color)
{
  ChamplainMarker *marker = CHAMPLAIN_MARKER (champlain_marker_new ());

  champlain_marker_set_text (marker, text);

  if (font != NULL)
    champlain_marker_set_font_name (marker, font);

  if (text_color != NULL)
    champlain_marker_set_text_color (marker, text_color);

  if (marker_color != NULL)
    champlain_marker_set_color (marker, marker_color);

  return CLUTTER_ACTOR (marker);
}

/**
 * champlain_marker_new_with_image:
 * @actor: The actor of the image.
 *
 * Creates a new instance of #ChamplainMarker with image.
 *
 * Returns: a new #ChamplainMarker with a drawn marker containing the given
 * image.
 *
 * Since: 0.4
 */
ClutterActor *
champlain_marker_new_with_image (ClutterActor *actor)
{
  ChamplainMarker *marker = CHAMPLAIN_MARKER (champlain_marker_new ());
  if (actor != NULL)
    {
      champlain_marker_set_image (marker, actor);
    }

  return CLUTTER_ACTOR (marker);
}

/**
 * champlain_marker_new_from_file:
 * @filename: The filename of the image.
 * @error: Return location for an error.
 *
 * Creates a new instance of #ChamplainMarker with image loaded from file.
 *
 * Returns: a new #ChamplainMarker with a drawn marker containing the given
 * image.
 *
 * Since: 0.4
 */
ClutterActor *
champlain_marker_new_from_file (const gchar *filename,
    GError **error)
{
  if (filename == NULL)
    return NULL;

  ChamplainMarker *marker = CHAMPLAIN_MARKER (champlain_marker_new ());
  ClutterActor *actor = clutter_texture_new_from_file (filename, error);

  if (actor != NULL)
    {
      champlain_marker_set_image (marker, actor);
    }

  return CLUTTER_ACTOR (marker);
}

/**
 * champlain_marker_new_full:
 * @text: The text
 * @actor: The image
 *
 * Creates a new instance of #ChamplainMarker consisting of a custom #ClutterActor.
 *
 * Returns: a new #ChamplainMarker with a drawn marker containing the given
 * image.
 *
 * Since: 0.4
 */
ClutterActor *
champlain_marker_new_full (const gchar *text,
    ClutterActor *actor)
{
  ChamplainMarker *marker = CHAMPLAIN_MARKER (champlain_marker_new ());
  if (actor != NULL)
    {
      champlain_marker_set_image (marker, actor);
    }
  champlain_marker_set_text (marker, text);

  return CLUTTER_ACTOR (marker);
}

/**
 * champlain_marker_set_text:
 * @marker: The marker
 * @text: The text
 *
 * Sets the marker's text.
 *
 * Since: 0.4
 */
void
champlain_marker_set_text (ChamplainMarker *marker,
    const gchar *text)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  ChamplainMarkerPrivate *priv = marker->priv;

  if (priv->text != NULL)
    g_free (priv->text);

  priv->text = g_strdup (text);
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_image:
 * @marker: The marker.
 * @image: The image as a @ClutterActor or NULL to remove the current image.
 *
 * Sets the marker's image.
 *
 * Since: 0.4
 */
void
champlain_marker_set_image (ChamplainMarker *marker,
    ClutterActor *image)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  ChamplainMarkerPrivate *priv = marker->priv;

  if (priv->image != NULL)
    {
      g_object_unref (priv->image);
    }

  if (image != NULL)
    {
      g_return_if_fail (CLUTTER_IS_ACTOR (image));
      priv->image = g_object_ref (image);
    }
  else
    priv->image = image;

  g_object_notify (G_OBJECT (marker), "image");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_use_markup:
 * @marker: The marker
 * @use_markup: The value
 *
 * Sets if the marker's text uses markup.
 *
 * Since: 0.4
 */
void
champlain_marker_set_use_markup (ChamplainMarker *marker,
    gboolean markup)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  marker->priv->use_markup = markup;
  g_object_notify (G_OBJECT (marker), "use-markup");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_alignment:
 * @marker: The marker
 * @alignment: The marker's alignment
 *
 * Set the marker's text alignment.
 *
 * Since: 0.4
 */
void
champlain_marker_set_alignment (ChamplainMarker *marker,
    PangoAlignment alignment)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  marker->priv->alignment = alignment;
  g_object_notify (G_OBJECT (marker), "alignment");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_color:
 * @marker: The marker
 * @color: The marker's background color or NULL to reset the background to the
 *         default color. The color parameter is copied.
 *
 * Set the marker's background color.
 *
 * Since: 0.4
 */
void
champlain_marker_set_color (ChamplainMarker *marker,
    const ClutterColor *color)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  ChamplainMarkerPrivate *priv = marker->priv;

  if (priv->color != NULL)
    clutter_color_free (priv->color);

  if (color == NULL)
    color = &DEFAULT_COLOR;

  priv->color = clutter_color_copy (color);
  g_object_notify (G_OBJECT (marker), "color");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_text_color:
 * @marker: The marker
 * @color: The marker's text color or NULL to reset the text to the default
 *         color. The color parameter is copied.
 *
 * Set the marker's text color.
 *
 * Since: 0.4
 */
void
champlain_marker_set_text_color (ChamplainMarker *marker,
    const ClutterColor *color)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  ChamplainMarkerPrivate *priv = marker->priv;

  if (priv->text_color != NULL)
    clutter_color_free (priv->text_color);

  if (color == NULL)
    color = &DEFAULT_TEXT_COLOR;

  priv->text_color = clutter_color_copy (color);
  g_object_notify (G_OBJECT (marker), "text-color");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_font_name:
 * @marker: The marker
 * @font_name: The marker's font name or NULL to reset the font to the default
 *             value.
 *
 * Set the marker's font name such as "Sans 12".
 *
 * Since: 0.4
 */
void
champlain_marker_set_font_name (ChamplainMarker *marker,
    const gchar *font_name)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  ChamplainMarkerPrivate *priv = marker->priv;

  if (priv->font_name != NULL)
    g_free (priv->font_name);

  if (font_name == NULL)
    font_name = DEFAULT_FONT_NAME;

  priv->font_name = g_strdup (font_name);
  g_object_notify (G_OBJECT (marker), "font-name");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_wrap:
 * @marker: The marker
 * @wrap: The marker's wrap.
 *
 * Set if the marker's text wrap.
 *
 * Since: 0.4
 */
void
champlain_marker_set_wrap (ChamplainMarker *marker,
    gboolean wrap)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  marker->priv->wrap = wrap;
  g_object_notify (G_OBJECT (marker), "wrap");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_wrap_mode:
 * @marker: The marker
 * @wrap_mode: The marker's wrap.
 *
 * Set the marker's text color.
 *
 * Since: 0.4
 */
void
champlain_marker_set_wrap_mode (ChamplainMarker *marker,
    PangoWrapMode wrap_mode)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  marker->priv->wrap_mode = wrap_mode;
  g_object_notify (G_OBJECT (marker), "wrap");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_attributes:
 * @marker: The marker
 * @list: The marker's text attributes.
 *
 * Set the marker's text attribute.
 *
 * Since: 0.4
 */
void
champlain_marker_set_attributes (ChamplainMarker *marker,
    PangoAttrList *attributes)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  ChamplainMarkerPrivate *priv = marker->priv;

  if (attributes)
    pango_attr_list_ref (attributes);

  if (priv->attributes)
    pango_attr_list_unref (priv->attributes);

  priv->attributes = attributes;

  g_object_notify (G_OBJECT (marker), "attributes");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_ellipsize:
 * @marker: The marker
 * @mode: The marker's ellipsize mode.
 *
 * Set the marker's text ellipsize mode.
 *
 * Since: 0.4
 */
void
champlain_marker_set_ellipsize (ChamplainMarker *marker,
    PangoEllipsizeMode ellipsize)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  marker->priv->ellipsize = ellipsize;
  g_object_notify (G_OBJECT (marker), "ellipsize");
  champlain_marker_queue_redraw (marker);
}
/**
 * champlain_marker_set_single_line_mode:
 * @marker: The marker
 * @mode: The marker's single line mode
 *
 * Set if the marker's text is on a single line.
 *
 * Since: 0.4
 */
void
champlain_marker_set_single_line_mode (ChamplainMarker *marker,
    gboolean mode)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  marker->priv->single_line_mode = mode;

  g_object_notify (G_OBJECT (marker), "single-line-mode");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_set_draw_background:
 * @marker: The marker
 * @background: value.
 *
 * Set if the marker has a background.
 *
 * Since: 0.4
 */
void
champlain_marker_set_draw_background (ChamplainMarker *marker,
    gboolean background)
{
  g_return_if_fail (CHAMPLAIN_IS_MARKER (marker));

  marker->priv->draw_background = background;
  g_object_notify (G_OBJECT (marker), "draw-background");
  champlain_marker_queue_redraw (marker);
}

/**
 * champlain_marker_get_image:
 * @marker: The marker
 *
 * Get the marker's image.
 *
 * Returns: the marker's image.
 *
 * Since: 0.4
 */
ClutterActor *
champlain_marker_get_image (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), NULL);

  return marker->priv->image;
}

/**
 * champlain_marker_get_use_markup:
 * @marker: The marker
 *
 * Check whether the marker uses markup.
 *
 * Returns: if the marker's text contains markup.
 *
 * Since: 0.4
 */
gboolean
champlain_marker_get_use_markup (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), FALSE);

  return marker->priv->use_markup;
}

/**
 * champlain_marker_get_text:
 * @marker: The marker
 *
 * Get the marker's text.
 *
 * Returns: the marker's text.
 *
 * Since: 0.4
 */
const gchar *
champlain_marker_get_text (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), FALSE);

  return marker->priv->text;
}

/**
 * champlain_marker_get_alignment:
 * @marker: The marker
 *
 * Get the marker's text alignment.
 *
 * Returns: the marker's text alignment.
 *
 * Since: 0.4
 */
PangoAlignment
champlain_marker_get_alignment (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), FALSE);

  return marker->priv->alignment;
}

/**
 * champlain_marker_get_color:
 * @marker: The marker
 *
 * Gets the marker's color.
 *
 * Returns: the marker's color.
 *
 * Since: 0.4
 */
ClutterColor *
champlain_marker_get_color (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), NULL);

  return marker->priv->color;
}

/**
 * champlain_marker_get_text_color:
 * @marker: The marker
 *
 * Gets the marker's text color.
 *
 * Returns: the marker's text color.
 *
 * Since: 0.4
 */
ClutterColor *
champlain_marker_get_text_color (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), NULL);

  return marker->priv->text_color;
}

/**
 * champlain_marker_get_font_name:
 * @marker: The marker
 *
 * Gets the marker's font name.
 *
 * Returns: the marker's font name.
 *
 * Since: 0.4
 */
const gchar *
champlain_marker_get_font_name (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), FALSE);

  return marker->priv->font_name;
}

/**
 * champlain_marker_get_wrap:
 * @marker: The marker
 *
 * Check whether the marker text wraps.
 *
 * Returns: if the marker's text wraps.
 *
 * Since: 0.4
 */
gboolean
champlain_marker_get_wrap (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), FALSE);

  return marker->priv->wrap;
}

/**
 * champlain_marker_get_wrap_mode:
 * @marker: The marker
 *
 * Get the marker's text wrap mode.
 *
 * Returns: the marker's text wrap mode.
 *
 * Since: 0.4
 */
PangoWrapMode
champlain_marker_get_wrap_mode (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), FALSE);

  return marker->priv->wrap_mode;
}

/**
 * champlain_marker_get_ellipsize:
 * @marker: The marker
 *
 * Get the marker's text ellipsize mode.
 *
 * Returns: the marker's text ellipsize mode.
 *
 * Since: 0.4
 */
PangoEllipsizeMode
champlain_marker_get_ellipsize (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), FALSE);

  return marker->priv->ellipsize;
}

/**
 * champlain_marker_get_single_line_mode:
 * @marker: The marker
 *
 * Checks the marker's single line mode.
 *
 * Returns: the marker's text single line mode.
 *
 * Since: 0.4
 */
gboolean
champlain_marker_get_single_line_mode (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), FALSE);

  return marker->priv->single_line_mode;
}

/**
 * champlain_marker_get_draw_background:
 * @marker: The marker
 *
 * Checks whether the marker has a background.
 *
 * Returns: if the marker's has a background.
 *
 * Since: 0.4
 */
gboolean
champlain_marker_get_draw_background (ChamplainMarker *marker)
{
  g_return_val_if_fail (CHAMPLAIN_IS_MARKER (marker), FALSE);

  return marker->priv->draw_background;
}

