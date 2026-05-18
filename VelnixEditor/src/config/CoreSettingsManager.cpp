#include "config/ConfigPaths.h"
#include "config/CoreSettingsManager.h"
#include "config/ConfigStore.h"
#include "core/Logger.h"
#include "config/RecentFilesManager.h"
#include "ui/EditorWindow.h"

CoreSettingsManager::CoreSettingsManager(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {}

std::string CoreSettingsManager::get_config_directory() const {
  return ::get_default_config_directory(editorWindow->getConfigDirectory());
}

std::string CoreSettingsManager::get_config_file_path() const {
  return ::get_config_file_path(editorWindow->getConfigDirectory());
}

void CoreSettingsManager::load_settings(RecentFilesManager *recentFilesManager) {
  std::string initialConfigPath = get_config_file_path();
  editorWindow->setConfigFilePath(initialConfigPath);

  ConfigStore store(initialConfigPath);
  bool loaded = store.load();
  if (!loaded) {
    Logger::warning("Core settings failed to load from '%s': %s",
                    initialConfigPath.c_str(), store.get_last_error().c_str());
    save_settings();
    (void)recentFilesManager;
    return;
  }

  std::string configDirectory = store.get_string(ConfigSchema::Items::ConfigDirectory.key);
  if (!configDirectory.empty() &&
      configDirectory != editorWindow->getConfigDirectory()) {
    editorWindow->setConfigDirectory(configDirectory);
    std::string redirectedPath = get_config_file_path();
    if (redirectedPath != initialConfigPath) {
      editorWindow->setConfigFilePath(redirectedPath);
      ConfigStore redirectedStore(redirectedPath);
      if (!redirectedStore.load()) {
        Logger::warning("Redirected configuration '%s' failed to load: %s",
                        redirectedPath.c_str(),
                        redirectedStore.get_last_error().c_str());
      } else {
        store = std::move(redirectedStore);
      }
    }
  }

  editorWindow->loadConfigFromStore(store);
  editorWindow->setConfigFilePath(get_config_file_path());

  (void)recentFilesManager;
}

void CoreSettingsManager::save_settings() const {
  editorWindow->setConfigFilePath(get_config_file_path());
  ConfigStore store(editorWindow->getConfigFilePath());
  editorWindow->saveConfigToStore(store);
  if (!store.save()) {
    Logger::error("Failed to persist configuration to '%s': %s",
                  editorWindow->getConfigFilePath().c_str(),
                  store.get_last_error().c_str());
  }
}
