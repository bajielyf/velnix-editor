#include "ui/WindowInitializer.h"

#include "config/SettingsManager.h"
#include "config/ShortcutManager.h"
#include "editor/SearchManager.h"
#include "ui/EditorWindow.h"
#include "ui/EventHandler.h"
#include "ui/Localization.h"

#include <cctype>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifndef VELNIX_EDITOR_VERSION
#define VELNIX_EDITOR_VERSION "0.0.0"
#endif

#ifndef VELNIX_SOURCE_DIR
#define VELNIX_SOURCE_DIR "."
#endif

#ifndef VELNIX_ASSET_DIR
#define VELNIX_ASSET_DIR "docs/assets"
#endif

#ifndef VELNIX_HOMEPAGE
#define VELNIX_HOMEPAGE "https://bajielyf.github.io/velnix-editor"
#endif

namespace {

constexpr const char *kProjectHomepage = VELNIX_HOMEPAGE;

GtkWidget *localized_menu_item(const char *key) {
  GtkWidget *item = gtk_menu_item_new_with_label(Localization::text(key));
  Localization::bind_widget_text(item, key);
  return item;
}

GtkWidget *localized_check_menu_item(const char *key) {
  GtkWidget *item = gtk_check_menu_item_new_with_label(Localization::text(key));
  Localization::bind_widget_text(item, key);
  return item;
}

constexpr int kCustomKeywordMenuColorCount = 6;
constexpr long kCustomKeywordMenuColors[kCustomKeywordMenuColorCount] = {
    0x00FFFF,
    0x8FEA8F,
    0xFFD07A,
    0xD78CFF,
    0x66B8FF,
    0xF2A4FF,
};
constexpr const char *kCustomKeywordMenuColorKeys[kCustomKeywordMenuColorCount] = {
    "menu.highlight_yellow",
    "menu.highlight_green",
    "menu.highlight_blue",
    "menu.highlight_pink",
    "menu.highlight_orange",
    "menu.highlight_purple",
};

GdkRGBA scintilla_color_to_rgba(long color) {
  return GdkRGBA{
      static_cast<double>(color & 0xff) / 255.0,
      static_cast<double>((color >> 8) & 0xff) / 255.0,
      static_cast<double>((color >> 16) & 0xff) / 255.0,
      1.0,
  };
}

gboolean on_highlight_color_swatch_draw(GtkWidget *widget, cairo_t *cr,
                                        gpointer data) {
  (void)data;
  const long color = GPOINTER_TO_INT(
      g_object_get_data(G_OBJECT(widget), "highlight-color"));
  const GdkRGBA rgba = scintilla_color_to_rgba(color);
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  cairo_set_source_rgba(cr, rgba.red, rgba.green, rgba.blue, rgba.alpha);
  cairo_rectangle(cr, 1.5, 1.5, allocation.width - 3.0,
                  allocation.height - 3.0);
  cairo_fill_preserve(cr);
  cairo_set_source_rgba(cr, 0.16, 0.16, 0.16, 0.5);
  cairo_set_line_width(cr, 1.0);
  cairo_stroke(cr);
  return FALSE;
}

GtkWidget *localized_color_menu_item(const char *key, long color) {
  GtkWidget *item = gtk_menu_item_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *swatch = gtk_drawing_area_new();
  GtkWidget *label = gtk_label_new(Localization::text(key));

  gtk_widget_set_size_request(swatch, 20, 14);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);
  gtk_box_pack_start(GTK_BOX(box), swatch, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(item), box);

  g_object_set_data(G_OBJECT(swatch), "highlight-color",
                    GINT_TO_POINTER(static_cast<int>(color)));
  g_signal_connect(swatch, "draw",
                   G_CALLBACK(on_highlight_color_swatch_draw), nullptr);
  Localization::bind_widget_text(label, key);
  return item;
}

void append_custom_highlight_menu_items(GtkWidget *menu,
                                        EditorWindow *editorWindow) {
  for (int i = 0; i < kCustomKeywordMenuColorCount; ++i) {
    GtkWidget *colorItem = localized_color_menu_item(
        kCustomKeywordMenuColorKeys[i], kCustomKeywordMenuColors[i]);
    g_object_set_data(G_OBJECT(colorItem), "highlight-color-index",
                      GINT_TO_POINTER(i));
    g_signal_connect(colorItem, "activate",
                     G_CALLBACK(EventHandler::on_custom_highlight_color_activate),
                     editorWindow);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), colorItem);
  }
}

void append_clear_custom_highlight_menu_items(GtkWidget *menu,
                                             EditorWindow *editorWindow) {
  for (int i = 0; i < kCustomKeywordMenuColorCount; ++i) {
    GtkWidget *colorItem = localized_color_menu_item(
        kCustomKeywordMenuColorKeys[i], kCustomKeywordMenuColors[i]);
    g_object_set_data(G_OBJECT(colorItem), "highlight-color-index",
                      GINT_TO_POINTER(i));
    g_signal_connect(
        colorItem, "activate",
        G_CALLBACK(EventHandler::on_clear_custom_highlight_color_activate),
        editorWindow);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), colorItem);
  }

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

  GtkWidget *clearItem = localized_menu_item("menu.clear_highlights");
  g_signal_connect(clearItem, "activate",
                   G_CALLBACK(EventHandler::on_clear_custom_highlights_activate),
                   editorWindow);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), clearItem);
}

GtkWidget *create_markup_label(const std::string &markup, float xalign = 0.0f) {
  GtkWidget *label = gtk_label_new(nullptr);
  gtk_label_set_markup(GTK_LABEL(label), markup.c_str());
  gtk_label_set_xalign(GTK_LABEL(label), xalign);
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);
  return label;
}

GtkWidget *create_text_label(const char *text, float xalign = 0.0f) {
  GtkWidget *label = gtk_label_new(text);
  gtk_label_set_xalign(GTK_LABEL(label), xalign);
  gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);
  return label;
}

GdkPixbuf *load_about_logo_pixbuf() {
  const std::vector<std::string> candidatePaths = {
      std::string(VELNIX_SOURCE_DIR) + "/docs/assets/logo-about.png",
      std::string(VELNIX_ASSET_DIR) + "/logo-about.png",
      "docs/assets/logo-about.png",
      "../docs/assets/logo-about.png",
      "../../docs/assets/logo-about.png",
  };

  for (const std::string &path : candidatePaths) {
    GError *error = nullptr;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path.c_str(), &error);
    if (pixbuf) {
      return pixbuf;
    }
    if (error) {
      g_error_free(error);
    }
  }

  return nullptr;
}

