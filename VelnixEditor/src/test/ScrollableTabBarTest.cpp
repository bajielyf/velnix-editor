#include "ui/ScrollableTabBar.h"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

void check(bool condition, const char *message) {
  if (!condition) {
    std::fprintf(stderr, "[FAIL] %s\n", message);
    std::exit(1);
  }
}

GtkWidget *childAt(GtkWidget *parent, int index) {
  GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
  GtkWidget *child = GTK_WIDGET(g_list_nth_data(children, index));
  g_list_free(children);
  return child;
}

void settle() {
  const gint64 until = g_get_monotonic_time() + 100000;
  do {
    while (gtk_events_pending()) gtk_main_iteration();
    g_usleep(1000);
  } while (g_get_monotonic_time() < until);
}

void click(GtkWidget *widget) {
  GdkWindow *window = GTK_IS_BUTTON(widget)
      ? gtk_button_get_event_window(GTK_BUTTON(widget))
      : gtk_widget_get_window(widget);
  const int x = gdk_window_get_width(window) / 2;
  const int y = gdk_window_get_height(window) / 2;
  GdkDevice *pointer = gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_window_get_display(window)));
  GdkEvent *enter = gdk_event_new(GDK_ENTER_NOTIFY);
  enter->crossing.window = GDK_WINDOW(g_object_ref(window));
  enter->crossing.x = x;
  enter->crossing.y = y;
  enter->crossing.mode = GDK_CROSSING_NORMAL;
  enter->crossing.detail = GDK_NOTIFY_NONLINEAR;
  gdk_event_set_device(enter, pointer);
  gtk_main_do_event(enter);
  gdk_event_free(enter);
  // Dispatch through GTK's normal event routing, including nested widgets.
  for (GdkEventType type : {GDK_BUTTON_PRESS, GDK_BUTTON_RELEASE}) {
    GdkEvent *event = gdk_event_new(type);
    event->button.window = GDK_WINDOW(g_object_ref(window));
    event->button.time = GDK_CURRENT_TIME;
    event->button.button = GDK_BUTTON_PRIMARY;
    event->button.x = x;
    event->button.y = y;
    event->button.state = type == GDK_BUTTON_RELEASE ? GDK_BUTTON1_MASK : 0;
    gdk_event_set_device(event, pointer);
    gtk_main_do_event(event);
    gdk_event_free(event);
  }
  settle();
}

void wheel(GtkWidget *widget, GdkScrollDirection direction, double dx = 0, double dy = 0) {
  GdkWindow *window = GTK_IS_BUTTON(widget)
      ? gtk_button_get_event_window(GTK_BUTTON(widget)) : gtk_widget_get_window(widget);
  GdkEvent *event = gdk_event_new(GDK_SCROLL);
  event->scroll.window = GDK_WINDOW(g_object_ref(window));
  event->scroll.x = gdk_window_get_width(window) / 2;
  event->scroll.y = gdk_window_get_height(window) / 2;
  event->scroll.direction = direction;
  event->scroll.delta_x = dx;
  event->scroll.delta_y = dy;
  gdk_event_set_device(event,
      gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_window_get_display(window))));
  gtk_main_do_event(event);
  gdk_event_free(event);
  settle();
}

GtkWidget *appendClosablePage(GtkNotebook *notebook, GtkWidget **closeOut) {
  GtkWidget *page = gtk_label_new("content");
  GtkWidget *label = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
  GtkWidget *close = gtk_button_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
  gtk_box_pack_start(GTK_BOX(label), gtk_label_new("tab"), FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(label), close, FALSE, FALSE, 0);
  GtkTargetEntry fileTarget = {const_cast<gchar *>("text/uri-list"), 0, 0};
  gtk_drag_dest_set(label, GTK_DEST_DEFAULT_ALL, &fileTarget, 1, GDK_ACTION_COPY);
  g_object_set_data(G_OBJECT(close), "test-page", page);
  g_signal_connect(close, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
    GtkNotebook *book = GTK_NOTEBOOK(data);
    GtkWidget *page = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "test-page"));
    gtk_notebook_remove_page(book, gtk_notebook_page_num(book, page));
  }), notebook);
  gtk_notebook_append_page(notebook, page, label);
  gtk_notebook_set_tab_reorderable(notebook, page, TRUE);
  *closeOut = close;
  return page;
}

