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
  GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  GtkWidget *tabs = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  GtkWidget *scroll = gtk_scrolled_window_new(nullptr, nullptr);
  GtkWidget *previous = gtk_button_new_from_icon_name("pan-start-symbolic", GTK_ICON_SIZE_MENU);
  GtkWidget *next = gtk_button_new_from_icon_name("pan-end-symbolic", GTK_ICON_SIZE_MENU);
  std::vector<Tab> items;
  guint revealSource = 0;
  GtkWidget *dragPage = nullptr;
  bool dragging = false;

  explicit ScrollableTabBar(GtkNotebook *book) : notebook(book) {
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
    gtk_box_pack_start(GTK_BOX(root), header, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), GTK_WIDGET(notebook), TRUE, TRUE, 0);
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
        self->dragPage = nullptr;
        self->dragging = false;
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
      for (const auto &tab : self->items) {
        gtk_box_reorder_child(GTK_BOX(self->tabs), tab.button,
                             gtk_notebook_page_num(self->notebook, tab.page));
      }
      self->sync();
    }), this);
    g_signal_connect(root, "destroy", G_CALLBACK(+[](GtkWidget *, gpointer data) {
      auto *self = static_cast<ScrollableTabBar *>(data);
      if (self->revealSource) g_source_remove(self->revealSource);
      self->revealSource = 0;
      g_signal_handlers_disconnect_by_data(self->notebook, self);
      GtkAdjustment *adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(self->scroll));
      g_signal_handlers_disconnect_by_data(adjustment, self);
    }), this);
    updateArrows();
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

  void reorderAt(GtkWidget *source, double x, double y) {
    if (!dragPage || !gtk_notebook_get_tab_reorderable(notebook, dragPage)) return;
    const int from = gtk_notebook_page_num(notebook, dragPage);
    if (from < 0) return;
    int tabX = 0, tabY = 0;
    if (!gtk_widget_translate_coordinates(source, tabs, static_cast<int>(x),
                                         static_cast<int>(y), &tabX, &tabY)) return;
    int insertion = gtk_notebook_get_n_pages(notebook);
    GList *children = gtk_container_get_children(GTK_CONTAINER(tabs));
    int index = 0;
    for (GList *child = children; child; child = child->next, ++index) {
      GtkWidget *tab = GTK_WIDGET(child->data);
      if (tab == source) continue;
      int left = 0, top = 0;
      gtk_widget_translate_coordinates(tab, tabs, 0, 0, &left, &top);
      if (tabX < left + gtk_widget_get_allocated_width(tab) / 2) {
        insertion = index;
        break;
      }
    }
    g_list_free(children);
    if (from < insertion) --insertion;
    if (from != insertion) gtk_notebook_reorder_child(notebook, dragPage, insertion);
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
        gdouble, gdouble, gpointer data) {
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
      self->dragging = true;
      if (self->revealSource) g_source_remove(self->revealSource);
      self->revealSource = 0;
      gtk_gesture_set_state(GTK_GESTURE(gesture), GTK_EVENT_SEQUENCE_CLAIMED);
      double x = 0;
      double y = 0;
      if (gtk_gesture_get_point(GTK_GESTURE(gesture), nullptr, &x, &y))
        self->reorderAt(source, x, y);
    }), this);
    g_signal_connect(gesture, "drag-end", G_CALLBACK(+[](GtkGestureDrag *, gdouble,
                                                        gdouble, gpointer data) {
      auto *self = static_cast<ScrollableTabBar *>(data);
      const bool wasDragging = self->dragging;
      self->dragPage = nullptr;
      self->dragging = false;
      if (wasDragging) self->sync();
    }), this);
    g_signal_connect(gesture, "cancel", G_CALLBACK(+[](GtkGesture *, GdkEventSequence *, gpointer data) {
      auto *self = static_cast<ScrollableTabBar *>(data);
      self->dragPage = nullptr;
      self->dragging = false;
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
