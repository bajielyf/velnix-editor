#include "editor/SearchManager.h"
#include "ui/EditorWindow.h"
#include "ui/Localization.h"
#include "editor/DocumentManager.h"
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <Scintilla.h>
#include <ScintillaWidget.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <regex>

namespace {
gint compare_tree_paths_desc(gconstpointer a, gconstpointer b) {
    return -gtk_tree_path_compare(static_cast<GtkTreePath *>(const_cast<gpointer>(a)),
                                  static_cast<GtkTreePath *>(const_cast<gpointer>(b)));
}

GtkWidget *localized_label(const char *key) {
    GtkWidget *widget = gtk_label_new(Localization::text(key));
    Localization::bind_widget_text(widget, key);
    return widget;
}

GtkWidget *localized_button(const char *key) {
    GtkWidget *widget = gtk_button_new_with_label(Localization::text(key));
    Localization::bind_widget_text(widget, key);
    return widget;
}

GtkWidget *localized_check_button(const char *key) {
    GtkWidget *widget = gtk_check_button_new_with_label(Localization::text(key));
    Localization::bind_widget_text(widget, key);
    return widget;
}

GtkWidget *localized_toggle_button(const char *key) {
    GtkWidget *widget = gtk_toggle_button_new_with_label(Localization::text(key));
    Localization::bind_widget_text(widget, key);
    return widget;
}

void bind_dialog_response_button(GtkWidget *dialog, gint responseId,
                                 const char *key) {
    GtkWidget *button = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog),
                                                           responseId);
    if (button) {
        Localization::bind_widget_text(button, key);
    }
}
}

SearchManager::SearchManager(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {}

SearchManager::~SearchManager() {
    cancel_active_async_search();
    async_search_lifetime.reset();
    if (search_results_refresh_idle_id != 0) {
        g_source_remove(search_results_refresh_idle_id);
        search_results_refresh_idle_id = 0;
    }
    if (find_dialog) {
        gtk_widget_destroy(find_dialog);
    }
    if (replace_dialog) {
        gtk_widget_destroy(replace_dialog);
    }
}

void SearchManager::show_find_dialog() {
    if (!find_dialog || !GTK_IS_WIDGET(find_dialog)) {
        create_find_dialog();
    }
    sync_find_entry_with_selection();
    gtk_widget_show_all(find_dialog);
    gtk_widget_show(find_dialog);
    gtk_window_present(GTK_WINDOW(find_dialog));

    // Focus on the find entry
    if (find_entry) {
        gtk_widget_grab_focus(find_entry);
        gtk_editable_select_region(GTK_EDITABLE(find_entry), 0, -1);
    }
}

void SearchManager::show_replace_dialog() {
    if (!replace_dialog || !GTK_IS_WIDGET(replace_dialog)) {
        create_replace_dialog();
    }
    sync_find_entry_with_selection();
    gtk_widget_show_all(replace_dialog);
    gtk_widget_show(replace_dialog);
    gtk_window_present(GTK_WINDOW(replace_dialog));

    // Focus on the find entry
    if (find_entry) {
        gtk_widget_grab_focus(find_entry);
        gtk_editable_select_region(GTK_EDITABLE(find_entry), 0, -1);
    }
}

void SearchManager::show_go_to_line_dialog() {
    if (!editorWindow) {
        return;
    }

    GtkWidget *scintilla = editorWindow->getCurrentScintilla();
    if (!scintilla) {
        return;
    }

    const int line_count = std::max(
        1, static_cast<int>(scintilla_send_message(
               SCINTILLA(scintilla), SCI_GETLINECOUNT, 0, 0)));
    const sptr_t current_pos =
        scintilla_send_message(SCINTILLA(scintilla), SCI_GETCURRENTPOS, 0, 0);
    const int current_line = std::max(
        0, static_cast<int>(scintilla_send_message(
               SCINTILLA(scintilla), SCI_LINEFROMPOSITION, current_pos, 0)));

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        Localization::text("search.go_to_line"), editorWindow->getDialogParentWindow(),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
        Localization::text("dialog.go"), GTK_RESPONSE_ACCEPT, nullptr);
    Localization::bind_window_title(dialog, "search.go_to_line");
    bind_dialog_response_button(dialog, GTK_RESPONSE_CANCEL, "dialog.cancel");
    bind_dialog_response_button(dialog, GTK_RESPONSE_ACCEPT, "dialog.go");
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    GtkWidget *label = localized_label("search.line_number");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    GtkAdjustment *adjustment = gtk_adjustment_new(
        current_line + 1, 1, line_count, 1, 10, 0);
    GtkWidget *line_spin = gtk_spin_button_new(adjustment, 1, 0);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(line_spin), TRUE);
    gtk_entry_set_activates_default(GTK_ENTRY(line_spin), TRUE);

    std::string range_text = "1 - " + std::to_string(line_count);
    GtkWidget *range_label = gtk_label_new(range_text.c_str());
    gtk_widget_set_halign(range_label, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(range_label),
                                "dim-label");

    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), line_spin, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), range_label, 1, 1, 1, 1);
    gtk_container_add(GTK_CONTAINER(content_area), grid);

    gtk_widget_show_all(dialog);
    gtk_widget_grab_focus(line_spin);
    gtk_editable_select_region(GTK_EDITABLE(line_spin), 0, -1);

    const gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_ACCEPT) {
        const int target_line =
            gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(line_spin)) - 1;
        const int bounded_line = std::max(0, std::min(target_line, line_count - 1));
        scintilla_send_message(SCINTILLA(scintilla), SCI_GOTOLINE, bounded_line, 0);
        scintilla_send_message(SCINTILLA(scintilla), SCI_SCROLLCARET, 0, 0);
        gtk_widget_grab_focus(scintilla);
    }

    gtk_widget_destroy(dialog);
}

void SearchManager::find_next(const std::string &pattern, bool case_sensitive,
                              bool whole_word, bool regex) {
    if (editorWindow) {
        editorWindow->recordMacroSearchAction(
            MacroAppCommand::FindNext, pattern, "", case_sensitive, whole_word, regex);
    }
    perform_search(pattern, true, case_sensitive, whole_word, regex);
}

void SearchManager::find_previous(const std::string &pattern, bool case_sensitive,
                                  bool whole_word, bool regex) {
    if (editorWindow) {
        editorWindow->recordMacroSearchAction(
            MacroAppCommand::FindPrevious, pattern, "", case_sensitive, whole_word, regex);
    }
    perform_search(pattern, false, case_sensitive, whole_word, regex);
}

void SearchManager::find_all(const std::string &pattern, bool case_sensitive,
                             bool whole_word, bool regex) {
    if (editorWindow) {
        editorWindow->recordMacroSearchAction(
            MacroAppCommand::FindAll, pattern, "", case_sensitive, whole_word, regex);
    }

    last_search_pattern = pattern;
    last_search_case_sensitive = case_sensitive;
    last_search_whole_word = whole_word;
    last_search_regex = regex;

    bool search_all_docs = search_all_docs_check && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search_all_docs_check));
    last_search_all_documents = search_all_docs;

    SearchQuery query;
    query.pattern = pattern;
    query.caseSensitive = case_sensitive;
    query.wholeWord = whole_word;
    query.regex = regex;
    query.searchAllDocuments = search_all_docs;
    start_async_search({query}, false, true);
}

void SearchManager::find_all_batch(const std::vector<MacroSearchRequest> &requests) {
    if (requests.empty()) {
        return;
    }

    bool search_all_docs = search_all_docs_check && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search_all_docs_check));
    last_search_all_documents = search_all_docs;

    std::vector<SearchQuery> queries;
    queries.reserve(requests.size());
    for (const MacroSearchRequest &request : requests) {
        if (request.findPattern.empty()) {
            continue;
        }

        SearchQuery query;
        query.pattern = request.findPattern;
        query.caseSensitive = request.caseSensitive;
        query.wholeWord = request.wholeWord;
        query.regex = request.regex;
        query.searchAllDocuments = search_all_docs;
        queries.push_back(query);

        last_search_pattern = query.pattern;
        last_search_case_sensitive = query.caseSensitive;
        last_search_whole_word = query.wholeWord;
        last_search_regex = query.regex;
    }

    start_async_search(queries, false, true, {}, true);
}

void SearchManager::replace_next(const std::string &find_pattern, const std::string &replace_text,
                                 bool case_sensitive, bool whole_word, bool regex) {
    if (editorWindow) {
        editorWindow->recordMacroSearchAction(
            MacroAppCommand::ReplaceNext, find_pattern, replace_text,
            case_sensitive, whole_word, regex);
    }
    perform_replace(find_pattern, replace_text, case_sensitive, whole_word, regex, false);
}

