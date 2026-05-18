#include "editor/DocumentUI.h"
#include "editor/DocumentIO.h"
#include "ui/EditorWindow.h"
#include <gtk/gtk.h>

DocumentUI::DocumentUI(EditorWindow *editorWindow)
    : editorWindow(editorWindow) {}

void DocumentUI::updateDocumentTitle(int index) {
  Document *doc = this->editorWindow->getDocument(index);
  if (!doc) {
    return;
  }

  std::string displayTitle =
      doc->filePath.empty() ? doc->title : DocumentIO::basename_from_path(doc->filePath);

  if (doc->status == DocumentStatus::Missing) {
    displayTitle += " [Missing]";
  } else if (doc->status == DocumentStatus::Inaccessible) {
    displayTitle += " [Offline]";
  } else if (doc->status == DocumentStatus::ExternallyModified) {
    displayTitle += " [Changed on Disk]";
  }

  if (doc->readOnly) {
    displayTitle += " [RO]";
  }

  if (doc->status == DocumentStatus::Modified) {
    displayTitle += " *";
  }

  gtk_label_set_text(GTK_LABEL(doc->tabTextLabel), displayTitle.c_str());
  gtk_widget_show(doc->tabTextLabel);
  gtk_widget_queue_draw(doc->tabLabel);
  this->editorWindow->queueNotebookDraw();

  this->editorWindow->update_window_title();
}

void DocumentUI::reorderDocumentsToMatchNotebook() {
  std::vector<Document> reordered;
  int pageCount = this->editorWindow->getNotebookPageCount();
  reordered.reserve(this->editorWindow->getDocumentCount());

  for (int page = 0; page < pageCount; ++page) {
    GtkWidget *pageWidget = this->editorWindow->getNotebookPageAt(page);
    int index = findDocumentIndexByScintilla(pageWidget);
    if (index >= 0) {
      const Document *doc = this->editorWindow->getDocument(index);
      if (doc) {
        reordered.push_back(*doc);
      }
    }
  }

  if (static_cast<int>(reordered.size()) == this->editorWindow->getDocumentCount()) {
    this->editorWindow->replaceDocuments(std::move(reordered));
  }
}

void DocumentUI::switchToDocument(int index) {
  if (index >= 0 && index < this->editorWindow->getDocumentCount()) {
    this->editorWindow->activateDocument(index);
  }
}

int DocumentUI::findDocumentIndexByScintilla(GtkWidget *scintilla) {
  for (int i = 0; i < this->editorWindow->getDocumentCount(); ++i) {
    const Document *doc = this->editorWindow->getDocument(i);
    if (doc && doc->scintilla == scintilla) {
      return i;
    }
  }
  return -1;
}

int DocumentUI::findDocumentIndexByCloseButton(GtkWidget *button) {
  for (int i = 0; i < this->editorWindow->getDocumentCount(); ++i) {
    const Document *doc = this->editorWindow->getDocument(i);
    if (doc && doc->closeButton == button) {
      return i;
    }
  }
  return -1;
}

int DocumentUI::findDocumentIndexByPath(const std::string &path) {
  std::string normalizedPath = DocumentIO::normalize_path(path);
  for (int i = 0; i < this->editorWindow->getDocumentCount(); ++i) {
    const Document *doc = this->editorWindow->getDocument(i);
    if (doc && doc->filePath == normalizedPath) {
      return i;
    }
  }
  return -1;
}
