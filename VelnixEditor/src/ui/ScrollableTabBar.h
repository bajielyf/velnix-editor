#pragma once

#include <gtk/gtk.h>

// Wrap a notebook with an independently scrollable document tab strip.
GtkWidget *createScrollableTabBar(GtkNotebook *notebook);
