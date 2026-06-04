#include "config/SettingsManager.h"
#include "config/ConfigPaths.h"
#include "config/CoreSettingsManager.h"
#include "config/ShortcutManager.h"
#include "config/RecentFilesManager.h"
#include "editor/DocumentManager.h"
#include "editor/MacroManager.h"
#include "ui/EditorWindow.h"
#include "ui/Localization.h"

#include <filesystem>

namespace {

bool migrate_config_file(const std::string &sourcePath,
                         const std::string &targetPath,
                         std::string &errorMessage) {
  namespace fs = std::filesystem;

  if (sourcePath.empty() || targetPath.empty() || sourcePath == targetPath) {
    return true;
  }

  std::error_code error;
  if (!fs::exists(sourcePath, error) || error) {
    return true;
  }

  fs::create_directories(fs::path(targetPath).parent_path(), error);
  if (error) {
    errorMessage = "Failed to create target configuration directory: " + error.message();
    return false;
  }

  error.clear();
  fs::copy_file(sourcePath, targetPath, fs::copy_options::overwrite_existing,
                error);
  if (error) {
    errorMessage = "Failed to copy " + sourcePath + " to " + targetPath + ": " + error.message();
    return false;
  }

  error.clear();
  fs::remove(sourcePath, error);
  if (error) {
    errorMessage = "Copied file but failed to remove old file " + sourcePath + ": " + error.message();
    return false;
  }

  return true;
}

GtkWidget *create_section_grid() {
  GtkWidget *grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
  gtk_widget_set_hexpand(grid, TRUE);
  gtk_widget_set_vexpand(grid, TRUE);
  return grid;
}

GtkWidget *create_scrolled_section(GtkWidget *grid) {
  GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(scrolled), grid);
  gtk_widget_set_hexpand(scrolled, TRUE);
  gtk_widget_set_vexpand(scrolled, TRUE);
  return scrolled;
}

void center_window_on_parent(GtkWidget *window, GtkWindow *parent) {
  if (!window || !parent) {
    return;
  }

  int parentX = 0;
  int parentY = 0;
  int parentWidth = 0;
  int parentHeight = 0;
  int windowWidth = 0;
  int windowHeight = 0;

  gtk_window_get_position(parent, &parentX, &parentY);
  gtk_window_get_size(parent, &parentWidth, &parentHeight);
  gtk_window_get_size(GTK_WINDOW(window), &windowWidth, &windowHeight);

  if (parentWidth <= 0 || parentHeight <= 0 ||
      windowWidth <= 0 || windowHeight <= 0) {
    return;
  }

  const int x = parentX + (parentWidth - windowWidth) / 2;
  const int y = parentY + (parentHeight - windowHeight) / 2;
  gtk_window_move(GTK_WINDOW(window), x, y);
}

void append_section_tab(GtkWidget *notebook,
                        const char *title,
                        GtkWidget *grid) {
  GtkWidget *label = gtk_label_new(title);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), create_scrolled_section(grid),
                           label);
}

struct DialogTitlebarControls {
  GtkWidget *dialog = nullptr;
  GtkWidget *maximizeButton = nullptr;
  GtkWidget *restoreButton = nullptr;
  gint closeResponse = GTK_RESPONSE_CANCEL;
  int previousX = 0;
  int previousY = 0;
  int previousWidth = 720;
  int previousHeight = 560;
  bool hasPreviousBounds = false;
};

struct LanguageOption {
  const char *id;
  const char *name;
};

const LanguageOption kInterfaceLanguages[] = {
    {"en", "English"},
    {"ja", "日本語"},
    {"ko", "한국어"},
    {"es", "Español"},
    {"pt", "Português"},
    {"de", "German"},
    {"fr", "Français"},
    {"ar", "العربية"},
    {"yue", "粵語"},
    {"zh-Hans", "简体中文"},
    {"zh-Hant", "繁體中文"},
};

GtkWidget *create_titlebar_icon_button(const char *iconName,
                                       const char *tooltip) {
  GtkWidget *button = gtk_button_new();
  GtkWidget *image = gtk_image_new_from_icon_name(iconName, GTK_ICON_SIZE_BUTTON);
  gtk_button_set_image(GTK_BUTTON(button), image);
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
  gtk_widget_set_tooltip_text(button, tooltip);
  return button;
}

