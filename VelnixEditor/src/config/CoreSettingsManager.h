#ifndef CORE_SETTINGS_MANAGER_H
#define CORE_SETTINGS_MANAGER_H

#include <string>

// Forward declaration
class EditorWindow;
class RecentFilesManager;

class CoreSettingsManager {
 public:
  explicit CoreSettingsManager(EditorWindow *editorWindow);

  // Load core settings from config file
  void load_settings(RecentFilesManager *recentFilesManager);

  // Save core settings to config file
  void save_settings() const;

  // Get config directory path
  std::string get_config_directory() const;

  // Get config file path
  std::string get_config_file_path() const;

 private:
  EditorWindow *editorWindow;
};

#endif  // CORE_SETTINGS_MANAGER_H