void SearchManager::replace_all(const std::string &find_pattern, const std::string &replace_text,
                                bool case_sensitive, bool whole_word, bool regex) {
    if (editorWindow) {
        editorWindow->recordMacroSearchAction(
            MacroAppCommand::ReplaceAll, find_pattern, replace_text,
            case_sensitive, whole_word, regex);
    }
    perform_replace(find_pattern, replace_text, case_sensitive, whole_word, regex, true);
}

void SearchManager::select_all_occurrences(const std::string &pattern, bool case_sensitive,
                                           bool whole_word, bool regex) {
    // This would require more complex implementation with Scintilla markers
    // For now, just find the first occurrence
    perform_search(pattern, true, case_sensitive, whole_word, regex);
}

std::string SearchManager::get_current_selection_text() const {
    if (!editorWindow) {
        return "";
    }

    GtkWidget *scintilla = editorWindow->getCurrentScintilla();
    if (!scintilla) {
        return "";
    }

    sptr_t sel_start = scintilla_send_message(SCINTILLA(scintilla), SCI_GETSELECTIONSTART, 0, 0);
    sptr_t sel_end = scintilla_send_message(SCINTILLA(scintilla), SCI_GETSELECTIONEND, 0, 0);
    if (sel_start == sel_end) {
        return "";
    }

    sptr_t length = sel_end - sel_start;
    if (length <= 0) {
        return "";
    }

    std::string selected_text(static_cast<size_t>(length), '\0');
    scintilla_send_message(
        SCINTILLA(scintilla),
        SCI_GETSELTEXT,
        0,
        reinterpret_cast<sptr_t>(selected_text.data()));

    if (!selected_text.empty() && selected_text.back() == '\0') {
        selected_text.pop_back();
    }

    return selected_text;
}

void SearchManager::sync_find_entry_with_selection() {
    if (!find_entry) {
        return;
    }

    const std::string selected_text = get_current_selection_text();
    if (!selected_text.empty()) {
        gtk_entry_set_text(GTK_ENTRY(find_entry), selected_text.c_str());
    }
}

void SearchManager::create_search_results_panel(GtkWidget *parent) {
    if (!parent) {
        return;
    }

    search_results_header_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    search_results_toggle_button = localized_toggle_button("menu.search_results");
    GtkWidget *header_spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    GtkWidget *auto_refresh_button = localized_check_button("search.auto_refresh");
    GtkWidget *refresh_results_button_local = localized_button("search.refresh");
    GtkWidget *expand_all_button = localized_button("search.expand");
    GtkWidget *collapse_all_button = localized_button("search.collapse");
    GtkWidget *clear_results_button = localized_button("search.clear");

    gtk_button_set_relief(GTK_BUTTON(search_results_toggle_button), GTK_RELIEF_NONE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(search_results_toggle_button), FALSE);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_refresh_button), FALSE);

    gtk_box_pack_start(GTK_BOX(search_results_header_bar), search_results_toggle_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(search_results_header_bar), header_spacer, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(search_results_header_bar), clear_results_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(search_results_header_bar), refresh_results_button_local, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(search_results_header_bar), auto_refresh_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(search_results_header_bar), collapse_all_button, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(search_results_header_bar), expand_all_button, FALSE, FALSE, 0);

    search_results_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(search_results_content), 8);

    find_results_store = nullptr;
    find_results_view = nullptr;
    find_results_status_label = nullptr;
    GtkWidget *results_view = nullptr;
    GtkWidget *status_label = nullptr;
    GtkWidget *contents = create_results_panel(&results_view, &status_label, &find_results_store);
    find_results_view = results_view;
    find_results_status_label = status_label;
    refresh_results_button = refresh_results_button_local;
    auto_refresh_toggle = auto_refresh_button;
    update_refresh_controls();

    g_signal_connect(clear_results_button, "clicked",
                     G_CALLBACK(on_clear_results_clicked), this);
    g_signal_connect(expand_all_button, "clicked",
                     G_CALLBACK(on_expand_all_results_clicked), this);
    g_signal_connect(collapse_all_button, "clicked",
                     G_CALLBACK(on_collapse_all_results_clicked), this);
    g_signal_connect(refresh_results_button_local, "clicked",
                     G_CALLBACK(on_refresh_results_clicked), this);
    g_signal_connect(auto_refresh_button, "toggled",
                     G_CALLBACK(on_auto_refresh_toggled), this);
    g_signal_connect(search_results_toggle_button, "toggled",
                     G_CALLBACK(on_search_results_toggled), this);

    gtk_container_add(GTK_CONTAINER(search_results_content), contents);
    gtk_widget_set_vexpand(parent, TRUE);
    gtk_widget_set_vexpand(search_results_content, TRUE);
    gtk_widget_set_hexpand(search_results_content, TRUE);
    gtk_box_pack_start(GTK_BOX(parent), search_results_header_bar, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(parent), search_results_content, TRUE, TRUE, 0);

    gtk_widget_show_all(parent);
    gtk_widget_set_visible(search_results_content, FALSE);
    gtk_widget_set_visible(parent, FALSE);
}

void SearchManager::show_search_results_panel() {
    if (editorWindow && editorWindow->getSearchResultsPanel()) {
        gtk_widget_show(editorWindow->getSearchResultsPanel());
    }
    if (search_results_content) {
        gtk_widget_set_visible(search_results_content, TRUE);
    }
    if (search_results_toggle_button) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(search_results_toggle_button), TRUE);
    }
}

void SearchManager::hide_search_results_panel() {
    if (search_results_toggle_button) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(search_results_toggle_button), FALSE);
    }
    if (search_results_content) {
        gtk_widget_set_visible(search_results_content, FALSE);
    }
    if (editorWindow && editorWindow->getSearchResultsPanel()) {
        gtk_widget_hide(editorWindow->getSearchResultsPanel());
    }
}

bool SearchManager::is_search_results_panel_visible() const {
    return editorWindow && editorWindow->getSearchResultsPanel() &&
           gtk_widget_get_visible(editorWindow->getSearchResultsPanel());
}

void SearchManager::refresh_localized_ui() {
    Localization::refresh_widget_tree(find_dialog);
    Localization::refresh_widget_tree(replace_dialog);
    Localization::refresh_widget_tree(search_results_header_bar);
    Localization::refresh_widget_tree(search_results_content);
    if (find_results_store && has_last_search() && !search_in_progress) {
        refresh_search_results();
    }
}

void SearchManager::create_find_dialog() {
    find_dialog = gtk_dialog_new_with_buttons(Localization::text("search.find_title"),
                                              editorWindow->getDialogParentWindow(),
                                              GTK_DIALOG_MODAL,
                                              Localization::text("dialog.find_next"), GTK_RESPONSE_OK,
                                              Localization::text("dialog.find_all"), GTK_RESPONSE_APPLY,
                                              Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
                                              NULL);
    Localization::bind_window_title(find_dialog, "search.find_title");
    bind_dialog_response_button(find_dialog, GTK_RESPONSE_OK, "dialog.find_next");
    bind_dialog_response_button(find_dialog, GTK_RESPONSE_APPLY, "dialog.find_all");
    bind_dialog_response_button(find_dialog, GTK_RESPONSE_CANCEL, "dialog.cancel");

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(find_dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(content_area), grid);

    // Find label and entry
    GtkWidget *find_label = localized_label("search.find_what");
    find_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), find_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), find_entry, 1, 0, 1, 1);

    // Options
    case_sensitive_check = localized_check_button("search.case_sensitive");
    whole_word_check = localized_check_button("search.whole_word");
    regex_check = localized_check_button("search.regex");
    wrap_around_check = localized_check_button("search.wrap_around");
    search_all_docs_check = localized_check_button("search.all_open_documents");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wrap_around_check),
                                 editorWindow->isSearchWrapAroundEnabled());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(search_all_docs_check),
                                 editorWindow->isSearchAllDocumentsEnabled());

    gtk_grid_attach(GTK_GRID(grid), case_sensitive_check, 0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), whole_word_check, 0, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), regex_check, 0, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), wrap_around_check, 0, 4, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), search_all_docs_check, 0, 5, 2, 1);

    // Connect signals
    g_signal_connect(find_dialog, "response", G_CALLBACK(on_find_close_clicked), this);
    g_signal_connect(find_dialog, "delete-event", G_CALLBACK(on_dialog_delete_event), this);
    g_signal_connect(find_dialog, "destroy", G_CALLBACK(on_find_dialog_destroyed), this);
    g_signal_connect(find_entry, "activate", G_CALLBACK(on_find_next_clicked), this);

    gtk_widget_set_size_request(find_dialog, 400, 200);
}

