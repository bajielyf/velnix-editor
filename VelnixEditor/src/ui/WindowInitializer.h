#pragma once

#include "ui/EditorWindow.h"

#include <gtk/gtk.h>
#include <vector>

class WindowInitializer {
public:
    explicit WindowInitializer(EditorWindow *editorWindow);
    ~WindowInitializer() = default;

    void initialize();

    static void on_preferences_activate(GtkWidget *widget, gpointer data);
    static void on_shortcuts_activate(GtkWidget *widget, gpointer data);
    static void on_about_activate(GtkWidget *widget, gpointer data);

private:
    EditorWindow::WindowComponents createWindowComponents();
    EditorWindow::FileMenuItems buildMenuBar(
        EditorWindow::WindowComponents &components);
    EditorWindow::SettingsMenuItems buildSettingsMenu(
        GtkWidget *settingsMenu);
    void setupLayout(EditorWindow::WindowComponents &components);

    EditorWindow *editorWindow;
};