void dragTab(GtkWidget *source, GtkWidget *target, bool after, bool cancel = false) {
  GtkWidget *root = gtk_bin_get_child(GTK_BIN(gtk_widget_get_toplevel(source)));
  GtkWidget *layer = childAt(root, 1);
  GtkNotebook *book = GTK_NOTEBOOK(childAt(gtk_bin_get_child(GTK_BIN(root)), 1));
  GtkWidget *page = GTK_WIDGET(g_object_get_data(G_OBJECT(source), "document-page"));
  const int originalIndex = gtk_notebook_page_num(book, page);
  const int originalWidth = gtk_widget_get_allocated_width(source);
  GdkWindow *window = gtk_widget_get_window(source);
  int rootX = 0, rootY = 0;
  gdk_window_get_origin(window, &rootX, &rootY);
  const int startX = rootX + gdk_window_get_width(window) / 4;
  const int startY = rootY + gdk_window_get_height(window) / 2;
  int targetX = 0, targetY = 0;
  gdk_window_get_origin(gtk_widget_get_window(target), &targetX, &targetY);
  targetX += gtk_widget_get_allocated_width(target) * (after ? 3 : 1) / 4;
  GdkDevice *pointer = gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_window_get_display(window)));
  auto send = [&](GdkEventType type, int x, int y) {
    int originX = 0, originY = 0;
    gdk_window_get_origin(window, &originX, &originY);
    GdkEvent *event = gdk_event_new(type);
    event->any.window = GDK_WINDOW(g_object_ref(window));
    if (type == GDK_MOTION_NOTIFY) {
      event->motion.x = x - originX;
      event->motion.y = y - originY;
      event->motion.x_root = x;
      event->motion.y_root = y;
      event->motion.state = GDK_BUTTON1_MASK;
    } else {
      event->button.button = GDK_BUTTON_PRIMARY;
      event->button.x = x - originX;
      event->button.y = y - originY;
      event->button.x_root = x;
      event->button.y_root = y;
      event->button.state = type == GDK_BUTTON_RELEASE ? GDK_BUTTON1_MASK : 0;
    }
    gdk_event_set_device(event, pointer);
    gtk_main_do_event(event);
    gdk_event_free(event);
    settle();
  };
  send(GDK_BUTTON_PRESS, startX, startY);
  for (int i = 1; i <= 8; ++i)
    send(GDK_MOTION_NOTIFY, startX + (targetX - startX) * i / 8, startY);
  check(gtk_widget_get_mapped(layer), "drag preview overlay is visible");
  check(gtk_overlay_get_overlay_pass_through(GTK_OVERLAY(root), layer),
        "drag preview does not intercept pointer events");
  check(gtk_widget_get_opacity(source) < 0.5, "original tab becomes a faded placeholder");
  check(gtk_widget_get_allocated_width(source) == originalWidth, "placeholder retains tab width");
  check(gtk_notebook_page_num(book, page) == originalIndex, "drag preview does not commit ordering early");
  if (const char *screenshot = std::getenv("VELNIX_TEST_DRAG_SCREENSHOT")) {
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
        gtk_widget_get_allocated_width(root), gtk_widget_get_allocated_height(root));
    cairo_t *cr = cairo_create(surface);
    gtk_widget_draw(root, cr);
    cairo_destroy(cr);
    cairo_surface_write_to_png(surface, screenshot);
    cairo_surface_destroy(surface);
  }
  if (cancel) {
    auto *gesture = GTK_EVENT_CONTROLLER(g_object_get_data(G_OBJECT(source), "tab-drag-gesture"));
    gtk_event_controller_reset(gesture);
    check(gtk_notebook_page_num(book, page) == originalIndex, "cancelled drag preserves document order");
  }
  send(GDK_BUTTON_RELEASE, targetX, startY);
  check(!gtk_widget_get_visible(layer), "drag preview disappears after drop");
  check(gtk_widget_get_opacity(source) == 1, "placeholder opacity is restored after drop");
}

} // namespace

