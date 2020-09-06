/*******************************************************************************
 * install_gui.c
 * A module of J-Pilot http://jpilot.org
 *
 * Copyright (C) 1999-2014 by Judd Montgomery
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ******************************************************************************/

/********************************* Includes ***********************************/
#include "config.h"
#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "i18n.h"
#include "utils.h"
#include "prefs.h"
#include "log.h"

/********************************* Constants **********************************/
#define INST_SDCARD_COLUMN 0
#define INST_FNAME_COLUMN  1

/******************************* Global vars **********************************/
static GtkWidget *treeView;
static GtkListStore *listStore;
static int row_selected;
static int column_selected;
enum {
    INSTALL_SDCARD_COLUMN_ENUM = 0,
    INSTALL_FNAME_COLUMN_ENUM,
    INSTALL_DATA_COLUMN_ENUM,
    INSTALL_BACKGROUND_COLOR_ENUM,
    INSTALL_BACKGROUND_COLOR_ENABLED_ENUM,
    INSTALL_NUM_COLS
};

/****************************** Prototypes ************************************/
static int install_update_listStore(void);
/****************************** Main Code *************************************/
static int install_remove_line(int deleted_line_num) {
    FILE *in;
    FILE *out;
    char line[1002];
    char *Pc;
    int r, line_count;

    in = jp_open_home_file(EPN".install", "r");
    if (!in) {
        jp_logf(JP_LOG_DEBUG, "failed opening install_file\n");
        return EXIT_FAILURE;
    }

    out = jp_open_home_file(EPN".install.tmp", "w");
    if (!out) {
        fclose(in);
        jp_logf(JP_LOG_DEBUG, "failed opening install_file.tmp\n");
        return EXIT_FAILURE;
    }

    /* Delete line by copying file and skipping over line to delete */
    for (line_count = 0; !feof(in); line_count++) {
        line[0] = '\0';
        Pc = fgets(line, 1000, in);
        if (!Pc) {
            break;
        }
        if (line_count == deleted_line_num) {
            continue;
        }
        r = fprintf(out, "%s", line);
        if (r == EOF) {
            break;
        }
    }
    fclose(in);
    fclose(out);

    rename_file(EPN".install.tmp", EPN".install");

    return EXIT_SUCCESS;
}

int install_append_line(const char *line) {
    FILE *out;
    int r;

    out = jp_open_home_file(EPN".install", "a");
    if (!out) {
        return EXIT_FAILURE;
    }

    r = fprintf(out, "%s\n", line);
    if (r == EOF) {
        fclose(out);
        return EXIT_FAILURE;
    }
    fclose(out);

    return EXIT_SUCCESS;
}

static int install_modify_line(int modified_line_num, const char *modified_line) {
    FILE *in;
    FILE *out;
    char line[1002];
    char *Pc;
    int r, line_count;

    in = jp_open_home_file(EPN".install", "r");
    if (!in) {
        jp_logf(JP_LOG_DEBUG, "failed opening install_file\n");
        return EXIT_FAILURE;
    }

    out = jp_open_home_file(EPN".install.tmp", "w");
    if (!out) {
        fclose(in);
        jp_logf(JP_LOG_DEBUG, "failed opening install_file.tmp\n");
        return EXIT_FAILURE;
    }

    /* Delete line by copying file and skipping over line to delete */
    for (line_count = 0; !feof(in); line_count++) {
        line[0] = '\0';
        Pc = fgets(line, 1000, in);
        if (!Pc) {
            break;
        }
        if (line_count == modified_line_num) {
            r = fprintf(out, "%s\n", modified_line);
        } else {
            r = fprintf(out, "%s", line);
        }
        if (r == EOF) {
            break;
        }
    }
    fclose(in);
    fclose(out);

    rename_file(EPN".install.tmp", EPN".install");

    return EXIT_SUCCESS;
}

static gboolean cb_destroy(GtkWidget *widget) {
   gtk_widget_destroy(widget);
    return TRUE;
}

