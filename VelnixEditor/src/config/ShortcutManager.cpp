#include "config/ConfigPaths.h"
#include "config/ShortcutManager.h"
#include "core/Logger.h"
#include "ui/EditorWindow.h"
#include "ui/Localization.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>

namespace {

std::string trim_copy(const std::string &text) {
  const size_t first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return "";
  }

  const size_t last = text.find_last_not_of(" \t\r\n");
  return text.substr(first, last - first + 1);
}

std::string make_shortcut_conflict_key(guint key, GdkModifierType mods) {
  return std::to_string(key) + ":" +
         std::to_string(static_cast<unsigned int>(mods));
}

std::string join_actions(const std::vector<std::string> &actions) {
  std::ostringstream stream;
  for (size_t i = 0; i < actions.size(); ++i) {
    if (i > 0) {
      stream << ", ";
    }
    stream << actions[i];
  }
  return stream.str();
}

std::string json_escape(const std::string &text) {
  std::ostringstream stream;
  for (char ch : text) {
    switch (ch) {
      case '\\':
        stream << "\\\\";
        break;
      case '"':
        stream << "\\\"";
        break;
      case '\n':
        stream << "\\n";
        break;
      case '\r':
        stream << "\\r";
        break;
      case '\t':
        stream << "\\t";
        break;
      default:
        stream << ch;
        break;
    }
  }
  return stream.str();
}

std::string json_unescape(const std::string &text) {
  std::string result;
  result.reserve(text.size());
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\' || i + 1 >= text.size()) {
      result.push_back(text[i]);
      continue;
    }

    const char escaped = text[++i];
    switch (escaped) {
      case 'n':
        result.push_back('\n');
        break;
      case 'r':
        result.push_back('\r');
        break;
      case 't':
        result.push_back('\t');
        break;
      default:
        result.push_back(escaped);
        break;
    }
  }
  return result;
}

std::map<std::string, std::string> read_shortcuts_json(std::istream &input) {
  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string content = buffer.str();
  std::map<std::string, std::string> shortcuts;

  const std::regex objectPattern("\\{[^{}]*\\}");
  const std::regex commandPattern("\"(?:command|action)\"\\s*:\\s*\"((?:\\\\.|[^\"])*)\"");
  const std::regex shortcutPattern("\"shortcut\"\\s*:\\s*\"((?:\\\\.|[^\"])*)\"");
  const std::regex shortcutsPattern("\"shortcuts\"\\s*:\\s*\\[([^\\]]*)\\]");
  const std::regex arrayValuePattern("\"((?:\\\\.|[^\"])*)\"");
  for (std::sregex_iterator it(content.begin(), content.end(), objectPattern);
       it != std::sregex_iterator(); ++it) {
    const std::string objectText = it->str();
    std::smatch commandMatch;
    std::smatch shortcutMatch;
    std::smatch shortcutsMatch;
    if (!std::regex_search(objectText, commandMatch, commandPattern)) {
      continue;
    }

    std::vector<std::string> values;
    if (std::regex_search(objectText, shortcutsMatch, shortcutsPattern)) {
      const std::string arrayText = shortcutsMatch[1].str();
      for (std::sregex_iterator valueIt(arrayText.begin(), arrayText.end(),
                                        arrayValuePattern);
           valueIt != std::sregex_iterator(); ++valueIt) {
        values.push_back(json_unescape((*valueIt)[1].str()));
        if (values.size() == 2) {
          break;
        }
      }
    } else if (std::regex_search(objectText, shortcutMatch, shortcutPattern)) {
      values.push_back(json_unescape(shortcutMatch[1].str()));
    }

    if (!values.empty()) {
      shortcuts[json_unescape(commandMatch[1].str())] = values.front();
      if (values.size() > 1) {
        shortcuts[json_unescape(commandMatch[1].str())] += "\n" + values[1];
      }
    }
  }

  return shortcuts;
}

std::string legacy_shortcuts_path(const std::string &shortcutsPath) {
  const std::string jsonSuffix = ".json";
  if (shortcutsPath.size() >= jsonSuffix.size() &&
      shortcutsPath.compare(shortcutsPath.size() - jsonSuffix.size(),
                            jsonSuffix.size(), jsonSuffix) == 0) {
    return shortcutsPath.substr(0, shortcutsPath.size() - jsonSuffix.size()) +
           ".ini";
  }
  return shortcutsPath + ".ini";
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

std::map<std::string, std::string> read_legacy_shortcuts_ini(std::istream &input) {
  std::map<std::string, std::string> shortcuts;
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    const size_t equals = line.find('=');
    if (equals == std::string::npos) {
      continue;
    }

    std::string actionName = trim_copy(line.substr(0, equals));
    std::replace(actionName.begin(), actionName.end(), '_', ' ');
    shortcuts[actionName] = trim_copy(line.substr(equals + 1));
  }
  return shortcuts;
}

std::string lowercase_copy(const std::string &text) {
  std::string result = text;
  std::transform(result.begin(), result.end(), result.begin(),
                 [](unsigned char ch) { return std::tolower(ch); });
  return result;
}

