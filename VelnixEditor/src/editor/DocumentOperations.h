#pragma once

#include "core/EditorTypes.h"
#include <gtk/gtk.h>
#include <string>

class EditorWindow;

class IDocumentOperationsConfig {
public:
    virtual ~IDocumentOperationsConfig() = default;
    virtual bool getOverwriteReadonly() const = 0;
    virtual bool getTrimTrailingWhitespaceOnSave() const { return false; }
    virtual bool getEnsureFinalNewlineOnSave() const { return false; }
};

class DocumentOperations {
public:
    explicit DocumentOperations(EditorWindow *editorWindow);
    explicit DocumentOperations(IDocumentOperationsConfig *config); // For testing
    ~DocumentOperations() = default;

    // Document creation and management
    int createNewDocument(const std::string &title = "Untitled");
    void closeDocument(int index);
    bool saveDocument(int index);
    bool saveDocumentAs(int index);
    bool saveDocumentToPath(int index, const std::string &path);
    bool loadDocumentFromPath(int index, const std::string &path);
    bool reloadDocumentFromDisk(int index);

    std::string get_last_error() const { return lastError; }

    // Document text operations
    std::string getDocumentText(int index);
    void setDocumentText(int index, const std::string &text);

private:
    void set_last_error(std::string message) { lastError = std::move(message); }
    EditorWindow *editorWindow = nullptr;
    IDocumentOperationsConfig *config = nullptr;
    bool testMode = false;
    std::string testDocumentText; // For testing
    std::string lastError;
};