GdkPixbuf *load_app_logo_pixbuf() {
  const std::vector<std::string> candidatePaths = {
      std::string(VELNIX_SOURCE_DIR) + "/docs/assets/logo-app.png",
      std::string(VELNIX_ASSET_DIR) + "/logo-app.png",
      "docs/assets/logo-app.png",
      "../docs/assets/logo-app.png",
      "../../docs/assets/logo-app.png",
  };

  for (const std::string &path : candidatePaths) {
    GError *error = nullptr;
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path.c_str(), &error);
    if (pixbuf) {
      return pixbuf;
    }
    if (error) {
      g_error_free(error);
    }
  }

  return nullptr;
}

GdkPixbuf *crop_transparent_padding(GdkPixbuf *source) {
  if (!source || !gdk_pixbuf_get_has_alpha(source)) {
    return source ? static_cast<GdkPixbuf *>(g_object_ref(source)) : nullptr;
  }

  const int width = gdk_pixbuf_get_width(source);
  const int height = gdk_pixbuf_get_height(source);
  const int rowstride = gdk_pixbuf_get_rowstride(source);
  const int channels = gdk_pixbuf_get_n_channels(source);
  const guchar *pixels = gdk_pixbuf_get_pixels(source);
  if (width <= 0 || height <= 0 || channels < 4 || !pixels) {
    return static_cast<GdkPixbuf *>(g_object_ref(source));
  }

  int minX = width;
  int minY = height;
  int maxX = -1;
  int maxY = -1;
  for (int y = 0; y < height; ++y) {
    const guchar *row = pixels + y * rowstride;
    for (int x = 0; x < width; ++x) {
      const guchar alpha = row[x * channels + 3];
      if (alpha <= 8) {
        continue;
      }
      minX = std::min(minX, x);
      minY = std::min(minY, y);
      maxX = std::max(maxX, x);
      maxY = std::max(maxY, y);
    }
  }

  if (maxX < minX || maxY < minY) {
    return static_cast<GdkPixbuf *>(g_object_ref(source));
  }

  GdkPixbuf *subpixbuf =
      gdk_pixbuf_new_subpixbuf(source, minX, minY, maxX - minX + 1,
                               maxY - minY + 1);
  if (!subpixbuf) {
    return static_cast<GdkPixbuf *>(g_object_ref(source));
  }

  GdkPixbuf *cropped = gdk_pixbuf_copy(subpixbuf);
  g_object_unref(subpixbuf);
  return cropped ? cropped : static_cast<GdkPixbuf *>(g_object_ref(source));
}

struct MenuLogoData {
  GdkPixbuf *pixbuf = nullptr;
  int requestedWidth = 0;
};

int calculate_menu_logo_width(GdkPixbuf *source, int targetHeight) {
  if (!source || targetHeight <= 0) {
    return 1;
  }

  const int sourceWidth = gdk_pixbuf_get_width(source);
  const int sourceHeight = gdk_pixbuf_get_height(source);
  if (sourceWidth <= 0 || sourceHeight <= 0) {
    return 1;
  }

  return std::max(1, sourceWidth * targetHeight / sourceHeight);
}

void update_menu_logo_width_request(GtkWidget *widget, int height,
                                    MenuLogoData *logoData) {
  if (!widget || !logoData || !logoData->pixbuf || height <= 0) {
    return;
  }

  const int desiredWidth = calculate_menu_logo_width(logoData->pixbuf, height);
  if (desiredWidth == logoData->requestedWidth) {
    return;
  }

  logoData->requestedWidth = desiredWidth;
  gtk_widget_set_size_request(widget, desiredWidth, 1);
}

void on_menu_logo_size_allocate(GtkWidget *widget, GtkAllocation *allocation,
                                gpointer data) {
  auto *logoData = static_cast<MenuLogoData *>(data);
  if (!allocation) {
    return;
  }
  update_menu_logo_width_request(widget, allocation->height, logoData);
}

gboolean draw_menu_logo(GtkWidget *widget, cairo_t *cr, gpointer data) {
  auto *logoData = static_cast<MenuLogoData *>(data);
  if (!logoData || !logoData->pixbuf) {
    return FALSE;
  }

  GdkPixbuf *source = logoData->pixbuf;
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  const int sourceWidth = gdk_pixbuf_get_width(source);
  const int sourceHeight = gdk_pixbuf_get_height(source);
  if (sourceWidth <= 0 || sourceHeight <= 0 || allocation.width <= 0 ||
      allocation.height <= 0) {
    return FALSE;
  }

  const int targetHeight = std::max(1, allocation.height);
  const int targetWidth = calculate_menu_logo_width(source, targetHeight);
  update_menu_logo_width_request(widget, targetHeight, logoData);
  const int x = std::max(0, (allocation.width - targetWidth) / 2);
  const int y = std::max(0, (allocation.height - targetHeight) / 2);

  GdkPixbuf *scaled = gdk_pixbuf_scale_simple(source, targetWidth,
                                             targetHeight,
                                             GDK_INTERP_BILINEAR);
  if (!scaled) {
    return FALSE;
  }

  gdk_cairo_set_source_pixbuf(cr, scaled, x, y);
  cairo_paint(cr);
  g_object_unref(scaled);
  return FALSE;
}

