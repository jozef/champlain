/*
 * Copyright (C) 2008 Pierre-Luc Beaudoin <pierre-luc@pierlux.com>
 * Copyright (C) 2009 Simon Wenner <simon@wenner.ch>
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
#include <champlain/champlain.h>
#include <champlain-memphis/champlain-memphis.h>
#include <champlain-gtk/champlain-gtk.h>
#include <clutter-gtk/clutter-gtk.h>

#define N_COLS 2
#define COL_ID 0
#define COL_NAME 1

guint map_index = 0;
guint rules_index = 0;
static const char *maps[] = { "schaffhausen.osm", "las_palmas.osm" };
static const char *rules[] = { "default-rules.xml", "high-contrast.xml" };

static GtkWidget *window;
static GtkWidget *memphis_box, *memphis_net_box, *memphis_local_box;
static GtkWidget *rules_tree_view, *bg_button, *map_data_state_img;

static GtkWidget *rule_edit_window = NULL;
static GtkWidget *polycolor, *polyminz, *polymaxz;
static GtkWidget *linecolor, *linesize, *lineminz, *linemaxz;
static GtkWidget *bordercolor, *bordersize, *borderminz, *bordermaxz;
static GtkWidget *textcolor, *textsize, *textminz, *textmaxz;
static MemphisRule *current_rule = NULL;

static ChamplainMapSource *tile_source = NULL;

/*
 * Terminate the main loop.
 */
static void
on_destroy (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}

static void color_gdk_to_clutter (const GdkColor *gdk_color,
                                  ClutterColor *clutter_color)
{
  clutter_color->red = CLAMP (((gdk_color->red / 65535.0) * 255), 0, 255);
  clutter_color->green = CLAMP (((gdk_color->green / 65535.0) * 255), 0, 255);
  clutter_color->blue = CLAMP (((gdk_color->blue / 65535.0) * 255), 0, 255);
  clutter_color->alpha = 255;
}

static void color_clutter_to_gdk (const ClutterColor *clutter_color,
                                  GdkColor *gdk_color)
{
  gdk_color->red = ((guint16) clutter_color->red) << 8;
  gdk_color->green = ((guint16) clutter_color->green) << 8;
  gdk_color->blue = ((guint16) clutter_color->blue) << 8;
}

static void
data_source_state_changed (ChamplainView *view,
                           GParamSpec *gobject,
                           GtkImage *image)
{
  ChamplainState state;

  g_object_get (G_OBJECT (view), "state", &state, NULL);
  if (state == CHAMPLAIN_STATE_LOADING)
    {
      gtk_image_set_from_stock (image, GTK_STOCK_NETWORK, GTK_ICON_SIZE_BUTTON);
      g_print ("NET DATA SOURCE STATE: loading\n");
    }
  else
    {
      gtk_image_clear (image);
      g_print ("NET DATA SOURCE STATE: done\n");
    }
}

static void
load_local_map_data (ChamplainMapSource *source)
{
  ChamplainLocalMapDataSource *map_data_source;

  map_data_source = CHAMPLAIN_LOCAL_MAP_DATA_SOURCE (
                      champlain_memphis_tile_source_get_map_data_source (
                        CHAMPLAIN_MEMPHIS_TILE_SOURCE (source)));

  champlain_local_map_data_source_load_map_data (map_data_source,
      maps[map_index]);
}

static void
load_network_map_data (ChamplainMapSource *source, ChamplainView *view)
{
  ChamplainNetworkMapDataSource *map_data_source;
  gdouble lat, lon;

  map_data_source = CHAMPLAIN_NETWORK_MAP_DATA_SOURCE (
                      champlain_memphis_tile_source_get_map_data_source (
                        CHAMPLAIN_MEMPHIS_TILE_SOURCE (source)));

  g_signal_connect (map_data_source, "notify::state", G_CALLBACK (data_source_state_changed),
                    map_data_state_img);

  g_object_get (G_OBJECT (view), "latitude", &lat, "longitude", &lon, NULL);

  champlain_network_map_data_source_load_map_data (map_data_source,
      lon - 0.008, lat - 0.008, lon + 0.008, lat + 0.008);
}