void SearchManager::create_replace_dialog() {
    replace_dialog = gtk_dialog_new_with_buttons(Localization::text("search.replace_title"),
                                                 editorWindow->getDialogParentWindow(),
                                                 GTK_DIALOG_MODAL,
                                                 Localization::text("dialog.find_next"), GTK_RESPONSE_OK,
                                                 Localization::text("dialog.replace"), GTK_RESPONSE_APPLY,
                                                 Localization::text("dialog.replace_all"), GTK_RESPONSE_YES,
                                                 Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
                                                 NULL);
    Localization::bind_window_title(replace_dialog, "search.replace_title");
    bind_dialog_response_button(replace_dialog, GTK_RESPONSE_OK,
                                "dialog.find_next");
    bind_dialog_response_button(replace_dialog, GTK_RESPONSE_APPLY,
                                "dialog.replace");
    bind_dialog_response_button(replace_dialog, GTK_RESPONSE_YES,
                                "dialog.replace_all");
    bind_dialog_response_button(replace_dialog, GTK_RESPONSE_CANCEL,
                                "dialog.cancel");

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(replace_dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(content_area), grid);

    // Find label and entry
    GtkWidget *find_label = localized_label("search.find_what");
    find_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), find_label, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), find_entry, 1, 0, 1, 1);

    // Replace label and entry
    GtkWidget *replace_label = localized_label("search.replace_with");
    replace_entry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), replace_label, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), replace_entry, 1, 1, 1, 1);

    // Options
    case_sensitive_check = localized_check_button("search.case_sensitive");
    whole_word_check = localized_check_button("search.whole_word");
    regex_check = localized_check_button("search.regex");
    wrap_around_check = localized_check_button("search.wrap_around");
    search_all_docs_check = localized_check_button("search.all_open_documents");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wrap_around_check),
                                 editorWindow->isSearchWrapAroundEnabled());
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(search_all_docs_check),
                                 editorWindow->isSearchAllDocumentsEnabled());

    gtk_grid_attach(GTK_GRID(grid), case_sensitive_check, 0, 2, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), whole_word_check, 0, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), regex_check, 0, 4, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), wrap_around_check, 0, 5, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), search_all_docs_check, 0, 6, 2, 1);

    // Connect signals
    g_signal_connect(replace_dialog, "response", G_CALLBACK(on_replace_close_clicked), this);
    g_signal_connect(replace_dialog, "delete-event", G_CALLBACK(on_dialog_delete_event), this);
    g_signal_connect(replace_dialog, "destroy", G_CALLBACK(on_replace_dialog_destroyed), this);
    g_signal_connect(find_entry, "activate", G_CALLBACK(on_find_next_clicked), this);
    g_signal_connect(replace_entry, "activate", G_CALLBACK(on_replace_next_clicked), this);

    gtk_widget_set_size_request(replace_dialog, 400, 250);
}

GtkWidget *SearchManager::create_results_panel(GtkWidget **view_out, GtkWidget **status_label_out,
                                               GtkTreeStore **store_out) {
    GtkWidget *panel_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    *store_out = gtk_tree_store_new(SearchResultColumnCount,
                                    G_TYPE_STRING,  // File
                                    G_TYPE_STRING,  // Text
                                    G_TYPE_INT,     // Start
                                    G_TYPE_INT,     // End
                                    G_TYPE_INT,     // DocIndex
                                    G_TYPE_BOOLEAN, // Is group
                                    G_TYPE_STRING,  // Pattern
                                    G_TYPE_BOOLEAN, // Case sensitive
                                    G_TYPE_BOOLEAN, // Whole word
                                    G_TYPE_BOOLEAN, // Regex
                                    G_TYPE_BOOLEAN);// Search all docs

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(scrolled, TRUE);
    gtk_widget_set_hexpand(scrolled, TRUE);

    *view_out = gtk_tree_view_new_with_model(GTK_TREE_MODEL(*store_out));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(*view_out), FALSE);
    gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(*view_out), FALSE);
    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(*view_out));
    gtk_tree_selection_set_mode(selection, GTK_SELECTION_MULTIPLE);

    GtkCellRenderer *file_renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *file_column = gtk_tree_view_column_new();
    gtk_tree_view_column_pack_start(file_column, file_renderer, TRUE);
    gtk_tree_view_column_set_cell_data_func(file_column, file_renderer,
                                            on_file_cell_data, nullptr, nullptr);
    gtk_tree_view_column_set_resizable(file_column, TRUE);
    gtk_tree_view_column_set_expand(file_column, TRUE);
    gtk_tree_view_column_set_min_width(file_column, 180);
    gtk_tree_view_append_column(GTK_TREE_VIEW(*view_out), file_column);
    gtk_tree_view_set_expander_column(GTK_TREE_VIEW(*view_out), file_column);

    g_signal_connect(*view_out, "row-activated", G_CALLBACK(on_results_row_activated), this);
    g_signal_connect(*view_out, "key-press-event", G_CALLBACK(on_results_key_press), this);

    gtk_container_add(GTK_CONTAINER(scrolled), *view_out);
    gtk_box_pack_start(GTK_BOX(panel_box), scrolled, TRUE, TRUE, 0);

    *status_label_out = gtk_label_new("");
    gtk_widget_set_halign(*status_label_out, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(panel_box), *status_label_out, FALSE, FALSE, 0);

    return panel_box;
}

void SearchManager::clear_search_results() {
    cancel_active_async_search();
    set_search_in_progress_state(false);
    if (search_results_refresh_idle_id != 0) {
        g_source_remove(search_results_refresh_idle_id);
        search_results_refresh_idle_id = 0;
    }

    if (find_results_store) {
        gtk_tree_store_clear(find_results_store);
    }

    last_search_pattern.clear();
    last_search_case_sensitive = false;
    last_search_whole_word = false;
    last_search_regex = false;
    last_search_all_documents = false;
    update_refresh_controls();

    if (find_results_status_label) {
        gtk_label_set_text(GTK_LABEL(find_results_status_label),
                           Localization::text("search.results_cleared"));
    }

    hide_search_results_panel();
}

void SearchManager::expand_all_search_results() {
    if (find_results_view) {
        gtk_tree_view_expand_all(GTK_TREE_VIEW(find_results_view));
    }
}

void SearchManager::collapse_all_search_results() {
    if (find_results_view) {
        gtk_tree_view_collapse_all(GTK_TREE_VIEW(find_results_view));
    }
}

void SearchManager::refresh_search_results() {
    if (search_results_refresh_idle_id != 0) {
        g_source_remove(search_results_refresh_idle_id);
        search_results_refresh_idle_id = 0;
    }

    const std::vector<SearchQuery> queries = collect_search_queries_from_results();
    const std::set<std::string> expandedGroups = collect_expanded_result_groups();
    if (queries.empty()) {
        update_refresh_controls();
        if (find_results_status_label) {
            gtk_label_set_text(GTK_LABEL(find_results_status_label),
                               Localization::text("search.no_refresh"));
        }
        return;
    }
    for (const SearchQuery &query : queries) {
        last_search_pattern = query.pattern;
        last_search_case_sensitive = query.caseSensitive;
        last_search_whole_word = query.wholeWord;
        last_search_regex = query.regex;
        last_search_all_documents = query.searchAllDocuments;
    }
    start_async_search(queries, true, false, expandedGroups);
}

bool SearchManager::is_auto_refresh_enabled() const {
    return auto_refresh_enabled;
}

bool SearchManager::has_last_search() const {
    return search_in_progress || !collect_search_queries_from_results().empty() || !last_search_pattern.empty();
}

void SearchManager::schedule_search_results_refresh() {
    if (!auto_refresh_enabled || last_search_pattern.empty()) {
        return;
    }

    if (search_results_refresh_idle_id != 0) {
        g_source_remove(search_results_refresh_idle_id);
    }
    search_results_refresh_idle_id =
        g_timeout_add(150, on_search_results_refresh_idle, this);
}

gboolean SearchManager::on_search_results_refresh_idle(gpointer data) {
    SearchManager *self = static_cast<SearchManager *>(data);
    self->search_results_refresh_idle_id = 0;
    self->refresh_search_results();
    return FALSE;
}

void SearchManager::update_refresh_controls() {
    const bool hasSearch = has_last_search();
    if (refresh_results_button) {
        gtk_widget_set_sensitive(refresh_results_button, hasSearch && !search_in_progress);
    }
    if (auto_refresh_toggle) {
        gtk_widget_set_sensitive(auto_refresh_toggle, hasSearch && !search_in_progress);
        if (!hasSearch && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_refresh_toggle))) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_refresh_toggle), FALSE);
        }
    }
}

void SearchManager::on_find_all_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SearchManager *self = static_cast<SearchManager *>(data);
    const char *pattern = gtk_entry_get_text(GTK_ENTRY(self->find_entry));
    if (!pattern || !*pattern) {
        return;
    }

    bool case_sensitive = self->case_sensitive_check &&
                         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->case_sensitive_check));
    bool whole_word = self->whole_word_check &&
                     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->whole_word_check));
    bool regex = self->regex_check &&
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->regex_check));

    self->find_all(pattern, case_sensitive, whole_word, regex);
}