GtkWidget *create_menu_logo_widget() {
  GdkPixbuf *pixbuf = load_app_logo_pixbuf();
  if (!pixbuf) {
    return nullptr;
  }

  GdkPixbuf *cropped = crop_transparent_padding(pixbuf);
  g_object_unref(pixbuf);
  pixbuf = cropped;
  if (!pixbuf) {
    return nullptr;
  }

  GtkWidget *drawingArea = gtk_drawing_area_new();
  auto *logoData = new MenuLogoData{pixbuf, 0};
  const int widthRequest = std::max(28, calculate_menu_logo_width(pixbuf, 32));
  logoData->requestedWidth = widthRequest;

  gtk_widget_set_size_request(drawingArea, widthRequest, 1);
  gtk_widget_set_margin_start(drawingArea, 6);
  gtk_widget_set_margin_end(drawingArea, 6);
  g_signal_connect(drawingArea, "size-allocate",
                   G_CALLBACK(on_menu_logo_size_allocate), logoData);
  g_signal_connect_data(drawingArea, "draw", G_CALLBACK(draw_menu_logo),
                        logoData,
                        +[](gpointer data, GClosure *) {
                          auto *logoData = static_cast<MenuLogoData *>(data);
                          if (logoData && logoData->pixbuf) {
                            g_object_unref(logoData->pixbuf);
                          }
                          delete logoData;
                        },
                        static_cast<GConnectFlags>(0));

  gtk_widget_set_tooltip_text(drawingArea, Localization::text("app.name"));
  gtk_widget_set_can_focus(drawingArea, FALSE);
  return drawingArea;
}

GtkWidget *create_about_logo() {
  GdkPixbuf *pixbuf = load_about_logo_pixbuf();
  if (!pixbuf) {
    return gtk_label_new("V");
  }

  GdkPixbuf *cropped = crop_transparent_padding(pixbuf);
  g_object_unref(pixbuf);
  if (!cropped) {
    return gtk_label_new("V");
  }

  const int sourceWidth = gdk_pixbuf_get_width(cropped);
  const int sourceHeight = gdk_pixbuf_get_height(cropped);
  const int targetHeight = 96;
  const int targetWidth =
      sourceHeight > 0 ? std::max(1, sourceWidth * targetHeight / sourceHeight)
                       : targetHeight;
  GdkPixbuf *scaled = gdk_pixbuf_scale_simple(cropped, targetWidth,
                                             targetHeight,
                                             GDK_INTERP_BILINEAR);
  g_object_unref(cropped);
  if (!scaled) {
    return gtk_label_new("V");
  }

  GtkWidget *image = gtk_image_new_from_pixbuf(scaled);
  g_object_unref(scaled);
  gtk_widget_set_halign(image, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(image, GTK_ALIGN_START);
  return image;
}

}  // namespace

WindowInitializer::WindowInitializer(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {}

void WindowInitializer::initialize() {
  EditorWindow::WindowComponents components = createWindowComponents();
  EditorWindow::FileMenuItems fileMenuItems = buildMenuBar(components);
  setupLayout(components);

  editorWindow->registerFileMenuItems(fileMenuItems);
}

EditorWindow::WindowComponents WindowInitializer::createWindowComponents() {
  EditorWindow::WindowComponents components;

  components.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(components.window),
                       Localization::text("app.name"));
  gtk_window_set_default_size(GTK_WINDOW(components.window), 1000, 700);

  components.accelGroup = gtk_accel_group_new();
  gtk_window_add_accel_group(GTK_WINDOW(components.window),
                             components.accelGroup);
  g_signal_connect(components.window, "delete-event",
                   G_CALLBACK(EventHandler::on_window_delete_event),
                   editorWindow);
  g_signal_connect(components.window, "focus-in-event",
                   G_CALLBACK(EventHandler::on_window_focus_in_event),
                   editorWindow);
  g_signal_connect(components.window, "key-press-event",
                   G_CALLBACK(EventHandler::on_window_key_press_event),
                   editorWindow);
  g_signal_connect(components.window, "window-state-event",
                   G_CALLBACK(EventHandler::on_window_state_event),
                   editorWindow);
  EventHandler::enable_file_drop_target(components.window, editorWindow);

  components.menubar = gtk_menu_bar_new();

  components.notebook = gtk_notebook_new();
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(components.notebook), TRUE);
  g_signal_connect(components.notebook, "switch-page",
                   G_CALLBACK(EventHandler::on_notebook_switch_page),
                   editorWindow);
  g_signal_connect(components.notebook, "page-reordered",
                   G_CALLBACK(EventHandler::on_notebook_page_reordered),
                   editorWindow);
  EventHandler::enable_file_drop_target(components.notebook, editorWindow);

  components.vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  components.editorPane = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  EventHandler::enable_file_drop_target(components.vbox, editorWindow);
  EventHandler::enable_file_drop_target(components.editorPane, editorWindow);
  components.searchResultsPanel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  components.recentMenu = gtk_menu_new();
  components.statusBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  components.statusBarLabel = gtk_label_new("");
  gtk_widget_set_halign(components.statusBarLabel, GTK_ALIGN_START);
  gtk_widget_set_margin_start(components.statusBarLabel, 8);
  gtk_widget_set_margin_end(components.statusBarLabel, 8);
  gtk_widget_set_margin_top(components.statusBarLabel, 4);
  gtk_widget_set_margin_bottom(components.statusBarLabel, 4);
  gtk_box_pack_start(GTK_BOX(components.statusBar), components.statusBarLabel,
                     TRUE, TRUE, 0);

  return components;
}

EditorWindow::SettingsMenuItems WindowInitializer::buildSettingsMenu(
    GtkWidget *settingsMenu) {
  EditorWindow::SettingsMenuItems items;
  items.preferencesItem = localized_menu_item("menu.preferences");
  items.shortcutConfigItem =
      localized_menu_item("menu.keyboard_shortcuts");

  gtk_menu_shell_append(GTK_MENU_SHELL(settingsMenu), items.preferencesItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(settingsMenu),
                        items.shortcutConfigItem);
  g_signal_connect(items.preferencesItem, "activate",
                   G_CALLBACK(on_preferences_activate), editorWindow);
  g_signal_connect(items.shortcutConfigItem, "activate",
                   G_CALLBACK(on_shortcuts_activate), editorWindow);

  return items;
}