static void
load_rules_into_gui (ChamplainView *view)
{
  GList* ids, *ptr;
  GtkTreeModel *store;
  GtkTreeIter iter;
  GdkColor gdk_color;
  ClutterColor *clutter_color;

  ids = champlain_memphis_tile_source_get_rule_ids (
          CHAMPLAIN_MEMPHIS_TILE_SOURCE(tile_source));

  clutter_color = champlain_memphis_tile_source_get_background_color (
                    CHAMPLAIN_MEMPHIS_TILE_SOURCE(tile_source));

  color_clutter_to_gdk (clutter_color, &gdk_color);
  clutter_color_free (clutter_color);

  gtk_color_button_set_color (GTK_COLOR_BUTTON (bg_button), &gdk_color);

  store = gtk_tree_view_get_model (GTK_TREE_VIEW (rules_tree_view));
  gtk_list_store_clear (GTK_LIST_STORE (store));

  ptr = ids;
  while (ptr != NULL)
    {
      gtk_list_store_append (GTK_LIST_STORE (store), &iter);
      gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, ptr->data, -1);
      ptr = ptr->next;
    }

  g_list_free (ids);
}

static void
rule_window_close_cb (GtkWidget *widget, gpointer data)
{
  gtk_widget_destroy (rule_edit_window);
  memphis_rule_free (current_rule);
  current_rule = NULL;
  rule_edit_window = NULL;
}

static void
rule_apply_cb (GtkWidget *widget, ChamplainMemphisTileSource *source)
{
  MemphisRule *rule = current_rule;
  GdkColor color;

  if (rule->polygon)
    {
      gtk_color_button_get_color (GTK_COLOR_BUTTON (polycolor), &color);
      rule->polygon->color_red = color.red >> 8;
      rule->polygon->color_green = color.green >> 8;
      rule->polygon->color_blue = color.blue >> 8;
      rule->polygon->z_min = gtk_spin_button_get_value (GTK_SPIN_BUTTON (polyminz));
      rule->polygon->z_max = gtk_spin_button_get_value (GTK_SPIN_BUTTON (polymaxz));
    }
  if (rule->line)
    {
      gtk_color_button_get_color (GTK_COLOR_BUTTON (linecolor), &color);
      rule->line->color_red = color.red >> 8;
      rule->line->color_green = color.green >> 8;
      rule->line->color_blue = color.blue >> 8;
      rule->line->size = gtk_spin_button_get_value (GTK_SPIN_BUTTON (linesize));
      rule->line->z_min = gtk_spin_button_get_value (GTK_SPIN_BUTTON (lineminz));
      rule->line->z_max = gtk_spin_button_get_value (GTK_SPIN_BUTTON (linemaxz));
    }
  if (rule->border)
    {
      gtk_color_button_get_color (GTK_COLOR_BUTTON (bordercolor), &color);
      rule->border->color_red = color.red >> 8;
      rule->border->color_green = color.green >> 8;
      rule->border->color_blue = color.blue >> 8;
      rule->border->size = gtk_spin_button_get_value (GTK_SPIN_BUTTON (bordersize));
      rule->border->z_min = gtk_spin_button_get_value (GTK_SPIN_BUTTON (borderminz));
      rule->border->z_max = gtk_spin_button_get_value (GTK_SPIN_BUTTON (bordermaxz));
    }
  if (rule->text)
    {
      gtk_color_button_get_color (GTK_COLOR_BUTTON (textcolor), &color);
      rule->text->color_red = color.red >> 8;
      rule->text->color_green = color.green >> 8;
      rule->text->color_blue = color.blue >> 8;
      rule->text->size = gtk_spin_button_get_value (GTK_SPIN_BUTTON (textsize));
      rule->text->z_min = gtk_spin_button_get_value (GTK_SPIN_BUTTON (textminz));
      rule->text->z_max = gtk_spin_button_get_value (GTK_SPIN_BUTTON (textmaxz));
    }

  champlain_memphis_tile_source_set_rule (source, rule);
}

