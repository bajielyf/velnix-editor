#include "editor/MacroManager.h"
#include "config/ConfigPaths.h"
#include "editor/SearchManager.h"
#include "ui/EditorWindow.h"
#include "ui/Localization.h"
#include <gtk/gtk.h>
#include <iomanip>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <sstream>
#include <algorithm>

namespace {

bool message_uses_string_parameter(int message) {
  switch (message) {
    case SCI_REPLACESEL:
    case SCI_ADDTEXT:
    case SCI_INSERTTEXT:
    case SCI_APPENDTEXT:
    case SCI_SEARCHNEXT:
    case SCI_SEARCHPREV:
      return true;
    default:
      return false;
  }
}

void write_quoted_field(std::ostream& output, const std::string& value) {
  output << std::quoted(value);
}

bool read_quoted_field(std::istringstream& input, std::string& value) {
  input >> std::quoted(value);
  return !input.fail();
}

std::string build_search_command_payload(const std::string& findPattern,
                                         const std::string& replaceText,
                                         bool caseSensitive,
                                         bool wholeWord,
                                         bool regex) {
  return findPattern + "\n" + replaceText + "\n" +
         (caseSensitive ? "1" : "0") + "\n" +
         (wholeWord ? "1" : "0") + "\n" +
         (regex ? "1" : "0");
}

bool parse_search_command_payload(const std::string& payload,
                                  std::string& findPattern,
                                  std::string& replaceText,
                                  bool& caseSensitive,
                                  bool& wholeWord,
                                  bool& regex) {
  std::istringstream input(payload);
  std::string caseSensitiveText;
  std::string wholeWordText;
  std::string regexText;
  if (!std::getline(input, findPattern) ||
      !std::getline(input, replaceText) ||
      !std::getline(input, caseSensitiveText) ||
      !std::getline(input, wholeWordText) ||
      !std::getline(input, regexText)) {
    return false;
  }

  caseSensitive = caseSensitiveText == "1";
  wholeWord = wholeWordText == "1";
  regex = regexText == "1";
  return true;
}

}  // namespace

MacroManager::MacroManager(EditorWindow* editorWindow)
  : _editorWindow(editorWindow), _isRecording(false), _nextMacroId(1) {}

void MacroManager::startRecording() {
  if (_isRecording) return;

  _isRecording = true;
  _currentMacro.clear();
  if (GtkWidget* sci = _editorWindow->getCurrentScintilla()) {
    scintilla_send_message(SCINTILLA(sci), SCI_STARTRECORD, 0, 0);
  }
  std::cout << "Macro recording started" << std::endl;
}

void MacroManager::stopRecording() {
  if (!_isRecording) return;

  if (GtkWidget* sci = _editorWindow->getCurrentScintilla()) {
    scintilla_send_message(SCINTILLA(sci), SCI_STOPRECORD, 0, 0);
  }
  _isRecording = false;
  std::cout << "Macro recording stopped. Recorded " << _currentMacro.size() << " steps" << std::endl;
}

void MacroManager::playbackMacro(const Macro& macro) {
  std::cout << "Playing back macro with " << macro.size() << " steps" << std::endl;

  _isPlayingBack = true;
  for (const auto& step : macro) {
    executeMacroStep(step);
  }
  _isPlayingBack = false;
}

void MacroManager::saveCurrentMacro(const std::string& name) {
  if (_currentMacro.empty()) {
    std::cout << "No macro to save" << std::endl;
    return;
  }

  MacroShortcut macroShortcut;
  macroShortcut.name = name;
  macroShortcut.macro = _currentMacro;
  macroShortcut.id = _nextMacroId++;

  _macros.push_back(macroShortcut);
  saveMacros();
  refreshMacroMenu();

  std::cout << "Macro saved: " << name << std::endl;
}