void on_dialog_titlebar_maximize(GtkButton *, gpointer userData) {
  auto *controls = static_cast<DialogTitlebarControls *>(userData);
  GtkWindow *window = GTK_WINDOW(controls->dialog);
  gtk_window_get_position(window, &controls->previousX, &controls->previousY);
  gtk_window_get_size(window, &controls->previousWidth,
                      &controls->previousHeight);
  controls->hasPreviousBounds = true;

  gtk_window_maximize(window);

  GdkDisplay *display = gtk_widget_get_display(controls->dialog);
  GdkWindow *gdkWindow = gtk_widget_get_window(controls->dialog);
  if (display) {
    GdkMonitor *monitor = gdkWindow
        ? gdk_display_get_monitor_at_window(display, gdkWindow)
        : gdk_display_get_primary_monitor(display);
    if (!monitor) {
      monitor = gdk_display_get_monitor(display, 0);
    }
    if (!monitor) {
      return;
    }
    GdkRectangle workArea;
    gdk_monitor_get_workarea(monitor, &workArea);
    gtk_window_move(window, workArea.x, workArea.y);
    gtk_window_resize(window, workArea.width, workArea.height);
  }

  gtk_widget_hide(controls->maximizeButton);
  gtk_widget_show(controls->restoreButton);
}

void on_dialog_titlebar_restore(GtkButton *, gpointer userData) {
  auto *controls = static_cast<DialogTitlebarControls *>(userData);
  GtkWindow *window = GTK_WINDOW(controls->dialog);
  gtk_window_unmaximize(window);
  if (controls->hasPreviousBounds) {
    gtk_window_resize(window, controls->previousWidth,
                      controls->previousHeight);
    gtk_window_move(window, controls->previousX, controls->previousY);
  }
  gtk_widget_hide(controls->restoreButton);
  gtk_widget_show(controls->maximizeButton);
}

void on_dialog_titlebar_close(GtkButton *, gpointer userData) {
  auto *controls = static_cast<DialogTitlebarControls *>(userData);
  gtk_dialog_response(GTK_DIALOG(controls->dialog), controls->closeResponse);
}

void install_dialog_titlebar(GtkWidget *dialog,
                             const char *title,
                             DialogTitlebarControls &controls,
                             gint closeResponse) {
  controls.dialog = dialog;
  controls.closeResponse = closeResponse;
  gtk_window_set_type_hint(GTK_WINDOW(dialog), GDK_WINDOW_TYPE_HINT_NORMAL);

  GtkWidget *header = gtk_header_bar_new();
  gtk_header_bar_set_title(GTK_HEADER_BAR(header), title);
  gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header), FALSE);

  GtkWidget *closeButton =
      create_titlebar_icon_button("window-close-symbolic",
                                  Localization::text("tooltip.close"));
  controls.maximizeButton =
      create_titlebar_icon_button("window-maximize-symbolic",
                                  Localization::text("tooltip.maximize"));
  controls.restoreButton =
      create_titlebar_icon_button("window-restore-symbolic",
                                  Localization::text("tooltip.restore"));
  gtk_widget_set_no_show_all(controls.restoreButton, TRUE);
  gtk_widget_hide(controls.restoreButton);

  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), closeButton);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), controls.restoreButton);
  gtk_header_bar_pack_end(GTK_HEADER_BAR(header), controls.maximizeButton);
  gtk_window_set_titlebar(GTK_WINDOW(dialog), header);

  g_signal_connect(controls.maximizeButton, "clicked",
                   G_CALLBACK(on_dialog_titlebar_maximize), &controls);
  g_signal_connect(controls.restoreButton, "clicked",
                   G_CALLBACK(on_dialog_titlebar_restore), &controls);
  g_signal_connect(closeButton, "clicked",
                   G_CALLBACK(on_dialog_titlebar_close), &controls);
}

void attach_check(GtkWidget *grid, GtkWidget *check, int &row) {
  gtk_widget_set_halign(check, GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(grid), check, 0, row++, 2, 1);
}

void attach_labeled_widget(GtkWidget *grid,
                           const char *labelText,
                           GtkWidget *widget,
                           int &row,
                           bool expandWidget = true) {
  GtkWidget *label = gtk_label_new(labelText);
  gtk_widget_set_halign(label, GTK_ALIGN_START);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);
  gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

  gtk_widget_set_halign(widget, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand(widget, expandWidget ? TRUE : FALSE);
  gtk_grid_attach(GTK_GRID(grid), widget, 1, row++, 1, 1);
}

}  // namespace