int main(int argc, char **argv) {
  gtk_init(&argc, &argv);
  g_log_set_always_fatal(static_cast<GLogLevelFlags>(G_LOG_FATAL_MASK | G_LOG_LEVEL_CRITICAL));
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 200);
  GtkNotebook *notebook = GTK_NOTEBOOK(gtk_notebook_new());
  GtkWidget *root = createScrollableTabBar(notebook);
  gtk_container_add(GTK_CONTAINER(window), root);
  GtkWidget *first = nullptr;
  GtkWidget *last = nullptr;
  int editorClicks = 0;
  for (int i = 0; i < 12; ++i) {
    GtkWidget *page = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(page), gtk_label_new("Document content"));
    const std::string name = "Document " + std::to_string(i) + ".cpp";
    gtk_notebook_append_page(notebook, page, gtk_label_new(name.c_str()));
    if (i == 0) {
      first = page;
      g_signal_connect(page, "button-press-event", G_CALLBACK(+[](GtkWidget *, GdkEventButton *,
                                                                 gpointer data) -> gboolean {
        ++*static_cast<int *>(data);
        return TRUE;
      }), &editorClicks);
    }
    last = page;
  }
  gtk_widget_show_all(window);
  settle();
  GtkWidget *header = childAt(gtk_bin_get_child(GTK_BIN(root)), 0);
  GtkWidget *previous = childAt(header, 0);
  GtkWidget *scroll = childAt(header, 1);
  GtkWidget *next = childAt(header, 2);
  GtkWidget *tabs = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(scroll))));
  GtkAdjustment *adjustment = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scroll));
  check(gtk_widget_get_sensitive(next), "overflow enables right arrow");
  const double initial = gtk_adjustment_get_value(adjustment);
  gtk_button_clicked(GTK_BUTTON(next));
  settle();
  check(gtk_adjustment_get_value(adjustment) > initial, "right arrow scrolls visible tabs");
  check(gtk_notebook_get_current_page(notebook) == 0, "right arrow preserves active document");
  check(gtk_widget_get_state_flags(childAt(tabs, 0)) & GTK_STATE_FLAG_CHECKED,
        "scrolling preserves active tab highlight");
  gtk_button_clicked(GTK_BUTTON(previous));
  settle();
  check(gtk_adjustment_get_value(adjustment) == initial, "left arrow scrolls back");
  check(gtk_notebook_get_current_page(notebook) == 0, "left arrow preserves active document");
  wheel(childAt(tabs, 0), GDK_SCROLL_DOWN);
  const double wheelStep = gtk_adjustment_get_value(adjustment);
  check(wheelStep > initial, "wheel over tab scrolls the tab strip");
  check(gtk_notebook_get_current_page(notebook) == 0, "wheel preserves active document");
  wheel(previous, GDK_SCROLL_UP);
  check(gtk_adjustment_get_value(adjustment) == initial, "wheel over arrow scrolls left");
  wheel(childAt(tabs, 0), GDK_SCROLL_SMOOTH, 0, 0.5);
  check(gtk_adjustment_get_value(adjustment) > initial &&
        gtk_adjustment_get_value(adjustment) < wheelStep, "smooth vertical scrolling supports fractional deltas");
  wheel(previous, GDK_SCROLL_SMOOTH, -0.5, 0);
  check(gtk_adjustment_get_value(adjustment) == initial, "smooth horizontal scrolling scrolls left");
  for (int i = 0; i < 30; ++i) wheel(next, GDK_SCROLL_DOWN);
  check(gtk_adjustment_get_value(adjustment) > initial, "wheel can browse beyond active tab");
  check(gtk_notebook_get_current_page(notebook) == 0, "repeated wheel scrolling preserves active document");
  click(first);
  check(gtk_adjustment_get_value(adjustment) < 1, "editor click reveals current tab after wheel scrolling");
  check(gtk_notebook_get_current_page(notebook) == 0, "editor click does not switch documents");
  check(editorClicks == 1, "tab reveal does not consume the editor click");
  for (int i = 0; i < 30; ++i) gtk_button_clicked(GTK_BUTTON(next));
  settle();
  check(!gtk_widget_get_sensitive(next), "right arrow stops at final tab");
  check(gtk_notebook_get_current_page(notebook) == 0, "repeated scrolling preserves document");
  click(first);
  check(gtk_adjustment_get_value(adjustment) < 1, "editor click reveals current tab after arrow scrolling");
  check(editorClicks == 2, "editor still receives clicks after arrow browsing");
  for (int i = 0; i < 30; ++i) gtk_button_clicked(GTK_BUTTON(next));
  settle();
  click(childAt(tabs, 11));
  settle();
  check(gtk_notebook_get_current_page(notebook) == 11, "clicking tab activates document");
  gtk_notebook_set_current_page(notebook, 0);
  settle();
  check(gtk_adjustment_get_value(adjustment) < 1, "programmatic activation reveals selected tab");
  gboolean handled = FALSE;
  g_signal_emit_by_name(notebook, "change-current-page", 1, &handled);
  check(handled && gtk_notebook_get_current_page(notebook) == 1,
        "keyboard page navigation remains available");
  g_signal_emit_by_name(notebook, "change-current-page", -1, &handled);
  check(gtk_notebook_get_current_page(notebook) == 0, "keyboard previous page navigation");
  gtk_notebook_reorder_child(notebook, last, 0);
  settle();
  check(g_object_get_data(G_OBJECT(childAt(tabs, 0)), "document-page") == last,
        "tab order follows document reorder");
  gtk_notebook_remove_page(notebook, 0);
  settle();
  check(gtk_notebook_get_nth_page(notebook, gtk_notebook_get_current_page(notebook)) == first,
        "closing inactive tab preserves active document");
  while (gtk_notebook_get_n_pages(notebook)) gtk_notebook_remove_page(notebook, 0);
  settle();
  check(!gtk_widget_get_sensitive(previous) && !gtk_widget_get_sensitive(next),
        "empty tab strip disables both arrows");
  gtk_notebook_append_page(notebook, gtk_label_new("new content"), gtk_label_new("new tab"));
  gtk_widget_show_all(window);
  settle();
  check(!gtk_widget_get_sensitive(previous) && !gtk_widget_get_sensitive(next),
        "tabs that fit in the viewport disable both arrows");
  gtk_notebook_remove_page(notebook, 0);
  GtkWidget *firstClose = nullptr;
  GtkWidget *secondClose = nullptr;
  GtkWidget *firstPage = appendClosablePage(notebook, &firstClose);
  appendClosablePage(notebook, &secondClose);
  GtkWidget *thirdClose = nullptr;
  GtkWidget *thirdPage = appendClosablePage(notebook, &thirdClose);
  gtk_widget_show_all(window);
  settle();
  dragTab(childAt(tabs, 0), childAt(tabs, 2), true);
  check(gtk_notebook_page_num(notebook, firstPage) == 2, "mouse drag moves first tab after last tab");
  check(g_object_get_data(G_OBJECT(childAt(tabs, 2)), "document-page") == firstPage,
        "mouse drag synchronizes visible tab order");
  check(gtk_notebook_get_nth_page(notebook, gtk_notebook_get_current_page(notebook)) == firstPage,
        "dragging preserves the dragged document as active");
  dragTab(childAt(tabs, 2), childAt(tabs, 0), false);
  check(gtk_notebook_page_num(notebook, firstPage) == 0, "mouse drag moves last tab before first tab");
  dragTab(childAt(tabs, 0), childAt(tabs, 2), true, true);
  gtk_notebook_remove_page(notebook, gtk_notebook_page_num(notebook, thirdPage));
  settle();
  click(secondClose);
  check(gtk_notebook_get_n_pages(notebook) == 1, "mouse click closes inactive tab");
  check(gtk_notebook_get_nth_page(notebook, gtk_notebook_get_current_page(notebook)) == firstPage,
        "closing inactive tab by mouse preserves current document");
  click(firstClose);
  check(gtk_notebook_get_n_pages(notebook) == 0, "mouse click closes active tab");
  gtk_widget_destroy(window);
  // Exercise cancellation of a pending reveal callback on window destruction.
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  notebook = GTK_NOTEBOOK(gtk_notebook_new());
  gtk_container_add(GTK_CONTAINER(window), createScrollableTabBar(notebook));
  gtk_notebook_append_page(notebook, gtk_label_new("content"), gtk_label_new("tab"));
  gtk_widget_destroy(window);
  settle();
  std::puts("[PASS] tab scrolling, activation, reordering, removal, and destruction");
  return 0;
}