void SearchManager::on_clear_results_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SearchManager *self = static_cast<SearchManager *>(data);
    self->clear_search_results();
}

void SearchManager::on_expand_all_results_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SearchManager *self = static_cast<SearchManager *>(data);
    self->expand_all_search_results();
}

void SearchManager::on_collapse_all_results_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SearchManager *self = static_cast<SearchManager *>(data);
    self->collapse_all_search_results();
}

void SearchManager::on_results_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                             GtkTreeViewColumn *column, gpointer data) {
    (void)column;
    SearchManager *self = static_cast<SearchManager *>(data);
    self->jump_to_search_result(tree_view, path);
}

gboolean SearchManager::on_results_key_press(GtkTreeView *tree_view, GdkEventKey *event, gpointer data) {
    SearchManager *self = static_cast<SearchManager *>(data);

    if ((event->state & GDK_CONTROL_MASK) != 0 &&
        (event->keyval == GDK_KEY_a || event->keyval == GDK_KEY_A)) {
        gtk_tree_view_expand_all(tree_view);
        gtk_tree_selection_select_all(gtk_tree_view_get_selection(tree_view));
        return TRUE;
    }

    if (event->keyval == GDK_KEY_Delete || event->keyval == GDK_KEY_KP_Delete) {
        self->delete_selected_search_results(tree_view);
        return TRUE;
    }

    return FALSE;
}

void SearchManager::delete_selected_search_results(GtkTreeView *tree_view) {
    GtkTreeSelection *selection = gtk_tree_view_get_selection(tree_view);
    GList *selected_paths = gtk_tree_selection_get_selected_rows(selection, nullptr);
    if (!selected_paths) {
        return;
    }

    selected_paths = g_list_sort(selected_paths,
                                 reinterpret_cast<GCompareFunc>(gtk_tree_path_compare));

    GList *normalized_paths = nullptr;
    for (GList *node = selected_paths; node != nullptr; node = node->next) {
        GtkTreePath *candidate = static_cast<GtkTreePath *>(node->data);
        bool has_selected_ancestor = false;

        for (GList *existing = normalized_paths; existing != nullptr; existing = existing->next) {
            GtkTreePath *selected = static_cast<GtkTreePath *>(existing->data);
            if (gtk_tree_path_is_ancestor(selected, candidate)) {
                has_selected_ancestor = true;
                break;
            }
        }

        if (!has_selected_ancestor) {
            normalized_paths = g_list_append(normalized_paths, gtk_tree_path_copy(candidate));
        }
    }

    normalized_paths = g_list_sort(normalized_paths, compare_tree_paths_desc);

    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeStore *store = GTK_TREE_STORE(model);
    int removed_count = 0;
    for (GList *node = normalized_paths; node != nullptr; node = node->next) {
        GtkTreePath *path = static_cast<GtkTreePath *>(node->data);
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter(model, &iter, path)) {
            gtk_tree_store_remove(store, &iter);
            ++removed_count;
        }
    }

    if (tree_view == GTK_TREE_VIEW(find_results_view) && find_results_status_label) {
        std::string status = removed_count > 0
            ? std::string(Localization::text("search.removed_prefix")) +
                  std::to_string(removed_count) +
                  Localization::text("search.removed_suffix")
            : Localization::text("search.none_removed");
        gtk_label_set_text(GTK_LABEL(find_results_status_label), status.c_str());
    } else if (tree_view == GTK_TREE_VIEW(replace_results_view) && replace_results_status_label) {
        std::string status = removed_count > 0
            ? std::string(Localization::text("search.removed_prefix")) +
                  std::to_string(removed_count) +
                  Localization::text("search.removed_suffix")
            : Localization::text("search.none_removed");
        gtk_label_set_text(GTK_LABEL(replace_results_status_label), status.c_str());
    }

    g_list_free_full(normalized_paths, reinterpret_cast<GDestroyNotify>(gtk_tree_path_free));
    g_list_free_full(selected_paths, reinterpret_cast<GDestroyNotify>(gtk_tree_path_free));
}

std::vector<SearchManager::SearchDocumentSnapshot>
SearchManager::snapshot_search_documents(bool search_all_docs) const {
    std::vector<SearchDocumentSnapshot> snapshots;
    if (!editorWindow) {
        return snapshots;
    }

    int start_doc = search_all_docs ? 0 : editorWindow->getCurrentDocumentIndex();
    int end_doc = search_all_docs ? editorWindow->getDocumentCount()
                                  : editorWindow->getCurrentDocumentIndex() + 1;
    if (start_doc < 0 || end_doc <= start_doc) {
        return snapshots;
    }

    snapshots.reserve(static_cast<size_t>(std::max(0, end_doc - start_doc)));
    for (int doc_index = start_doc; doc_index < end_doc; ++doc_index) {
        const Document *document = editorWindow->getDocument(doc_index);
        if (!document) {
            continue;
        }

        SearchDocumentSnapshot snapshot;
        snapshot.docIndex = doc_index;
        snapshot.documentName =
            document->filePath.empty() ? document->title : document->filePath;
        snapshot.text = editorWindow->getDocumentText(doc_index);
        snapshots.push_back(std::move(snapshot));
    }

    return snapshots;
}

void SearchManager::cancel_active_async_search() {
    if (active_search_cancellable) {
        g_cancellable_cancel(active_search_cancellable);
        g_object_unref(active_search_cancellable);
        active_search_cancellable = nullptr;
    }
}

void SearchManager::set_search_in_progress_state(bool in_progress, const std::string &status_message) {
    search_in_progress = in_progress;
    if (find_results_status_label && !status_message.empty()) {
        gtk_label_set_text(GTK_LABEL(find_results_status_label), status_message.c_str());
    }
    update_refresh_controls();
}

void SearchManager::start_async_search(const std::vector<SearchQuery> &queries, bool clear_existing,
                                       bool expand_new_group,
                                       const std::set<std::string> &expanded_groups,
                                       bool append_results) {
    cancel_active_async_search();

    if (!find_results_store || !find_results_status_label) {
        return;
    }

    if (queries.empty()) {
        set_search_in_progress_state(false);
        gtk_label_set_text(GTK_LABEL(find_results_status_label),
                           Localization::text("search.no_query"));
        return;
    }

    AsyncSearchRequest *request = new AsyncSearchRequest();
    request->requestId = ++active_async_search_request_id;
    request->currentDocumentIndex = editorWindow ? editorWindow->getCurrentDocumentIndex() : -1;
    request->clearExisting = clear_existing;
    request->expandNewGroup = expand_new_group;
    request->appendResults = append_results;
    request->queries = queries;
    request->expandedGroups = expanded_groups;

    bool search_all_docs = false;
    for (const SearchQuery &query : queries) {
        if (query.searchAllDocuments) {
            search_all_docs = true;
            break;
        }
    }
    request->documents = snapshot_search_documents(search_all_docs);

    if (request->documents.empty()) {
        delete request;
        set_search_in_progress_state(false,
                                     Localization::text("search.no_document"));
        return;
    }

    active_search_cancellable = g_cancellable_new();

    AsyncSearchCallbackContext *callback_context = new AsyncSearchCallbackContext();
    callback_context->manager = this;
    callback_context->lifetime = async_search_lifetime;

    GTask *task = g_task_new(nullptr, active_search_cancellable,
                             on_async_search_task_complete, callback_context);
    g_task_set_task_data(task, request, destroy_async_search_request);

    std::string status = queries.size() == 1
        ? Localization::text("search.searching")
        : Localization::text("search.refreshing");
    set_search_in_progress_state(true, status);
    show_search_results_panel();

    g_task_run_in_thread(task, run_async_search_task);
    g_object_unref(task);
}

gboolean SearchManager::is_word_boundary(const std::string &text, size_t index) {
    if (index >= text.size()) {
        return TRUE;
    }

    const unsigned char c = static_cast<unsigned char>(text[index]);
    return !(std::isalnum(c) || c == '_');
}

