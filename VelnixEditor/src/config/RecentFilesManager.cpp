#include "config/RecentFilesManager.h"
#include "config/ConfigPaths.h"
#include "editor/DocumentManager.h"
#include "ui/EditorWindow.h"
#include "ui/Localization.h"
#include <algorithm>
#include <fstream>

RecentFilesManager::RecentFilesManager(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {}

std::string RecentFilesManager::get_recent_files_file_path() const {
  return ::get_recent_files_file_path(editorWindow->getConfigDirectory());
}

void RecentFilesManager::load_recent_files() {
  recentFilesLoaded = false;
  editorWindow->clearRecentFiles();

  const std::string recentFilesPath = get_recent_files_file_path();
  std::ifstream input(recentFilesPath);
  if (!input) {
    recentFilesLoaded = true;
    refresh_recent_menu();
    return;
  }

  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }

    const size_t equals = line.find('=');
    if (equals == std::string::npos) {
      continue;
    }

    const std::string key = line.substr(0, equals);
    const std::string value = line.substr(equals + 1);
    if (key == "recent_file" && !value.empty()) {
      editorWindow->appendRecentFileEntry(value);
    }
  }

  std::vector<std::string> recentFiles = editorWindow->getRecentFiles();
  if (recentFiles.size() > static_cast<size_t>(editorWindow->getMaxRecentFiles())) {
    recentFiles.resize(editorWindow->getMaxRecentFiles());
    editorWindow->setRecentFiles(std::move(recentFiles));
  }

  recentFilesLoaded = true;
  refresh_recent_menu();
}

void RecentFilesManager::save_recent_files() const {
  if (!recentFilesLoaded) {
    return;
  }

  const std::string recentFilesPath = get_recent_files_file_path();
  std::ofstream output(recentFilesPath, std::ios::trunc);
  if (!output) {
    return;
  }

  for (const auto &path : editorWindow->getRecentFiles()) {
    output << "recent_file=" << path << "\n";
  }
}

void RecentFilesManager::add_recent_file(const std::string &path) {
  std::string normalizedPath = DocumentManager::normalize_path(path);
  if (normalizedPath.empty() || !editorWindow->isRecentFilesEnabled()) {
    return;
  }
  std::vector<std::string> recentFiles = editorWindow->getRecentFiles();
  auto it = std::find(recentFiles.begin(), recentFiles.end(), normalizedPath);
  if (it != recentFiles.end()) {
    recentFiles.erase(it);
  }
  recentFiles.insert(recentFiles.begin(), normalizedPath);
  while (recentFiles.size() > static_cast<size_t>(editorWindow->getMaxRecentFiles())) {
    recentFiles.pop_back();
  }
  editorWindow->setRecentFiles(std::move(recentFiles));
  save_recent_files();
  refresh_recent_menu();
}

void RecentFilesManager::open_recent_file(const std::string &path) {
  editorWindow->openDocumentPath(path);
}

void RecentFilesManager::on_recent_file_activate(GtkWidget *widget, gpointer data) {
  RecentFilesManager *manager = static_cast<RecentFilesManager *>(data);
  const char *path = static_cast<const char *>(
      g_object_get_data(G_OBJECT(widget), "recent-file-path"));
  if (path) {
    manager->open_recent_file(path);
  }
}

void RecentFilesManager::refresh_recent_menu() {
  GtkWidget *recentMenu = editorWindow->getRecentMenu();
  GList *children = gtk_container_get_children(GTK_CONTAINER(recentMenu));
  for (GList *l = children; l != NULL; l = l->next) {
    gtk_widget_destroy(GTK_WIDGET(l->data));
  }
  g_list_free(children);

  const std::vector<std::string> &recentFiles = editorWindow->getRecentFiles();
  if (!editorWindow->isRecentFilesEnabled()) {
    GtkWidget *disabled_item =
        gtk_menu_item_new_with_label(
            Localization::text("menu.recent_files_disabled"));
    gtk_widget_set_sensitive(disabled_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(recentMenu), disabled_item);
  } else if (recentFiles.empty()) {
    GtkWidget *empty_item =
        gtk_menu_item_new_with_label(Localization::text("menu.no_recent_files"));
    gtk_widget_set_sensitive(empty_item, FALSE);
    gtk_menu_shell_append(GTK_MENU_SHELL(recentMenu), empty_item);
  } else {
    for (const std::string &path : recentFiles) {
      GtkWidget *item = gtk_menu_item_new_with_label(path.c_str());
      g_object_set_data_full(G_OBJECT(item), "recent-file-path",
                             g_strdup(path.c_str()), g_free);
      g_signal_connect(item, "activate", G_CALLBACK(on_recent_file_activate),
                       this);
      gtk_menu_shell_append(GTK_MENU_SHELL(recentMenu), item);
    }
  }
  gtk_widget_show_all(recentMenu);
}