std::vector<std::string> split_stored_shortcuts(const std::string &text) {
  std::vector<std::string> values;
  std::stringstream stream(text);
  std::string value;
  while (std::getline(stream, value, '\n')) {
    values.push_back(trim_copy(value));
    if (values.size() == 2) {
      break;
    }
  }
  while (values.size() < 2) {
    values.push_back("");
  }
  return values;
}

enum ShortcutColumns {
  ShortcutColumnIndex,
  ShortcutColumnCommand,
  ShortcutColumnShortcut1,
  ShortcutColumnShortcut2,
  ShortcutColumnStatus,
  ShortcutColumnReset,
  ShortcutColumnDefault1,
  ShortcutColumnDefault2,
  ShortcutColumnCount
};

struct ShortcutsDialogState {
  ShortcutManager *manager;
  GtkListStore *store;
  GtkTreeModelFilter *filter;
  GtkWidget *dialog;
  GtkWidget *searchEntry;
  GtkWidget *statusLabel;
  GtkTreeViewColumn *resetColumn;
};

gboolean shortcut_filter_visible(GtkTreeModel *model, GtkTreeIter *iter,
                                 gpointer userData) {
  auto *state = static_cast<ShortcutsDialogState *>(userData);
  const char *searchText = gtk_entry_get_text(GTK_ENTRY(state->searchEntry));
  const std::string query = lowercase_copy(trim_copy(searchText ? searchText : ""));
  if (query.empty()) {
    return TRUE;
  }

  gchar *command = nullptr;
  gchar *shortcut1 = nullptr;
  gchar *shortcut2 = nullptr;
  gchar *status = nullptr;
  gtk_tree_model_get(model, iter, ShortcutColumnCommand, &command,
                     ShortcutColumnShortcut1, &shortcut1,
                     ShortcutColumnShortcut2, &shortcut2,
                     ShortcutColumnStatus, &status, -1);
  const std::string haystack =
      lowercase_copy(std::string(command ? command : "") + " " +
                     (shortcut1 ? shortcut1 : "") + " " +
                     (shortcut2 ? shortcut2 : "") + " " +
                     (status ? status : ""));
  g_free(command);
  g_free(shortcut1);
  g_free(shortcut2);
  g_free(status);
  return haystack.find(query) != std::string::npos;
}

void update_shortcut_row_status(GtkListStore *store, GtkTreeIter *iter,
                                const std::string &status) {
  gtk_list_store_set(store, iter, ShortcutColumnStatus, status.c_str(), -1);
}

void on_shortcut_search_changed(GtkEditable *, gpointer userData) {
  auto *state = static_cast<ShortcutsDialogState *>(userData);
  gtk_tree_model_filter_refilter(state->filter);
}

void on_shortcut_cell_edited(GtkCellRendererText *renderer, gchar *pathText,
                             gchar *newText, gpointer userData) {
  auto *state = static_cast<ShortcutsDialogState *>(userData);
  const size_t slot = static_cast<size_t>(
      GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(renderer), "shortcut-slot")));
  GtkTreePath *filterPath = gtk_tree_path_new_from_string(pathText);
  GtkTreePath *childPath =
      gtk_tree_model_filter_convert_path_to_child_path(state->filter, filterPath);
  gtk_tree_path_free(filterPath);
  if (!childPath) {
    return;
  }

  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(state->store), &iter, childPath)) {
    gtk_tree_path_free(childPath);
    return;
  }
  gtk_tree_path_free(childPath);

  gint bindingIndex = -1;
  gchar *command = nullptr;
  gtk_tree_model_get(GTK_TREE_MODEL(state->store), &iter, ShortcutColumnIndex,
                     &bindingIndex, ShortcutColumnCommand, &command, -1);
  if (bindingIndex < 0) {
    g_free(command);
    return;
  }

  std::string entryText = trim_copy(newText ? newText : "");
  guint newKey = 0;
  GdkModifierType newMods = static_cast<GdkModifierType>(0);
  if (!entryText.empty() &&
      !state->manager->parse_shortcut(entryText, newKey, newMods)) {
    update_shortcut_row_status(state->store, &iter,
                               Localization::text("shortcut.invalid_format"));
    gtk_label_set_text(GTK_LABEL(state->statusLabel),
                       Localization::text("shortcut.not_changed_invalid"));
    g_free(command);
    return;
  }

  const std::vector<std::string> conflicts =
      state->manager->get_conflicting_actions(newKey, newMods,
                                              static_cast<size_t>(bindingIndex),
                                              slot);
  if (newKey != 0 && !conflicts.empty()) {
    std::string message =
        std::string(Localization::text("shortcut.conflict_prefix")) +
        join_actions(conflicts);
    update_shortcut_row_status(state->store, &iter, message);
    gtk_label_set_text(GTK_LABEL(state->statusLabel), message.c_str());
    g_free(command);
    return;
  }

  if (!state->manager->apply_shortcut(static_cast<size_t>(bindingIndex), slot,
                                      newKey, newMods, entryText)) {
    update_shortcut_row_status(state->store, &iter,
                               Localization::text("shortcut.unable_apply"));
    gtk_label_set_text(GTK_LABEL(state->statusLabel),
                       Localization::text("shortcut.not_changed"));
    g_free(command);
    return;
  }

  gtk_list_store_set(state->store, &iter,
                     slot == 0 ? ShortcutColumnShortcut1
                               : ShortcutColumnShortcut2,
                     entryText.c_str(), ShortcutColumnStatus,
                     Localization::text("shortcut.active"), -1);
  const std::string statusText =
      std::string(command ? command : "Shortcut") +
      (entryText.empty() ? " shortcut cleared." : " shortcut updated.");
  gtk_label_set_text(GTK_LABEL(state->statusLabel), statusText.c_str());
  state->manager->save_shortcuts();
  gtk_tree_model_filter_refilter(state->filter);
  g_free(command);
}

