// Scintilla GTK 测试程序
// 作者：englyf

#include <gtk/gtk.h>
#define GTK 1
#include <Scintilla.h>
#include <ScintillaWidget.h>

extern "C" {
GtkWidget* scintilla_new(void);
}

static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Velnix Editor GTK Test");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

    // Create Scintilla widget
    GtkWidget *scintilla = scintilla_new();
    gtk_container_add(GTK_CONTAINER(window), scintilla);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.velnix.editor.test", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