std::vector<std::pair<size_t, size_t>> SearchManager::find_pattern_matches(
    const std::string &text, const SearchQuery &query, bool *cancelled, GCancellable *cancellable,
    std::string *error_message) {
    std::vector<std::pair<size_t, size_t>> matches;
    if (query.pattern.empty() || text.empty()) {
        return matches;
    }

    if (query.regex) {
        try {
            std::regex_constants::syntax_option_type options = std::regex::ECMAScript;
            if (!query.caseSensitive) {
                options |= std::regex::icase;
            }

            const std::regex pattern(query.pattern, options);
            std::sregex_iterator iter(text.begin(), text.end(), pattern);
            std::sregex_iterator end;
            for (; iter != end; ++iter) {
                if (cancellable && g_cancellable_is_cancelled(cancellable)) {
                    if (cancelled) {
                        *cancelled = true;
                    }
                    return {};
                }

                const std::smatch &match = *iter;
                if (match.length() == 0) {
                    continue;
                }

                const size_t start = static_cast<size_t>(match.position());
                const size_t finish = start + static_cast<size_t>(match.length());
                if (query.wholeWord &&
                    ((start > 0 && !is_word_boundary(text, start - 1)) ||
                     !is_word_boundary(text, finish))) {
                    continue;
                }

                matches.emplace_back(start, finish);
            }
        } catch (const std::regex_error &error) {
            if (error_message) {
                *error_message =
                    std::string(Localization::text("search.invalid_regex")) +
                    error.what();
            }
        }
        return matches;
    }

    std::string haystack = text;
    std::string needle = query.pattern;
    if (!query.caseSensitive) {
        std::transform(haystack.begin(), haystack.end(), haystack.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        std::transform(needle.begin(), needle.end(), needle.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    }

    size_t pos = 0;
    while (pos < haystack.size()) {
        if (cancellable && g_cancellable_is_cancelled(cancellable)) {
            if (cancelled) {
                *cancelled = true;
            }
            return {};
        }

        pos = haystack.find(needle, pos);
        if (pos == std::string::npos) {
            break;
        }

        const size_t finish = pos + needle.size();
        if (!query.wholeWord ||
            ((pos == 0 || is_word_boundary(text, pos - 1)) && is_word_boundary(text, finish))) {
            matches.emplace_back(pos, finish);
        }

        pos = finish > pos ? finish : pos + 1;
    }

    return matches;
}

SearchManager::SearchGroupResultData SearchManager::build_search_group_result(
    const SearchQuery &query, int current_document_index,
    const std::vector<SearchDocumentSnapshot> &documents, bool *cancelled, GCancellable *cancellable,
    std::string *error_message) {
    SearchGroupResultData group;
    group.query = query;

    for (const SearchDocumentSnapshot &document : documents) {
        if (!query.searchAllDocuments && document.docIndex != current_document_index) {
            continue;
        }

        if (cancellable && g_cancellable_is_cancelled(cancellable)) {
            if (cancelled) {
                *cancelled = true;
            }
            return group;
        }

        const std::vector<std::pair<size_t, size_t>> matches =
            find_pattern_matches(document.text, query, cancelled, cancellable, error_message);
        if ((cancelled && *cancelled) || (error_message && !error_message->empty())) {
            return group;
        }
        if (matches.empty()) {
            continue;
        }

        SearchFileResultData file_result;
        file_result.docIndex = document.docIndex;
        file_result.documentName = document.documentName;

        size_t line_start = 0;
        size_t line_end = document.text.find('\n');
        int line_number = 0;

        for (const auto &match : matches) {
            while (line_start < document.text.size() && line_end != std::string::npos &&
                   match.first > line_end) {
                line_start = line_end + 1;
                line_end = document.text.find('\n', line_start);
                ++line_number;
            }

            const size_t effective_line_end =
                line_end == std::string::npos ? document.text.size() : line_end;
            std::string preview = document.text.substr(
                line_start, effective_line_end > line_start ? effective_line_end - line_start : 0);
            preview = sanitize_preview_text(preview);

            SearchResultMatchData match_data;
            match_data.start = static_cast<int>(match.first);
            match_data.end = static_cast<int>(match.second);
            match_data.line = line_number;
            match_data.preview = std::move(preview);
            file_result.matches.push_back(std::move(match_data));
        }

        group.resultCount += static_cast<int>(file_result.matches.size());
        ++group.fileCount;
        group.files.push_back(std::move(file_result));
    }

    return group;
}

void SearchManager::destroy_async_search_request(gpointer data) {
    delete static_cast<AsyncSearchRequest *>(data);
}

void SearchManager::destroy_async_search_response(gpointer data) {
    delete static_cast<AsyncSearchResponse *>(data);
}

void SearchManager::destroy_async_search_callback_context(gpointer data) {
    delete static_cast<AsyncSearchCallbackContext *>(data);
}

void SearchManager::run_async_search_task(GTask *task, gpointer source_object,
                                          gpointer task_data, GCancellable *cancellable) {
    (void)source_object;
    AsyncSearchRequest *request = static_cast<AsyncSearchRequest *>(task_data);
    AsyncSearchResponse *response = new AsyncSearchResponse();
    response->requestId = request->requestId;
    response->clearExisting = request->clearExisting;
    response->expandNewGroup = request->expandNewGroup;
    response->appendResults = request->appendResults;
    response->expandedGroups = request->expandedGroups;

    for (const SearchQuery &query : request->queries) {
        bool cancelled = false;
        std::string error_message;
        SearchGroupResultData group = build_search_group_result(
            query, request->currentDocumentIndex, request->documents, &cancelled, cancellable,
            &error_message);
        if (cancelled) {
            response->cancelled = true;
            break;
        }
        if (!error_message.empty()) {
            response->errorMessage = error_message;
            break;
        }
        response->groups.push_back(std::move(group));
    }

    if (!response->cancelled && response->errorMessage.empty()) {
        const size_t group_count = response->groups.size();
        if (group_count > 1) {
            response->statusMessage =
                std::string(Localization::text("search.refreshed_prefix")) +
                std::to_string(group_count) +
                Localization::text("search.refreshed_suffix");
        }
    }

    g_task_return_pointer(task, response, destroy_async_search_response);
}

void SearchManager::apply_search_group_result(const SearchGroupResultData &group, GtkTreeStore *store,
                                              GtkWidget *status_label, bool expand_new_group,
                                              bool append_result) {
    if (!store || !status_label) {
        return;
    }

    GtkTreeIter search_iter;
    if (append_result) {
        gtk_tree_store_append(store, &search_iter, nullptr);
    } else {
        gtk_tree_store_prepend(store, &search_iter, nullptr);
    }
    gtk_tree_store_set(store, &search_iter,
                       SearchResultFile, group.query.pattern.c_str(),
                       SearchResultText, "",
                       SearchResultStart, -1,
                       SearchResultEnd, -1,
                       SearchResultDocIndex, -1,
                       SearchResultIsGroup, TRUE,
                       SearchResultPattern, group.query.pattern.c_str(),
                       SearchResultCaseSensitive, group.query.caseSensitive,
                       SearchResultWholeWord, group.query.wholeWord,
                       SearchResultRegex, group.query.regex,
                       SearchResultSearchAllDocs, group.query.searchAllDocuments,
                       -1);

    for (const SearchFileResultData &file : group.files) {
        GtkTreeIter file_iter;
        gtk_tree_store_append(store, &file_iter, &search_iter);
        std::string grouped_label =
            file.documentName + " (" + std::to_string(file.matches.size()) +
            (file.matches.size() == 1 ? Localization::text("search.match_one")
                                      : Localization::text("search.match_many"));
        gtk_tree_store_set(store, &file_iter,
                           SearchResultFile, grouped_label.c_str(),
                           SearchResultText, "",
                           SearchResultStart, -1,
                           SearchResultEnd, -1,
                           SearchResultDocIndex, file.docIndex,
                           SearchResultIsGroup, TRUE,
                           SearchResultPattern, group.query.pattern.c_str(),
                           SearchResultCaseSensitive, group.query.caseSensitive,
                           SearchResultWholeWord, group.query.wholeWord,
                           SearchResultRegex, group.query.regex,
                           SearchResultSearchAllDocs, group.query.searchAllDocuments,
                           -1);

        for (const SearchResultMatchData &match : file.matches) {
            GtkTreeIter result_iter;
            gtk_tree_store_append(store, &result_iter, &file_iter);
            std::string result_text =
                std::string(Localization::text("search.line_prefix")) +
                std::to_string(match.line + 1) +
                Localization::text("search.line_separator") + match.preview;
            gtk_tree_store_set(store, &result_iter,
                               SearchResultFile, result_text.c_str(),
                               SearchResultText, result_text.c_str(),
                               SearchResultStart, match.start,
                               SearchResultEnd, match.end,
                               SearchResultDocIndex, file.docIndex,
                               SearchResultIsGroup, FALSE,
                               SearchResultPattern, group.query.pattern.c_str(),
                               SearchResultCaseSensitive, group.query.caseSensitive,
                               SearchResultWholeWord, group.query.wholeWord,
                               SearchResultRegex, group.query.regex,
                               SearchResultSearchAllDocs, group.query.searchAllDocuments,
                               -1);
        }
    }

    std::string search_group_label =
        std::string(Localization::text("search.label_prefix")) +
        group.query.pattern + Localization::text("search.label_middle_one") +
        std::to_string(group.resultCount) +
        (group.resultCount == 1 ? Localization::text("search.hit_one")
                                : Localization::text("search.hit_many")) +
        std::to_string(group.fileCount) +
        (group.fileCount == 1 ? Localization::text("search.file_one")
                              : Localization::text("search.file_many"));
    gtk_tree_store_set(store, &search_iter, SearchResultFile, search_group_label.c_str(), -1);

    if (group.resultCount == 0) {
        gtk_label_set_text(GTK_LABEL(status_label), search_group_label.c_str());
    } else {
        std::string status =
            std::string(Localization::text("search.showing_prefix")) +
            search_group_label;
        gtk_label_set_text(GTK_LABEL(status_label), status.c_str());
    }

    GtkWidget *tree_view = store == find_results_store ? find_results_view : replace_results_view;
    if (expand_new_group && tree_view) {
        GtkTreePath *search_path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &search_iter);
        gtk_tree_view_expand_row(GTK_TREE_VIEW(tree_view), search_path, FALSE);
        gtk_tree_view_expand_row(GTK_TREE_VIEW(tree_view), search_path, TRUE);
        gtk_tree_path_free(search_path);
    }
}

void SearchManager::apply_async_search_response(const AsyncSearchResponse &response) {
    if (response.requestId != active_async_search_request_id) {
        return;
    }

    set_search_in_progress_state(false);
    if (active_search_cancellable) {
        g_object_unref(active_search_cancellable);
        active_search_cancellable = nullptr;
    }

    if (response.cancelled) {
        return;
    }

    if (!response.errorMessage.empty()) {
        if (find_results_status_label) {
            gtk_label_set_text(GTK_LABEL(find_results_status_label), response.errorMessage.c_str());
        }
        return;
    }

    if (response.clearExisting && find_results_store) {
        gtk_tree_store_clear(find_results_store);
    }

    if (response.appendResults) {
        for (const SearchGroupResultData &group : response.groups) {
            apply_search_group_result(group, find_results_store,
                                      find_results_status_label,
                                      response.expandNewGroup, true);
        }
    } else {
        for (auto group = response.groups.rbegin(); group != response.groups.rend(); ++group) {
            apply_search_group_result(*group, find_results_store,
                                      find_results_status_label,
                                      response.expandNewGroup, false);
        }
    }

    if (!response.expandedGroups.empty()) {
        restore_expanded_result_groups(response.expandedGroups);
    }

    if (find_results_status_label && !response.statusMessage.empty()) {
        gtk_label_set_text(GTK_LABEL(find_results_status_label), response.statusMessage.c_str());
    }
    show_search_results_panel();
}

void SearchManager::on_async_search_task_complete(GObject *source_object, GAsyncResult *result,
                                                  gpointer user_data) {
    (void)source_object;
    AsyncSearchCallbackContext *context = static_cast<AsyncSearchCallbackContext *>(user_data);
    GTask *task = G_TASK(result);
    AsyncSearchResponse *response = static_cast<AsyncSearchResponse *>(g_task_propagate_pointer(task, nullptr));

    if (!context->lifetime.expired() && context->manager && response) {
        context->manager->apply_async_search_response(*response);
    }

    destroy_async_search_callback_context(context);
}

std::vector<SearchManager::SearchQuery> SearchManager::collect_search_queries_from_results() const {
    std::vector<SearchQuery> queries;
    if (!find_results_store) {
        return queries;
    }

    GtkTreeModel *model = GTK_TREE_MODEL(find_results_store);
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(model, &iter);
    while (valid) {
        int doc_index = -1;
        gboolean is_group = FALSE;
        gchar *pattern = nullptr;
        gboolean case_sensitive = FALSE;
        gboolean whole_word = FALSE;
        gboolean regex = FALSE;
        gboolean search_all_docs = FALSE;

        gtk_tree_model_get(model, &iter,
                           SearchResultDocIndex, &doc_index,
                           SearchResultIsGroup, &is_group,
                           SearchResultPattern, &pattern,
                           SearchResultCaseSensitive, &case_sensitive,
                           SearchResultWholeWord, &whole_word,
                           SearchResultRegex, &regex,
                           SearchResultSearchAllDocs, &search_all_docs,
                           -1);

        if (is_group && doc_index == -1 && pattern && *pattern) {
            SearchQuery query;
            query.pattern = pattern;
            query.caseSensitive = case_sensitive;
            query.wholeWord = whole_word;
            query.regex = regex;
            query.searchAllDocuments = search_all_docs;
            queries.push_back(query);
        }

        g_free(pattern);
        valid = gtk_tree_model_iter_next(model, &iter);
    }

    return queries;
}

std::string SearchManager::search_query_key(const SearchQuery &query) {
    return query.pattern + "|" +
           (query.caseSensitive ? "1" : "0") + "|" +
           (query.wholeWord ? "1" : "0") + "|" +
           (query.regex ? "1" : "0") + "|" +
           (query.searchAllDocuments ? "1" : "0");
}

std::string SearchManager::file_group_key(const SearchQuery &query, int docIndex) {
    return search_query_key(query) + "|" + std::to_string(docIndex);
}

std::set<std::string> SearchManager::collect_expanded_result_groups() const {
    std::set<std::string> expandedGroups;
    if (!find_results_store || !find_results_view) {
        return expandedGroups;
    }

    GtkTreeModel *model = GTK_TREE_MODEL(find_results_store);
    GtkTreeIter searchIter;
    gboolean hasSearch = gtk_tree_model_get_iter_first(model, &searchIter);
    while (hasSearch) {
        GtkTreePath *searchPath = gtk_tree_model_get_path(model, &searchIter);
        SearchQuery query;
        gchar *pattern = nullptr;
        gboolean caseSensitive = FALSE;
        gboolean wholeWord = FALSE;
        gboolean regex = FALSE;
        gboolean searchAllDocs = FALSE;
        gtk_tree_model_get(model, &searchIter,
                           SearchResultPattern, &pattern,
                           SearchResultCaseSensitive, &caseSensitive,
                           SearchResultWholeWord, &wholeWord,
                           SearchResultRegex, &regex,
                           SearchResultSearchAllDocs, &searchAllDocs,
                           -1);
        query.pattern = pattern ? pattern : "";
        query.caseSensitive = caseSensitive;
        query.wholeWord = wholeWord;
        query.regex = regex;
        query.searchAllDocuments = searchAllDocs;
        g_free(pattern);

        if (gtk_tree_view_row_expanded(GTK_TREE_VIEW(find_results_view), searchPath)) {
            expandedGroups.insert(search_query_key(query));
        }

        GtkTreeIter fileIter;
        gboolean hasFile = gtk_tree_model_iter_children(model, &fileIter, &searchIter);
        while (hasFile) {
            GtkTreePath *filePath = gtk_tree_model_get_path(model, &fileIter);
            int docIndex = -1;
            gboolean isGroup = FALSE;
            gtk_tree_model_get(model, &fileIter,
                               SearchResultDocIndex, &docIndex,
                               SearchResultIsGroup, &isGroup,
                               -1);
            if (isGroup && docIndex >= 0 &&
                gtk_tree_view_row_expanded(GTK_TREE_VIEW(find_results_view), filePath)) {
                expandedGroups.insert(file_group_key(query, docIndex));
            }
            gtk_tree_path_free(filePath);
            hasFile = gtk_tree_model_iter_next(model, &fileIter);
        }

        gtk_tree_path_free(searchPath);
        hasSearch = gtk_tree_model_iter_next(model, &searchIter);
    }

    return expandedGroups;
}

void SearchManager::restore_expanded_result_groups(const std::set<std::string> &expandedGroups) {
    if (!find_results_store || !find_results_view) {
        return;
    }

    GtkTreeModel *model = GTK_TREE_MODEL(find_results_store);
    GtkTreeIter searchIter;
    gboolean hasSearch = gtk_tree_model_get_iter_first(model, &searchIter);
    while (hasSearch) {
        SearchQuery query;
        gchar *pattern = nullptr;
        gboolean caseSensitive = FALSE;
        gboolean wholeWord = FALSE;
        gboolean regex = FALSE;
        gboolean searchAllDocs = FALSE;
        gtk_tree_model_get(model, &searchIter,
                           SearchResultPattern, &pattern,
                           SearchResultCaseSensitive, &caseSensitive,
                           SearchResultWholeWord, &wholeWord,
                           SearchResultRegex, &regex,
                           SearchResultSearchAllDocs, &searchAllDocs,
                           -1);
        query.pattern = pattern ? pattern : "";
        query.caseSensitive = caseSensitive;
        query.wholeWord = wholeWord;
        query.regex = regex;
        query.searchAllDocuments = searchAllDocs;
        g_free(pattern);

        GtkTreePath *searchPath = gtk_tree_model_get_path(model, &searchIter);
        if (expandedGroups.count(search_query_key(query)) > 0) {
            gtk_tree_view_expand_row(GTK_TREE_VIEW(find_results_view), searchPath, FALSE);
        }

        GtkTreeIter fileIter;
        gboolean hasFile = gtk_tree_model_iter_children(model, &fileIter, &searchIter);
        while (hasFile) {
            int docIndex = -1;
            gboolean isGroup = FALSE;
            gtk_tree_model_get(model, &fileIter,
                               SearchResultDocIndex, &docIndex,
                               SearchResultIsGroup, &isGroup,
                               -1);
            if (isGroup && docIndex >= 0 &&
                expandedGroups.count(file_group_key(query, docIndex)) > 0) {
                GtkTreePath *filePath = gtk_tree_model_get_path(model, &fileIter);
                gtk_tree_view_expand_row(GTK_TREE_VIEW(find_results_view), filePath, FALSE);
                gtk_tree_path_free(filePath);
            }
            hasFile = gtk_tree_model_iter_next(model, &fileIter);
        }

        gtk_tree_path_free(searchPath);
        hasSearch = gtk_tree_model_iter_next(model, &searchIter);
    }
}

void SearchManager::jump_to_search_result(GtkTreeView *tree_view, GtkTreePath *path) {
    GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
    GtkTreeIter iter;
    if (!gtk_tree_model_get_iter(model, &iter, path)) {
        return;
    }

    int start = 0;
    int end = 0;
    int doc_index = -1;
    gboolean is_group = FALSE;

    gtk_tree_model_get(model, &iter,
                       SearchResultStart, &start,
                       SearchResultEnd, &end,
                       SearchResultDocIndex, &doc_index,
                       SearchResultIsGroup, &is_group,
                       -1);

    if (is_group) {
        GtkTreePath *tree_path = gtk_tree_model_get_path(model, &iter);
        if (gtk_tree_view_row_expanded(tree_view, tree_path)) {
            gtk_tree_view_collapse_row(tree_view, tree_path);
        } else {
            gtk_tree_view_expand_row(tree_view, tree_path, FALSE);
        }
        gtk_tree_path_free(tree_path);
        return;
    }

    if (doc_index >= 0) {
        editorWindow->switchToDocument(doc_index);
    }

    GtkWidget *scintilla = editorWindow->getCurrentScintilla();
    if (!scintilla) {
        return;
    }

    scintilla_send_message(SCINTILLA(scintilla), SCI_SETSEL, start, end);
    scintilla_send_message(SCINTILLA(scintilla), SCI_SCROLLCARET, 0, 0);
    gtk_widget_grab_focus(scintilla);
}

void SearchManager::on_file_cell_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer,
                                      GtkTreeModel *model, GtkTreeIter *iter, gpointer data) {
    (void)column;
    (void)data;

    gchar *text = nullptr;
    gboolean is_group = FALSE;
    gtk_tree_model_get(model, iter,
                       SearchResultFile, &text,
                       SearchResultIsGroup, &is_group,
                       -1);

    GtkTreePath *path = gtk_tree_model_get_path(model, iter);
    const int depth = path ? gtk_tree_path_get_depth(path) : 0;
    if (path) {
        gtk_tree_path_free(path);
    }

    const bool is_search_group = is_group && depth == 1;
    const bool is_file_group = is_group && depth > 1;

    g_object_set(renderer,
                 "text", text ? text : "",
                 "weight", is_search_group ? PANGO_WEIGHT_HEAVY :
                           (is_file_group ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL),
                 "foreground", is_search_group ? "#111827" :
                               (is_file_group ? "#1f2933" : "#374151"),
                 "style", PANGO_STYLE_NORMAL,
                 NULL);

    g_free(text);
}