void MacroManager::promptAndSaveCurrentMacro() {
  if (_currentMacro.empty()) {
    std::cout << "No macro to save" << std::endl;
    return;
  }

  GtkWidget* dialog = gtk_dialog_new_with_buttons(
      Localization::text("macro.save_current"),
      _editorWindow ? _editorWindow->getDialogParentWindow() : nullptr,
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
      Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
      Localization::text("dialog.save"), GTK_RESPONSE_ACCEPT,
      nullptr);

  GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget* grid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
  gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
  gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
  gtk_container_add(GTK_CONTAINER(content), grid);

  GtkWidget* nameLabel = gtk_label_new(Localization::text("macro.name"));
  gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(grid), nameLabel, 0, 0, 1, 1);

  GtkWidget* nameEntry = gtk_entry_new();
  const std::string defaultName = generateDefaultMacroName();
  gtk_entry_set_text(GTK_ENTRY(nameEntry), defaultName.c_str());
  gtk_grid_attach(GTK_GRID(grid), nameEntry, 1, 0, 1, 1);

  gtk_widget_show_all(dialog);
  gtk_widget_grab_focus(nameEntry);
  gtk_editable_select_region(GTK_EDITABLE(nameEntry), 0, -1);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    std::string name = gtk_entry_get_text(GTK_ENTRY(nameEntry));
    if (!name.empty()) {
      saveCurrentMacro(name);
    }
  }

  gtk_widget_destroy(dialog);
}

void MacroManager::loadMacros() {
  std::string filePath = getMacrosFilePath();

  if (!std::filesystem::exists(filePath)) {
    saveMacros();
    return;
  }

  std::ifstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Failed to open macros file: " << filePath << std::endl;
    return;
  }

  // Simple text-based format for now
  std::string line;
  MacroShortcut* currentMacro = nullptr;
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') continue;

    std::istringstream input(line);
    std::string token;
    if (!(input >> token)) {
      continue;
    }

    if (token == "macro") {
      int id = 0;
      std::string name;
      if (!(input >> id) || !read_quoted_field(input, name)) {
        currentMacro = nullptr;
        continue;
      }

      MacroShortcut macroShortcut;
      macroShortcut.id = id;
      macroShortcut.name = name;
      _macros.push_back(macroShortcut);
      currentMacro = &_macros.back();
      _nextMacroId = std::max(_nextMacroId, id + 1);
    } else if (token == "step" && currentMacro) {
      int message = 0;
      uptr_t wParam = 0;
      sptr_t lParam = 0;
      int macroType = 0;
      std::string sParam;
      if (!(input >> message >> wParam >> lParam >> macroType) ||
          !read_quoted_field(input, sParam)) {
        continue;
      }

      currentMacro->macro.emplace_back(message, wParam, lParam, sParam.c_str(),
                                       macroType);
    } else if (token == "end") {
      currentMacro = nullptr;
    }
  }

  file.close();
  std::cout << "Loaded " << _macros.size() << " macros" << std::endl;
  refreshMacroMenu();
}

void MacroManager::reloadMacros() {
  _macros.clear();
  _currentMacro.clear();
  _nextMacroId = 1;
  loadMacros();
}

void MacroManager::saveMacros() {
  std::string filePath = getMacrosFilePath();

  std::ofstream file(filePath);
  if (!file.is_open()) {
    std::cerr << "Failed to open macros file for writing: " << filePath << std::endl;
    return;
  }

  // Simple text-based format for now
  for (const auto& macro : _macros) {
    file << "macro " << macro.id << " ";
    write_quoted_field(file, macro.name);
    file << "\n";
    for (const auto& step : macro.macro) {
      file << "step "
           << step.message << " "
           << step.wParameter << " "
           << step.lParameter << " "
           << static_cast<int>(step.macroType) << " ";
      write_quoted_field(file, step.sParameter);
      file << "\n";
    }
    file << "end\n";
  }

  file.close();
  std::cout << "Saved " << _macros.size() << " macros" << std::endl;
}

void MacroManager::handleScintillaNotification(SCNotification* notification) {
  if (!_isRecording || _isPlayingBack) return;

  if (notification->nmhdr.code == SCN_MACRORECORD) {
    const bool usesString = message_uses_string_parameter(notification->message);
    addMacroStep(notification->message, notification->wParam,
                 usesString ? 0 : notification->lParam,
                 usesString ? reinterpret_cast<const char*>(notification->lParam)
                            : nullptr,
                 usesString ? static_cast<int>(MacroTypeIndex::mtUseSParameter)
                            : static_cast<int>(MacroTypeIndex::mtUseLParameter));
  }
}

