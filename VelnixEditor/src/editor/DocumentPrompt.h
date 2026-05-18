#pragma once

#include <gtk/gtk.h>
#include <string>

class EditorWindow;

class DocumentPrompt {
public:
    explicit DocumentPrompt(EditorWindow *editorWindow);
    ~DocumentPrompt() = default;

    // User prompts
    bool promptForMissingFile(int index);
    bool promptForInaccessibleFile(int index);
    bool checkExternalModificationForDocument(int index);
    void show_error_dialog(const std::string &primary, const std::string &secondary);

private:
    EditorWindow *editorWindow;
};