GtkWidget *
gtk_memphis_prop_new (gint type, MemphisRuleAttr *attr)
{
  GtkWidget *hbox, *cb, *sb1, *sb2, *sb3;
  GdkColor gcolor;

  hbox = gtk_hbox_new (FALSE, 0);

  gcolor.red = ((guint16) attr->color_red) << 8;
  gcolor.green = ((guint16) attr->color_green) << 8;
  gcolor.blue = ((guint16) attr->color_blue) << 8;
  cb = gtk_color_button_new_with_color (&gcolor);
  gtk_box_pack_start (GTK_BOX (hbox), cb, FALSE, FALSE, 0);

  if (type != 0)
    {
      sb1 = gtk_spin_button_new_with_range (0.0, 20.0, 0.1);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (sb1), attr->size);
      gtk_box_pack_start (GTK_BOX (hbox), sb1, FALSE, FALSE, 0);
    }

  sb2 = gtk_spin_button_new_with_range (12, 18, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (sb2), attr->z_min);
  gtk_box_pack_start (GTK_BOX (hbox), sb2, FALSE, FALSE, 0);
  sb3 = gtk_spin_button_new_with_range (12, 18, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (sb3), attr->z_max);
  gtk_box_pack_start (GTK_BOX (hbox), sb3, FALSE, FALSE, 0);

  if (type == 0)
    {
      polycolor = cb;
      polyminz = sb2;
      polymaxz = sb3;
    }
  else if (type == 1)
    {
      linecolor = cb;
      linesize = sb1;
      lineminz = sb2;
      linemaxz = sb3;
    }
  else if (type == 2)
    {
      bordercolor = cb;
      bordersize = sb1;
      borderminz = sb2;
      bordermaxz = sb3;
    }
  else
    {
      textcolor = cb;
      textsize = sb1;
      textminz = sb2;
      textmaxz = sb3;
    }
  return hbox;
}

static void
create_rule_edit_window (MemphisRule *rule, gchar* id,
                         ChamplainMemphisTileSource *source)
{
  GtkWidget *label, *table, *props, *button;

  current_rule = rule;

  rule_edit_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_set_border_width (GTK_CONTAINER (rule_edit_window), 10);
  gtk_window_set_title (GTK_WINDOW (rule_edit_window), id);
  gtk_window_set_position (GTK_WINDOW (rule_edit_window),
                           GTK_WIN_POS_CENTER_ON_PARENT);
  gtk_window_set_transient_for (GTK_WINDOW (rule_edit_window),
                                GTK_WINDOW (window));
  g_signal_connect (G_OBJECT (rule_edit_window), "destroy",
                    G_CALLBACK (rule_window_close_cb), NULL);

  table = gtk_table_new (6, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 8);
  gtk_table_set_row_spacings (GTK_TABLE (table), 8);
  label = gtk_label_new (NULL);

  if (rule->type == MEMPHIS_RULE_TYPE_WAY)
    gtk_label_set_markup (GTK_LABEL (label), "<b>Way properties</b>");
  else if (rule->type == MEMPHIS_RULE_TYPE_NODE)
    gtk_label_set_markup (GTK_LABEL (label), "<b>Node properties</b>");
  else if (rule->type == MEMPHIS_RULE_TYPE_RELATION)
    gtk_label_set_markup (GTK_LABEL (label), "<b>Relation properties</b>");
  else
    gtk_label_set_markup (GTK_LABEL (label), "<b>Unknown type</b>");

  gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 2, 0, 1);

  if (rule->polygon != NULL)
    {
      label = gtk_label_new ("Polygon: ");
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
      props = gtk_memphis_prop_new (0, rule->polygon);
      gtk_table_attach_defaults (GTK_TABLE (table), props, 1, 2, 1, 2);
    }
  if (rule->line != NULL)
    {
      label = gtk_label_new ("Line: ");
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 2, 3);
      props = gtk_memphis_prop_new (1, rule->line);
      gtk_table_attach_defaults (GTK_TABLE (table), props, 1, 2, 2, 3);
    }
  if (rule->border != NULL)
    {
      label = gtk_label_new ("Border: ");
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 3, 4);
      props = gtk_memphis_prop_new (2, rule->border);
      gtk_table_attach_defaults (GTK_TABLE (table), props, 1, 2, 3, 4);
    }
  if (rule->text != NULL)
    {
      label = gtk_label_new ("Text: ");
      gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 4, 5);
      props = gtk_memphis_prop_new (3, rule->text);
      gtk_table_attach_defaults (GTK_TABLE (table), props, 1, 2, 4, 5);
    }

  GtkWidget *vbox = gtk_hbox_new (TRUE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (rule_window_close_cb), NULL);
  button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (rule_apply_cb), source);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);

  GtkWidget *mainbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (mainbox), table, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (mainbox), vbox, FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (rule_edit_window), mainbox);
  gtk_widget_show_all (rule_edit_window);
}