gboolean on_shortcut_table_button_press(GtkWidget *widget, GdkEventButton *event,
                                        gpointer userData) {
  if (event->type != GDK_BUTTON_PRESS || event->button != 1) {
    return FALSE;
  }

  auto *state = static_cast<ShortcutsDialogState *>(userData);
  GtkTreePath *filterPath = nullptr;
  GtkTreeViewColumn *column = nullptr;
  if (!gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(widget),
                                     static_cast<gint>(event->x),
                                     static_cast<gint>(event->y), &filterPath,
                                     &column, nullptr, nullptr)) {
    return FALSE;
  }

  const bool resetClicked = column == state->resetColumn;
  if (!resetClicked) {
    gtk_tree_path_free(filterPath);
    return FALSE;
  }

  GtkTreePath *childPath =
      gtk_tree_model_filter_convert_path_to_child_path(state->filter, filterPath);
  gtk_tree_path_free(filterPath);
  if (!childPath) {
    return TRUE;
  }

  GtkTreeIter iter;
  if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(state->store), &iter, childPath)) {
    gtk_tree_path_free(childPath);
    return TRUE;
  }
  gtk_tree_path_free(childPath);

  gint bindingIndex = -1;
  gchar *command = nullptr;
  gchar *default1 = nullptr;
  gchar *default2 = nullptr;
  gtk_tree_model_get(GTK_TREE_MODEL(state->store), &iter, ShortcutColumnIndex,
                     &bindingIndex, ShortcutColumnCommand, &command,
                     ShortcutColumnDefault1, &default1, ShortcutColumnDefault2,
                     &default2, -1);
  if (bindingIndex >= 0 &&
      state->manager->restore_default_shortcuts(static_cast<size_t>(bindingIndex))) {
    gtk_list_store_set(state->store, &iter, ShortcutColumnShortcut1,
                       default1 ? default1 : "", ShortcutColumnShortcut2,
                       default2 ? default2 : "", ShortcutColumnStatus,
                       Localization::text("shortcut.default"), -1);
    const std::string statusText =
        std::string(command ? command : "Shortcut") + " restored to default.";
    gtk_label_set_text(GTK_LABEL(state->statusLabel), statusText.c_str());
    state->manager->save_shortcuts();
    gtk_tree_model_filter_refilter(state->filter);
  }

  g_free(command);
  g_free(default1);
  g_free(default2);
  return TRUE;
}

struct DialogTitlebarControls {
  GtkWidget *dialog = nullptr;
  GtkWidget *maximizeButton = nullptr;
  GtkWidget *restoreButton = nullptr;
  gint closeResponse = GTK_RESPONSE_CLOSE;
  int previousX = 0;
  int previousY = 0;
  int previousWidth = 720;
  int previousHeight = 560;
  bool hasPreviousBounds = false;
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

GtkFileFilter *create_shortcuts_json_filter() {
  GtkFileFilter *filter = gtk_file_filter_new();
  gtk_file_filter_set_name(filter, Localization::text("shortcut.json_files"));
  gtk_file_filter_add_pattern(filter, "*.json");
  gtk_file_filter_add_mime_type(filter, "application/json");
  return filter;
}

void on_shortcuts_export_clicked(GtkButton *, gpointer userData) {
  auto *state = static_cast<ShortcutsDialogState *>(userData);
  GtkWidget *chooser = gtk_file_chooser_dialog_new(
      Localization::text("shortcut.export_title"), GTK_WINDOW(state->dialog),
      GTK_FILE_CHOOSER_ACTION_SAVE,
      Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
      Localization::text("shortcut.export_button"), GTK_RESPONSE_ACCEPT,
      nullptr);
  gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser), TRUE);
  gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser), "shortcuts.json");
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser),
                              create_shortcuts_json_filter());

  if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    std::string errorMessage;
    if (state->manager->export_shortcuts_to_path(filename ? filename : "",
                                                 errorMessage)) {
      gtk_label_set_text(GTK_LABEL(state->statusLabel),
                         Localization::text("shortcut.exported"));
    } else {
      gtk_label_set_text(GTK_LABEL(state->statusLabel), errorMessage.c_str());
    }
    g_free(filename);
  }

  gtk_widget_destroy(chooser);
}

