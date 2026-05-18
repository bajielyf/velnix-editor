#pragma once

#include "core/EditorTypes.h"
#include "editor/DocumentOperations.h"
#include "editor/DocumentUI.h"
#include "editor/DocumentRefresh.h"
#include "editor/DocumentPrompt.h"
#include "editor/SyntaxHighlighter.h"
#include <gtk/gtk.h>
#include <string>
#include <memory>
#include <vector>

class EditorWindow;

class DocumentManager {
public:
    explicit DocumentManager(EditorWindow *editorWindow);
    ~DocumentManager();

    // Document state methods
    Document *getCurrentDocument();
    GtkWidget *getCurrentScintilla();
    bool isDocumentBlank(int index);
    int getCurrentDocumentIndex() const;
    std::string getEditorFontFamily() const;
    int getEditorFontSize() const;
    void applyCurrentSyntaxHighlighting() {
        if (syntaxHighlighter) syntaxHighlighter->applyCurrentDocumentHighlighting();
    }

    // Syntax highlighting methods
    void setLexer(const std::string& lexerName) {
        if (syntaxHighlighter) syntaxHighlighter->setLexer(lexerName);
    }

    void setLexerByFileExtension(const std::string& filePath) {
        if (syntaxHighlighter) syntaxHighlighter->setLexerByFileExtension(filePath);
    }

    std::string getCurrentLexer() const {
        return syntaxHighlighter ? syntaxHighlighter->getCurrentLexer() : "null";
    }

    const std::vector<LexerDefinition>& getAvailableLexers() const {
        static std::vector<LexerDefinition> empty;
        return syntaxHighlighter ? syntaxHighlighter->getAvailableLexers() : empty;
    }

    void enableSyntaxHighlighting(bool enable) {
        if (syntaxHighlighter) syntaxHighlighter->enableHighlighting(enable);
    }

    bool isSyntaxHighlightingEnabled() const {
        return syntaxHighlighter ? syntaxHighlighter->isHighlightingEnabled() : false;
    }

    // Delegated methods to sub-modules
    int createNewDocument(const std::string &title = "Untitled") {
        return operations ? operations->createNewDocument(title) : -1;
    }

    void closeDocument(int index) {
        if (operations) operations->closeDocument(index);
    }

    bool saveDocument(int index) {
        return operations ? operations->saveDocument(index) : false;
    }

    bool saveDocumentAs(int index) {
        return operations ? operations->saveDocumentAs(index) : false;
    }

    bool saveDocumentToPath(int index, const std::string &path) {
        return operations ? operations->saveDocumentToPath(index, path) : false;
    }

    bool loadDocumentFromPath(int index, const std::string &path) {
        return operations ? operations->loadDocumentFromPath(index, path) : false;
    }

    bool reloadDocumentFromDisk(int index) {
        return operations ? operations->reloadDocumentFromDisk(index) : false;
    }

    std::string get_last_error() const {
        return operations ? operations->get_last_error() : "";
    }

    std::string getDocumentText(int index) {
        return operations ? operations->getDocumentText(index) : "";
    }

    void setDocumentText(int index, const std::string &text) {
        if (operations) operations->setDocumentText(index, text);
    }

    void updateDocumentTitle(int index) {
        if (ui) ui->updateDocumentTitle(index);
    }

    void reorderDocumentsToMatchNotebook() {
        if (ui) ui->reorderDocumentsToMatchNotebook();
    }

    void switchToDocument(int index) {
        if (ui) ui->switchToDocument(index);
    }

    int findDocumentIndexByScintilla(GtkWidget *scintilla) {
        return ui ? ui->findDocumentIndexByScintilla(scintilla) : -1;
    }

    int findDocumentIndexByCloseButton(GtkWidget *button) {
        return ui ? ui->findDocumentIndexByCloseButton(button) : -1;
    }

    int findDocumentIndexByPath(const std::string &path) {
        return ui ? ui->findDocumentIndexByPath(path) : -1;
    }

    void refreshDocumentState(int index, bool syncDiskState) {
        if (refresh) refresh->refreshDocumentState(index, syncDiskState);
    }

    void refreshAllDocumentStates(bool syncDiskState) {
        if (refresh) refresh->refreshAllDocumentStates(syncDiskState);
    }

    void syncDocumentFileState(int index) {
        if (refresh) refresh->syncDocumentFileState(index);
    }

    void acknowledgeFileState(int index) {
        if (refresh) refresh->acknowledgeFileState(index);
    }

    void checkOpenDocumentFileStates() {
        if (refresh) refresh->checkOpenDocumentFileStates();
    }

    void refreshDocumentStatesFromWorkspace(bool allowPrompts) {
        if (refresh) refresh->refreshDocumentStatesFromWorkspace(allowPrompts);
    }

    void applyDocumentEolMode(int index) {
        if (refresh) refresh->applyDocumentEolMode(index);
    }

    void applyDocumentReadOnlyState(int index) {
        if (refresh) refresh->applyDocumentReadOnlyState(index);
    }

    void setDocumentModified(int index, bool modified) {
        if (refresh) refresh->setDocumentModified(index, modified);
    }

    void set_document_modified(bool modified) {
        if (refresh) refresh->set_document_modified(modified);
    }

    void markDocumentSaved(int index) {
        if (refresh) refresh->markDocumentSaved(index);
    }

    bool promptForMissingFile(int index) {
        return prompt ? prompt->promptForMissingFile(index) : false;
    }

    bool promptForInaccessibleFile(int index) {
        return prompt ? prompt->promptForInaccessibleFile(index) : false;
    }

    bool checkExternalModificationForDocument(int index) {
        return prompt ? prompt->checkExternalModificationForDocument(index) : false;
    }

    void show_error_dialog(const std::string &primary, const std::string &secondary) {
        if (prompt) prompt->show_error_dialog(primary, secondary);
    }

    // Static utility methods (delegated to DocumentIO)
    static std::string basename_from_path(const std::string &path);
    static std::string normalize_path(const std::string &path);
    static int scintilla_eol_mode(EolMode mode);
    static EolMode detect_eol_mode(const std::string &text);
    static std::string convert_eol_text(const std::string &text, EolMode mode);
    static DocumentStatus computeDocumentStatus(const Document &doc);

private:
    EditorWindow *editorWindow;
    std::unique_ptr<DocumentOperations> operations;
    std::unique_ptr<DocumentUI> ui;
    std::unique_ptr<DocumentRefresh> refresh;
    std::unique_ptr<DocumentPrompt> prompt;
    std::unique_ptr<SyntaxHighlighter> syntaxHighlighter;
};