static void
zoom_to_map_data (GtkWidget *widget, ChamplainView *view)
{
  ChamplainMapDataSource *data_source;
  ChamplainBoundingBox *bbox;
  gdouble lat, lon;

  data_source = champlain_memphis_tile_source_get_map_data_source (CHAMPLAIN_MEMPHIS_TILE_SOURCE(tile_source));
  g_object_get (G_OBJECT (data_source), "bounding-box", &bbox, NULL);
  champlain_bounding_box_get_center (bbox, &lat, &lon);

  champlain_view_center_on (CHAMPLAIN_VIEW(view), lat, lon);
  champlain_view_set_zoom_level (CHAMPLAIN_VIEW(view), 15);
}

static void
request_osm_data_cb (GtkWidget *widget, ChamplainView *view)
{
  gdouble lat, lon;
  g_object_get (G_OBJECT (view), "latitude", &lat, "longitude", &lon, NULL);

  if (g_strcmp0 (champlain_map_source_get_id (tile_source), "memphis-network") == 0)
    {
      ChamplainNetworkMapDataSource *data_source =
        CHAMPLAIN_NETWORK_MAP_DATA_SOURCE (
          champlain_memphis_tile_source_get_map_data_source (
            CHAMPLAIN_MEMPHIS_TILE_SOURCE(tile_source)));

      champlain_network_map_data_source_load_map_data (data_source,
          lon - 0.008, lat - 0.008, lon + 0.008, lat + 0.008);
    }
}

void
bg_color_set_cb (GtkColorButton *widget, ChamplainView *view)
{
  GdkColor gdk_color;

  gtk_color_button_get_color (widget, &gdk_color);

  if (strncmp (champlain_map_source_get_id (tile_source), "memphis", 7) == 0)
    {
      ClutterColor clutter_color;
      color_gdk_to_clutter (&gdk_color, &clutter_color);

      champlain_memphis_tile_source_set_background_color (
        CHAMPLAIN_MEMPHIS_TILE_SOURCE (tile_source), &clutter_color);
    }
}

static void
map_source_changed (GtkWidget *widget, ChamplainView *view)
{
  gchar* id;
  ChamplainMapSource *source;
  ChamplainLocalMapDataSource *data;
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    return;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));

  gtk_tree_model_get (model, &iter, COL_ID, &id, -1);

  ChamplainMapSourceFactory *factory = champlain_map_source_factory_dup_default ();
  source = champlain_map_source_factory_create (factory, id);

  if (source != NULL)
    {
      ChamplainMapSourceChain *source_chain;
      ChamplainMapSource *src;
      guint tile_size;

      if (g_strcmp0 (id, "memphis-local") == 0)
        {
          champlain_memphis_tile_source_load_rules (
            CHAMPLAIN_MEMPHIS_TILE_SOURCE (source), rules[rules_index]);
          load_local_map_data (source);
          gtk_widget_hide_all (memphis_box);
          gtk_widget_set_no_show_all (memphis_box, FALSE);
          gtk_widget_set_no_show_all (memphis_local_box, FALSE);
          gtk_widget_set_no_show_all (memphis_net_box, TRUE);
          gtk_widget_show_all (memphis_box);
        }
      else if (g_strcmp0 (id, "memphis-network") == 0)
        {
          champlain_memphis_tile_source_load_rules (
            CHAMPLAIN_MEMPHIS_TILE_SOURCE (source), rules[rules_index]);
          load_network_map_data (source, view);
          gtk_widget_hide_all (memphis_box);
          gtk_widget_set_no_show_all (memphis_box, FALSE);
          gtk_widget_set_no_show_all (memphis_local_box, TRUE);
          gtk_widget_set_no_show_all (memphis_net_box, FALSE);
          gtk_widget_show_all (memphis_box);
        }
      else
        {
          gtk_widget_hide_all (memphis_box);
          gtk_widget_set_no_show_all (memphis_box, TRUE);
        }

      tile_source = CHAMPLAIN_MAP_SOURCE(source);

      source_chain = champlain_map_source_chain_new ();

      tile_size = champlain_map_source_get_tile_size (tile_source);
      src = CHAMPLAIN_MAP_SOURCE(champlain_error_tile_source_new_full (tile_size));

      champlain_map_source_chain_push (source_chain, src);
      champlain_map_source_chain_push (source_chain, tile_source);

      src = CHAMPLAIN_MAP_SOURCE(champlain_file_cache_new_full (100000000, NULL, FALSE));

      champlain_map_source_chain_push (source_chain, src);

      g_object_set (G_OBJECT (view), "map-source", source_chain, NULL);
      if (strncmp (id, "memphis", 7) == 0)
        load_rules_into_gui (view);
    }

  g_object_unref (factory);
}

