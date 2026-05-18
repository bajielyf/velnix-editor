#include "ui/EditorWindow.h"

int main(int argc, char **argv) {
  g_set_prgname("org.velnix.VelnixEditor");
  g_set_application_name("Velnix Editor");

  gtk_init(&argc, &argv);

  EditorWindow *editorWindow = new EditorWindow();
  editorWindow->show();

  gtk_main();

  delete editorWindow;
  return 0;
}