EditorWindow::FileMenuItems WindowInitializer::buildMenuBar(
    EditorWindow::WindowComponents &components) {
  GtkWidget *menubar = components.menubar;

  GtkWidget *fileMenu = gtk_menu_new();
  GtkWidget *fileItem = localized_menu_item("menu.file");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(fileItem), fileMenu);

  EditorWindow::FileMenuItems fileItems;
  fileItems.newItem = localized_menu_item("menu.new");
  fileItems.openItem = localized_menu_item("menu.open");
  GtkWidget *reloadItem = localized_menu_item("menu.reload");
  fileItems.saveItem = localized_menu_item("menu.save");
  fileItems.saveAsItem = localized_menu_item("menu.save_as");
  GtkWidget *saveAllItem = localized_menu_item("menu.save_all");
  GtkWidget *closeAllItem = localized_menu_item("menu.close_all");
  fileItems.exitItem = localized_menu_item("menu.exit");

  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), fileItems.newItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), fileItems.openItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), reloadItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), fileItems.saveItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), fileItems.saveAsItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), saveAllItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), closeAllItem);

  GtkWidget *recentMenuItem = localized_menu_item("menu.recent_files");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(recentMenuItem),
                            components.recentMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), recentMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(fileMenu), fileItems.exitItem);

  g_signal_connect(fileItems.newItem, "activate",
                   G_CALLBACK(EventHandler::on_new_activate), editorWindow);
  g_signal_connect(fileItems.openItem, "activate",
                   G_CALLBACK(EventHandler::on_open_activate), editorWindow);
  g_signal_connect(reloadItem, "activate",
                   G_CALLBACK(EventHandler::on_reload_activate), editorWindow);
  g_signal_connect(fileItems.saveItem, "activate",
                   G_CALLBACK(EventHandler::on_save_activate), editorWindow);
  g_signal_connect(fileItems.saveAsItem, "activate",
                   G_CALLBACK(EventHandler::on_save_as_activate),
                   editorWindow);
  g_signal_connect(saveAllItem, "activate",
                   G_CALLBACK(EventHandler::on_save_all_activate),
                   editorWindow);
  g_signal_connect(closeAllItem, "activate",
                   G_CALLBACK(EventHandler::on_close_all_activate),
                   editorWindow);
  g_signal_connect(fileItems.exitItem, "activate",
                   G_CALLBACK(EventHandler::on_exit_activate), editorWindow);

  GtkWidget *editMenu = gtk_menu_new();
  GtkWidget *editItem = localized_menu_item("menu.edit");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(editItem), editMenu);

  GtkWidget *undoItem = localized_menu_item("menu.undo");
  GtkWidget *redoItem = localized_menu_item("menu.redo");
  GtkWidget *cutItem = localized_menu_item("menu.cut");
  GtkWidget *copyItem = localized_menu_item("menu.copy");
  GtkWidget *pasteItem = localized_menu_item("menu.paste");
  GtkWidget *columnEditorItem = localized_check_menu_item("menu.column_editor");
  GtkWidget *convertCaseMenu = gtk_menu_new();
  GtkWidget *convertCaseItem = localized_menu_item("menu.convert_case");
  GtkWidget *uppercaseItem = localized_menu_item("menu.uppercase");
  GtkWidget *lowercaseItem = localized_menu_item("menu.lowercase");
  GtkWidget *titleCaseItem = localized_menu_item("menu.title_case");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(convertCaseItem), convertCaseMenu);
  GtkWidget *blankOperationsMenu = gtk_menu_new();
  GtkWidget *blankOperationsItem =
      localized_menu_item("menu.blank_operations");
  GtkWidget *trimTrailingWhitespaceItem =
      localized_menu_item("menu.trim_trailing_whitespace");
  GtkWidget *spacesToTabsItem = localized_menu_item("menu.spaces_to_tabs");
  GtkWidget *tabsToSpacesItem = localized_menu_item("menu.tabs_to_spaces");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(blankOperationsItem),
                            blankOperationsMenu);

  gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), undoItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), redoItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(editMenu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), cutItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), copyItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), pasteItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(editMenu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), columnEditorItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), convertCaseItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(editMenu), blankOperationsItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(convertCaseMenu), uppercaseItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(convertCaseMenu), lowercaseItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(convertCaseMenu), titleCaseItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(blankOperationsMenu),
                        trimTrailingWhitespaceItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(blankOperationsMenu), spacesToTabsItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(blankOperationsMenu), tabsToSpacesItem);

  g_signal_connect(undoItem, "activate",
                   G_CALLBACK(EventHandler::on_undo_activate), editorWindow);
  g_signal_connect(redoItem, "activate",
                   G_CALLBACK(EventHandler::on_redo_activate), editorWindow);
  g_signal_connect(cutItem, "activate",
                   G_CALLBACK(EventHandler::on_cut_activate), editorWindow);
  g_signal_connect(copyItem, "activate",
                   G_CALLBACK(EventHandler::on_copy_activate), editorWindow);
  g_signal_connect(pasteItem, "activate",
                   G_CALLBACK(EventHandler::on_paste_activate), editorWindow);
  g_signal_connect(columnEditorItem, "toggled",
                   G_CALLBACK(EventHandler::on_column_editor_toggled),
                   editorWindow);
  g_signal_connect(uppercaseItem, "activate",
                   G_CALLBACK(EventHandler::on_uppercase_activate),
                   editorWindow);
  g_signal_connect(lowercaseItem, "activate",
                   G_CALLBACK(EventHandler::on_lowercase_activate),
                   editorWindow);
  g_signal_connect(titleCaseItem, "activate",
                   G_CALLBACK(EventHandler::on_title_case_activate),
                   editorWindow);
  g_signal_connect(trimTrailingWhitespaceItem, "activate",
                   G_CALLBACK(EventHandler::on_trim_trailing_whitespace_activate),
                   editorWindow);
  g_signal_connect(spacesToTabsItem, "activate",
                   G_CALLBACK(EventHandler::on_spaces_to_tabs_activate),
                   editorWindow);
  g_signal_connect(tabsToSpacesItem, "activate",
                   G_CALLBACK(EventHandler::on_tabs_to_spaces_activate),
                   editorWindow);

  GtkWidget *searchMenu = gtk_menu_new();
  GtkWidget *searchItem = localized_menu_item("menu.search");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(searchItem), searchMenu);

  GtkWidget *findItem = localized_menu_item("menu.find");
  GtkWidget *replaceItem = localized_menu_item("menu.replace");
  GtkWidget *goToLineItem = localized_menu_item("menu.go_to");

  gtk_menu_shell_append(GTK_MENU_SHELL(searchMenu), findItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(searchMenu), replaceItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(searchMenu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(searchMenu), goToLineItem);

  g_signal_connect(findItem, "activate",
                   G_CALLBACK(EventHandler::on_find_activate), editorWindow);
  g_signal_connect(replaceItem, "activate",
                   G_CALLBACK(EventHandler::on_replace_activate), editorWindow);
  g_signal_connect(goToLineItem, "activate",
                   G_CALLBACK(EventHandler::on_go_to_line_activate),
                   editorWindow);

  GtkWidget *viewMenu = gtk_menu_new();
  GtkWidget *viewItem = localized_menu_item("menu.view");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(viewItem), viewMenu);

  components.wordWrapMenuItem = localized_check_menu_item("menu.word_wrap");
  components.fullScreenMenuItem = localized_check_menu_item("menu.full_screen");
  GtkWidget *showSymbolsMenu = gtk_menu_new();
  GtkWidget *showSymbolsItem = localized_menu_item("menu.show_symbols");
  components.showWhitespaceMenuItem =
      localized_check_menu_item("menu.show_spaces_tabs");
  components.showEolMarkersMenuItem =
      localized_check_menu_item("menu.show_eol");
  GtkWidget *zoomMenu = gtk_menu_new();
  GtkWidget *zoomItem = localized_menu_item("menu.zoom");
  GtkWidget *zoomInItem = localized_menu_item("menu.zoom_in");
  GtkWidget *zoomOutItem = localized_menu_item("menu.zoom_out");
  GtkWidget *resetZoomItem = localized_menu_item("menu.reset_zoom");
  GtkWidget *searchResultsItem = localized_menu_item("menu.search_results");
  GtkWidget *highlightMenu = gtk_menu_new();
  GtkWidget *highlightItem = localized_menu_item("menu.highlight");
  GtkWidget *clearHighlightMenu = gtk_menu_new();
  GtkWidget *clearHighlightItem =
      localized_menu_item("menu.clear_custom_highlights");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(showSymbolsItem), showSymbolsMenu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(zoomItem), zoomMenu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(highlightItem), highlightMenu);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(clearHighlightItem),
                            clearHighlightMenu);

  gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), components.wordWrapMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), components.fullScreenMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), showSymbolsItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), zoomItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), searchResultsItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), highlightItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(viewMenu), clearHighlightItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(showSymbolsMenu),
                        components.showWhitespaceMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(showSymbolsMenu),
                        components.showEolMarkersMenuItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(zoomMenu), zoomInItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(zoomMenu), zoomOutItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(zoomMenu), resetZoomItem);
  append_custom_highlight_menu_items(highlightMenu, editorWindow);
  append_clear_custom_highlight_menu_items(clearHighlightMenu, editorWindow);
  g_signal_connect(components.wordWrapMenuItem, "toggled",
                   G_CALLBACK(EventHandler::on_word_wrap_toggled),
                   editorWindow);
  g_signal_connect(components.fullScreenMenuItem, "toggled",
                   G_CALLBACK(EventHandler::on_fullscreen_toggled),
                   editorWindow);
  g_signal_connect(components.showWhitespaceMenuItem, "toggled",
                   G_CALLBACK(EventHandler::on_show_whitespace_toggled),
                   editorWindow);
  g_signal_connect(components.showEolMarkersMenuItem, "toggled",
                   G_CALLBACK(EventHandler::on_show_eol_markers_toggled),
                   editorWindow);
  g_signal_connect(zoomInItem, "activate",
                   G_CALLBACK(EventHandler::on_zoom_in_activate),
                   editorWindow);
  g_signal_connect(zoomOutItem, "activate",
                   G_CALLBACK(EventHandler::on_zoom_out_activate),
                   editorWindow);
  g_signal_connect(resetZoomItem, "activate",
                   G_CALLBACK(EventHandler::on_reset_zoom_activate),
                   editorWindow);
  g_signal_connect(searchResultsItem, "activate",
                   G_CALLBACK(EventHandler::on_search_results_activate),
                   editorWindow);

  GtkWidget *encodingMenu = gtk_menu_new();
  GtkWidget *encodingItem = localized_menu_item("menu.encoding");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(encodingItem), encodingMenu);
  GtkWidget *convertToAnsiItem = localized_menu_item("menu.convert_ansi");
  GtkWidget *convertToUtf8Item = localized_menu_item("menu.convert_utf8");
  GtkWidget *convertToUtf8BomItem =
      localized_menu_item("menu.convert_utf8_bom");
  GtkWidget *convertToUtf16LeItem =
      localized_menu_item("menu.convert_utf16_le");
  GtkWidget *convertToUtf16BeItem =
      localized_menu_item("menu.convert_utf16_be");
  gtk_menu_shell_append(GTK_MENU_SHELL(encodingMenu), convertToAnsiItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(encodingMenu), convertToUtf8Item);
  gtk_menu_shell_append(GTK_MENU_SHELL(encodingMenu), convertToUtf8BomItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(encodingMenu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(encodingMenu), convertToUtf16LeItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(encodingMenu), convertToUtf16BeItem);

  g_object_set_data(G_OBJECT(convertToAnsiItem), "target-encoding",
                    GINT_TO_POINTER(static_cast<int>(FileEncoding::Ansi)));
  g_object_set_data(G_OBJECT(convertToAnsiItem), "target-use-bom",
                    GINT_TO_POINTER(0));
  g_object_set_data(G_OBJECT(convertToUtf8Item), "target-encoding",
                    GINT_TO_POINTER(static_cast<int>(FileEncoding::Utf8)));
  g_object_set_data(G_OBJECT(convertToUtf8Item), "target-use-bom",
                    GINT_TO_POINTER(0));
  g_object_set_data(G_OBJECT(convertToUtf8BomItem), "target-encoding",
                    GINT_TO_POINTER(static_cast<int>(FileEncoding::Utf8)));
  g_object_set_data(G_OBJECT(convertToUtf8BomItem), "target-use-bom",
                    GINT_TO_POINTER(1));
  g_object_set_data(G_OBJECT(convertToUtf16LeItem), "target-encoding",
                    GINT_TO_POINTER(static_cast<int>(FileEncoding::Utf16LE)));
  g_object_set_data(G_OBJECT(convertToUtf16LeItem), "target-use-bom",
                    GINT_TO_POINTER(1));
  g_object_set_data(G_OBJECT(convertToUtf16BeItem), "target-encoding",
                    GINT_TO_POINTER(static_cast<int>(FileEncoding::Utf16BE)));
  g_object_set_data(G_OBJECT(convertToUtf16BeItem), "target-use-bom",
                    GINT_TO_POINTER(1));
  g_signal_connect(convertToAnsiItem, "activate",
                   G_CALLBACK(EventHandler::on_convert_encoding_activate),
                   editorWindow);
  g_signal_connect(convertToUtf8Item, "activate",
                   G_CALLBACK(EventHandler::on_convert_encoding_activate),
                   editorWindow);
  g_signal_connect(convertToUtf8BomItem, "activate",
                   G_CALLBACK(EventHandler::on_convert_encoding_activate),
                   editorWindow);
  g_signal_connect(convertToUtf16LeItem, "activate",
                   G_CALLBACK(EventHandler::on_convert_encoding_activate),
                   editorWindow);
  g_signal_connect(convertToUtf16BeItem, "activate",
                   G_CALLBACK(EventHandler::on_convert_encoding_activate),
                   editorWindow);

  GtkWidget *settingsMenu = gtk_menu_new();
  GtkWidget *settingsItem = localized_menu_item("menu.settings");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(settingsItem), settingsMenu);
  EditorWindow::SettingsMenuItems settingsItems =
      buildSettingsMenu(settingsMenu);
  editorWindow->registerSettingsMenuItems(settingsItems);

  GtkWidget *macroMenu = gtk_menu_new();
  GtkWidget *macroItem = localized_menu_item("menu.macro");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(macroItem), macroMenu);

  GtkWidget *startRecordingItem =
      localized_menu_item("menu.start_recording");
  GtkWidget *stopRecordingItem = localized_menu_item("menu.stop_recording");
  GtkWidget *playbackItem = localized_menu_item("menu.play_macro");
  GtkWidget *manageMacroItem =
      localized_menu_item("menu.macro_management");
  GtkWidget *saveMacroItem =
      localized_menu_item("menu.save_current_macro");

  gtk_menu_shell_append(GTK_MENU_SHELL(macroMenu), startRecordingItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(macroMenu), stopRecordingItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(macroMenu),
                        gtk_separator_menu_item_new());
  gtk_menu_shell_append(GTK_MENU_SHELL(macroMenu), playbackItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(macroMenu), manageMacroItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(macroMenu), saveMacroItem);

  editorWindow->addMacroMenuItems(macroMenu);

  g_signal_connect(startRecordingItem, "activate",
                   G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
                     (void)widget;
                     auto *window = static_cast<EditorWindow *>(data);
                     window->startMacroRecording();
                   }),
                   editorWindow);

  g_signal_connect(stopRecordingItem, "activate",
                   G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
                     (void)widget;
                     auto *window = static_cast<EditorWindow *>(data);
                     window->stopMacroRecording();
                   }),
                   editorWindow);

  g_signal_connect(playbackItem, "activate",
                   G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
                     (void)widget;
                     auto *window = static_cast<EditorWindow *>(data);
                     window->showMacroPlaybackDialog();
                   }),
                   editorWindow);

  g_signal_connect(saveMacroItem, "activate",
                   G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
                     (void)widget;
                     auto *window = static_cast<EditorWindow *>(data);
                     window->promptAndSaveCurrentMacro();
                   }),
                   editorWindow);

  g_signal_connect(manageMacroItem, "activate",
                   G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
                     (void)widget;
                     auto *window = static_cast<EditorWindow *>(data);
                     window->showMacroManagementDialog();
                   }),
                   editorWindow);

  GtkWidget *languageMenu = gtk_menu_new();
  GtkWidget *languageItem = localized_menu_item("menu.language");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(languageItem), languageMenu);

  const auto &lexers = editorWindow->getAvailableLexers();
  std::map<char, std::vector<LexerDefinition>> lexersByLetter;
  for (const auto &lexer : lexers) {
    if (!lexer.description.empty()) {
      char firstLetter = std::toupper(
          static_cast<unsigned char>(lexer.description[0]));
      lexersByLetter[firstLetter].push_back(lexer);
    }
  }

  for (const auto &[letter, lexerList] : lexersByLetter) {
    GtkWidget *submenu = gtk_menu_new();
    std::string letterLabel(1, letter);
    GtkWidget *submenuItem =
        gtk_menu_item_new_with_label(letterLabel.c_str());
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(submenuItem), submenu);
    gtk_menu_shell_append(GTK_MENU_SHELL(languageMenu), submenuItem);

    for (const auto &lexer : lexerList) {
      GtkWidget *languageEntry =
          gtk_menu_item_new_with_label(lexer.description.c_str());
      g_object_set_data_full(G_OBJECT(languageEntry), "lexer-name",
                             g_strdup(lexer.name.c_str()), g_free);
      g_signal_connect(languageEntry, "activate",
                       G_CALLBACK(+[](GtkWidget *widget, gpointer data) {
                         auto *window = static_cast<EditorWindow *>(data);
                         const char *lexerName = static_cast<const char *>(
                             g_object_get_data(G_OBJECT(widget), "lexer-name"));
                         if (lexerName) {
                           window->setLexer(lexerName);
                         }
                       }),
                       editorWindow);
      gtk_menu_shell_append(GTK_MENU_SHELL(submenu), languageEntry);
    }
  }

  GtkWidget *pluginsItem = localized_menu_item("menu.plugins");
  gtk_widget_set_sensitive(pluginsItem, FALSE);

  GtkWidget *helpMenu = gtk_menu_new();
  GtkWidget *helpItem = localized_menu_item("menu.help");
  GtkWidget *aboutItem = localized_menu_item("menu.about");
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(helpItem), helpMenu);
  gtk_menu_shell_append(GTK_MENU_SHELL(helpMenu), aboutItem);
  g_signal_connect(aboutItem, "activate",
                   G_CALLBACK(on_about_activate), editorWindow);

  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), fileItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), editItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), searchItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), viewItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), encodingItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), languageItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), settingsItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), macroItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), pluginsItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menubar), helpItem);

  editorWindow->applyShortcutBindings({
      {"New", fileItems.newItem, GDK_KEY_n, GDK_CONTROL_MASK, "Ctrl+N"},
      {"Open", fileItems.openItem, GDK_KEY_o, GDK_CONTROL_MASK, "Ctrl+O"},
      {"Reload", reloadItem, GDK_KEY_r, GDK_CONTROL_MASK, "Ctrl+R"},
      {"Save", fileItems.saveItem, GDK_KEY_s, GDK_CONTROL_MASK, "Ctrl+S"},
      {"Save As", fileItems.saveAsItem, GDK_KEY_S,
       static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
       "Ctrl+Shift+S"},
      {"Save All", saveAllItem, GDK_KEY_s,
       static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_MOD1_MASK),
       "Ctrl+Alt+S"},
      {"Close All", closeAllItem, GDK_KEY_w,
       static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
       "Ctrl+Shift+W"},
      {"Exit", fileItems.exitItem, GDK_KEY_q, GDK_CONTROL_MASK, "Ctrl+Q"},
      {"Undo", undoItem, GDK_KEY_z, GDK_CONTROL_MASK, "Ctrl+Z"},
      {"Redo", redoItem, GDK_KEY_y, GDK_CONTROL_MASK, "Ctrl+Y"},
      {"Cut", cutItem, GDK_KEY_x, GDK_CONTROL_MASK, "Ctrl+X"},
      {"Copy", copyItem, GDK_KEY_c, GDK_CONTROL_MASK, "Ctrl+C"},
      {"Paste", pasteItem, GDK_KEY_v, GDK_CONTROL_MASK, "Ctrl+V"},
      {"Column Editor", columnEditorItem, GDK_KEY_c, GDK_MOD1_MASK, "Alt+C"},
      {"Find", findItem, GDK_KEY_f, GDK_CONTROL_MASK, "Ctrl+F"},
      {"Replace", replaceItem, GDK_KEY_h, GDK_CONTROL_MASK, "Ctrl+H"},
      {"Go To Line", goToLineItem, GDK_KEY_g, GDK_CONTROL_MASK, "Ctrl+G"},
      {"Search Results", searchResultsItem, GDK_KEY_f,
       static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
       "Ctrl+Shift+F"},
      {"Word Wrap", components.wordWrapMenuItem, GDK_KEY_z, GDK_MOD1_MASK,
       "Alt+Z"},
      {"Full Screen Mode", components.fullScreenMenuItem, GDK_KEY_F11,
       static_cast<GdkModifierType>(0), "F11"},
      {"Zoom In", zoomInItem, GDK_KEY_equal, GDK_CONTROL_MASK, "Ctrl++"},
      {"Zoom Out", zoomOutItem, GDK_KEY_minus, GDK_CONTROL_MASK, "Ctrl+-"},
      {"Reset Zoom", resetZoomItem, GDK_KEY_0, GDK_CONTROL_MASK, "Ctrl+0"},
      {"Start Recording", startRecordingItem, GDK_KEY_R,
       static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
       "Ctrl+Shift+R"},
      {"Stop Recording", stopRecordingItem, GDK_KEY_T,
       static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
       "Ctrl+Shift+T"},
      {"Playback", playbackItem, GDK_KEY_P,
       static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
       "Ctrl+Shift+P"},
      {"Save Current Macro", saveMacroItem, GDK_KEY_M,
       static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
       "Ctrl+Shift+M"},
      {"Macro Management", manageMacroItem, GDK_KEY_G,
       static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
       "Ctrl+Shift+G"},
      {"Preferences", settingsItems.preferencesItem, GDK_KEY_comma,
       GDK_CONTROL_MASK, "Ctrl+,"},
      {"Keyboard Shortcuts", settingsItems.shortcutConfigItem, GDK_KEY_K,
       static_cast<GdkModifierType>(GDK_CONTROL_MASK | GDK_SHIFT_MASK),
       "Ctrl+Shift+K"},
      {"About", aboutItem, GDK_KEY_F1, static_cast<GdkModifierType>(0),
       "F1"}});

  return fileItems;
}