void MacroManager::recordSearchAction(MacroAppCommand command,
                                      const std::string& findPattern,
                                      const std::string& replaceText,
                                      bool caseSensitive,
                                      bool wholeWord,
                                      bool regex) {
  if (!_isRecording || _isPlayingBack) {
    return;
  }

  const std::string payload = build_search_command_payload(
      findPattern, replaceText, caseSensitive, wholeWord, regex);
  addMacroStep(static_cast<int>(command), 0, 0, payload.c_str(),
               static_cast<int>(MacroTypeIndex::mtAppCommand));
}

void MacroManager::addMacroStep(int message, uptr_t wParam, sptr_t lParam, const char* sParam, int type) {
  RecordedMacroStep step(message, wParam, lParam, sParam, type);
  if (step.isMacroable()) {
    _currentMacro.push_back(step);
    std::cout << "Added macro step: message=" << message << std::endl;
  }
}

void MacroManager::executeMacroStep(const RecordedMacroStep& step) {
  if (!step.isMacroable()) return;

  if (step.macroType == MacroTypeIndex::mtAppCommand) {
    if (!_editorWindow) {
      return;
    }

    std::string findPattern;
    std::string replaceText;
    bool caseSensitive = false;
    bool wholeWord = false;
    bool regex = false;
    if (!parse_search_command_payload(step.sParameter, findPattern, replaceText,
                                      caseSensitive, wholeWord, regex)) {
      return;
    }

    switch (static_cast<MacroAppCommand>(step.message)) {
      case MacroAppCommand::FindNext:
        _editorWindow->executeMacroSearchCommand(
            MacroAppCommand::FindNext, findPattern, replaceText, caseSensitive,
            wholeWord, regex);
        break;
      case MacroAppCommand::FindPrevious:
        _editorWindow->executeMacroSearchCommand(
            MacroAppCommand::FindPrevious, findPattern, replaceText,
            caseSensitive, wholeWord, regex);
        break;
      case MacroAppCommand::FindAll:
        _editorWindow->executeMacroSearchCommand(
            MacroAppCommand::FindAll, findPattern, replaceText, caseSensitive,
            wholeWord, regex);
        break;
      case MacroAppCommand::ReplaceNext:
        _editorWindow->executeMacroSearchCommand(
            MacroAppCommand::ReplaceNext, findPattern, replaceText,
            caseSensitive, wholeWord, regex);
        break;
      case MacroAppCommand::ReplaceAll:
        _editorWindow->executeMacroSearchCommand(
            MacroAppCommand::ReplaceAll, findPattern, replaceText,
            caseSensitive, wholeWord, regex);
        break;
    }
    return;
  }

  GtkWidget* sci = _editorWindow->getCurrentScintilla();
  if (!sci) return;

  if (step.macroType == MacroTypeIndex::mtUseSParameter) {
    scintilla_send_message(SCINTILLA(sci), step.message, step.wParameter, (sptr_t)step.sParameter.c_str());
  } else {
    scintilla_send_message(SCINTILLA(sci), step.message, step.wParameter, step.lParameter);
  }
}

std::string MacroManager::getMacrosFilePath() const {
  return _editorWindow ? _editorWindow->getMacrosFilePath() : std::string();
}

void MacroManager::showPlaybackDialog() {
  if (!_editorWindow) {
    return;
  }

  if (_macros.empty()) {
    GtkWidget *dialog = gtk_message_dialog_new(
        _editorWindow->getDialogParentWindow(),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_CLOSE,
        "%s", Localization::text("macro.no_saved"));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return;
  }

  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      Localization::text("macro.play"), _editorWindow->getDialogParentWindow(),
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
      Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
      Localization::text("macro.play_button"), GTK_RESPONSE_ACCEPT,
      nullptr);

  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *scrolled = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(scrolled, 360, 240);

  GtkWidget *listBox = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(listBox), GTK_SELECTION_SINGLE);
  for (const auto &macro : _macros) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(macro.name.c_str());
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    g_object_set_data(G_OBJECT(row), "macro-id", GINT_TO_POINTER(macro.id));
    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_container_add(GTK_CONTAINER(listBox), row);
  }

  gtk_container_add(GTK_CONTAINER(scrolled), listBox);
  gtk_container_add(GTK_CONTAINER(content), scrolled);
  gtk_widget_show_all(dialog);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(listBox));
    if (selected) {
      const int macroId = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(selected), "macro-id"));
      if (const MacroShortcut *macro = findMacroById(macroId)) {
        playbackMacro(macro->macro);
      }
    } else {
      showInfoDialog(Localization::text("macro.play"),
                     Localization::text("macro.select_to_play"));
    }
  }

  gtk_widget_destroy(dialog);
}