static void
map_data_changed (GtkWidget *widget, ChamplainView *view)
{
  gint index;
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    return;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  gtk_tree_model_get (model, &iter, 1, &index, -1);

  map_index = index;

  if (g_strcmp0 (champlain_map_source_get_id (tile_source), "memphis-local") == 0)
    load_local_map_data (tile_source);
}

static void
rules_changed (GtkWidget *widget, ChamplainView *view)
{
  gchar* file;
  GtkTreeIter iter;
  GtkTreeModel *model;

  if (!gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter))
    return;

  model = gtk_combo_box_get_model (GTK_COMBO_BOX (widget));
  gtk_tree_model_get (model, &iter, 0, &file, -1);

  if (strncmp (champlain_map_source_get_id (tile_source), "memphis", 7) == 0)
    {
      champlain_memphis_tile_source_load_rules (
        CHAMPLAIN_MEMPHIS_TILE_SOURCE (tile_source),
        file);
      load_rules_into_gui (view);
    }
}

static void
zoom_changed (GtkSpinButton *spinbutton,
              ChamplainView *view)
{
  gint zoom = gtk_spin_button_get_value_as_int (spinbutton);
  g_object_set (G_OBJECT(view), "zoom-level", zoom, NULL);
}

static void
map_zoom_changed (ChamplainView *view,
                  GParamSpec *gobject,
                  GtkSpinButton *spinbutton)
{
  gint zoom;
  g_object_get (G_OBJECT(view), "zoom-level", &zoom, NULL);
  gtk_spin_button_set_value (spinbutton, zoom);
}

static void
zoom_in (GtkWidget *widget,
         ChamplainView *view)
{
  champlain_view_zoom_in (view);
}

static void
zoom_out (GtkWidget *widget,
          ChamplainView *view)
{
  champlain_view_zoom_out (view);
}

static void
build_source_combo_box (GtkComboBox *box)
{
  ChamplainMapSourceFactory *factory;
  GSList *sources, *iter;
  gint i = 0;
  GtkTreeStore *store;
  GtkTreeIter parent;
  GtkCellRenderer *cell;

  store = gtk_tree_store_new (N_COLS, G_TYPE_STRING, /* id */
                              G_TYPE_STRING, /* name */
                              -1);

  factory = champlain_map_source_factory_dup_default ();
  sources = champlain_map_source_factory_dup_list (factory);

  iter = sources;
  while (iter != NULL)
    {
      ChamplainMapSourceDesc *desc;

      desc = (ChamplainMapSourceDesc*) iter->data;

      gtk_tree_store_append (store, &parent, NULL);
      gtk_tree_store_set (store, &parent, COL_ID, desc->id,
                          COL_NAME, desc->name, -1);

      iter = g_slist_next (iter);
    }

  g_slist_free (sources);
  g_object_unref (factory);

  gtk_combo_box_set_model (box, GTK_TREE_MODEL (store));

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (box), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (box), cell,
                                  "text", COL_NAME, NULL);
}

static void
build_data_combo_box (GtkComboBox *box)
{
  GtkTreeStore *store;
  GtkTreeIter parent;
  GtkCellRenderer *cell;

  store = gtk_tree_store_new (2, G_TYPE_STRING, /* file name */
                              G_TYPE_INT, /* index */ -1);

  gtk_tree_store_append (store, &parent, NULL);
  gtk_tree_store_set (store, &parent, 0, maps[0],
                      1, 0, -1);

  gtk_tree_store_append (store, &parent, NULL);
  gtk_tree_store_set (store, &parent, 0, maps[1],
                      1, 1, -1);

  gtk_combo_box_set_model (box, GTK_TREE_MODEL (store));

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (box), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (box), cell,
                                  "text", 0, NULL);
}