/* Save working directory for future installs */
static void cb_quit(GtkWidget *widget, gpointer data) {
    const char *sel;
    char dir[MAX_PREF_LEN + 2];
    struct stat statb;
    int i;

    jp_logf(JP_LOG_DEBUG, "Quit\n");

    sel = gtk_file_selection_get_filename(GTK_FILE_SELECTION(data));

    g_strlcpy(dir, sel, MAX_PREF_LEN);

    if (stat(sel, &statb)) {
        jp_logf(JP_LOG_WARN, "File selected was not stat-able\n");
    }

    if (S_ISDIR(statb.st_mode)) {
        /* For directory, add '/' indicator to path */
        i = strlen(dir);
        dir[i] = '/', dir[i + 1] = '\0';
    } else {
        /* Otherwise, strip off filename to find actual directory */
        for (i = strlen(dir); i >= 0; i--) {
            if (dir[i] == '/') {
                dir[i + 1] = '\0';
                break;
            }
        }
    }

    set_pref(PREF_INSTALL_PATH, 0, dir, TRUE);

    gtk_widget_destroy(widget);
}

static void cb_add(GtkWidget *widget, gpointer data) {


    const char *sel;
    struct stat statb;

    jp_logf(JP_LOG_DEBUG, "install: cb_add\n");
    sel = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
    jp_logf(JP_LOG_DEBUG, "file selected [%s]\n", sel);

    /* Check to see if its a regular file */
    if (stat(sel, &statb)) {
        jp_logf(JP_LOG_DEBUG, "File selected was not stat-able\n");
        return;
    }
    if (!S_ISREG(statb.st_mode)) {
        jp_logf(JP_LOG_DEBUG, "File selected was not a regular file\n");
        return;
    }

    install_append_line(sel);
    install_update_listStore();
}

static void cb_remove(GtkWidget *widget, gpointer data) {
    if (row_selected < 0) {
        return;
    }
    jp_logf(JP_LOG_DEBUG, "Remove line %d\n", row_selected);
    install_remove_line(row_selected);
    install_update_listStore();
}

