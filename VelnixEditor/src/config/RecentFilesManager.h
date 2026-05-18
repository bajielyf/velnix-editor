#ifndef RECENT_FILES_MANAGER_H
#define RECENT_FILES_MANAGER_H

#include <gtk/gtk.h>
#include <string>

// Forward declaration
class EditorWindow;

class RecentFilesManager {
 public:
  explicit RecentFilesManager(EditorWindow *editorWindow);

  // Load recent files from the dedicated persistence file.
  void load_recent_files();

  // Save recent files to the dedicated persistence file.
  void save_recent_files() const;

  // Get the effective persistence path used by recent files.
  std::string get_recent_files_file_path() const;

  // Add a file to recent files list (with deduplication and limit enforcement)
  void add_recent_file(const std::string &path);

  // Open a recently used file
  void open_recent_file(const std::string &path);

  // Refresh the recent files menu UI
  void refresh_recent_menu();

 private:
  EditorWindow *editorWindow;
  bool recentFilesLoaded = false;

  // GTK signal handler for menu item activation
  static void on_recent_file_activate(GtkWidget *widget, gpointer data);
};

#endif  // RECENT_FILES_MANAGER_H