void on_shortcuts_import_clicked(GtkButton *, gpointer userData) {
  auto *state = static_cast<ShortcutsDialogState *>(userData);
  GtkWidget *chooser = gtk_file_chooser_dialog_new(
      Localization::text("shortcut.import_title"), GTK_WINDOW(state->dialog),
      GTK_FILE_CHOOSER_ACTION_OPEN,
      Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
      Localization::text("shortcut.import_button"), GTK_RESPONSE_ACCEPT,
      nullptr);
  gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser),
                              create_shortcuts_json_filter());

  if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT) {
    gchar *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
    std::string errorMessage;
    if (state->manager->import_shortcuts_from_path(filename ? filename : "",
                                                 errorMessage)) {
      state->manager->populate_shortcuts_store(state->store);
      gtk_tree_model_filter_refilter(state->filter);
      gtk_label_set_text(GTK_LABEL(state->statusLabel),
                         Localization::text("shortcut.imported"));
    } else {
      gtk_label_set_text(GTK_LABEL(state->statusLabel), errorMessage.c_str());
    }
    g_free(filename);
  }

  gtk_widget_destroy(chooser);
}

}  // namespace

ShortcutManager::ShortcutManager(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {}

bool ShortcutManager::parse_shortcut(const std::string &text, guint &key,
                                     GdkModifierType &mods) const {
  std::string normalized = normalize_shortcut_text(trim_copy(text));
  mods = static_cast<GdkModifierType>(0);

  if (normalized == "CTRL++" || normalized == "CONTROL++") {
    key = GDK_KEY_equal;
    mods = static_cast<GdkModifierType>(GDK_CONTROL_MASK);
    return true;
  }

  if (normalized == "CTRL+=" || normalized == "CONTROL+=") {
    key = GDK_KEY_equal;
    mods = static_cast<GdkModifierType>(GDK_CONTROL_MASK);
    return true;
  }

  std::string remainder;
  size_t start = 0;

  while (start < normalized.size()) {
    size_t plus = normalized.find('+', start);
    std::string token = trim_copy(normalized.substr(start, plus - start));
    if (plus == std::string::npos) {
      remainder = token;
      break;
    }

    if (token == "CTRL" || token == "CONTROL") {
      mods = static_cast<GdkModifierType>(mods | GDK_CONTROL_MASK);
    } else if (token == "SHIFT") {
      mods = static_cast<GdkModifierType>(mods | GDK_SHIFT_MASK);
    } else if (token == "ALT") {
      mods = static_cast<GdkModifierType>(mods | GDK_MOD1_MASK);
    } else if (token == "SUPER") {
      mods = static_cast<GdkModifierType>(mods | GDK_SUPER_MASK);
    }
    start = plus + 1;
  }

  if (remainder.empty()) {
    return false;
  }

  key = get_keyval_from_name(remainder);
  return key != 0;
}

guint ShortcutManager::get_keyval_from_name(const std::string &name) const {
  if (name == ",") {
    return GDK_KEY_comma;
  }
  if (name == "-") {
    return GDK_KEY_minus;
  }
  if (name == "<") {
    return GDK_KEY_less;
  }
  if (name == ".") {
    return GDK_KEY_period;
  }
  if (name == "#") {
    return GDK_KEY_numbersign;
  }
  if (name == "/") {
    return GDK_KEY_slash;
  }
  return gdk_keyval_from_name(name.c_str());
}

std::vector<std::string> ShortcutManager::check_shortcut_conflicts(
    guint key, GdkModifierType mods) const {
  return get_conflicting_actions(key, mods);
}

std::vector<std::string> ShortcutManager::get_conflicting_actions(
    guint key, GdkModifierType mods) const {
  return get_conflicting_actions(key, mods, static_cast<size_t>(-1),
                                 static_cast<size_t>(-1));
}

std::vector<std::string> ShortcutManager::get_conflicting_actions(
    guint key, GdkModifierType mods, size_t ignoredIndex,
    size_t ignoredSlot) const {
  std::vector<std::string> conflicts;
  const auto &shortcutBindings = editorWindow->getShortcutBindings();
  for (size_t i = 0; i < shortcutBindings.size(); ++i) {
    const auto &binding = shortcutBindings[i];
    for (size_t slot = 0; slot < binding.keys.size() &&
                          slot < binding.modifiers.size() && slot < 2;
         ++slot) {
      if (i == ignoredIndex && slot == ignoredSlot) {
        continue;
      }
      if (binding.keys[slot] == key && binding.modifiers[slot] == mods) {
        conflicts.push_back(binding.actionName);
        break;
      }
    }
  }
  return conflicts;
}

std::string ShortcutManager::normalize_shortcut_text(
    const std::string &text) const {
  std::string input = text;
  std::transform(input.begin(), input.end(), input.begin(),
                 [](unsigned char ch) { return std::toupper(ch); });
  return input;
}

std::string ShortcutManager::get_shortcuts_file_path() const {
  return ::get_shortcuts_file_path(editorWindow->getConfigDirectory());
}

void ShortcutManager::load_shortcuts() {
  auto &shortcutBindings = editorWindow->getShortcutBindings();
  std::string shortcutsPath = get_shortcuts_file_path();
  std::ifstream input(shortcutsPath);
  std::map<std::string, std::string> storedShortcuts;
  if (input) {
    storedShortcuts = read_shortcuts_json(input);
  } else {
    std::ifstream legacyInput(legacy_shortcuts_path(shortcutsPath));
    if (legacyInput) {
      storedShortcuts = read_legacy_shortcuts_ini(legacyInput);
    }
  }

  std::vector<std::vector<guint>> resolvedKeys;
  std::vector<std::vector<GdkModifierType>> resolvedMods;
  std::vector<std::vector<std::string>> resolvedDisplayTexts;
  resolvedKeys.reserve(shortcutBindings.size());
  resolvedMods.reserve(shortcutBindings.size());
  resolvedDisplayTexts.reserve(shortcutBindings.size());
  for (const auto &binding : shortcutBindings) {
    std::vector<guint> keys = binding.keys;
    std::vector<GdkModifierType> mods = binding.modifiers;
    std::vector<std::string> texts = binding.displayTexts;
    if (keys.empty() && binding.key != 0) {
      keys.push_back(binding.key);
      mods.push_back(binding.mods);
    }
    if (texts.empty() && !binding.displayText.empty()) {
      texts.push_back(binding.displayText);
    }
    while (keys.size() < 2) {
      keys.push_back(0);
    }
    while (mods.size() < 2) {
      mods.push_back(static_cast<GdkModifierType>(0));
    }
    while (texts.size() < 2) {
      texts.push_back("");
    }
    resolvedKeys.push_back(keys);
    resolvedMods.push_back(mods);
    resolvedDisplayTexts.push_back(texts);
  }

  for (const auto &storedShortcut : storedShortcuts) {
    const std::string &actionName = storedShortcut.first;
    const std::vector<std::string> values =
        split_stored_shortcuts(storedShortcut.second);
    for (size_t index = 0; index < shortcutBindings.size(); ++index) {
      if (shortcutBindings[index].actionName != actionName) {
        continue;
      }

      for (size_t slot = 0; slot < values.size() && slot < 2; ++slot) {
        const std::string value = trim_copy(values[slot]);
        guint parsedKey = 0;
        GdkModifierType parsedMods = static_cast<GdkModifierType>(0);
        if (value.empty()) {
          resolvedKeys[index][slot] = 0;
          resolvedMods[index][slot] = static_cast<GdkModifierType>(0);
          resolvedDisplayTexts[index][slot].clear();
        } else if (parse_shortcut(value, parsedKey, parsedMods)) {
          resolvedKeys[index][slot] = parsedKey;
          resolvedMods[index][slot] = parsedMods;
          resolvedDisplayTexts[index][slot] = value;
        } else {
          Logger::warning(
              "Ignoring invalid shortcut '%s' for action '%s' in '%s'.",
              value.c_str(), actionName.c_str(), shortcutsPath.c_str());
        }
      }
      break;
    }
  }

  std::map<std::string, size_t> firstBindingByShortcut;
  for (size_t index = 0; index < resolvedKeys.size(); ++index) {
    for (size_t slot = 0; slot < resolvedKeys[index].size() && slot < 2;
         ++slot) {
      if (resolvedKeys[index][slot] == 0) {
        continue;
      }

      const std::string shortcutKey =
          make_shortcut_conflict_key(resolvedKeys[index][slot],
                                     resolvedMods[index][slot]);
      auto existing = firstBindingByShortcut.find(shortcutKey);
      if (existing == firstBindingByShortcut.end()) {
        firstBindingByShortcut[shortcutKey] = index;
        continue;
      }

      const size_t firstIndex = existing->second;
      Logger::warning(
          "Shortcut '%s' for action '%s' conflicts with action '%s' in '%s'; keeping the first binding.",
          resolvedDisplayTexts[index][slot].c_str(),
          shortcutBindings[index].actionName.c_str(),
          shortcutBindings[firstIndex].actionName.c_str(),
          shortcutsPath.c_str());
      resolvedKeys[index][slot] = 0;
      resolvedMods[index][slot] = static_cast<GdkModifierType>(0);
      resolvedDisplayTexts[index][slot].clear();
    }
  }

  for (size_t index = 0; index < shortcutBindings.size(); ++index) {
    auto &binding = shortcutBindings[index];
    for (size_t slot = 0; slot < binding.keys.size() &&
                          slot < binding.modifiers.size() && slot < 2;
         ++slot) {
      if (binding.keys[slot] != 0) {
        gtk_widget_remove_accelerator(binding.item, editorWindow->getAccelGroup(),
                                      binding.keys[slot],
                                      binding.modifiers[slot]);
      }
    }
    if (binding.key != 0 && binding.keys.empty()) {
      gtk_widget_remove_accelerator(binding.item, editorWindow->getAccelGroup(),
                                    binding.key, binding.mods);
    }

    binding.keys = resolvedKeys[index];
    binding.modifiers = resolvedMods[index];
    binding.displayTexts = resolvedDisplayTexts[index];
    binding.key = binding.keys.empty() ? 0 : binding.keys.front();
    binding.mods = binding.modifiers.empty()
                       ? static_cast<GdkModifierType>(0)
                       : binding.modifiers.front();
    binding.displayText =
        binding.displayTexts.empty() ? "" : binding.displayTexts.front();

    for (size_t slot = 0; slot < binding.keys.size() &&
                          slot < binding.modifiers.size() && slot < 2;
         ++slot) {
      if (binding.keys[slot] != 0) {
        gtk_widget_add_accelerator(binding.item, "activate",
                                   editorWindow->getAccelGroup(),
                                   binding.keys[slot], binding.modifiers[slot],
                                   GTK_ACCEL_VISIBLE);
      }
    }
  }

  save_shortcuts();
}

void ShortcutManager::save_shortcuts() const {
  std::string shortcutsPath = get_shortcuts_file_path();
  std::ofstream output(shortcutsPath, std::ios::trunc);
  if (!output) {
    return;
  }

  output << "{\n";
  output << "  \"shortcuts\": [\n";
  const auto &shortcutBindings = editorWindow->getShortcutBindings();
  for (size_t i = 0; i < shortcutBindings.size(); ++i) {
    const auto &binding = shortcutBindings[i];
    output << "    {\"command\": \"" << json_escape(binding.actionName)
           << "\", \"shortcuts\": [";
    for (size_t slot = 0; slot < 2; ++slot) {
      if (slot > 0) {
        output << ", ";
      }
      const std::string value =
          slot < binding.displayTexts.size() ? binding.displayTexts[slot] : "";
      output << "\"" << json_escape(value) << "\"";
    }
    output << "]}";
    if (i + 1 < shortcutBindings.size()) {
      output << ",";
    }
    output << "\n";
  }
  output << "  ]\n";
  output << "}\n";
}

bool ShortcutManager::export_shortcuts_to_path(
    const std::string &targetPath, std::string &errorMessage) const {
  if (targetPath.empty()) {
    errorMessage = Localization::text("shortcut.export_no_destination");
    return false;
  }

  save_shortcuts();

  namespace fs = std::filesystem;
  const fs::path sourcePath(get_shortcuts_file_path());
  const fs::path destinationPath(targetPath);
  std::error_code error;
  if (!destinationPath.parent_path().empty()) {
    fs::create_directories(destinationPath.parent_path(), error);
    if (error) {
      errorMessage =
          std::string(Localization::text("shortcut.export_create_dir_prefix")) +
          error.message();
      return false;
    }
  }

  if (fs::equivalent(sourcePath, destinationPath, error) && !error) {
    return true;
  }
  error.clear();
  fs::copy_file(sourcePath, destinationPath, fs::copy_options::overwrite_existing,
                error);
  if (error) {
    errorMessage =
        std::string(Localization::text("shortcut.export_failed_prefix")) +
        error.message();
    return false;
  }

  return true;
}

bool ShortcutManager::import_shortcuts_from_path(
    const std::string &sourcePath, std::string &errorMessage) {
  if (sourcePath.empty()) {
    errorMessage = Localization::text("shortcut.import_no_source");
    return false;
  }

  std::ifstream input(sourcePath);
  if (!input) {
    errorMessage = Localization::text("shortcut.import_open_failed");
    return false;
  }
  if (read_shortcuts_json(input).empty()) {
    errorMessage = Localization::text("shortcut.import_empty");
    return false;
  }

  namespace fs = std::filesystem;
  const fs::path source(sourcePath);
  const fs::path destination(get_shortcuts_file_path());
  std::error_code error;
  if (!destination.parent_path().empty()) {
    fs::create_directories(destination.parent_path(), error);
    if (error) {
      errorMessage =
          std::string(Localization::text("shortcut.import_create_dir_prefix")) +
          error.message();
      return false;
    }
  }

  if (!(fs::equivalent(source, destination, error) && !error)) {
    error.clear();
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing,
                  error);
    if (error) {
      errorMessage =
          std::string(Localization::text("shortcut.import_failed_prefix")) +
          error.message();
      return false;
    }
  }

  load_shortcuts();
  return true;
}

