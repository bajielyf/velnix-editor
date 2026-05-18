#include "editor/DocumentManager.h"
#include "editor/DocumentIO.h"
#include "editor/DocumentState.h"
#include "ui/EditorWindow.h"
#include "ui/EventHandler.h"
#include "config/SettingsManager.h"
#include "config/RecentFilesManager.h"

DocumentManager::DocumentManager(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {
    operations = std::make_unique<DocumentOperations>(editorWindow);
    ui = std::make_unique<DocumentUI>(editorWindow);
    refresh = std::make_unique<DocumentRefresh>(editorWindow);
    prompt = std::make_unique<DocumentPrompt>(editorWindow);
    syntaxHighlighter = std::make_unique<SyntaxHighlighter>(this);
}

DocumentManager::~DocumentManager() = default;

Document *DocumentManager::getCurrentDocument() {
  return editorWindow ? editorWindow->getDocument(editorWindow->getCurrentDocumentIndex())
                      : nullptr;
}

GtkWidget *DocumentManager::getCurrentScintilla() {
  Document *doc = getCurrentDocument();
  return doc ? doc->scintilla : nullptr;
}

bool DocumentManager::isDocumentBlank(int index) {
  Document *doc = editorWindow ? editorWindow->getDocument(index) : nullptr;
  if (!doc) {
    return false;
  }

  return doc->filePath.empty() && !doc->modified && getDocumentText(index).empty();
}

int DocumentManager::getCurrentDocumentIndex() const {
  return editorWindow ? editorWindow->getCurrentDocumentIndex() : -1;
}

std::string DocumentManager::getEditorFontFamily() const {
  return editorWindow ? editorWindow->getEditorFontFamily() : "Monospace";
}

int DocumentManager::getEditorFontSize() const {
  return editorWindow ? editorWindow->getEditorFontSize() : 11;
}

// Static utility methods (delegated to DocumentIO/DocumentState)
std::string DocumentManager::basename_from_path(const std::string &path) {
  return DocumentIO::basename_from_path(path);
}

std::string DocumentManager::normalize_path(const std::string &path) {
  return DocumentIO::normalize_path(path);
}

int DocumentManager::scintilla_eol_mode(EolMode mode) {
  return DocumentIO::scintilla_eol_mode(mode);
}

EolMode DocumentManager::detect_eol_mode(const std::string &text) {
  return DocumentIO::detect_eol_mode(text);
}

std::string DocumentManager::convert_eol_text(const std::string &text,
                                              EolMode targetMode) {
  return DocumentIO::convert_eol_text(text, targetMode);
}

DocumentStatus DocumentManager::computeDocumentStatus(const Document &doc) {
  return DocumentState::compute_document_status(doc);
}
