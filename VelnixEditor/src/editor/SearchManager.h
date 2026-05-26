#pragma once

#include "core/EditorTypes.h"

#include <gio/gio.h>
#include <gtk/gtk.h>
#include <pango/pango.h>
#include <memory>
#include <set>
#include <string>
#include <vector>

class EditorWindow;

class SearchManager {
public:
    explicit SearchManager(EditorWindow *editorWindow);
    ~SearchManager();

    // Dialog management
    void show_find_dialog();
    void show_replace_dialog();
    void show_go_to_line_dialog();
    void create_search_results_panel(GtkWidget *parent);
    void show_search_results_panel();
    void hide_search_results_panel();
    bool is_search_results_panel_visible() const;
    void refresh_localized_ui();

    // Search operations
    void find_next(const std::string &pattern, bool case_sensitive = false,
                   bool whole_word = false, bool regex = false);
    void find_previous(const std::string &pattern, bool case_sensitive = false,
                       bool whole_word = false, bool regex = false);
    void find_all(const std::string &pattern, bool case_sensitive = false,
                  bool whole_word = false, bool regex = false);
    void find_all_batch(const std::vector<MacroSearchRequest> &requests);
    void replace_next(const std::string &find_pattern, const std::string &replace_text,
                      bool case_sensitive = false, bool whole_word = false, bool regex = false);
    void replace_all(const std::string &find_pattern, const std::string &replace_text,
                     bool case_sensitive = false, bool whole_word = false, bool regex = false);

    // Selection operations
    void select_all_occurrences(const std::string &pattern, bool case_sensitive = false,
                                bool whole_word = false, bool regex = false);

    // Auto-refresh operations
    bool is_auto_refresh_enabled() const;
    bool has_last_search() const;
    void refresh_search_results();
    void schedule_search_results_refresh();

private:
    struct SearchQuery {
        std::string pattern;
        bool caseSensitive = false;
        bool wholeWord = false;
        bool regex = false;
        bool searchAllDocuments = false;
    };

    struct SearchResultMatchData {
        int start = 0;
        int end = 0;
        int line = 0;
        std::string preview;
    };

    struct SearchFileResultData {
        int docIndex = -1;
        std::string documentName;
        std::vector<SearchResultMatchData> matches;
    };

    struct SearchGroupResultData {
        SearchQuery query;
        int resultCount = 0;
        int fileCount = 0;
        std::vector<SearchFileResultData> files;
    };

    struct SearchDocumentSnapshot {
        int docIndex = -1;
        std::string documentName;
        std::string text;
    };

    struct AsyncSearchRequest {
        guint requestId = 0;
        int currentDocumentIndex = -1;
        bool clearExisting = false;
        bool expandNewGroup = true;
        bool appendResults = false;
        std::vector<SearchQuery> queries;
        std::set<std::string> expandedGroups;
        std::vector<SearchDocumentSnapshot> documents;
    };

    struct AsyncSearchResponse {
        guint requestId = 0;
        bool clearExisting = false;
        bool expandNewGroup = true;
        bool appendResults = false;
        bool cancelled = false;
        std::vector<SearchGroupResultData> groups;
        std::set<std::string> expandedGroups;
        std::string statusMessage;
        std::string errorMessage;
    };

    struct AsyncSearchCallbackContext {
        SearchManager *manager = nullptr;
        std::weak_ptr<int> lifetime;
    };

    enum SearchResultColumns {
        SearchResultFile = 0,
        SearchResultText,
        SearchResultStart,
        SearchResultEnd,
        SearchResultDocIndex,
        SearchResultIsGroup,
        SearchResultPattern,
        SearchResultCaseSensitive,
        SearchResultWholeWord,
        SearchResultRegex,
        SearchResultSearchAllDocs,
        SearchResultColumnCount
    };

    EditorWindow *editorWindow;

    // Dialog widgets
    GtkWidget *find_dialog = nullptr;
    GtkWidget *replace_dialog = nullptr;

    // Dialog controls
    GtkWidget *find_entry = nullptr;
    GtkWidget *replace_entry = nullptr;
    GtkWidget *case_sensitive_check = nullptr;
    GtkWidget *whole_word_check = nullptr;
    GtkWidget *regex_check = nullptr;
    GtkWidget *wrap_around_check = nullptr;
    GtkWidget *search_all_docs_check = nullptr;
    GtkWidget *find_results_view = nullptr;
    GtkWidget *replace_results_view = nullptr;
    GtkWidget *find_results_status_label = nullptr;
    GtkWidget *replace_results_status_label = nullptr;
    GtkWidget *search_results_header_bar = nullptr;
    GtkWidget *search_results_toggle_button = nullptr;
    GtkWidget *search_results_content = nullptr;
    GtkWidget *refresh_results_button = nullptr;
    GtkWidget *auto_refresh_toggle = nullptr;
    GtkTreeStore *find_results_store = nullptr;
    GtkTreeStore *replace_results_store = nullptr;
    int search_results_pane_position = 520;
    bool auto_refresh_enabled = false;
    guint search_results_refresh_idle_id = 0;
    guint active_async_search_request_id = 0;
    bool search_in_progress = false;
    GCancellable *active_search_cancellable = nullptr;
    std::shared_ptr<int> async_search_lifetime = std::make_shared<int>(0);
    std::string last_search_pattern;
    bool last_search_case_sensitive = false;
    bool last_search_whole_word = false;
    bool last_search_regex = false;
    bool last_search_all_documents = false;