std::string SearchManager::sanitize_preview_text(const std::string &text) {
    std::string result;
    for (char c : text) {
        if (c == '\r' || c == '\n') {
            break;
        }
        if (c == '\t') {
            result += ' ';
        } else {
            result += c;
        }
    }
    return result;
}

bool SearchManager::perform_search(const std::string &pattern, bool forward,
                                   bool case_sensitive, bool whole_word, bool regex) {
    if (pattern.empty()) {
        return false;
    }

    GtkWidget *scintilla = editorWindow->getCurrentScintilla();
    if (!scintilla) {
        return false;
    }

    // Get current position
    sptr_t current_pos = scintilla_send_message(SCINTILLA(scintilla), SCI_GETCURRENTPOS, 0, 0);
    const sptr_t doc_length = scintilla_send_message(SCINTILLA(scintilla), SCI_GETLENGTH, 0, 0);

    // Set search flags
    int flags = 0;
    if (case_sensitive) flags |= SCFIND_MATCHCASE;
    if (whole_word) flags |= SCFIND_WHOLEWORD;
    if (regex) flags |= SCFIND_REGEXP;

    scintilla_send_message(SCINTILLA(scintilla), SCI_SETSEARCHFLAGS, flags, 0);

    sptr_t found_pos = -1;
    if (forward) {
        // Search forward from current position
        scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETSTART, current_pos, 0);
        scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETEND, doc_length, 0);
        found_pos = scintilla_send_message(SCINTILLA(scintilla), SCI_SEARCHINTARGET, pattern.length(),
                       reinterpret_cast<sptr_t>(pattern.c_str()));

        // If not found and wrap around is enabled, search from beginning
        if (found_pos == -1 && wrap_around_check && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wrap_around_check))) {
            scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETSTART, 0, 0);
            scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETEND, current_pos, 0);
            found_pos = scintilla_send_message(SCINTILLA(scintilla), SCI_SEARCHINTARGET, pattern.length(),
                           reinterpret_cast<sptr_t>(pattern.c_str()));
        }
    } else {
        // Search backward
        scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETSTART, current_pos, 0);
        scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETEND, 0, 0);
        found_pos = scintilla_send_message(SCINTILLA(scintilla), SCI_SEARCHINTARGET, pattern.length(),
                       reinterpret_cast<sptr_t>(pattern.c_str()));

        // If not found and wrap around is enabled, search from end
        if (found_pos == -1 && wrap_around_check && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wrap_around_check))) {
            scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETSTART, doc_length, 0);
            scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETEND, current_pos, 0);
            found_pos = scintilla_send_message(SCINTILLA(scintilla), SCI_SEARCHINTARGET, pattern.length(),
                           reinterpret_cast<sptr_t>(pattern.c_str()));
        }
    }

    if (found_pos != -1) {
        // Select the found text
        sptr_t start = scintilla_send_message(SCINTILLA(scintilla), SCI_GETTARGETSTART, 0, 0);
        sptr_t end = scintilla_send_message(SCINTILLA(scintilla), SCI_GETTARGETEND, 0, 0);
        scintilla_send_message(SCINTILLA(scintilla), SCI_SETSEL, start, end);
        scintilla_send_message(SCINTILLA(scintilla), SCI_SCROLLCARET, 0, 0);
        return true;
    }

    return false;
}

