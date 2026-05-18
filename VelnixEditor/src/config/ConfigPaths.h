#ifndef CONFIG_PATHS_H
#define CONFIG_PATHS_H

#include <string>

// Returns the base configuration directory for the application.
// If customDir is empty, the GTK user config directory is used.
std::string get_default_config_directory(const std::string &customDir);

// Returns the full path to the main config file using the unified default directory.
std::string get_config_file_path(const std::string &customDir);

// Returns the full path to the shortcuts config file using the unified default directory.
std::string get_shortcuts_file_path(const std::string &customDir);

// Returns the full path to the macros file using the unified default directory.
std::string get_macros_file_path(const std::string &customDir);

// Returns the full path to the recent-files persistence file.
std::string get_recent_files_file_path(const std::string &customDir);

// Returns the full path to the session restore persistence file.
std::string get_session_file_path(const std::string &customDir);

#endif  // CONFIG_PATHS_H
