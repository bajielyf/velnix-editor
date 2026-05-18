#pragma once

#include "core/EditorTypes.h"
#include <gtk/gtk.h>
#include <string>
#include <vector>

class EditorWindow;

class EventHandler {
public:
    explicit EventHandler(EditorWindow *editorWindow);
    ~EventHandler() = default;

    // Event callbacks
    static void on_tab_close_clicked(GtkWidget *button, gpointer data);
    static void on_notebook_switch_page(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer data);
    static void on_notebook_page_reordered(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer data);
    static void on_scintilla_notify(ScintillaObject *sci, gint id, SCNotification *notification, gpointer data);
    static gboolean on_scintilla_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
    static gboolean on_scintilla_button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
    static gboolean on_scintilla_motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data);
    static gboolean on_scintilla_scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer data);
    static gboolean on_window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
    static gboolean on_window_focus_in_event(GtkWidget *widget, GdkEvent *event, gpointer data);
    static gboolean on_window_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data);
    static gboolean on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, gpointer data);
    static gboolean on_periodic_file_state_refresh(gpointer data);
    static void on_new_activate(GtkWidget *widget, gpointer data);
    static void on_open_activate(GtkWidget *widget, gpointer data);
    static void on_reload_activate(GtkWidget *widget, gpointer data);
    static void on_save_activate(GtkWidget *widget, gpointer data);
    static void on_save_as_activate(GtkWidget *widget, gpointer data);
    static void on_save_all_activate(GtkWidget *widget, gpointer data);
    static void on_close_all_activate(GtkWidget *widget, gpointer data);
    static void on_exit_activate(GtkWidget *widget, gpointer data);
    static void on_undo_activate(GtkWidget *widget, gpointer data);
    static void on_redo_activate(GtkWidget *widget, gpointer data);
    static void on_cut_activate(GtkWidget *widget, gpointer data);
    static void on_copy_activate(GtkWidget *widget, gpointer data);
    static void on_paste_activate(GtkWidget *widget, gpointer data);
    static void on_column_editor_toggled(GtkWidget *widget, gpointer data);
    static void on_uppercase_activate(GtkWidget *widget, gpointer data);
    static void on_lowercase_activate(GtkWidget *widget, gpointer data);
    static void on_title_case_activate(GtkWidget *widget, gpointer data);
    static void on_trim_trailing_whitespace_activate(GtkWidget *widget, gpointer data);
    static void on_spaces_to_tabs_activate(GtkWidget *widget, gpointer data);
    static void on_tabs_to_spaces_activate(GtkWidget *widget, gpointer data);
    static void on_convert_encoding_activate(GtkWidget *widget, gpointer data);
    static void on_find_activate(GtkWidget *widget, gpointer data);
    static void on_replace_activate(GtkWidget *widget, gpointer data);
    static void on_go_to_line_activate(GtkWidget *widget, gpointer data);
    static void on_search_results_activate(GtkWidget *widget, gpointer data);
    static void on_fullscreen_toggled(GtkWidget *widget, gpointer data);
    static void on_word_wrap_toggled(GtkWidget *widget, gpointer data);
    static void on_show_whitespace_toggled(GtkWidget *widget, gpointer data);
    static void on_show_eol_markers_toggled(GtkWidget *widget, gpointer data);
    static void on_zoom_in_activate(GtkWidget *widget, gpointer data);
    static void on_zoom_out_activate(GtkWidget *widget, gpointer data);
    static void on_reset_zoom_activate(GtkWidget *widget, gpointer data);

private:
    EditorWindow *editorWindow;
};
