#pragma once

#include <gtk/gtk.h>

class EditorWindow;

class DocumentRefresh {
public:
    explicit DocumentRefresh(EditorWindow *editorWindow);
    ~DocumentRefresh() = default;

    // State refresh operations
    void refreshDocumentState(int index, bool syncDiskState);
    void refreshAllDocumentStates(bool syncDiskState);
    void syncDocumentFileState(int index);
    void acknowledgeFileState(int index);
    void checkOpenDocumentFileStates();
    void refreshDocumentStatesFromWorkspace(bool allowPrompts);

    // Document state operations
    void applyDocumentEolMode(int index);
    void applyDocumentReadOnlyState(int index);
    void setDocumentModified(int index, bool modified);
    void set_document_modified(bool modified);
    void markDocumentSaved(int index);

private:
    EditorWindow *editorWindow;

    void refreshPeriodicDocumentStates();
};