gboolean
selectInstallRecordByRow (GtkTreeModel *model,
                   GtkTreePath  *path,
                   GtkTreeIter  *iter,
                   gpointer data) {
    int * i = gtk_tree_path_get_indices ( path ) ;
    if(i[0] == row_selected){
        GtkTreeSelection * selection = NULL;
        selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
        gtk_tree_selection_select_path(selection, path);
        gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(treeView), path,  INSTALL_SDCARD_COLUMN_ENUM,FALSE, 1.0, 0.0);
        return TRUE;
    }

    return FALSE;
}
static int install_update_listStore(void) {
    GtkTreeIter iter;
    GdkPixbuf *sdCardColumnDisplay;
    FILE *in;
    char line[1002];
    char *Pc;
    char *new_line[3];

    int last_row_selected;
    int count;
    int len;
    int sdcard_install;

    new_line[0] = "";
    new_line[1] = line;
    new_line[2] = NULL;

    last_row_selected = row_selected;

    in = jp_open_home_file(EPN".install", "r");
    if (!in) {
        return EXIT_FAILURE;
    }


    gtk_list_store_clear(listStore);
    for (count = 0; !feof(in); count++) {
        line[0] = '\0';
        sdCardColumnDisplay = NULL;
        Pc = fgets(line, 1000, in);
        if (!Pc) {
            break;
        }

        /* Strip newline characters from end of string */
        len = strlen(line);
        if ((line[len - 1] == '\n') || (line[len - 1] == '\r')) line[len - 1] = '\0';
        if ((line[len - 2] == '\n') || (line[len - 2] == '\r')) line[len - 2] = '\0';

        sdcard_install = (line[0] == '\001');
        /* Strip char indicating SDCARD install from start of string */
        if (sdcard_install) {
            new_line[1] = &line[1];
        } else {
            new_line[1] = &line[0];
        }



        /* Add SDCARD icon for files to be installed on SDCARD */
        if (sdcard_install) {
          get_pixbufs(PIXMAP_SDCARD, &sdCardColumnDisplay);

        }
        gtk_list_store_append(listStore, &iter);
        gtk_list_store_set(listStore, &iter,
                           INSTALL_SDCARD_COLUMN_ENUM, sdCardColumnDisplay,
                           INSTALL_FNAME_COLUMN_ENUM, new_line[1],
                           -1);
    }
    fclose(in);


    if (last_row_selected > count - 1) {
        last_row_selected = count - 1;
    }

    if (last_row_selected >= 0) {
        row_selected = last_row_selected;
        gtk_tree_model_foreach(GTK_TREE_MODEL(listStore), selectInstallRecordByRow, NULL);
    }
    return EXIT_SUCCESS;
}
void
columnClicked (GtkTreeView       *tree_view,
               GtkTreePath       *path,
               GtkTreeViewColumn *column,
               gpointer           user_data){
    GtkTreeIter iter;

    column_selected = gtk_tree_view_column_get_sort_column_id(column);
    char fname[1000];
    char *gtk_str;
    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(listStore), &iter, path)) {
        int *i = gtk_tree_path_get_indices(path);
        GdkPixbuf *sdCardColumnDisplay = NULL;
        if (column_selected == INSTALL_SDCARD_COLUMN_ENUM) {
            /* Toggle display of SDCARD pixmap */
            gtk_tree_model_get(GTK_TREE_MODEL(listStore),&iter,INSTALL_SDCARD_COLUMN_ENUM,&sdCardColumnDisplay,
                               INSTALL_FNAME_COLUMN_ENUM,&gtk_str,-1);
            if (sdCardColumnDisplay == NULL) {
                fname[0] = '\001';
                g_strlcpy(&fname[1], gtk_str, sizeof(fname) - 1);
                install_modify_line(i[0], fname);

            } else {
                g_strlcpy(&fname[0], gtk_str, sizeof(fname));
                install_modify_line(i[0], fname);
            }
             install_update_listStore();
        }
    }
}
static gboolean handleInstallRowSelection(GtkTreeSelection *selection,
                                          GtkTreeModel *model,
                                          GtkTreePath *path,
                                          gboolean path_currently_selected,
                                          gpointer userdata) {
   GtkTreeIter iter;

    if ((gtk_tree_model_get_iter(model, &iter, path)) && (!path_currently_selected)) {
        int *i = gtk_tree_path_get_indices(path);
        row_selected = i[0];
    }

    return TRUE;
}


