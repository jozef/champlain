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
 
#if !defined (__CHAMPLAIN_GTK_CHAMPLAIN_GTK_H_INSIDE__) && !defined (CHAMPLAIN_GTK_COMPILATION)
#error "Only <champlain-gtk/champlain-gtk.h> can be included directly."
#endif

#ifndef GTK_CHAMPLAIN_EMBED_H
#define GTK_CHAMPLAIN_EMBED_H

#include <gtk/gtk.h>
#include <champlain/champlain.h>

#define GTK_TYPE_CHAMPLAIN_EMBED     (gtk_champlain_embed_get_type())
#define GTK_CHAMPLAIN_EMBED(obj)     (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_CHAMPLAIN_EMBED, GtkChamplainEmbed))
#define GTK_CHAMPLAIN_EMBED_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass),  GTK_TYPE_CHAMPLAIN_EMBED, GtkChamplainEmbedClass))
#define GTK_CHAMPLAIN_IS_EMBED(obj)  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_CHAMPLAIN_EMBED))
#define GTK_CHAMPLAIN_IS_EMBED_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),  GTK_TYPE_CHAMPLAIN_EMBED))
#define GTK_CHAMPLAIN_EMBED_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),  GTK_TYPE_CHAMPLAIN_EMBED, GtkChamplainEmbedClass))

typedef struct _GtkChamplainEmbedPrivate GtkChamplainEmbedPrivate;

typedef struct _GtkChamplainEmbed GtkChamplainEmbed;

typedef struct _GtkChamplainEmbedClass GtkChamplainEmbedClass;

struct _GtkChamplainEmbed
{
  GtkAlignment bin;

  GtkChamplainEmbedPrivate *priv;
};

struct _GtkChamplainEmbedClass
{
  GtkAlignmentClass parent_class;

};

GType gtk_champlain_embed_get_type (void);

GtkWidget *gtk_champlain_embed_new (void);
ChamplainView *gtk_champlain_embed_get_view (GtkChamplainEmbed* embed);

/* DEPRECATED API */
GtkWidget *champlain_view_embed_new (ChamplainView *view) G_GNUC_DEPRECATED;
ChamplainView *champlain_view_embed_get_view (GtkChamplainEmbed* embed) G_GNUC_DEPRECATED;
void champlain_view_embed_set_view (GtkChamplainEmbed *embed, ChamplainView *view) G_GNUC_DEPRECATED;

#endif