void MacroManager::showMacroManagementDialog() {
  if (!_editorWindow) {
    return;
  }

  if (_macros.empty()) {
    GtkWidget *dialog = gtk_message_dialog_new(
        _editorWindow->getDialogParentWindow(),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_CLOSE,
        "%s", Localization::text("macro.no_saved"));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return;
  }

  GtkWidget *dialog = gtk_dialog_new_with_buttons(
      Localization::text("macro.management"), _editorWindow->getDialogParentWindow(),
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
      Localization::text("dialog.close"), GTK_RESPONSE_CLOSE,
      nullptr);

  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
  gtk_container_add(GTK_CONTAINER(content), vbox);

  // Instructions
  GtkWidget *instructions =
      gtk_label_new(Localization::text("macro.management_hint"));
  gtk_widget_set_halign(instructions, GTK_ALIGN_START);
  gtk_box_pack_start(GTK_BOX(vbox), instructions, FALSE, FALSE, 0);

  // Scrolled window for macro list
  GtkWidget *scrolled = gtk_scrolled_window_new(nullptr, nullptr);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(scrolled, 360, 200);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);

  // List box for macros
  GtkWidget *listBox = gtk_list_box_new();
  gtk_list_box_set_selection_mode(GTK_LIST_BOX(listBox), GTK_SELECTION_SINGLE);
  for (const auto &macro : _macros) {
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(macro.name.c_str());
    gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
    g_object_set_data(G_OBJECT(row), "macro-id", GINT_TO_POINTER(macro.id));
    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_container_add(GTK_CONTAINER(listBox), row);
  }
  gtk_container_add(GTK_CONTAINER(scrolled), listBox);

  // Buttons
  GtkWidget *buttonBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start(GTK_BOX(vbox), buttonBox, FALSE, FALSE, 0);

  GtkWidget *renameButton =
      gtk_button_new_with_label(Localization::text("macro.rename"));
  GtkWidget *deleteButton =
      gtk_button_new_with_label(Localization::text("macro.delete"));
  gtk_box_pack_end(GTK_BOX(buttonBox), deleteButton, FALSE, FALSE, 0);
  gtk_box_pack_end(GTK_BOX(buttonBox), renameButton, FALSE, FALSE, 0);

  // Connect signals
  g_object_set_data(G_OBJECT(renameButton), "macro-manager", this);
  g_object_set_data(G_OBJECT(renameButton), "macro-list-box", listBox);
  g_signal_connect(renameButton, "clicked", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
    (void)data;
    auto* manager = static_cast<MacroManager*>(
        g_object_get_data(G_OBJECT(widget), "macro-manager"));
    GtkWidget *macroListBox = GTK_WIDGET(
        g_object_get_data(G_OBJECT(widget), "macro-list-box"));
    if (!manager || !macroListBox) {
      return;
    }

    GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(macroListBox));
    if (!selected) {
      manager->showInfoDialog(Localization::text("macro.rename_title"),
                              Localization::text("macro.select_to_rename"));
      return;
    }

    const int macroId = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(selected), "macro-id"));
    const MacroShortcut *macro = manager->findMacroById(macroId);
    if (!macro) {
      manager->showWarningDialog(Localization::text("macro.rename_title"),
                                 Localization::text("macro.not_found"));
      return;
    }

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        Localization::text("macro.rename_title"), manager->_editorWindow->getDialogParentWindow(),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
        Localization::text("macro.rename_button"), GTK_RESPONSE_ACCEPT,
        nullptr);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);
    gtk_container_add(GTK_CONTAINER(content), grid);

    GtkWidget *currentLabel =
        gtk_label_new(Localization::text("macro.current_name"));
    gtk_widget_set_halign(currentLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), currentLabel, 0, 0, 1, 1);

    GtkWidget *currentValue = gtk_label_new(macro->name.c_str());
    gtk_widget_set_halign(currentValue, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), currentValue, 1, 0, 1, 1);

    GtkWidget *nameLabel = gtk_label_new(Localization::text("macro.new_name"));
    gtk_widget_set_halign(nameLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), nameLabel, 0, 1, 1, 1);

    GtkWidget *nameEntry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(nameEntry), macro->name.c_str());
    gtk_grid_attach(GTK_GRID(grid), nameEntry, 1, 1, 1, 1);

    gtk_widget_show_all(dialog);
    gtk_widget_grab_focus(nameEntry);
    gtk_editable_select_region(GTK_EDITABLE(nameEntry), 0, -1);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
      const char *newName = gtk_entry_get_text(GTK_ENTRY(nameEntry));
      if (!newName || strlen(newName) == 0) {
        manager->showWarningDialog(Localization::text("macro.rename_title"),
                                   Localization::text("macro.empty_name"));
      } else if (manager->renameMacro(macroId, newName)) {
        GtkWidget *rowChild = gtk_bin_get_child(GTK_BIN(selected));
        if (GTK_IS_LABEL(rowChild)) {
          gtk_label_set_text(GTK_LABEL(rowChild), newName);
        }
      } else {
        manager->showWarningDialog(Localization::text("macro.rename_title"),
                                   Localization::text("macro.rename_failed"));
      }
    }

    gtk_widget_destroy(dialog);
  }), nullptr);

  g_object_set_data(G_OBJECT(deleteButton), "macro-manager", this);
  g_object_set_data(G_OBJECT(deleteButton), "macro-list-box", listBox);
  g_signal_connect(deleteButton, "clicked", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
    (void)data;
    auto* manager = static_cast<MacroManager*>(
        g_object_get_data(G_OBJECT(widget), "macro-manager"));
    GtkWidget *macroListBox = GTK_WIDGET(
        g_object_get_data(G_OBJECT(widget), "macro-list-box"));
    if (!manager || !macroListBox) {
      return;
    }

    GtkListBoxRow *selected = gtk_list_box_get_selected_row(GTK_LIST_BOX(macroListBox));
    if (!selected) {
      manager->showInfoDialog(Localization::text("macro.delete_title"),
                              Localization::text("macro.select_to_delete"));
      return;
    }

    const int macroId = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(selected), "macro-id"));
    const MacroShortcut *macro = manager->findMacroById(macroId);
    if (!macro) {
      manager->showWarningDialog(Localization::text("macro.delete_title"),
                                 Localization::text("macro.not_found"));
      return;
    }

    const std::string question =
        std::string(Localization::text("macro.delete_question_prefix")) +
        macro->name + Localization::text("macro.delete_question_suffix");

    GtkWidget *dialog = gtk_message_dialog_new(
        manager->_editorWindow->getDialogParentWindow(),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_MESSAGE_WARNING,
        GTK_BUTTONS_NONE,
        "%s", question.c_str());
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog), "%s",
        Localization::text("macro.delete_warning"));
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
                           Localization::text("macro.delete"), GTK_RESPONSE_ACCEPT,
                           nullptr);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
      manager->deleteMacro(macroId);
      gtk_widget_destroy(GTK_WIDGET(selected));
    }
    gtk_widget_destroy(dialog);
  }), nullptr);

  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