SettingsManager::SettingsManager(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {
  coreSettings = std::make_unique<CoreSettingsManager>(editorWindow);
  shortcuts = std::make_unique<ShortcutManager>(editorWindow);
  recentFiles = std::make_unique<RecentFilesManager>(editorWindow);
}

SettingsManager::~SettingsManager() = default;

void SettingsManager::load_settings() {
  coreSettings->load_settings(recentFiles.get());
  recentFiles->load_recent_files();
  shortcuts->load_shortcuts();
  editorWindow->reloadMacros();
}

void SettingsManager::save_settings() const {
  coreSettings->save_settings();
  if (recentFiles) {
    recentFiles->save_recent_files();
  }
  if (shortcuts) {
    shortcuts->save_shortcuts();
  }
}

bool SettingsManager::save_file_to_path(const std::string &path) {
  if (!editorWindow->saveDocumentToPath(editorWindow->getCurrentDocumentIndex(),
                                        path)) {
    return false;
  }
  recentFiles->add_recent_file(path);
  save_settings();
  return true;
}

bool SettingsManager::migrate_config_directory(const std::string &oldConfigDir,
                                               const std::string &newConfigDir,
                                               std::string &errorMessage) const {
  if (oldConfigDir == newConfigDir) {
    return true;
  }

  if (!migrate_config_file(::get_config_file_path(oldConfigDir),
                           ::get_config_file_path(newConfigDir),
                           errorMessage)) {
    return false;
  }
  if (!migrate_config_file(::get_shortcuts_file_path(oldConfigDir),
                           ::get_shortcuts_file_path(newConfigDir),
                           errorMessage)) {
    return false;
  }
  if (!migrate_config_file(::get_macros_file_path(oldConfigDir),
                           ::get_macros_file_path(newConfigDir),
                           errorMessage)) {
    return false;
  }
  if (!migrate_config_file(::get_recent_files_file_path(oldConfigDir),
                           ::get_recent_files_file_path(newConfigDir),
                           errorMessage)) {
    return false;
  }
  if (!migrate_config_file(::get_session_file_path(oldConfigDir),
                           ::get_session_file_path(newConfigDir),
                           errorMessage)) {
    return false;
  }

  return true;
}

void SettingsManager::show_warning_dialog(const std::string &primary,
                                          const std::string &secondary) const {
  GtkWidget *dialog = gtk_message_dialog_new(
      editorWindow->getDialogParentWindow(),
      GTK_DIALOG_MODAL,
      GTK_MESSAGE_WARNING,
      GTK_BUTTONS_CLOSE,
      "%s",
      primary.c_str());
  gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                           secondary.c_str());
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

void SettingsManager::show_preferences_dialog() {
  GtkWindow *parent = editorWindow ? editorWindow->getDialogParentWindow()
                                   : nullptr;
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      Localization::text("dialog.preferences"), parent,
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
      Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
      Localization::text("dialog.ok"), GTK_RESPONSE_ACCEPT, NULL);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 720, 560);
  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);

  DialogTitlebarControls titlebarControls;
  install_dialog_titlebar(dialog, Localization::text("dialog.preferences"),
                          titlebarControls,
                          GTK_RESPONSE_CANCEL);

  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_widget_set_hexpand(content, TRUE);
  gtk_widget_set_vexpand(content, TRUE);

  GtkWidget *notebook = gtk_notebook_new();
  gtk_widget_set_size_request(notebook, 640, 440);
  gtk_widget_set_hexpand(notebook, TRUE);
  gtk_widget_set_vexpand(notebook, TRUE);
  gtk_container_add(GTK_CONTAINER(content), notebook);

  GtkWidget *display_grid = create_section_grid();
  GtkWidget *syntax_grid = create_section_grid();
  GtkWidget *file_grid = create_section_grid();
  GtkWidget *editor_grid = create_section_grid();
  GtkWidget *indentation_grid = create_section_grid();
  GtkWidget *search_grid = create_section_grid();
  GtkWidget *config_grid = create_section_grid();

  append_section_tab(notebook, Localization::text("tab.display"),
                     display_grid);
  append_section_tab(notebook, Localization::text("tab.syntax"), syntax_grid);
  append_section_tab(notebook, Localization::text("tab.files"), file_grid);
  append_section_tab(notebook, Localization::text("tab.editor"), editor_grid);
  append_section_tab(notebook, Localization::text("tab.indentation"),
                     indentation_grid);
  append_section_tab(notebook, Localization::text("tab.search"), search_grid);
  append_section_tab(notebook, Localization::text("tab.config"), config_grid);

  EditorWindow::ConfigState currentConfig = editorWindow->getConfigState();

  int file_row = 0;
  GtkWidget *enable_recent_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.enable_recent_files"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(enable_recent_check),
                               currentConfig.useRecentFiles);
  attach_check(file_grid, enable_recent_check, file_row);

  int display_row = 0;
  GtkWidget *interface_language_combo = gtk_combo_box_text_new();
  for (const auto &language : kInterfaceLanguages) {
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(interface_language_combo),
                              language.id, language.name);
  }
  if (!gtk_combo_box_set_active_id(GTK_COMBO_BOX(interface_language_combo),
                                   currentConfig.uiLanguage.c_str())) {
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(interface_language_combo), "en");
  }
  attach_labeled_widget(display_grid, Localization::text("pref.interface_language"),
                        interface_language_combo, display_row);

  GtkWidget *show_line_numbers_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.show_line_numbers"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_line_numbers_check),
                               currentConfig.showLineNumbers);
  attach_check(display_grid, show_line_numbers_check, display_row);

  GtkAdjustment *adjustment =
      gtk_adjustment_new(currentConfig.maxRecentFiles, 1, 20, 1, 5, 0);
  GtkWidget *recent_count_spin = gtk_spin_button_new(adjustment, 1, 0);
  attach_labeled_widget(file_grid, Localization::text("pref.recent_files_count"), recent_count_spin,
                        file_row, false);

  int config_row = 0;
  GtkWidget *config_dir_entry = gtk_entry_new();
  std::string current_config_dir =
      currentConfig.configDir.empty()
          ? coreSettings->get_config_directory()
          : currentConfig.configDir;
  gtk_entry_set_text(GTK_ENTRY(config_dir_entry), current_config_dir.c_str());
  attach_labeled_widget(config_grid, Localization::text("pref.config_directory"), config_dir_entry,
                        config_row);

  GtkWidget *line_wrap_combo = gtk_combo_box_text_new();
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(line_wrap_combo), "off",
                            Localization::text("pref.wrap_off"));
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(line_wrap_combo), "window",
                            Localization::text("pref.wrap_window"));
  gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(line_wrap_combo), "word",
                            Localization::text("pref.wrap_word"));
  if (currentConfig.lineWrapMode == "window") {
    gtk_combo_box_set_active(GTK_COMBO_BOX(line_wrap_combo), 1);
  } else if (currentConfig.lineWrapMode == "word") {
    gtk_combo_box_set_active(GTK_COMBO_BOX(line_wrap_combo), 2);
  } else {
    gtk_combo_box_set_active(GTK_COMBO_BOX(line_wrap_combo), 0);
  }

  GtkWidget *word_wrap_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.enable_word_wrap"));
  const bool word_wrap_enabled = currentConfig.lineWrapMode != "off";
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(word_wrap_check),
                               word_wrap_enabled);
  gtk_widget_set_sensitive(line_wrap_combo, word_wrap_enabled ? TRUE : FALSE);
  g_signal_connect(
      word_wrap_check, "toggled",
      G_CALLBACK(+[](GtkToggleButton *toggle, gpointer user_data) {
        GtkComboBox *combo = GTK_COMBO_BOX(user_data);
        const gboolean enabled = gtk_toggle_button_get_active(toggle);
        gtk_widget_set_sensitive(GTK_WIDGET(combo), enabled);
        if (!enabled) {
          gtk_combo_box_set_active_id(combo, "off");
        } else if (gtk_combo_box_get_active_id(combo) == nullptr ||
                   std::string(gtk_combo_box_get_active_id(combo)) == "off") {
          gtk_combo_box_set_active_id(combo, "word");
        }
      }),
      line_wrap_combo);
  g_signal_connect(
      line_wrap_combo, "changed",
      G_CALLBACK(+[](GtkComboBox *combo, gpointer user_data) {
        GtkToggleButton *toggle = GTK_TOGGLE_BUTTON(user_data);
        const gchar *id = gtk_combo_box_get_active_id(combo);
        const bool enabled = id && std::string(id) != "off";
        if (gtk_toggle_button_get_active(toggle) != enabled) {
          gtk_toggle_button_set_active(toggle, enabled);
        }
      }),
      word_wrap_check);

  attach_check(display_grid, word_wrap_check, display_row);
  attach_labeled_widget(display_grid, Localization::text("pref.line_wrap_mode"), line_wrap_combo,
                        display_row);

  GtkWidget *font_family_entry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(font_family_entry),
                     currentConfig.fontFamily.c_str());
  attach_labeled_widget(display_grid, Localization::text("pref.font_family"), font_family_entry,
                        display_row);

  GtkAdjustment *font_size_adjustment =
      gtk_adjustment_new(currentConfig.fontSize, 8, 72, 1, 5, 0);
  GtkWidget *font_size_spin = gtk_spin_button_new(font_size_adjustment, 1, 0);
  attach_labeled_widget(display_grid, Localization::text("pref.font_size"), font_size_spin,
                        display_row, false);

  GtkWidget *show_whitespace_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.show_whitespace"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_whitespace_check),
                               currentConfig.showWhitespace);
  attach_check(display_grid, show_whitespace_check, display_row);

  GtkWidget *show_eol_markers_check =
      gtk_check_button_new_with_label(Localization::text("pref.show_eol"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_eol_markers_check),
                               currentConfig.showEolMarkers);
  attach_check(display_grid, show_eol_markers_check, display_row);

  GtkWidget *highlight_current_line_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.highlight_current_line"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(highlight_current_line_check),
                               currentConfig.highlightCurrentLine);
  attach_check(display_grid, highlight_current_line_check, display_row);

  int editor_row = 0;
  GtkWidget *highlight_current_column_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.show_current_column"));
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(highlight_current_column_check),
      currentConfig.highlightCurrentColumn);
  attach_check(editor_grid, highlight_current_column_check, editor_row);

  GtkWidget *ctrl_mouse_wheel_zoom_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.enable_ctrl_wheel_zoom"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ctrl_mouse_wheel_zoom_check),
                               currentConfig.ctrlMouseWheelZoom);
  attach_check(editor_grid, ctrl_mouse_wheel_zoom_check, editor_row);

  GtkWidget *smart_keyword_highlighting_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.smart_keyword_highlighting"));
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(smart_keyword_highlighting_check),
      currentConfig.smartKeywordHighlighting);
  attach_check(editor_grid, smart_keyword_highlighting_check, editor_row);

  GtkWidget *trim_trailing_whitespace_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.trim_trailing_whitespace_on_save"));
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(trim_trailing_whitespace_check),
      currentConfig.trimTrailingWhitespaceOnSave);
  attach_check(editor_grid, trim_trailing_whitespace_check, editor_row);

  GtkWidget *ensure_final_newline_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.ensure_final_newline"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ensure_final_newline_check),
                               currentConfig.ensureFinalNewlineOnSave);
  attach_check(editor_grid, ensure_final_newline_check, editor_row);

  GtkWidget *show_indent_guides_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.show_indent_guides"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(show_indent_guides_check),
                               currentConfig.showIndentGuides);
  attach_check(display_grid, show_indent_guides_check, display_row);

  GtkAdjustment *long_line_column_adjustment =
      gtk_adjustment_new(currentConfig.longLineColumn, 0, 240, 1, 5, 0);
  GtkWidget *long_line_column_spin =
      gtk_spin_button_new(long_line_column_adjustment, 1, 0);
  attach_labeled_widget(display_grid, Localization::text("pref.long_line_column"),
                        long_line_column_spin, display_row, false);

  GtkWidget *show_right_margin_background_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.highlight_right_margin"));
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(show_right_margin_background_check),
      currentConfig.showRightMarginBackground);
  attach_check(display_grid, show_right_margin_background_check, display_row);

  GtkWidget *scroll_past_last_line_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.scroll_past_last_line"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(scroll_past_last_line_check),
                               currentConfig.scrollPastLastLine);
  attach_check(display_grid, scroll_past_last_line_check, display_row);

  GtkWidget *highlight_matching_braces_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.highlight_matching_braces"));
  gtk_toggle_button_set_active(
      GTK_TOGGLE_BUTTON(highlight_matching_braces_check),
      currentConfig.highlightMatchingBraces);
  attach_check(display_grid, highlight_matching_braces_check, display_row);

  int syntax_row = 0;
  GtkWidget *syntax_highlighting_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.enable_syntax_highlighting"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(syntax_highlighting_check),
                               currentConfig.syntaxHighlightingEnabled);
  attach_check(syntax_grid, syntax_highlighting_check, syntax_row);

  GtkWidget *default_lexer_combo = gtk_combo_box_text_new();

  // Populate lexer combo box with available lexers
  const auto& availableLexers = editorWindow->getAvailableLexers();
  for (const auto& lexer : availableLexers) {
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(default_lexer_combo),
                              lexer.name.c_str(), lexer.description.c_str());
  }

  // Set active lexer
  if (!currentConfig.defaultLexer.empty()) {
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(default_lexer_combo),
                                currentConfig.defaultLexer.c_str());
  } else {
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(default_lexer_combo), "null");
  }
  attach_labeled_widget(syntax_grid, Localization::text("pref.default_lexer"),
                        default_lexer_combo, syntax_row);

  GtkWidget *default_lexer_note =
      gtk_label_new(Localization::text("pref.default_lexer_note"));
  gtk_widget_set_halign(default_lexer_note, GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(syntax_grid), default_lexer_note, 1, syntax_row++, 1,
                  1);

  GtkWidget *auto_save_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.enable_auto_save"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_save_check),
                               currentConfig.autoSave);
  attach_check(editor_grid, auto_save_check, editor_row);

  GtkAdjustment *auto_save_interval_adjustment =
      gtk_adjustment_new(currentConfig.autoSaveInterval, 10, 300, 1, 5, 0);
  GtkWidget *auto_save_interval_spin =
      gtk_spin_button_new(auto_save_interval_adjustment, 1, 0);
  attach_labeled_widget(editor_grid, Localization::text("pref.auto_save_interval"),
                        auto_save_interval_spin, editor_row, false);

  GtkWidget *overwrite_readonly_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.overwrite_readonly"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(overwrite_readonly_check),
                               currentConfig.overwriteReadonly);
  attach_check(editor_grid, overwrite_readonly_check, editor_row);

  int indentation_row = 0;
  GtkAdjustment *tab_size_adjustment =
      gtk_adjustment_new(currentConfig.tabSize, 2, 8, 1, 1, 0);
  GtkWidget *tab_size_spin = gtk_spin_button_new(tab_size_adjustment, 1, 0);
  attach_labeled_widget(indentation_grid, Localization::text("pref.tab_size"), tab_size_spin,
                        indentation_row, false);

  GtkWidget *use_spaces_for_tabs_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.use_spaces_for_tabs"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(use_spaces_for_tabs_check),
                               currentConfig.useSpacesForTabs);
  attach_check(indentation_grid, use_spaces_for_tabs_check, indentation_row);

  GtkWidget *auto_indent_check =
      gtk_check_button_new_with_label(Localization::text("pref.auto_indent"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_indent_check),
                               currentConfig.autoIndent);
  attach_check(indentation_grid, auto_indent_check, indentation_row);

  int search_row = 0;
  GtkWidget *wrap_search_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.search_wrap_around"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(wrap_search_check),
                               currentConfig.searchWrapAround);
  attach_check(search_grid, wrap_search_check, search_row);

  GtkWidget *search_all_docs_check =
      gtk_check_button_new_with_label(
          Localization::text("pref.search_all_documents"));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(search_all_docs_check),
                               currentConfig.searchAllDocuments);
  attach_check(search_grid, search_all_docs_check, search_row);

  GtkWidget *shortcuts_file_entry = gtk_entry_new();
  std::string current_shortcuts_path = shortcuts->get_shortcuts_file_path();
  gtk_entry_set_text(GTK_ENTRY(shortcuts_file_entry),
                     current_shortcuts_path.c_str());
  gtk_editable_set_editable(GTK_EDITABLE(shortcuts_file_entry), FALSE);
  gtk_widget_set_sensitive(shortcuts_file_entry, FALSE);
  attach_labeled_widget(config_grid, Localization::text("pref.shortcuts_file"), shortcuts_file_entry,
                        config_row);

  GtkWidget *recent_files_path_entry = gtk_entry_new();
  std::string current_recent_files_path =
      recentFiles->get_recent_files_file_path();
  gtk_entry_set_text(GTK_ENTRY(recent_files_path_entry),
                     current_recent_files_path.c_str());
  gtk_editable_set_editable(GTK_EDITABLE(recent_files_path_entry), FALSE);
  gtk_widget_set_sensitive(recent_files_path_entry, FALSE);
  attach_labeled_widget(config_grid, Localization::text("pref.recent_files_file"),
                        recent_files_path_entry, config_row);

  gtk_widget_show_all(dialog);
  center_window_on_parent(dialog, parent);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    EditorWindow::ConfigState updatedConfig = currentConfig;
    updatedConfig.useRecentFiles =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(enable_recent_check));
    updatedConfig.showLineNumbers =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_line_numbers_check));
    const gchar *selected_language =
        gtk_combo_box_get_active_id(GTK_COMBO_BOX(interface_language_combo));
    if (selected_language) {
      updatedConfig.uiLanguage = selected_language;
    }
    updatedConfig.maxRecentFiles =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(recent_count_spin));

    const gchar *selected_wrap_mode =
        gtk_combo_box_get_active_id(GTK_COMBO_BOX(line_wrap_combo));
    if (selected_wrap_mode) {
      updatedConfig.lineWrapMode = selected_wrap_mode;
    }
    updatedConfig.fontFamily = gtk_entry_get_text(GTK_ENTRY(font_family_entry));
    updatedConfig.fontSize =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(font_size_spin));
    updatedConfig.showWhitespace =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_whitespace_check));
    updatedConfig.showEolMarkers =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_eol_markers_check));
    updatedConfig.highlightCurrentLine = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(highlight_current_line_check));
    updatedConfig.highlightCurrentColumn = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(highlight_current_column_check));
    updatedConfig.ctrlMouseWheelZoom = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(ctrl_mouse_wheel_zoom_check));
    updatedConfig.smartKeywordHighlighting = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(smart_keyword_highlighting_check));
    updatedConfig.trimTrailingWhitespaceOnSave = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(trim_trailing_whitespace_check));
    updatedConfig.ensureFinalNewlineOnSave = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(ensure_final_newline_check));
    updatedConfig.showIndentGuides =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(show_indent_guides_check));
    updatedConfig.longLineColumn = gtk_spin_button_get_value_as_int(
        GTK_SPIN_BUTTON(long_line_column_spin));
    updatedConfig.showRightMarginBackground = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(show_right_margin_background_check));
    updatedConfig.scrollPastLastLine = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(scroll_past_last_line_check));
    updatedConfig.highlightMatchingBraces = gtk_toggle_button_get_active(
        GTK_TOGGLE_BUTTON(highlight_matching_braces_check));
    updatedConfig.syntaxHighlightingEnabled =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(syntax_highlighting_check));

    const gchar *selected_lexer = gtk_combo_box_get_active_id(GTK_COMBO_BOX(default_lexer_combo));
    if (selected_lexer) {
      updatedConfig.defaultLexer = selected_lexer;
    }

    updatedConfig.autoSave =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_save_check));
    updatedConfig.autoSaveInterval =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(auto_save_interval_spin));
    updatedConfig.overwriteReadonly =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(overwrite_readonly_check));
    updatedConfig.tabSize =
        gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(tab_size_spin));
    updatedConfig.useSpacesForTabs =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(use_spaces_for_tabs_check));
    updatedConfig.autoIndent =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_indent_check));
    updatedConfig.searchWrapAround =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(wrap_search_check));
    updatedConfig.searchAllDocuments =
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(search_all_docs_check));
    updatedConfig.configDir = gtk_entry_get_text(GTK_ENTRY(config_dir_entry));

    const std::string previousConfigDir = currentConfig.configDir;
    if (updatedConfig.configDir != currentConfig.configDir) {
      std::string migrationError;
      if (!migrate_config_directory(previousConfigDir, updatedConfig.configDir,
                                    migrationError)) {
        show_warning_dialog("Configuration migration did not complete cleanly.",
                            migrationError);
      }
    }

    editorWindow->applyConfigState(updatedConfig);
    save_settings();
    editorWindow->saveMacros();
    editorWindow->reloadMacros();
    shortcuts->load_shortcuts();
    recentFiles->refresh_recent_menu();
  }

  gtk_widget_destroy(dialog);
}