void ShortcutManager::populate_shortcuts_store(GtkListStore *store) const {
  gtk_list_store_clear(store);
  const auto &shortcutBindings = editorWindow->getShortcutBindings();
  for (size_t i = 0; i < shortcutBindings.size(); ++i) {
    std::vector<std::string> texts = shortcutBindings[i].displayTexts;
    std::vector<std::string> defaults = shortcutBindings[i].defaultDisplayTexts;
    while (texts.size() < 2) {
      texts.push_back("");
    }
    while (defaults.size() < 2) {
      defaults.push_back("");
    }

    GtkTreeIter iter;
    gtk_list_store_append(store, &iter);
    gtk_list_store_set(store, &iter, ShortcutColumnIndex, static_cast<int>(i),
                       ShortcutColumnCommand,
                       shortcutBindings[i].actionName.c_str(),
                       ShortcutColumnShortcut1, texts[0].c_str(),
                       ShortcutColumnShortcut2, texts[1].c_str(),
                       ShortcutColumnStatus, Localization::text("shortcut.active"),
                       ShortcutColumnReset, Localization::text("shortcut.default"),
                       ShortcutColumnDefault1, defaults[0].c_str(),
                       ShortcutColumnDefault2, defaults[1].c_str(), -1);
  }
}

bool ShortcutManager::apply_shortcut(size_t index, size_t slot, guint key,
                                     GdkModifierType mods,
                                     const std::string &displayText) {
  auto &shortcutBindings = editorWindow->getShortcutBindings();
  if (index >= shortcutBindings.size() || slot >= 2) {
    return false;
  }

  auto &binding = shortcutBindings[index];
  while (binding.keys.size() < 2) {
    binding.keys.push_back(0);
  }
  while (binding.modifiers.size() < 2) {
    binding.modifiers.push_back(static_cast<GdkModifierType>(0));
  }
  while (binding.displayTexts.size() < 2) {
    binding.displayTexts.push_back("");
  }

  if (binding.keys[slot] == key && binding.modifiers[slot] == mods &&
      binding.displayTexts[slot] == displayText) {
    return true;
  }

  if (binding.keys[slot] != 0) {
    gtk_widget_remove_accelerator(binding.item, editorWindow->getAccelGroup(),
                                  binding.keys[slot], binding.modifiers[slot]);
  }

  binding.keys[slot] = key;
  binding.modifiers[slot] = mods;
  binding.displayTexts[slot] = displayText;
  binding.key = binding.keys.front();
  binding.mods = binding.modifiers.front();
  binding.displayText = binding.displayTexts.front();

  if (binding.keys[slot] != 0) {
    gtk_widget_add_accelerator(binding.item, "activate",
                               editorWindow->getAccelGroup(),
                               binding.keys[slot], binding.modifiers[slot],
                               GTK_ACCEL_VISIBLE);
  }

  return true;
}