void intializeInstallTreeView(GtkWidget *pixbufwid, GdkPixbuf **pixbuf) {
    listStore = gtk_list_store_new(INSTALL_NUM_COLS, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_POINTER,
                                   GDK_TYPE_COLOR, G_TYPE_BOOLEAN);
    treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(listStore));
    GtkCellRenderer *sdRenderer = gtk_cell_renderer_pixbuf_new();
    GtkTreeViewColumn *sdColumn = gtk_tree_view_column_new_with_attributes("",
                                                                           sdRenderer,
                                                                           "pixbuf", INSTALL_SDCARD_COLUMN_ENUM,
                                                                           "cell-background-gdk",
                                                                           INSTALL_BACKGROUND_COLOR_ENUM,
                                                                           "cell-background-set",
                                                                           INSTALL_BACKGROUND_COLOR_ENABLED_ENUM,
                                                                           NULL);
    gtk_tree_view_column_set_sort_column_id(sdColumn,INSTALL_SDCARD_COLUMN_ENUM);
    GtkCellRenderer *fileNameRenderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *fileNameColumn = gtk_tree_view_column_new_with_attributes("Files to install",
                                                                                 fileNameRenderer,
                                                                                 "text", INSTALL_FNAME_COLUMN_ENUM,
                                                                                 "cell-background-gdk",
                                                                                 INSTALL_BACKGROUND_COLOR_ENUM,
                                                                                 "cell-background-set",
                                                                                 INSTALL_BACKGROUND_COLOR_ENABLED_ENUM,
                                                                                 NULL);
    gtk_tree_view_column_set_sort_column_id(fileNameColumn,INSTALL_FNAME_COLUMN_ENUM);

    gtk_tree_view_column_set_clickable(sdColumn, gtk_false());
    gtk_tree_view_column_set_clickable(fileNameColumn, gtk_false());
    gtk_tree_view_column_set_sizing(sdColumn, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_widget_set_size_request(GTK_WIDGET(treeView), 0, 166);
    gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView)),
                                GTK_SELECTION_BROWSE);

    gtk_tree_view_insert_column(GTK_TREE_VIEW (treeView), sdColumn, INSTALL_SDCARD_COLUMN_ENUM);
    gtk_tree_view_insert_column(GTK_TREE_VIEW (treeView), fileNameColumn, INSTALL_FNAME_COLUMN_ENUM);
    get_pixbufs(PIXMAP_SDCARD, pixbuf);

    pixbufwid = gtk_image_new_from_pixbuf((*pixbuf));
    gtk_widget_show(GTK_WIDGET(pixbufwid));
    gtk_tree_view_column_set_widget(sdColumn, pixbufwid);
    gtk_tree_view_column_set_alignment(sdColumn, GTK_JUSTIFY_CENTER);
    GtkTreeSelection *treeSelection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
    column_selected = -1;
    gtk_tree_selection_set_select_function(treeSelection, handleInstallRowSelection, NULL, NULL);
    //todo: set this up to work on a single click once on gtk3.

    g_signal_connect (treeView, "row-activated", G_CALLBACK(columnClicked), NULL);
}

int install_gui(GtkWidget *main_window, int w, int h, int x, int y) {
    GtkWidget *pixbufwid;
    GdkPixbuf *pixbuf;
    char temp_str[256];
    const char *svalue;
    gchar *titles[] = {"", _("Files to install")};
    GtkWidget *fileChooserWidget;


    row_selected = 0;
    intializeInstallTreeView(pixbufwid, &pixbuf);
    install_update_listStore();
    g_snprintf(temp_str, sizeof(temp_str), "%s %s", PN, _("Install"));
    fileChooserWidget = gtk_file_chooser_dialog_new(_("Install"), main_window, GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT,GTK_STOCK_DELETE, GTK_RESPONSE_DELETE_EVENT,
                                                    GTK_STOCK_CLOSE,GTK_RESPONSE_CLOSE,
                                                    NULL);
    get_pref(PREF_INSTALL_PATH, NULL, &svalue);
    if (svalue && svalue[0]) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(fileChooserWidget), svalue);
    }
    GtkBox *extraWidget = GTK_BOX(gtk_hbox_new(FALSE, 0));
    gtk_box_pack_start(extraWidget,treeView,TRUE,TRUE,0);
    gtk_widget_show_all(extraWidget);
    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(fileChooserWidget), GTK_WIDGET(extraWidget));
    gtk_signal_connect(G_OBJECT(fileChooserWidget), "destroy",
                       G_CALLBACK(cb_destroy), fileChooserWidget);
    int dialogResponse = gtk_dialog_run(GTK_DIALOG (fileChooserWidget));
    do {
     if(dialogResponse == GTK_RESPONSE_DELETE_EVENT){
         //remove from list
         cb_remove(fileChooserWidget,fileChooserWidget);
     } else if(dialogResponse == GTK_RESPONSE_ACCEPT){
         // add to list.
         cb_add(fileChooserWidget,fileChooserWidget);
     } else {
         // handle close and destroy widget.
         cb_destroy(fileChooserWidget);

     }
     if(dialogResponse != GTK_RESPONSE_CLOSE){
        dialogResponse = gtk_dialog_run(GTK_DIALOG (fileChooserWidget));
     }
    }while (dialogResponse != GTK_RESPONSE_CLOSE);
    gtk_widget_destroy(fileChooserWidget);

   // gtk_main();

    return EXIT_SUCCESS;
}