    // Helper methods
    void sync_find_entry_with_selection();
    std::string get_current_selection_text() const;
    void create_find_dialog();
    void create_replace_dialog();
    GtkWidget *create_results_panel(GtkWidget **view_out, GtkWidget **status_label_out,
                                    GtkTreeStore **store_out);
    void clear_search_results();
    void expand_all_search_results();
    void collapse_all_search_results();
    bool perform_search(const std::string &pattern, bool forward = true,
                        bool case_sensitive = false, bool whole_word = false, bool regex = false);
    void delete_selected_search_results(GtkTreeView *tree_view);
    std::vector<SearchQuery> collect_search_queries_from_results() const;
    std::set<std::string> collect_expanded_result_groups() const;
    void restore_expanded_result_groups(const std::set<std::string> &expandedGroups);
    static std::string search_query_key(const SearchQuery &query);
    static std::string file_group_key(const SearchQuery &query, int docIndex);
    std::vector<SearchDocumentSnapshot> snapshot_search_documents(bool search_all_docs) const;
    void start_async_search(const std::vector<SearchQuery> &queries, bool clear_existing,
                            bool expand_new_group,
                            const std::set<std::string> &expanded_groups = {},
                            bool append_results = false);
    void cancel_active_async_search();
    void apply_search_group_result(const SearchGroupResultData &group, GtkTreeStore *store,
                                   GtkWidget *status_label, bool expand_new_group,
                                   bool append_result);
    void apply_async_search_response(const AsyncSearchResponse &response);
    void set_search_in_progress_state(bool in_progress, const std::string &status_message = "");
    static std::vector<std::pair<size_t, size_t>> find_pattern_matches(
        const std::string &text, const SearchQuery &query, bool *cancelled, GCancellable *cancellable,
        std::string *error_message);
    static SearchGroupResultData build_search_group_result(
        const SearchQuery &query, int current_document_index,
        const std::vector<SearchDocumentSnapshot> &documents, bool *cancelled,
        GCancellable *cancellable, std::string *error_message);
    static gboolean is_word_boundary(const std::string &text, size_t index);
    static void destroy_async_search_request(gpointer data);
    static void destroy_async_search_response(gpointer data);
    static void destroy_async_search_callback_context(gpointer data);
    static void run_async_search_task(GTask *task, gpointer source_object,
                                      gpointer task_data, GCancellable *cancellable);
    static void on_async_search_task_complete(GObject *source_object, GAsyncResult *result,
                                              gpointer user_data);
    void jump_to_search_result(GtkTreeView *tree_view, GtkTreePath *path);
    static std::string sanitize_preview_text(const std::string &text);
    void perform_replace(const std::string &find_pattern, const std::string &replace_text,
                         bool case_sensitive = false, bool whole_word = false, bool regex = false,
                         bool replace_all = false);
    void update_refresh_controls();
    static gboolean on_search_results_refresh_idle(gpointer data);

    // GTK callbacks
    static void on_find_next_clicked(GtkEntry *entry, gpointer data);
    static void on_find_previous_clicked(GtkButton *button, gpointer data);
    static void on_find_all_clicked(GtkButton *button, gpointer data);
    static void on_replace_next_clicked(GtkEntry *entry, gpointer data);
    static void on_replace_all_clicked(GtkButton *button, gpointer data);
    static void on_find_close_clicked(GtkDialog *dialog, gint response_id, gpointer data);
    static void on_replace_close_clicked(GtkDialog *dialog, gint response_id, gpointer data);
    static gboolean on_dialog_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
    static void on_results_row_activated(GtkTreeView *tree_view, GtkTreePath *path,
                                         GtkTreeViewColumn *column, gpointer data);
    static gboolean on_results_key_press(GtkTreeView *tree_view, GdkEventKey *event, gpointer data);
    static void on_file_cell_data(GtkTreeViewColumn *column, GtkCellRenderer *renderer,
                                  GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
    static void on_search_results_toggled(GtkToggleButton *button, gpointer data);
    static void on_refresh_results_clicked(GtkButton *button, gpointer data);
    static void on_auto_refresh_toggled(GtkToggleButton *button, gpointer data);
    static void on_find_dialog_destroyed(GtkWidget *widget, gpointer data);
    static void on_replace_dialog_destroyed(GtkWidget *widget, gpointer data);
    static void on_clear_results_clicked(GtkButton *button, gpointer data);
    static void on_expand_all_results_clicked(GtkButton *button, gpointer data);
    static void on_collapse_all_results_clicked(GtkButton *button, gpointer data);
};