std::string MacroManager::generateDefaultMacroName() const {
  int index = 1;
  while (true) {
    const std::string candidate =
        std::string(Localization::text("macro.default_name")) +
        std::to_string(index);
    bool exists = false;
    for (const auto& macro : _macros) {
      if (macro.name == candidate) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      return candidate;
    }
    ++index;
  }
}

void MacroManager::addMacroMenuItems(GtkWidget* macroMenu) {
  _macroMenu = macroMenu;
  refreshMacroMenu();
}

void MacroManager::refreshMacroMenu() {
  if (!_macroMenu) {
    return;
  }

  // Clear existing dynamic macro items only, keep the fixed menu items.
  GList* children = gtk_container_get_children(GTK_CONTAINER(_macroMenu));
  for (GList* iter = children; iter != nullptr; iter = iter->next) {
    GtkWidget* item = GTK_WIDGET(iter->data);
    if (g_object_get_data(G_OBJECT(item), "macro-dynamic-item")) {
      gtk_container_remove(GTK_CONTAINER(_macroMenu), item);
    }
  }
  g_list_free(children);

  if (_macros.empty()) {
    gtk_widget_show_all(_macroMenu);
    return;
  }

  GtkWidget* separator = gtk_separator_menu_item_new();
  g_object_set_data(G_OBJECT(separator), "macro-dynamic-item", GINT_TO_POINTER(1));
  gtk_menu_shell_append(GTK_MENU_SHELL(_macroMenu), separator);

  // Add macro items
  for (const auto& macro : _macros) {
    GtkWidget* item = gtk_menu_item_new_with_label(macro.name.c_str());
    g_object_set_data(G_OBJECT(item), "macro-dynamic-item", GINT_TO_POINTER(1));
    g_object_set_data_full(G_OBJECT(item), "macro-name", g_strdup(macro.name.c_str()), g_free);
    g_signal_connect(item, "activate", G_CALLBACK(+[](GtkWidget* widget, gpointer data) {
      auto* manager = static_cast<MacroManager*>(data);
      const char* name = static_cast<const char*>(g_object_get_data(G_OBJECT(widget), "macro-name"));
      if (!name) {
        return;
      }

      // Find and playback macro
      for (const auto& macro : manager->_macros) {
        if (macro.name == name) {
          manager->playbackMacro(macro.macro);
          break;
        }
      }
    }), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(_macroMenu), item);
  }

  gtk_widget_show_all(_macroMenu);
}

