/*
 * Copyright (C) 2009 Emmanuel Rodriguez <emmanuel.rodriguez@gmail.com>
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

#include <champlain/champlain.h>
#include <libsoup/soup.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

/* The data needed for constructing a marker */
typedef struct {
  ChamplainLayer *layer;
  gdouble latitude;
  gdouble longitude;
} MarkerData;

/**
 * Returns a GdkPixbuf from a given SoupMessage. This function assumes that the
 * message has completed successfully.
 * If there's an error building the GdkPixbuf the function will return NULL and
 * set error accordingly.
 *
 * The GdkPixbuf has to be freed with g_object_unref.
 */
static GdkPixbuf*
pixbuf_new_from_message (SoupMessage *message,
    GError **error)
{
  const gchar *mime_type = NULL;
  GdkPixbufLoader *loader = NULL;
  GdkPixbuf *pixbuf = NULL;
  gboolean pixbuf_is_open = FALSE;

  *error = NULL;

  /*  Use a pixbuf loader that can load images of the same mime-type as the
      message.
  */
  mime_type = soup_message_headers_get (message->response_headers,
      "Content-Type");
  loader = gdk_pixbuf_loader_new_with_mime_type (mime_type, error);
  if (loader != NULL)
    pixbuf_is_open = TRUE;
  if (*error != NULL)
    goto cleanup;


  gdk_pixbuf_loader_write (
      loader,
      (guchar *)message->response_body->data,
      message->response_body->length,
      error);
  if (*error != NULL)
    goto cleanup;

  gdk_pixbuf_loader_close (loader, error);
  pixbuf_is_open = FALSE;
  if (*error != NULL)
    goto cleanup;

  pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);
  if (pixbuf == NULL)
    goto cleanup;
  g_object_ref (G_OBJECT (pixbuf));

cleanup:
  if (pixbuf_is_open)
      gdk_pixbuf_loader_close (loader, NULL);

  if (loader != NULL)
      g_object_unref (G_OBJECT (loader));

  return pixbuf;
}

/**
 * Transforms a GdkPixbuf into a ClutterTexture.
 * If there's an error building the ClutterActor (the texture) the function
 * will return NULL and set error accordingly.
 *
 * If you are using ClutterGtk, you can also use gtk_clutter_texture_set_from_pixbuf
 * instead of cluter_texture_set_from_rgb_data.
 *
 * The ClutterActor has to be freed with clutter_actor_destroy.
 */
static ClutterActor*
texture_new_from_pixbuf (GdkPixbuf *pixbuf, GError **error)
{
  ClutterActor *texture = NULL;
  const guchar *data;
  gboolean has_alpha, success;
  int width, height, rowstride;
  ClutterTextureFlags flags = 0;

  *error = NULL;

  data = gdk_pixbuf_get_pixels (pixbuf);
  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);
  has_alpha = gdk_pixbuf_get_has_alpha (pixbuf);
  rowstride = gdk_pixbuf_get_rowstride (pixbuf);

  texture = clutter_texture_new ();
  success = clutter_texture_set_from_rgb_data (CLUTTER_TEXTURE (texture),
      data,
      has_alpha,
      width,
      height,
      rowstride,
      (has_alpha ? 4 : 3),
      flags,
      error);

  if (!success)
    {
      clutter_actor_destroy (CLUTTER_ACTOR (texture));
      texture = NULL;
    }

  return texture;
}

/**
 * Called when an image has been downloaded. This callback will transform the
 * image data (binary chunk sent by the remote web server) into a valid Clutter
 * actor (a texture) and will use this as the source image for a new marker.
 * The marker will then be added to an existing layer.
 *
 * This callback expects the parameter data to be a valid ChamplainLayer.
 */