void SearchManager::perform_replace(const std::string &find_pattern, const std::string &replace_text,
                                    bool case_sensitive, bool whole_word, bool regex,
                                    bool replace_all) {
    if (find_pattern.empty()) {
        return;
    }

    GtkWidget *scintilla = editorWindow->getCurrentScintilla();
    if (!scintilla) {
        return;
    }

    // Get current selection
    sptr_t sel_start = scintilla_send_message(SCINTILLA(scintilla), SCI_GETSELECTIONSTART, 0, 0);
    sptr_t sel_end = scintilla_send_message(SCINTILLA(scintilla), SCI_GETSELECTIONEND, 0, 0);

    // Set search flags
    int flags = 0;
    if (case_sensitive) flags |= SCFIND_MATCHCASE;
    if (whole_word) flags |= SCFIND_WHOLEWORD;
    if (regex) flags |= SCFIND_REGEXP;

    scintilla_send_message(SCINTILLA(scintilla), SCI_SETSEARCHFLAGS, flags, 0);

    if (replace_all) {
        // Replace all occurrences
        sptr_t pos = 0;
        int replacements = 0;
        sptr_t doc_length = scintilla_send_message(SCINTILLA(scintilla), SCI_GETLENGTH, 0, 0);

        while (pos < doc_length) {
            scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETSTART, pos, 0);
            scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETEND, doc_length, 0);

            sptr_t found_pos = scintilla_send_message(SCINTILLA(scintilla), SCI_SEARCHINTARGET, find_pattern.length(),
                                  reinterpret_cast<sptr_t>(find_pattern.c_str()));

            if (found_pos == -1) {
                break;
            }

            sptr_t start = scintilla_send_message(SCINTILLA(scintilla), SCI_GETTARGETSTART, 0, 0);
            sptr_t end = scintilla_send_message(SCINTILLA(scintilla), SCI_GETTARGETEND, 0, 0);

            scintilla_send_message(SCINTILLA(scintilla), SCI_SETSEL, start, end);
            scintilla_send_message(SCINTILLA(scintilla), SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(replace_text.c_str()));

            pos = start + replace_text.length();
            replacements++;
            doc_length = scintilla_send_message(SCINTILLA(scintilla), SCI_GETLENGTH, 0, 0);
        }
    } else {
        // Replace current selection if it matches
        std::string selected_text;
        if (sel_start != sel_end) {
            sptr_t length = sel_end - sel_start;
            char *buffer = new char[length + 1];
            scintilla_send_message(SCINTILLA(scintilla), SCI_GETSELTEXT, 0, reinterpret_cast<sptr_t>(buffer));
            selected_text = buffer;
            delete[] buffer;
        }

        // Check if selection matches the find pattern
        bool matches = false;
        if (regex) {
            try {
                std::regex reg(find_pattern, case_sensitive ? std::regex_constants::ECMAScript : std::regex_constants::icase);
                matches = std::regex_match(selected_text, reg);
            } catch (const std::regex_error&) {
                matches = false;
            }
        } else {
            if (case_sensitive) {
                matches = (selected_text == find_pattern);
            } else {
                matches = (g_ascii_strcasecmp(selected_text.c_str(), find_pattern.c_str()) == 0);
            }
        }

        if (matches) {
            scintilla_send_message(SCINTILLA(scintilla), SCI_REPLACESEL, 0, reinterpret_cast<sptr_t>(replace_text.c_str()));
        } else {
            // Find next occurrence
            perform_search(find_pattern, true, case_sensitive, whole_word, regex);
        }
    }
}