static void
build_rules_combo_box (GtkComboBox *box)
{
  GtkTreeStore *store;
  GtkTreeIter parent;
  GtkCellRenderer *cell;

  store = gtk_tree_store_new (1, G_TYPE_STRING, /* file name */ -1);
  gtk_tree_store_append (store, &parent, NULL);
  gtk_tree_store_set (store, &parent, 0, rules[0], -1);

  gtk_tree_store_append (store, &parent, NULL);
  gtk_tree_store_set (store, &parent, 0, rules[1], -1);

  gtk_combo_box_set_model (box, GTK_TREE_MODEL (store));

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (box), cell, FALSE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (box), cell,
                                  "text", 0, NULL);
}

void list_item_selected_cb (GtkTreeView *tree_view,
                            GtkTreePath *path,
                            GtkTreeViewColumn *column,
                            ChamplainView *view)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  char *id;
  GtkTreeSelection *selection = gtk_tree_view_get_selection (tree_view);
  MemphisRule *rule;

  if (rule_edit_window != NULL)
    return;

  if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
      gtk_tree_model_get (model, &iter, 0, &id, -1);

      rule = champlain_memphis_tile_source_get_rule (CHAMPLAIN_MEMPHIS_TILE_SOURCE(tile_source), id);

      if (rule != NULL)
        create_rule_edit_window (rule, id, CHAMPLAIN_MEMPHIS_TILE_SOURCE(tile_source));

      g_free (id);
    }
}

static gboolean
delete_window (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
  gtk_main_quit ();
}

