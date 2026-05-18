#ifndef SHORTCUT_MANAGER_H
#define SHORTCUT_MANAGER_H

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <cstddef>
#include <string>
#include <vector>
#include "core/EditorTypes.h"

// Forward declaration
class EditorWindow;

class ShortcutManager {
 public:
  explicit ShortcutManager(EditorWindow *editorWindow);

  // Parse shortcut text (e.g., "Ctrl+S") to key and modifiers
  bool parse_shortcut(const std::string &text, guint &key,
                     GdkModifierType &mods) const;

  // Load shortcuts from file
  void load_shortcuts();

  // Save shortcuts to file
  void save_shortcuts() const;

  // Get effective shortcuts file path
  std::string get_shortcuts_file_path() const;

  // Show shortcuts configuration dialog
  void show_shortcuts_dialog();

  bool export_shortcuts_to_path(const std::string &targetPath,
                                std::string &errorMessage) const;

  bool import_shortcuts_from_path(const std::string &sourcePath,
                                  std::string &errorMessage);

  void populate_shortcuts_store(GtkListStore *store) const;

  // Check if a shortcut conflicts with existing bindings
  std::vector<std::string> check_shortcut_conflicts(guint key, GdkModifierType mods) const;

  // Get all conflicting actions for a given shortcut
  std::vector<std::string> get_conflicting_actions(guint key, GdkModifierType mods) const;

  std::vector<std::string> get_conflicting_actions(guint key,
                                                   GdkModifierType mods,
                                                   size_t ignoredIndex,
                                                   size_t ignoredSlot) const;

  bool apply_shortcut(size_t index, size_t slot, guint key, GdkModifierType mods,
                      const std::string &displayText);

  bool restore_default_shortcuts(size_t index);

 private:
  EditorWindow *editorWindow;

  // Get keyval from name (handles special characters)
  guint get_keyval_from_name(const std::string &name) const;

  // Normalize shortcut text to uppercase
  std::string normalize_shortcut_text(const std::string &text) const;
};

#endif  // SHORTCUT_MANAGER_H