MacroShortcut *MacroManager::findMacroById(int id) {
  auto it = std::find_if(_macros.begin(), _macros.end(),
                         [id](const MacroShortcut& macro) { return macro.id == id; });
  return it != _macros.end() ? &(*it) : nullptr;
}

const MacroShortcut *MacroManager::findMacroById(int id) const {
  auto it = std::find_if(_macros.begin(), _macros.end(),
                         [id](const MacroShortcut& macro) { return macro.id == id; });
  return it != _macros.end() ? &(*it) : nullptr;
}

void MacroManager::showInfoDialog(const std::string& title, const std::string& message) const {
  if (!_editorWindow) {
    return;
  }

  GtkWidget *dialog = gtk_message_dialog_new(
      _editorWindow->getDialogParentWindow(),
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
      GTK_MESSAGE_INFO,
      GTK_BUTTONS_CLOSE,
      "%s", message.c_str());
  gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

void MacroManager::showWarningDialog(const std::string& title,
                                     const std::string& message) const {
  if (!_editorWindow) {
    return;
  }

  GtkWidget *dialog = gtk_message_dialog_new(
      _editorWindow->getDialogParentWindow(),
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
      GTK_MESSAGE_WARNING,
      GTK_BUTTONS_CLOSE,
      "%s", message.c_str());
  gtk_window_set_title(GTK_WINDOW(dialog), title.c_str());
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

void MacroManager::deleteMacro(int id) {
  auto it = std::find_if(_macros.begin(), _macros.end(),
                         [id](const MacroShortcut& macro) {
                           return macro.id == id;
                         });

  if (it != _macros.end()) {
    const std::string deletedName = it->name;
    _macros.erase(it);
    saveMacros();
    refreshMacroMenu();
    std::cout << "Macro deleted: " << deletedName << std::endl;
  }
}

bool MacroManager::renameMacro(int id, const std::string& newName) {
  if (newName.empty()) {
    return false;
  }

  // Check if new name already exists
  for (const auto& macro : _macros) {
    if (macro.id != id && macro.name == newName) {
      return false;
    }
  }

  auto *macro = findMacroById(id);
  if (!macro) {
    return false;
  }

  if (macro->name == newName) {
    return true;
  }

  const std::string oldName = macro->name;
  macro->name = newName;
  saveMacros();
  refreshMacroMenu();
  std::cout << "Macro renamed: " << oldName << " -> " << newName << std::endl;
  return true;
}
