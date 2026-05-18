#pragma once

#include <gtk/gtk.h>

#include <string>

namespace Localization {

void set_language(const std::string &language);
const std::string &current_language();

const char *text(const char *key);
const char *text_for_language(const std::string &language, const char *key);

void bind_widget_text(GtkWidget *widget, const char *key);
void bind_window_title(GtkWidget *widget, const char *key);
void bind_entry_placeholder(GtkWidget *widget, const char *key);
void refresh_widget_tree(GtkWidget *widget);
void refresh_open_windows();

}  // namespace Localization
