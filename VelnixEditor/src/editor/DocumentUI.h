#pragma once

#include "core/EditorTypes.h"
#include <gtk/gtk.h>
#include <string>

class EditorWindow;

class DocumentUI {
public:
    explicit DocumentUI(EditorWindow *editorWindow);
    ~DocumentUI() = default;

    // UI operations
    void updateDocumentTitle(int index);
    void reorderDocumentsToMatchNotebook();
    void switchToDocument(int index);

    // Document finding
    int findDocumentIndexByScintilla(GtkWidget *scintilla);
    int findDocumentIndexByCloseButton(GtkWidget *button);
    int findDocumentIndexByPath(const std::string &path);

private:
    EditorWindow *editorWindow;
};
