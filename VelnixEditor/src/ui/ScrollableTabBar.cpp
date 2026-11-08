#include "ui/ScrollableTabBar.h"

#include <algorithm>
#include <vector>

namespace {

// Use button styling without GtkButton's input handling: tab labels contain
// their own close button, which must receive mouse events independently.
struct DocumentTab { GtkEventBox parent; };
struct DocumentTabClass { GtkEventBoxClass parent; };
G_DEFINE_TYPE(DocumentTab, document_tab, GTK_TYPE_EVENT_BOX)

void document_tab_class_init(DocumentTabClass *klass) {
  gtk_widget_class_set_css_name(GTK_WIDGET_CLASS(klass), "button");
}

void document_tab_init(DocumentTab *tab) {
  gtk_event_box_set_above_child(GTK_EVENT_BOX(tab), FALSE);
  gtk_widget_add_events(GTK_WIDGET(tab), GDK_BUTTON_PRESS_MASK);
  gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(tab)), GTK_STYLE_CLASS_FLAT);
}

struct Tab {
  GtkWidget *page;
  GtkWidget *button;
};

class ScrollableTabBar {
public:
  GtkNotebook *notebook;
  GtkWidget *root = gtk_overlay_new();
  GtkWidget *body = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *dragLayer = gtk_drawing_area_new();
  GtkWidget *tabs = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  GtkWidget *scroll = gtk_scrolled_window_new(nullptr, nullptr);
  GtkWidget *previous = gtk_button_new_from_icon_name("pan-start-symbolic", GTK_ICON_SIZE_MENU);
  GtkWidget *next = gtk_button_new_from_icon_name("pan-end-symbolic", GTK_ICON_SIZE_MENU);
  std::vector<Tab> items;
  guint revealSource = 0;
  GtkWidget *dragPage = nullptr;
  bool dragging = false;
  GtkWidget *dragSource = nullptr;
  cairo_surface_t *dragSnapshot = nullptr;
  guint dragTick = 0;
  int dropIndex = -1;
  double grabX = 0;
  double grabY = 0;
  double ghostX = 0;
  double ghostY = 0;
  double sourceOpacity = 1;
  int ghostWidth = 0;
  int ghostHeight = 0;
  int markerX = 0;
  int markerY = 0;
  int markerHeight = 0;
  bool previewDirty = false;
  bool hasDamage = false;
  GdkRectangle lastDamage{};

