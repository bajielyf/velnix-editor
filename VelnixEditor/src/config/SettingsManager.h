#pragma once

#include <gtk/gtk.h>
#include <memory>
#include <string>

// Forward declarations
class EditorWindow;
class CoreSettingsManager;
class ShortcutManager;
class RecentFilesManager;

class SettingsManager {
 public:
  explicit SettingsManager(EditorWindow *editorWindow);
  ~SettingsManager();

  // Load all settings from disk
  void load_settings();

  // Save all settings to disk
  void save_settings() const;

  // Save current document to path and add to recent files
  bool save_file_to_path(const std::string &path);

  // Show preferences dialog
  void show_preferences_dialog();

  // Delegate access to sub-managers
  CoreSettingsManager *get_core_settings() const { return coreSettings.get(); }
  ShortcutManager *get_shortcuts() const { return shortcuts.get(); }
  RecentFilesManager *get_recent_files() const { return recentFiles.get(); }

 private:
  bool migrate_config_directory(const std::string &oldConfigDir,
                                const std::string &newConfigDir,
                                std::string &errorMessage) const;
  void show_warning_dialog(const std::string &primary,
                           const std::string &secondary) const;

  EditorWindow *editorWindow;
  std::unique_ptr<CoreSettingsManager> coreSettings;
  std::unique_ptr<ShortcutManager> shortcuts;
  std::unique_ptr<RecentFilesManager> recentFiles;
};