static void
image_downloaded_cb (SoupSession *session,
    SoupMessage *message,
    gpointer data)
{
  MarkerData *marker_data = NULL;
  SoupURI *uri = NULL;
  char *url = NULL;
  GError *error = NULL;
  GdkPixbuf *pixbuf = NULL;
  ClutterActor *texture = NULL;
  ClutterActor *marker = NULL;

  if (data == NULL)
    goto cleanup;
  marker_data = (MarkerData *) data;

  /* Deal only with finished messages */
  uri = soup_message_get_uri (message);
  url = soup_uri_to_string (uri, FALSE);
  if (! SOUP_STATUS_IS_SUCCESSFUL (message->status_code))
    {
      g_print ("Download of %s failed with error code %d\n", url,
          message->status_code);
      goto cleanup;
    }

  pixbuf = pixbuf_new_from_message (message, &error);
  if (error != NULL)
    {
      g_print ("Failed to convert %s into an image: %s\n", url, error->message);
      goto cleanup;
    }

  /* Then transform the pixbuf into a texture */
  texture = texture_new_from_pixbuf (pixbuf, &error);
  if (error != NULL)
    {
      g_print ("Failed to convert %s into a texture: %s\n", url,
        error->message);
      goto cleanup;
    }

  /* Finally create a marker with the texture */
  marker = champlain_marker_new_with_image (texture);
  texture = NULL;
  champlain_base_marker_set_position (CHAMPLAIN_BASE_MARKER (marker),
      marker_data->latitude, marker_data->longitude);
  clutter_container_add (CLUTTER_CONTAINER (marker_data->layer), marker, NULL);
  clutter_actor_show_all (marker);

cleanup:
  if (marker_data)
    g_object_unref (marker_data->layer);
  g_free (marker_data);
  g_free (url);

  if (error != NULL)
    g_error_free (error);

  if (pixbuf != NULL)
    g_object_unref (G_OBJECT (pixbuf));

  if (texture != NULL)
    clutter_actor_destroy (CLUTTER_ACTOR (texture));
}

/**
 * Creates a marker at the given position with an image that's downloaded from
 * the given URL.
 *
 */
static void
create_marker_from_url (ChamplainLayer *layer,
    SoupSession *session,
    gdouble latitude,
    gdouble longitude,
    const gchar *url)
{
  SoupMessage *message;
  MarkerData *data;

  data = g_new0(MarkerData, 1);
  data->layer = g_object_ref (layer);
  data->latitude = latitude;
  data->longitude = longitude;

  message = soup_message_new ("GET", url);
  soup_session_queue_message (session, message, image_downloaded_cb, data);
}

int
main (int argc, char *argv[])
{
  ClutterActor *view, *stage;
  ChamplainLayer *layer;
  SoupSession *session;

  g_thread_init (NULL);
  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();
  clutter_actor_set_size (stage, 800, 600);

  /* Create the map view */
  view = champlain_view_new ();
  clutter_actor_set_size (CLUTTER_ACTOR (view), 800, 600);
  clutter_container_add_actor (CLUTTER_CONTAINER (stage), view);

  /* Create the markers and marker layer */
  layer = champlain_layer_new ();
  champlain_view_add_layer (CHAMPLAIN_VIEW (view), layer);
  session = soup_session_async_new ();
  create_marker_from_url (layer, session, 48.218611, 17.146397,
      "http://hexten.net/cpan-faces/potyl.jpg");
  create_marker_from_url (layer, session, 48.21066, 16.31476,
      "http://hexten.net/cpan-faces/jkutej.jpg");
  create_marker_from_url (layer, session, 48.14838, 17.10791,
      "http://bratislava.pm.org/images/whoiswho/jnthn.jpg");

  /* Finish initialising the map view */
  g_object_set (G_OBJECT (view), "zoom-level", 10,
      "scroll-mode", CHAMPLAIN_SCROLL_MODE_KINETIC, NULL);
  champlain_view_center_on (CHAMPLAIN_VIEW (view), 48.22, 16.8);

  clutter_actor_show_all (stage);
  clutter_main ();

  g_object_unref (session);

  clutter_actor_destroy (view);
  return 0;
}