  explicit ScrollableTabBar(GtkNotebook *book) : notebook(book) {
    gtk_container_add(GTK_CONTAINER(root), body);
    gtk_overlay_add_overlay(GTK_OVERLAY(root), dragLayer);
    gtk_overlay_set_overlay_pass_through(GTK_OVERLAY(root), dragLayer, TRUE);
    gtk_widget_set_no_show_all(dragLayer, TRUE);
    gtk_widget_set_hexpand(dragLayer, TRUE);
    gtk_widget_set_vexpand(dragLayer, TRUE);
    g_signal_connect(dragLayer, "draw", G_CALLBACK(+[](GtkWidget *widget, cairo_t *cr,
                                                     gpointer data) -> gboolean {
      auto *self = static_cast<ScrollableTabBar *>(data);
      if (!self->dragSnapshot) return FALSE;
      cairo_set_source_rgba(cr, 0, 0, 0, 0.18);
      cairo_rectangle(cr, self->ghostX + 3, self->ghostY + 4,
                       self->ghostWidth, self->ghostHeight);
      cairo_fill(cr);
      cairo_set_source_surface(cr, self->dragSnapshot, self->ghostX, self->ghostY);
      cairo_paint_with_alpha(cr, 0.8);
      GdkRGBA accent;
      if (!gtk_style_context_lookup_color(gtk_widget_get_style_context(widget),
                                           "theme_selected_bg_color", &accent))
        gdk_rgba_parse(&accent, "#3584e4");
      gdk_cairo_set_source_rgba(cr, &accent);
      cairo_rectangle(cr, self->markerX - 1, self->markerY, 3, self->markerHeight);
      cairo_fill(cr);
      return FALSE;
    }), this);
    gtk_notebook_set_show_tabs(notebook, FALSE);
    gtk_widget_set_focus_on_click(previous, FALSE);
    gtk_widget_set_focus_on_click(next, FALSE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_EXTERNAL, GTK_POLICY_NEVER);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_container_add(GTK_CONTAINER(scroll), tabs);
    GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(header), previous, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header), scroll, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(header), next, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(body), GTK_WIDGET(notebook), TRUE, TRUE, 0);
    gtk_widget_set_vexpand(root, TRUE);

    g_signal_connect(previous, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
      static_cast<ScrollableTabBar *>(data)->move(-1);
    }), this);
    g_signal_connect(next, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
      static_cast<ScrollableTabBar *>(data)->move(1);
    }), this);
    GtkAdjustment *adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scroll));
    for (const char *signal : {"changed", "value-changed"}) {
      g_signal_connect(adjustment, signal, G_CALLBACK(+[](GtkAdjustment *, gpointer data) {
        static_cast<ScrollableTabBar *>(data)->updateArrows();
      }), this);
    }
    g_signal_connect(notebook, "page-added", G_CALLBACK(+[](GtkNotebook *, GtkWidget *page,
                                                          guint, gpointer data) {
      static_cast<ScrollableTabBar *>(data)->add(page);
    }), this);
    g_signal_connect(notebook, "page-removed", G_CALLBACK(+[](GtkNotebook *, GtkWidget *page,
                                                            guint, gpointer data) {
      auto *self = static_cast<ScrollableTabBar *>(data);
      if (self->dragPage == page) {
        self->finishDrag(false);
      }
      const auto it = std::find_if(self->items.begin(), self->items.end(),
                                  [page](const Tab &tab) { return tab.page == page; });
      if (it != self->items.end()) {
        GtkWidget *button = it->button;
        self->items.erase(it);
        gtk_widget_destroy(button);
      }
      self->sync();
    }), this);
    g_signal_connect_after(notebook, "switch-page", G_CALLBACK(+[](GtkNotebook *, GtkWidget *,
                                                                  guint, gpointer data) {
      static_cast<ScrollableTabBar *>(data)->sync();
    }), this);
    // Keep Ctrl+PageUp/PageDown working with the native tab header hidden.
    g_signal_connect(notebook, "change-current-page", G_CALLBACK(+[](GtkNotebook *book,
        gint offset, gpointer) -> gboolean {
      const int count = gtk_notebook_get_n_pages(book);
      if (count > 0) {
        const int index = (gtk_notebook_get_current_page(book) + offset % count + count) % count;
        gtk_notebook_set_current_page(book, index);
      }
      g_signal_stop_emission_by_name(book, "change-current-page");
      return TRUE;
    }), this);
    g_signal_connect(notebook, "page-reordered", G_CALLBACK(+[](GtkNotebook *, GtkWidget *,
                                                              guint, gpointer data) {
      auto *self = static_cast<ScrollableTabBar *>(data);
      self->finishDrag(false);
      for (const auto &tab : self->items) {
        gtk_box_reorder_child(GTK_BOX(self->tabs), tab.button,
                             gtk_notebook_page_num(self->notebook, tab.page));
      }
      self->sync();
    }), this);
    g_signal_connect(root, "destroy", G_CALLBACK(+[](GtkWidget *, gpointer data) {
      auto *self = static_cast<ScrollableTabBar *>(data);
      self->finishDrag(false);
      if (self->revealSource) g_source_remove(self->revealSource);
      self->revealSource = 0;
      g_signal_handlers_disconnect_by_data(self->notebook, self);
      GtkAdjustment *adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(self->scroll));
      g_signal_handlers_disconnect_by_data(adjustment, self);
    }), this);
    updateArrows();
  }

  void startPreview(GtkWidget *source) {
    dragSource = source;
    ghostWidth = gtk_widget_get_allocated_width(source);
    ghostHeight = gtk_widget_get_allocated_height(source);
    dragSnapshot = gdk_window_create_similar_surface(gtk_widget_get_window(source),
        CAIRO_CONTENT_COLOR_ALPHA, ghostWidth, ghostHeight);
    cairo_t *cr = cairo_create(dragSnapshot);
    gtk_widget_draw(source, cr);
    cairo_destroy(cr);
    sourceOpacity = gtk_widget_get_opacity(source);
    gtk_widget_set_opacity(source, 0.25);
    dragging = true;
    if (revealSource) g_source_remove(revealSource);
    revealSource = 0;
    gtk_widget_show(dragLayer);
    dragTick = gtk_widget_add_tick_callback(root, +[](GtkWidget *, GdkFrameClock *,
                                                     gpointer data) -> gboolean {
      auto *self = static_cast<ScrollableTabBar *>(data);
      if (self->previewDirty) {
        GdkRectangle damage{static_cast<int>(self->ghostX) - 2,
                            static_cast<int>(self->ghostY) - 2,
                            self->ghostWidth + 8, self->ghostHeight + 8};
        GdkRectangle marker{self->markerX - 2, self->markerY, 5, self->markerHeight};
        gdk_rectangle_union(&damage, &marker, &damage);
        GdkRectangle redraw = damage;
        if (self->hasDamage) gdk_rectangle_union(&redraw, &self->lastDamage, &redraw);
        gtk_widget_queue_draw_area(self->dragLayer, redraw.x, redraw.y, redraw.width, redraw.height);
        self->lastDamage = damage;
        self->hasDamage = true;
        self->previewDirty = false;
      }
      return G_SOURCE_CONTINUE;
    }, this, nullptr);
  }

  void finishDrag(bool commit) {
    GtkWidget *page = dragPage;
    const int destination = dropIndex;
    const bool wasDragging = dragging;
    if (dragTick) gtk_widget_remove_tick_callback(root, dragTick);
    dragTick = 0;
    gtk_widget_hide(dragLayer);
    if (dragSource) gtk_widget_set_opacity(dragSource, sourceOpacity);
    if (dragSnapshot) cairo_surface_destroy(dragSnapshot);
    dragSnapshot = nullptr;
    dragSource = nullptr;
    dragPage = nullptr;
    dragging = false;
    dropIndex = -1;
    hasDamage = false;
    previewDirty = false;
    if (commit && wasDragging && page && destination >= 0 &&
        gtk_notebook_page_num(notebook, page) >= 0 &&
        gtk_notebook_get_tab_reorderable(notebook, page))
      gtk_notebook_reorder_child(notebook, page, destination);
  }

  void updateArrows() {
    GtkAdjustment *adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scroll));
    const double value = gtk_adjustment_get_value(adjustment);
    gtk_widget_set_sensitive(previous, value > gtk_adjustment_get_lower(adjustment) + 0.5);
    gtk_widget_set_sensitive(next, value + gtk_adjustment_get_page_size(adjustment) <
                                   gtk_adjustment_get_upper(adjustment) - 0.5);
  }

  void move(int direction) {
    GtkAdjustment *adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scroll));
    const double step = std::max(80.0, gtk_adjustment_get_page_size(adjustment) / 2);
    const double maximum = std::max(gtk_adjustment_get_lower(adjustment),
        gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));
    gtk_adjustment_set_value(adjustment, std::clamp(
        gtk_adjustment_get_value(adjustment) + direction * step,
        gtk_adjustment_get_lower(adjustment), maximum));
  }

  void sync() {
    GtkWidget *active = gtk_notebook_get_nth_page(notebook, gtk_notebook_get_current_page(notebook));
    for (const auto &tab : items) {
      if (tab.page == active)
        gtk_widget_set_state_flags(tab.button, GTK_STATE_FLAG_CHECKED, FALSE);
      else
        gtk_widget_unset_state_flags(tab.button, GTK_STATE_FLAG_CHECKED);
    }
    if (!dragging && !revealSource) {
      revealSource = g_idle_add(+[](gpointer data) -> gboolean {
        auto *self = static_cast<ScrollableTabBar *>(data);
        self->revealSource = 0;
        GtkWidget *active = gtk_notebook_get_nth_page(self->notebook,
            gtk_notebook_get_current_page(self->notebook));
        for (const auto &tab : self->items) {
          if (tab.page != active) continue;
          int x = 0, y = 0;
          if (gtk_widget_translate_coordinates(tab.button, self->tabs, 0, 0, &x, &y)) {
            GtkAdjustment *adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(self->scroll));
            gtk_adjustment_clamp_page(adjustment, x, x + gtk_widget_get_allocated_width(tab.button));
          }
          break;
        }
        return G_SOURCE_REMOVE;
      }, this);
    }
  }

  void previewAt(GtkWidget *source, double x, double y) {
    if (!dragPage || !gtk_notebook_get_tab_reorderable(notebook, dragPage)) return;
    const int from = gtk_notebook_page_num(notebook, dragPage);
    if (from < 0) return;
    int tabX = 0, tabY = 0;
    if (!gtk_widget_translate_coordinates(source, tabs, static_cast<int>(x),
                                         static_cast<int>(y), &tabX, &tabY)) return;
    int insertion = gtk_notebook_get_n_pages(notebook);
    int insertionX = 0;
    GList *children = gtk_container_get_children(GTK_CONTAINER(tabs));
    int index = 0;
    for (GList *child = children; child; child = child->next, ++index) {
      GtkWidget *tab = GTK_WIDGET(child->data);
      int left = 0, top = 0;
      gtk_widget_translate_coordinates(tab, tabs, 0, 0, &left, &top);
      insertionX = left + gtk_widget_get_allocated_width(tab);
      if (tab == source) continue;
      if (tabX < left + gtk_widget_get_allocated_width(tab) / 2) {
        insertion = index;
        insertionX = left;
        break;
      }
    }
    g_list_free(children);
    gtk_widget_translate_coordinates(tabs, root, insertionX, 0, &markerX, &markerY);
    markerHeight = gtk_widget_get_allocated_height(tabs);
    int pointerX = 0, pointerY = 0;
    gtk_widget_translate_coordinates(source, root, static_cast<int>(x), static_cast<int>(y),
                                     &pointerX, &pointerY);
    ghostX = pointerX - grabX;
    ghostY = pointerY - grabY + 6;
    // Keep the preview inside the application while allowing vertical lift.
    ghostX = std::clamp(ghostX, 0.0, std::max(0.0, double(gtk_widget_get_allocated_width(root) - ghostWidth)));
    ghostY = std::clamp(ghostY, 0.0, std::max(0.0, double(gtk_widget_get_allocated_height(root) - ghostHeight)));
    if (from < insertion) --insertion;
    dropIndex = insertion;
    previewDirty = true;
  }

  void add(GtkWidget *page) {
    GtkWidget *label = gtk_notebook_get_tab_label(notebook, page);
    g_object_ref(label);
    gtk_notebook_set_tab_label(notebook, page, nullptr);
    GtkWidget *button = GTK_WIDGET(g_object_new(document_tab_get_type(), nullptr));
    gtk_widget_set_margin_start(label, 12);
    gtk_widget_set_margin_end(label, 8);
    gtk_widget_set_margin_top(label, 4);
    gtk_widget_set_margin_bottom(label, 4);
    if (GTK_IS_BOX(label)) gtk_box_set_spacing(GTK_BOX(label), 6);
    gtk_container_add(GTK_CONTAINER(button), label);
    g_object_unref(label);
    gtk_box_pack_start(GTK_BOX(tabs), button, FALSE, FALSE, 0);
    items.push_back({page, button});
    g_object_set_data(G_OBJECT(button), "document-page", page);
    g_signal_connect(button, "button-press-event", G_CALLBACK(+[](GtkWidget *button,
        GdkEventButton *event, gpointer data) -> gboolean {
      if (event->button != GDK_BUTTON_PRIMARY) return FALSE;
      GtkWidget *target = gtk_get_event_widget(reinterpret_cast<GdkEvent *>(event));
      for (GtkWidget *child = target; child && child != button; child = gtk_widget_get_parent(child)) {
        if (GTK_IS_BUTTON(child)) return FALSE;
      }
      auto *self = static_cast<ScrollableTabBar *>(data);
      GtkWidget *page = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "document-page"));
      gtk_notebook_set_current_page(self->notebook, gtk_notebook_page_num(self->notebook, page));
      self->sync();
      gtk_widget_grab_focus(page);
      return FALSE;
    }), this);
    GtkGesture *gesture = gtk_gesture_drag_new(button);
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), GDK_BUTTON_PRIMARY);
    g_object_set_data_full(G_OBJECT(button), "tab-drag-gesture", gesture, g_object_unref);
    g_signal_connect(gesture, "drag-begin", G_CALLBACK(+[](GtkGestureDrag *gesture,
        gdouble x, gdouble y, gpointer data) {
      auto *self = static_cast<ScrollableTabBar *>(data);
      GtkWidget *source = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
      const GdkEvent *event = gtk_gesture_get_last_event(GTK_GESTURE(gesture), nullptr);
      GtkWidget *target = event ? gtk_get_event_widget(const_cast<GdkEvent *>(event)) : nullptr;
      for (GtkWidget *child = target; child && child != source; child = gtk_widget_get_parent(child)) {
        if (GTK_IS_BUTTON(child)) {
          gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_DENIED);
          return;
        }
      }
      self->dragPage = GTK_WIDGET(g_object_get_data(G_OBJECT(source), "document-page"));
      self->grabX = x;
      self->grabY = y;
    }), this);
    g_signal_connect(gesture, "drag-update", G_CALLBACK(+[](GtkGestureDrag *gesture,
        gdouble offsetX, gdouble offsetY, gpointer data) {
      auto *self = static_cast<ScrollableTabBar *>(data);
      GtkWidget *source = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture));
      if (!self->dragPage || !gtk_notebook_get_tab_reorderable(self->notebook, self->dragPage)) return;
      double startX = 0;
      double startY = 0;
      gtk_gesture_drag_get_start_point(gesture, &startX, &startY);
      if (!self->dragging && !gtk_drag_check_threshold(source, startX, startY,
                                                       startX + offsetX, startY + offsetY)) return;
      if (!self->dragging) self->startPreview(source);
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
      double x = 0;
      double y = 0;
      if (gtk_gesture_get_point(GTK_GESTURE(gesture), nullptr, &x, &y))
        self->previewAt(source, x, y);
    }), this);
    g_signal_connect(gesture, "drag-end", G_CALLBACK(+[](GtkGestureDrag *, gdouble,
                                                        gdouble, gpointer data) {
      auto *self = static_cast<ScrollableTabBar *>(data);
      const bool wasDragging = self->dragging;
      self->finishDrag(true);
      if (wasDragging) self->sync();
    }), this);
    g_signal_connect(gesture, "cancel", G_CALLBACK(+[](GtkGesture *, GdkEventSequence *, gpointer data) {
      auto *self = static_cast<ScrollableTabBar *>(data);
      self->finishDrag(false);
    }), this);
    gtk_widget_show_all(button);
    sync();
  }
};

} // namespace

GtkWidget *createScrollableTabBar(GtkNotebook *notebook) {
  auto *bar = new ScrollableTabBar(notebook);
  g_object_set_data_full(G_OBJECT(bar->root), "document-tab-bar", bar,
                        +[](gpointer data) { delete static_cast<ScrollableTabBar *>(data); });
  return bar->root;
}
