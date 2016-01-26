/*
 * Copyright 2016 Colomban Wendling <colomban@geany.org>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * TODO:
 *  * optimize! (how?)
 *  * hide the bottom scrollbar if it's not useful
 *  * fix width on startup?
 */

#define AUTOHIDE 0 /* currently buggy */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <geanyplugin.h>


#define SSM(s, m, w, l) scintilla_send_message (s, m, (uptr_t)(w), (sptr_t)(l))


#if AUTOHIDE
static void
get_text_child (GtkWidget  *widget,
                gpointer    data)
{
  if (! GTK_IS_SCROLLBAR (widget)) {
    *((GtkWidget **) data) = widget;
  }
}

static GtkWidget *
get_text_widget (ScintillaObject *sci)
{
  GtkWidget *child = NULL;
  
  gtk_container_forall (GTK_CONTAINER (sci), get_text_child, &child);
  
  return child ? child : (GtkWidget *) sci;
}

static gint
get_margins_width (ScintillaObject *sci)
{
  gint width;
  gint i;
  
  width = (gint) SSM (sci, SCI_GETMARGINLEFT, 0, 0);
  width += (gint) SSM (sci, SCI_GETMARGINRIGHT, 0, 0);
  for (i = 0; i < SC_MAX_MARGIN; i++) {
    width += (gint) SSM (sci, SCI_GETMARGINWIDTHN, i, 0);
  }
  
  return width;
}
#endif /* AUTOHIDE */

static gint
get_longest_line_width (ScintillaObject *sci)
{
  const gint  lines = sci_get_line_count (sci);
  gint        x = 0;
  gint        line;
  
  for (line = 0; line < lines; line++) {
    gint end    = sci_get_line_end_position (sci, line);
    gint line_x = (gint) SSM (sci, SCI_POINTXFROMPOSITION, 0, end);
    
    if (line_x > x) {
      x = line_x;
    }
  }
  
  return x - (gint) SSM (sci, SCI_POINTXFROMPOSITION, 0, 0);
}

static void
update_hscrollbar (ScintillaObject *sci)
{
  gint width = get_longest_line_width (sci);
  
  SSM (sci, SCI_SETSCROLLWIDTH, MAX (1, width), 0);
  
#if AUTOHIDE
{
  GtkWidget *widget = get_text_widget (sci);
  gint alloc_w = gtk_widget_get_allocated_width (widget);
  gint margin = get_margins_width (sci);
  
  if (margin + width > alloc_w) {
    SSM (sci, SCI_SETHSCROLLBAR, 1, 0);
  } else {
    SSM (sci, SCI_SETHSCROLLBAR, 0, 0);
    SSM (sci, SCI_SETXOFFSET, 0, 0);
  }
}
#endif /* AUTOHIDE */
}

static void
on_editor_notify (GObject        *obj,
                  GeanyEditor    *editor,
                  SCNotification *nt,
                  gpointer        user_data)
{
  if (nt->nmhdr.code == SCN_UPDATEUI) {
    update_hscrollbar (editor->sci);
  }
}

static gboolean
asw_init (GeanyPlugin  *plugin,
          gpointer      data)
{
  GeanyDocument *doc = document_get_current ();
  
  plugin_signal_connect (plugin, NULL, "editor-notify", FALSE,
                         G_CALLBACK (on_editor_notify), plugin);
  
  /* in case we're loaded during the session, update the current document */
  if (doc) {
    update_hscrollbar (doc->editor->sci);
  }
  
  return TRUE;
}

static void
asw_cleanup (GeanyPlugin *plugin,
             gpointer     data)
{
}

G_MODULE_EXPORT void
geany_load_module (GeanyPlugin *plugin)
{
  main_locale_init (LOCALEDIR, GETTEXT_PACKAGE);
  
  plugin->info->name = _("Automatic Scroll Width");
  plugin->info->description = _("Automatically updates the bottom scrollbar "
                                "width to account for actual contents");
  plugin->info->version = "0.0";
  plugin->info->author = "Colomban Wendling <colomban@geany.org>";
  
  plugin->funcs->init = asw_init;
  plugin->funcs->cleanup = asw_cleanup;
  
  GEANY_PLUGIN_REGISTER (plugin, 225);
}