void WindowInitializer::on_preferences_activate(GtkWidget *widget,
                                                gpointer data) {
  (void)widget;
  auto *window = static_cast<EditorWindow *>(data);
  window->showPreferencesDialog();
}

void WindowInitializer::on_shortcuts_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  auto *window = static_cast<EditorWindow *>(data);
  window->showShortcutConfigurationDialog();
}

void WindowInitializer::on_about_activate(GtkWidget *widget, gpointer data) {
  (void)widget;
  auto *window = static_cast<EditorWindow *>(data);

  GtkWidget *dialog = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(dialog), "");
  gtk_window_set_transient_for(GTK_WINDOW(dialog),
                               window->getDialogParentWindow());
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
  gtk_window_set_default_size(GTK_WINDOW(dialog), 700, 540);
  gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
  gtk_window_set_decorated(GTK_WINDOW(dialog), FALSE);

  GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  gtk_container_set_border_width(GTK_CONTAINER(content), 0);

  GtkWidget *pageGrid = gtk_grid_new();
  gtk_grid_set_row_spacing(GTK_GRID(pageGrid), 30);
  gtk_widget_set_halign(pageGrid, GTK_ALIGN_FILL);
  gtk_widget_set_margin_start(pageGrid, 40);
  gtk_widget_set_margin_end(pageGrid, 40);
  gtk_widget_set_margin_top(pageGrid, 10);
  gtk_widget_set_margin_bottom(pageGrid, 40);
  gtk_container_add(GTK_CONTAINER(content), pageGrid);

  GtkWidget *productGrid = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(productGrid), 16);
  gtk_widget_set_halign(productGrid, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand(productGrid, TRUE);
  gtk_widget_set_size_request(productGrid, -1, 96);
  gtk_grid_attach(GTK_GRID(pageGrid), productGrid, 0, 0, 1, 1);

  GtkWidget *logo = create_about_logo();
  gtk_widget_set_margin_top(logo, 24);
  gtk_grid_attach(GTK_GRID(productGrid), logo, 0, 0, 1, 1);

  GtkWidget *productInfoGrid = gtk_grid_new();
  gtk_grid_set_row_homogeneous(GTK_GRID(productInfoGrid), TRUE);
  gtk_widget_set_halign(productInfoGrid, GTK_ALIGN_FILL);
  gtk_widget_set_hexpand(productInfoGrid, TRUE);
  gtk_widget_set_size_request(productInfoGrid, -1, 96);
  gtk_grid_attach(GTK_GRID(productGrid), productInfoGrid, 1, 0, 1, 1);

  GtkWidget *buildRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_halign(buildRow, GTK_ALIGN_END);
  gtk_widget_set_hexpand(buildRow, TRUE);
  gtk_widget_set_valign(buildRow, GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(productInfoGrid), buildRow, 0, 0, 1, 1);

  GtkWidget *buildLabel = create_text_label(Localization::text("about.build"));
  Localization::bind_widget_text(buildLabel, "about.build");
  gtk_box_pack_start(GTK_BOX(buildRow), buildLabel, FALSE, FALSE, 0);

  std::string buildValue = std::string(__DATE__) + " " + __TIME__;
  gtk_box_pack_start(GTK_BOX(buildRow), create_text_label(buildValue.c_str()),
                     FALSE, FALSE, 0);

  GtkWidget *nameVersionRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  gtk_widget_set_halign(nameVersionRow, GTK_ALIGN_START);
  gtk_widget_set_valign(nameVersionRow, GTK_ALIGN_END);
  gtk_grid_attach(GTK_GRID(productInfoGrid), nameVersionRow, 0, 1, 1, 1);

  gtk_box_pack_start(GTK_BOX(nameVersionRow),
                     create_markup_label("<span size=\"xx-large\" "
                                         "weight=\"bold\">Velnix Editor</span>"),
                     FALSE, FALSE, 0);

  std::ostringstream versionText;
  versionText << "v" << VELNIX_EDITOR_VERSION << "    ("
              << (sizeof(void *) * 8) << "bit)";
  gtk_box_pack_start(GTK_BOX(nameVersionRow),
                     create_text_label(versionText.str().c_str()),
                     FALSE, FALSE, 0);

  GtkWidget *homepageRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_widget_set_halign(homepageRow, GTK_ALIGN_START);
  gtk_grid_attach(GTK_GRID(pageGrid), homepageRow, 0, 1, 1, 1);

  GtkWidget *homeLabel = create_text_label(Localization::text("about.home"));
  Localization::bind_widget_text(homeLabel, "about.home");
  gtk_box_pack_start(GTK_BOX(homepageRow), homeLabel, FALSE, FALSE, 0);

  GtkWidget *website =
      gtk_link_button_new_with_label(kProjectHomepage, kProjectHomepage);
  gtk_box_pack_start(GTK_BOX(homepageRow), website, FALSE, FALSE, 0);

  GtkWidget *licenseFrame =
      gtk_frame_new(Localization::text("about.license_title"));
  Localization::bind_widget_text(licenseFrame, "about.license_title");
  gtk_frame_set_label_align(GTK_FRAME(licenseFrame), 0.5f, 0.5f);
  gtk_widget_set_halign(licenseFrame, GTK_ALIGN_CENTER);
  gtk_widget_set_size_request(licenseFrame, 580, -1);
  gtk_grid_attach(GTK_GRID(pageGrid), licenseFrame, 0, 2, 1, 1);

  GtkWidget *licenseLabel =
      create_text_label(Localization::text("about.gpl_notice"));
  Localization::bind_widget_text(licenseLabel, "about.gpl_notice");
  gtk_label_set_line_wrap_mode(GTK_LABEL(licenseLabel), PANGO_WRAP_WORD_CHAR);
  gtk_label_set_max_width_chars(GTK_LABEL(licenseLabel), 52);
  gtk_widget_set_margin_start(licenseLabel, 40);
  gtk_widget_set_margin_end(licenseLabel, 40);
  gtk_widget_set_margin_top(licenseLabel, 20);
  gtk_widget_set_margin_bottom(licenseLabel, 20);
  gtk_container_add(GTK_CONTAINER(licenseFrame), licenseLabel);

  GtkWidget *okButton =
      gtk_button_new_with_mnemonic(Localization::text("dialog.ok"));
  Localization::bind_widget_text(okButton, "dialog.ok");
  gtk_widget_set_halign(okButton, GTK_ALIGN_CENTER);
  gtk_widget_set_size_request(okButton, 112, 38);
  gtk_grid_attach(GTK_GRID(pageGrid), okButton, 0, 3, 1, 1);
  g_signal_connect(
      okButton, "clicked",
      G_CALLBACK(+[](GtkButton *, gpointer dialogData) {
        gtk_dialog_response(GTK_DIALOG(dialogData), GTK_RESPONSE_OK);
      }),
      dialog);

  gtk_widget_show_all(dialog);
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

void WindowInitializer::setupLayout(
    EditorWindow::WindowComponents &components) {
  gtk_container_add(GTK_CONTAINER(components.window), components.vbox);

  GtkWidget *menuRow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  GtkWidget *logo = create_menu_logo_widget();
  if (logo) {
    gtk_box_pack_start(GTK_BOX(menuRow), logo, FALSE, TRUE, 0);
  }
  gtk_box_pack_start(GTK_BOX(menuRow), components.menubar, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(components.vbox), menuRow, FALSE, FALSE, 0);

  gtk_widget_set_vexpand(components.editorPane, TRUE);
  gtk_widget_set_hexpand(components.editorPane, TRUE);
  gtk_box_pack_start(GTK_BOX(components.vbox), components.editorPane, TRUE,
                     TRUE, 0);

  gtk_box_pack_start(GTK_BOX(components.vbox), components.statusBar, FALSE,
                     FALSE, 0);

  gtk_widget_set_vexpand(components.notebook, TRUE);
  gtk_widget_set_hexpand(components.notebook, TRUE);
  gtk_paned_pack1(GTK_PANED(components.editorPane), components.notebook, TRUE,
                  FALSE);

  gtk_widget_hide(components.searchResultsPanel);
  gtk_widget_set_vexpand(components.searchResultsPanel, TRUE);
  gtk_widget_set_hexpand(components.searchResultsPanel, TRUE);
  gtk_paned_pack2(GTK_PANED(components.editorPane),
                  components.searchResultsPanel, FALSE, FALSE);

  editorWindow->installWindowComponents(components);
  editorWindow->createSearchResultsPanel(components.searchResultsPanel);
  gtk_paned_set_position(GTK_PANED(components.editorPane), G_MAXINT);
}
