/*
 * Copyright (C) 2010 Jiri Techet <techet@gmail.com>
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

#include <gtk/gtk.h>

/* include the libchamplain header */
#include <champlain-gtk/champlain-gtk.h>

#include <clutter-gtk/clutter-gtk.h>

int
main (int argc, char *argv[])
{
  GtkWidget *window, *widget;

  /* initialize threads and clutter */
  g_thread_init (NULL);
  gtk_clutter_init (&argc, &argv);

  /* create the top-level window and quit the main loop when it's closed */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit),
      NULL);

  /* create the libchamplain widget and set its size */
  widget = gtk_champlain_embed_new ();
  gtk_widget_set_size_request (widget, 640, 480);

  ChamplainView *view = gtk_champlain_embed_get_view (GTK_CHAMPLAIN_EMBED (widget));
  g_object_set (G_OBJECT (view), "scroll-mode", CHAMPLAIN_SCROLL_MODE_KINETIC,
          "zoom-level", 5, NULL);
  champlain_view_center_on(CHAMPLAIN_VIEW(view), 45.466, -73.75);

  /* insert it into the widget you wish */
  gtk_container_add (GTK_CONTAINER (window), widget);

  /* show everything */
  gtk_widget_show_all (window);

  /* start the main loop */
  gtk_main ();

  return 0;
}