// GTK Callbacks
void SearchManager::on_find_next_clicked(GtkEntry *entry, gpointer data) {
    (void)entry;
    SearchManager *self = static_cast<SearchManager *>(data);

    const char *pattern = gtk_entry_get_text(GTK_ENTRY(self->find_entry));
    if (!pattern || !*pattern) {
        return;
    }

    bool case_sensitive = self->case_sensitive_check &&
                         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->case_sensitive_check));
    bool whole_word = self->whole_word_check &&
                     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->whole_word_check));
    bool regex = self->regex_check &&
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->regex_check));

    self->find_next(pattern, case_sensitive, whole_word, regex);
}

void SearchManager::on_find_previous_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SearchManager *self = static_cast<SearchManager *>(data);

    const char *pattern = gtk_entry_get_text(GTK_ENTRY(self->find_entry));
    if (!pattern || !*pattern) {
        return;
    }

    bool case_sensitive = self->case_sensitive_check &&
                         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->case_sensitive_check));
    bool whole_word = self->whole_word_check &&
                     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->whole_word_check));
    bool regex = self->regex_check &&
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->regex_check));

    self->find_previous(pattern, case_sensitive, whole_word, regex);
}

void SearchManager::on_replace_next_clicked(GtkEntry *entry, gpointer data) {
    (void)entry;
    SearchManager *self = static_cast<SearchManager *>(data);

    const char *find_pattern = gtk_entry_get_text(GTK_ENTRY(self->find_entry));
    const char *replace_text = gtk_entry_get_text(GTK_ENTRY(self->replace_entry));

    if (!find_pattern || !*find_pattern) {
        return;
    }

    bool case_sensitive = self->case_sensitive_check &&
                         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->case_sensitive_check));
    bool whole_word = self->whole_word_check &&
                     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->whole_word_check));
    bool regex = self->regex_check &&
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->regex_check));

    self->replace_next(find_pattern, replace_text, case_sensitive, whole_word, regex);
}

void SearchManager::on_replace_all_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SearchManager *self = static_cast<SearchManager *>(data);

    const char *find_pattern = gtk_entry_get_text(GTK_ENTRY(self->find_entry));
    const char *replace_text = gtk_entry_get_text(GTK_ENTRY(self->replace_entry));

    if (!find_pattern || !*find_pattern) {
        return;
    }

    bool case_sensitive = self->case_sensitive_check &&
                         gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->case_sensitive_check));
    bool whole_word = self->whole_word_check &&
                     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->whole_word_check));
    bool regex = self->regex_check &&
                gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->regex_check));

    self->replace_all(find_pattern, replace_text, case_sensitive, whole_word, regex);
}

void SearchManager::on_find_close_clicked(GtkDialog *dialog, gint response_id, gpointer data) {
    if (response_id == GTK_RESPONSE_OK) {
        on_find_next_clicked(nullptr, data);
        g_signal_stop_emission_by_name(dialog, "response");
        return;
    }
    if (response_id == GTK_RESPONSE_APPLY) {
        on_find_all_clicked(nullptr, data);
        gtk_widget_hide(GTK_WIDGET(dialog));
        g_signal_stop_emission_by_name(dialog, "response");
        return;
    }

    gtk_widget_hide(GTK_WIDGET(dialog));
    g_signal_stop_emission_by_name(dialog, "response");
}

void SearchManager::on_replace_close_clicked(GtkDialog *dialog, gint response_id, gpointer data) {
    if (response_id == GTK_RESPONSE_OK) {
        // Find Next
        on_find_next_clicked(nullptr, data);
    } else if (response_id == GTK_RESPONSE_APPLY) {
        // Replace Next
        on_replace_next_clicked(nullptr, data);
    } else if (response_id == GTK_RESPONSE_YES) {
        // Replace All
        on_replace_all_clicked(nullptr, data);
    }

    if (response_id != GTK_RESPONSE_OK &&
        response_id != GTK_RESPONSE_APPLY &&
        response_id != GTK_RESPONSE_YES) {
        gtk_widget_hide(GTK_WIDGET(dialog));
    }

    g_signal_stop_emission_by_name(dialog, "response");
}

gboolean SearchManager::on_dialog_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data) {
    (void)event;
    (void)data;
    gtk_widget_hide(widget);
    return TRUE;
}

void SearchManager::on_search_results_toggled(GtkToggleButton *button, gpointer data) {
    SearchManager *self = static_cast<SearchManager *>(data);
    gboolean is_expanded = gtk_toggle_button_get_active(button);

    if (!self->editorWindow || !self->editorWindow->getSearchResultsPanel() ||
        !self->editorWindow->getEditorPane()) {
        return;
    }

    if (is_expanded) {
        gtk_widget_set_visible(self->editorWindow->getSearchResultsPanel(), TRUE);
        if (self->search_results_content) {
            gtk_widget_set_visible(self->search_results_content, TRUE);
        }
        if (self->search_results_pane_position >= 0) {
            gtk_paned_set_position(GTK_PANED(self->editorWindow->getEditorPane()),
                                   self->search_results_pane_position);
        }
        return;
    }

    self->search_results_pane_position =
        gtk_paned_get_position(GTK_PANED(self->editorWindow->getEditorPane()));

    gint min_height = 0;
    gint nat_height = 0;
    gtk_widget_get_preferred_height(self->search_results_header_bar, &min_height, &nat_height);

    if (self->search_results_content) {
        gtk_widget_set_visible(self->search_results_content, FALSE);
    }
    gtk_widget_set_visible(self->editorWindow->getSearchResultsPanel(), FALSE);

    gint paned_height = gtk_widget_get_allocated_height(self->editorWindow->getEditorPane());
    if (paned_height > 0) {
        gtk_paned_set_position(GTK_PANED(self->editorWindow->getEditorPane()),
                               std::max(0, paned_height - nat_height));
    }
}

void SearchManager::on_find_dialog_destroyed(GtkWidget *widget, gpointer data) {
    (void)widget;
    SearchManager *self = static_cast<SearchManager *>(data);
    self->find_dialog = nullptr;
    self->find_entry = nullptr;
    self->case_sensitive_check = nullptr;
    self->whole_word_check = nullptr;
    self->regex_check = nullptr;
    self->wrap_around_check = nullptr;
    self->search_all_docs_check = nullptr;
}

void SearchManager::on_replace_dialog_destroyed(GtkWidget *widget, gpointer data) {
    (void)widget;
    SearchManager *self = static_cast<SearchManager *>(data);
    self->replace_dialog = nullptr;
    self->find_entry = nullptr;
    self->replace_entry = nullptr;
    self->case_sensitive_check = nullptr;
    self->whole_word_check = nullptr;
    self->regex_check = nullptr;
    self->wrap_around_check = nullptr;
    self->search_all_docs_check = nullptr;
}

void SearchManager::on_refresh_results_clicked(GtkButton *button, gpointer data) {
    (void)button;
    SearchManager *self = static_cast<SearchManager *>(data);
    self->refresh_search_results();
}

void SearchManager::on_auto_refresh_toggled(GtkToggleButton *button, gpointer data) {
    (void)button;
    SearchManager *self = static_cast<SearchManager *>(data);
    self->auto_refresh_enabled = gtk_toggle_button_get_active(button);
    if (self->auto_refresh_enabled && self->has_last_search()) {
        self->schedule_search_results_refresh();
    }
}
