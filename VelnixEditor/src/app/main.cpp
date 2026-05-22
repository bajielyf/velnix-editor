#include "ui/EditorWindow.h"

#include <string>
#include <vector>

int main(int argc, char **argv) {
  g_set_prgname("org.velnix.VelnixEditor");
  g_set_application_name("Velnix Editor");

  gtk_init(&argc, &argv);

  std::vector<std::string> startupFiles;
  for (int i = 1; i < argc; ++i) {
    if (argv[i] && argv[i][0] != '\0') {
      startupFiles.emplace_back(argv[i]);
    }
  }

  EditorWindow *editorWindow = new EditorWindow(startupFiles);
  editorWindow->show();

  gtk_main();

  delete editorWindow;
  return 0;
}