int
main (int argc,
      char *argv[])
{
  GtkWidget *widget, *hbox, *bbox, *menubox, *button, *viewport, *label;
  ChamplainView *view;

  g_thread_init (NULL);
  gtk_clutter_init (&argc, &argv);

  /* create the main, top level, window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  /* give the window a 10px wide border */
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);

  /* give it the title */
  gtk_window_set_title (GTK_WINDOW (window), "libchamplain Gtk+ demo");

  /* Connect the destroy event of the window with our on_destroy function
   * When the window is about to be destroyed we get a notificaiton and
   * stop the main GTK loop
   */
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (on_destroy),
                    NULL);

  hbox = gtk_hbox_new (FALSE, 10);
  menubox = gtk_vbox_new (FALSE, 10);
  memphis_box = gtk_vbox_new (FALSE, 10);
  gtk_widget_set_no_show_all (memphis_box, TRUE);
  memphis_net_box = gtk_hbox_new (FALSE, 10);
  gtk_widget_set_no_show_all (memphis_net_box, TRUE);
  memphis_local_box = gtk_hbox_new (FALSE, 10);
  gtk_widget_set_no_show_all (memphis_local_box, TRUE);

  widget = gtk_champlain_embed_new ();
  view = gtk_champlain_embed_get_view (GTK_CHAMPLAIN_EMBED (widget));

  g_object_set (G_OBJECT (view), "scroll-mode", CHAMPLAIN_SCROLL_MODE_KINETIC,
                "zoom-level", 9, NULL);

  gtk_widget_set_size_request (widget, 640, 480);

  /* first line of buttons */
  bbox =  gtk_hbox_new (FALSE, 10);
  button = gtk_button_new_from_stock (GTK_STOCK_ZOOM_IN);
  g_signal_connect (button, "clicked", G_CALLBACK (zoom_in), view);
  gtk_container_add (GTK_CONTAINER (bbox), button);

  button = gtk_button_new_from_stock (GTK_STOCK_ZOOM_OUT);
  g_signal_connect (button, "clicked", G_CALLBACK (zoom_out), view);
  gtk_container_add (GTK_CONTAINER (bbox), button);

  button = gtk_spin_button_new_with_range (0, 20, 1);
  gtk_spin_button_set_value (GTK_SPIN_BUTTON (button),
                             champlain_view_get_zoom_level (view));
  g_signal_connect (button, "changed", G_CALLBACK (zoom_changed), view);
  g_signal_connect (view, "notify::zoom-level", G_CALLBACK (map_zoom_changed),
                    button);
  gtk_container_add (GTK_CONTAINER (bbox), button);

  gtk_box_pack_start (GTK_BOX (menubox), bbox, FALSE, FALSE, 0);

  /* map source combo box */
  button = gtk_combo_box_new ();
  build_source_combo_box (GTK_COMBO_BOX (button));
  gtk_combo_box_set_active (GTK_COMBO_BOX (button), 0);
  g_signal_connect (button, "changed", G_CALLBACK (map_source_changed), view);
  gtk_box_pack_start (GTK_BOX (menubox), button, FALSE, FALSE, 0);

  /* Memphis options */
  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), "<b>Memphis Rendering Options</b>");
  gtk_box_pack_start (GTK_BOX (memphis_box), label, FALSE, FALSE, 0);

  /* local source panel */
  button = gtk_combo_box_new ();
  build_data_combo_box (GTK_COMBO_BOX (button));
  gtk_combo_box_set_active (GTK_COMBO_BOX (button), 0);
  g_signal_connect (button, "changed", G_CALLBACK (map_data_changed), view);
  gtk_box_pack_start (GTK_BOX (memphis_local_box), button, FALSE, FALSE, 0);

  button = gtk_button_new_from_stock (GTK_STOCK_ZOOM_FIT);
  g_signal_connect (button, "clicked", G_CALLBACK (zoom_to_map_data), view);
  gtk_container_add (GTK_CONTAINER (memphis_local_box), button);

  gtk_box_pack_start (GTK_BOX (memphis_box), memphis_local_box, FALSE, FALSE, 0);

  /* network source panel */
  button = gtk_button_new_with_label ("Request OSM data");
  g_signal_connect (button, "clicked", G_CALLBACK (request_osm_data_cb), view);
  gtk_box_pack_start (GTK_BOX (memphis_net_box), button, FALSE, FALSE, 0);

  map_data_state_img = gtk_image_new ();
  gtk_box_pack_start (GTK_BOX (memphis_net_box), map_data_state_img, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (memphis_box), memphis_net_box, FALSE, FALSE, 0);

  /* rules chooser */
  button = gtk_combo_box_new ();
  build_rules_combo_box (GTK_COMBO_BOX (button));
  gtk_combo_box_set_active (GTK_COMBO_BOX (button), 0);
  g_signal_connect (button, "changed", G_CALLBACK (rules_changed), view);
  gtk_box_pack_start (GTK_BOX (memphis_box), button, FALSE, FALSE, 0);

  /* bg chooser */
  bbox =  gtk_hbox_new (FALSE, 10);

  label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (label), "Background color");
  gtk_box_pack_start (GTK_BOX (bbox), label, FALSE, FALSE, 0);

  bg_button = gtk_color_button_new ();
  gtk_color_button_set_title (GTK_COLOR_BUTTON (bg_button), "Background");
  g_signal_connect (bg_button, "color-set", G_CALLBACK (bg_color_set_cb), view);
  gtk_box_pack_start (GTK_BOX (bbox), bg_button, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (memphis_box), bbox, FALSE, FALSE, 0);

  /* rules list */
  label = gtk_label_new ("Rules");
  bbox =  gtk_hbox_new (FALSE, 10);
  gtk_box_pack_start (GTK_BOX (bbox), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (memphis_box), bbox, FALSE, FALSE, 0);

  GtkListStore *store;
  GtkWidget *tree_view, *scrolled;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *selection;

  store = gtk_list_store_new (1, G_TYPE_STRING);

  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL(store));
  rules_tree_view = tree_view;
  g_object_unref (store);
  renderer = gtk_cell_renderer_text_new ();

  column = gtk_tree_view_column_new_with_attributes (NULL, renderer, "text", 0, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), column);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), FALSE);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tree_view));
  g_signal_connect (tree_view, "row-activated", G_CALLBACK(list_item_selected_cb), view);

  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW(scrolled), tree_view);

  gtk_box_pack_start (GTK_BOX (memphis_box), scrolled, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (menubox), memphis_box, TRUE, TRUE, 0);

  /* viewport */
  viewport = gtk_frame_new (NULL);
  gtk_container_add (GTK_CONTAINER (viewport), widget);

  gtk_box_pack_end (GTK_BOX (hbox), menubox, FALSE, FALSE, 0);
  gtk_container_add (GTK_CONTAINER (hbox), viewport);

  /* and insert it into the main window  */
  gtk_container_add (GTK_CONTAINER (window), hbox);

  /* make sure that everything, window and label, are visible */
  gtk_widget_show_all (window);
  champlain_view_center_on (CHAMPLAIN_VIEW(view), 28.13476, -15.43814);
  /* start the main loop */
  gtk_main ();

  return 0;
}