bool ShortcutManager::restore_default_shortcuts(size_t index) {
  auto &shortcutBindings = editorWindow->getShortcutBindings();
  if (index >= shortcutBindings.size()) {
    return false;
  }

  auto &binding = shortcutBindings[index];
  std::vector<std::string> defaults = binding.defaultDisplayTexts;
  while (defaults.size() < 2) {
    defaults.push_back("");
  }

  for (size_t slot = 0; slot < 2; ++slot) {
    guint key = 0;
    GdkModifierType mods = static_cast<GdkModifierType>(0);
    if (!defaults[slot].empty() && !parse_shortcut(defaults[slot], key, mods)) {
      return false;
    }
    if (!apply_shortcut(index, slot, key, mods, defaults[slot])) {
      return false;
    }
  }

  return true;
}

void ShortcutManager::show_shortcuts_dialog() {
  GtkWindow *parent = editorWindow ? editorWindow->getDialogParentWindow()
                                   : nullptr;
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      Localization::text("shortcut.title"), parent,
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
      Localization::text("dialog.close"), GTK_RESPONSE_CLOSE, NULL);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 900, 560);
  gtk_window_set_resizable(GTK_WINDOW(dialog), TRUE);

  DialogTitlebarControls titlebarControls;
  install_dialog_titlebar(dialog, Localization::text("shortcut.title"),
                          titlebarControls, GTK_RESPONSE_CLOSE);

  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_widget_set_hexpand(content, TRUE);
  gtk_widget_set_vexpand(content, TRUE);

  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_size_request(box, 640, 440);
  gtk_widget_set_hexpand(box, TRUE);
  gtk_widget_set_vexpand(box, TRUE);
  gtk_container_set_border_width(GTK_CONTAINER(box), 12);
  gtk_container_add(GTK_CONTAINER(content), box);

  GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_hexpand(toolbar, TRUE);
  gtk_box_pack_start(GTK_BOX(box), toolbar, FALSE, FALSE, 0);

  GtkWidget *searchEntry = gtk_search_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(searchEntry),
                                 Localization::text("shortcut.search_placeholder"));
  gtk_entry_set_icon_from_icon_name(GTK_ENTRY(searchEntry),
                                    GTK_ENTRY_ICON_SECONDARY,
                                    "edit-find-symbolic");
  gtk_widget_set_hexpand(searchEntry, TRUE);
  gtk_box_pack_start(GTK_BOX(toolbar), searchEntry, TRUE, TRUE, 0);

  GtkWidget *importButton =
      gtk_button_new_with_label(Localization::text("shortcut.import"));
  gtk_widget_set_tooltip_text(importButton,
                              Localization::text("shortcut.import_tooltip"));
  gtk_box_pack_start(GTK_BOX(toolbar), importButton, FALSE, FALSE, 0);

  GtkWidget *exportButton =
      gtk_button_new_with_label(Localization::text("shortcut.export"));
  gtk_widget_set_tooltip_text(exportButton,
                              Localization::text("shortcut.export_tooltip"));
  gtk_box_pack_start(GTK_BOX(toolbar), exportButton, FALSE, FALSE, 0);

  GtkListStore *store =
      gtk_list_store_new(ShortcutColumnCount, G_TYPE_INT, G_TYPE_STRING,
                         G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                         G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  populate_shortcuts_store(store);

  GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(
      gtk_tree_model_filter_new(GTK_TREE_MODEL(store), nullptr));

  GtkWidget *treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(filter));
  gtk_widget_set_hexpand(treeView, TRUE);
  gtk_widget_set_vexpand(treeView, TRUE);
  gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), TRUE);
  gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(treeView), FALSE);
  gtk_tree_selection_set_mode(
      gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView)), GTK_SELECTION_SINGLE);

  GtkCellRenderer *commandRenderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *commandColumn =
      gtk_tree_view_column_new_with_attributes(Localization::text("shortcut.command"), commandRenderer,
                                               "text", ShortcutColumnCommand,
                                               nullptr);
  gtk_tree_view_column_set_expand(commandColumn, TRUE);
  gtk_tree_view_column_set_resizable(commandColumn, TRUE);
  gtk_tree_view_column_set_min_width(commandColumn, 260);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), commandColumn);

  GtkCellRenderer *shortcutRenderer1 = gtk_cell_renderer_text_new();
  g_object_set(shortcutRenderer1, "editable", TRUE, nullptr);
  g_object_set_data(G_OBJECT(shortcutRenderer1), "shortcut-slot",
                    GUINT_TO_POINTER(0));
  GtkTreeViewColumn *shortcutColumn1 =
      gtk_tree_view_column_new_with_attributes(Localization::text("shortcut.shortcut1"), shortcutRenderer1,
                                               "text", ShortcutColumnShortcut1,
                                               nullptr);
  gtk_tree_view_column_set_resizable(shortcutColumn1, TRUE);
  gtk_tree_view_column_set_expand(shortcutColumn1, FALSE);
  gtk_tree_view_column_set_min_width(shortcutColumn1, 130);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), shortcutColumn1);

  GtkCellRenderer *shortcutRenderer2 = gtk_cell_renderer_text_new();
  g_object_set(shortcutRenderer2, "editable", TRUE, nullptr);
  g_object_set_data(G_OBJECT(shortcutRenderer2), "shortcut-slot",
                    GUINT_TO_POINTER(1));
  GtkTreeViewColumn *shortcutColumn2 =
      gtk_tree_view_column_new_with_attributes(Localization::text("shortcut.shortcut2"), shortcutRenderer2,
                                               "text", ShortcutColumnShortcut2,
                                               nullptr);
  gtk_tree_view_column_set_resizable(shortcutColumn2, TRUE);
  gtk_tree_view_column_set_expand(shortcutColumn2, FALSE);
  gtk_tree_view_column_set_min_width(shortcutColumn2, 130);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), shortcutColumn2);

  GtkCellRenderer *statusRenderer = gtk_cell_renderer_text_new();
  GtkTreeViewColumn *statusColumn =
      gtk_tree_view_column_new_with_attributes(Localization::text("shortcut.status"), statusRenderer,
                                               "text", ShortcutColumnStatus,
                                               nullptr);
  gtk_tree_view_column_set_resizable(statusColumn, TRUE);
  gtk_tree_view_column_set_expand(statusColumn, TRUE);
  gtk_tree_view_column_set_min_width(statusColumn, 180);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), statusColumn);

  GtkCellRenderer *resetRenderer = gtk_cell_renderer_text_new();
  g_object_set(resetRenderer, "xalign", 0.5f, nullptr);
  GtkTreeViewColumn *resetColumn =
      gtk_tree_view_column_new_with_attributes(Localization::text("shortcut.reset"), resetRenderer, "text",
                                               ShortcutColumnReset, nullptr);
  gtk_tree_view_column_set_resizable(resetColumn, FALSE);
  gtk_tree_view_column_set_min_width(resetColumn, 90);
  gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), resetColumn);

  GtkWidget *scrolled = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_hexpand(scrolled, TRUE);
  gtk_widget_set_vexpand(scrolled, TRUE);
  gtk_container_add(GTK_CONTAINER(scrolled), treeView);
  gtk_box_pack_start(GTK_BOX(box), scrolled, TRUE, TRUE, 0);

  GtkWidget *statusLabel =
      gtk_label_new(Localization::text("shortcut.help"));
  gtk_widget_set_halign(statusLabel, GTK_ALIGN_START);
  gtk_widget_set_hexpand(statusLabel, TRUE);
  gtk_label_set_xalign(GTK_LABEL(statusLabel), 0.0f);
  gtk_label_set_ellipsize(GTK_LABEL(statusLabel), PANGO_ELLIPSIZE_END);
  gtk_box_pack_start(GTK_BOX(box), statusLabel, FALSE, FALSE, 0);

  ShortcutsDialogState state{this, store, filter, dialog, searchEntry, statusLabel,
                             resetColumn};
  gtk_tree_model_filter_set_visible_func(filter, shortcut_filter_visible, &state,
                                         nullptr);
  g_signal_connect(searchEntry, "changed", G_CALLBACK(on_shortcut_search_changed),
                   &state);
  g_signal_connect(importButton, "clicked",
                   G_CALLBACK(on_shortcuts_import_clicked), &state);
  g_signal_connect(exportButton, "clicked",
                   G_CALLBACK(on_shortcuts_export_clicked), &state);
  g_signal_connect(shortcutRenderer1, "edited",
                   G_CALLBACK(on_shortcut_cell_edited), &state);
  g_signal_connect(shortcutRenderer2, "edited",
                   G_CALLBACK(on_shortcut_cell_edited), &state);
  g_signal_connect(treeView, "button-press-event",
                   G_CALLBACK(on_shortcut_table_button_press), &state);

  gtk_widget_show_all(dialog);
  center_window_on_parent(dialog, parent);
  gtk_dialog_run(GTK_DIALOG(dialog));
  g_object_unref(filter);
  g_object_unref(store);
  gtk_widget_destroy(dialog);
}
