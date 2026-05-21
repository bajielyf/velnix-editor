#pragma once

#include "config/ConfigStore.h"
#include "core/EditorTypes.h"
#include "editor/DocumentManager.h"
#include "editor/MacroManager.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

class EventHandler;
class WindowInitializer;
class SettingsManager;
class CoreSettingsManager;
class ShortcutManager;
class RecentFilesManager;
class SearchManager;

class EditorWindow {
public:
    struct WindowComponents {
        GtkWidget *window = nullptr;
        GtkAccelGroup *accelGroup = nullptr;
        GtkWidget *menubar = nullptr;
        GtkWidget *notebook = nullptr;
        GtkWidget *vbox = nullptr;
        GtkWidget *editorPane = nullptr;
        GtkWidget *searchResultsPanel = nullptr;
        GtkWidget *recentMenu = nullptr;
        GtkWidget *wordWrapMenuItem = nullptr;
        GtkWidget *fullScreenMenuItem = nullptr;
        GtkWidget *showWhitespaceMenuItem = nullptr;
        GtkWidget *showEolMarkersMenuItem = nullptr;
        GtkWidget *statusBar = nullptr;
        GtkWidget *statusBarLabel = nullptr;
    };

    struct FileMenuItems {
        GtkWidget *newItem = nullptr;
        GtkWidget *openItem = nullptr;
        GtkWidget *saveItem = nullptr;
        GtkWidget *saveAsItem = nullptr;
        GtkWidget *exitItem = nullptr;
    };

    struct SettingsMenuItems {
        GtkWidget *preferencesItem = nullptr;
        GtkWidget *shortcutConfigItem = nullptr;
    };

    struct DocumentEditorState {
        GtkWidget *scintilla = nullptr;
        bool modified = false;
    };

    struct DocumentFileState {
        std::string path;
        FileState fileState = FileState::Inaccessible;
        bool readOnly = false;
        bool externallyModified = false;
        gint64 lastModifiedTime = 0;
        EolMode eolMode = EolMode::Lf;
        FileEncoding encoding = FileEncoding::Utf8;
        bool useBom = false;

        bool hasPath() const { return !path.empty(); }
    };

    struct ConfigState {
        bool useRecentFiles = true;
        int maxRecentFiles = 5;
        bool showLineNumbers = true;
        std::string uiLanguage = "en";
        std::string lineWrapMode = "off";
        std::string fontFamily = "Monospace";
        int fontSize = 11;
        bool autoSave = false;
        int autoSaveInterval = 60;
        bool overwriteReadonly = false;
        bool searchWrapAround = true;
        bool searchAllDocuments = false;
        std::string lastFileDialogDir;
        std::string configDir;
        std::string defaultLexer = "null";
        bool syntaxHighlightingEnabled = true;
        int tabSize = 4;
        bool useSpacesForTabs = true;
        bool autoIndent = true;
        bool showWhitespace = false;
        bool showEolMarkers = false;
        bool highlightCurrentLine = true;
        bool highlightCurrentColumn = false;
        bool ctrlMouseWheelZoom = true;
        bool smartKeywordHighlighting = false;
        bool trimTrailingWhitespaceOnSave = false;
        bool ensureFinalNewlineOnSave = false;
        bool showIndentGuides = true;
        int longLineColumn = 120;
        bool scrollPastLastLine = true;
        bool highlightMatchingBraces = true;
        bool showRightMarginBackground = false;
    };

    EditorWindow();
    ~EditorWindow();

    void show();
    GtkWidget *get_window();
    void installWindowComponents(const WindowComponents &components);
    void registerFileMenuItems(const FileMenuItems &items);
    void registerSettingsMenuItems(const SettingsMenuItems &items);
    void setWordWrapEnabled(bool enabled, bool persist = true);
    void toggleWordWrap();
    bool isWordWrapEnabled() const { return wordWrapEnabled; }
    void setFullscreenMode(bool enabled);
    void toggleFullscreenMode();
    bool isFullscreenMode() const { return fullscreenMode; }
    void setLineWrapMode(const std::string &mode);
    std::string getLineWrapMode() const { return lineWrapMode; }
    void setEditorFont(const std::string &family, int size);
    const std::string &getEditorFontFamily() const { return fontFamily; }
    int getEditorFontSize() const { return fontSize; }
    void setLineNumbersVisible(bool visible);
    bool areLineNumbersVisible() const { return showLineNumbers; }
    void setWhitespaceMarkersVisible(bool visible, bool persist = true);
    void setEolMarkersVisible(bool visible, bool persist = true);

    void updateAutoSaveTimer();
    bool performAutoSave();
    ConfigState getConfigState() const;
    void applyConfigState(const ConfigState &config);
    void loadConfigFromStore(const ConfigStore &store);
    void saveConfigToStore(ConfigStore &store) const;
    void saveSession() const;

    GtkWidget *getCurrentScintilla() {
        return documentManager ? documentManager->getCurrentScintilla() : nullptr;
    }

    std::string getDocumentText(int index) {
        return documentManager ? documentManager->getDocumentText(index) : "";
    }

    bool saveDocumentAs(int index) {
        return documentManager ? documentManager->saveDocumentAs(index) : false;
    }

    bool saveDocumentToPath(int index, const std::string &path) {
        return documentManager ? documentManager->saveDocumentToPath(index, path)
                               : false;
    }

    bool offer_save_if_modified();

    int getCurrentDocumentIndex() {
        return currentDocumentIndex;
    }

    void setCurrentDocumentIndex(int index) {
        currentDocumentIndex = index;
    }
    void activateDocument(int index, bool syncNotebookPage = true);

    void createNewDocument() {
        if (documentManager) {
            documentManager->createNewDocument();
        }
    }

    bool openDocumentPath(const std::string &path) {
        return open_document_path(path);
    }

    int findDocumentIndexByScintilla(GtkWidget *scintilla) {
        return documentManager ? documentManager->findDocumentIndexByScintilla(scintilla) : -1;
    }

    int findDocumentIndexByCloseButton(GtkWidget *button) {
        return documentManager ? documentManager->findDocumentIndexByCloseButton(button) : -1;
    }

    void closeDocument(int index) {
        if (documentManager) {
            documentManager->closeDocument(index);
        }
    }

    void switchToDocument(int index) {
        if (documentManager) {
            documentManager->switchToDocument(index);
        }
    }

    void refreshDocumentState(int index, bool syncDiskState) {
        if (documentManager) {
            documentManager->refreshDocumentState(index, syncDiskState);
        }
    }

    void refreshDocumentStatesFromWorkspace(bool allowPrompts) {
        if (documentManager) {
            documentManager->refreshDocumentStatesFromWorkspace(allowPrompts);
        }
    }

    void updateDocumentTitle(int index) {
        if (documentManager) {
            documentManager->updateDocumentTitle(index);
        }
    }

    void syncDocumentFileState(int index) {
        if (documentManager) {
            documentManager->syncDocumentFileState(index);
        }
    }

    void acknowledgeFileState(int index) {
        if (documentManager) {
            documentManager->acknowledgeFileState(index);
        }
    }

    bool checkExternalModificationForDocument(int index) {
        return documentManager
                   ? documentManager->checkExternalModificationForDocument(index)
                   : false;
    }

    void applyDocumentEolMode(int index) {
        if (documentManager) {
            documentManager->applyDocumentEolMode(index);
        }
    }

    void markDocumentSaved(int index) {
        if (documentManager) {
            documentManager->markDocumentSaved(index);
        }
    }

    void addRecentFile(const std::string &path);

    bool reloadDocumentFromDisk(int index) {
        return documentManager ? documentManager->reloadDocumentFromDisk(index)
                               : false;
    }
    bool hasDocument(int index) const;
    DocumentEditorState getDocumentEditorState(int index) const;
    DocumentFileState getDocumentFileStateSnapshot(int index) const;
    void applyDocumentFileStateSnapshot(int index,
                                        const DocumentFileState &state);
    void setDocumentModifiedFlag(int index, bool modified);
    void recomputeDocumentStatus(int index);

    void applyCurrentSyntaxHighlighting() {
        if (documentManager) {
            documentManager->applyCurrentSyntaxHighlighting();
            GtkWidget *scintilla = getCurrentScintilla();
            applyLineNumberStyle(scintilla, true);
            applyEditorBehaviorSettings(scintilla);
        }
    }

    void setLexer(const std::string &lexerName) {
        if (documentManager) {
            documentManager->setLexer(lexerName);
            GtkWidget *scintilla = getCurrentScintilla();
            applyLineNumberStyle(scintilla, true);
            applyEditorBehaviorSettings(scintilla);
        }
    }

    void setLexerByFileExtension(const std::string &path) {
        if (documentManager) {
            documentManager->setLexerByFileExtension(path);
            GtkWidget *scintilla = getCurrentScintilla();
            applyLineNumberStyle(scintilla, true);
            applyEditorBehaviorSettings(scintilla);
        }
    }

    const std::vector<LexerDefinition> &getAvailableLexers() const {
        static std::vector<LexerDefinition> empty;
        return documentManager ? documentManager->getAvailableLexers() : empty;
    }

    void applyEditorSettingsToDocument(GtkWidget *scintilla) {
        applyLineNumberStyle(scintilla);
        applyIndentationSettings(scintilla);
        applyEditorBehaviorSettings(scintilla);
        applyColumnEditorSettings(scintilla);
    }

    void setDocumentModified(int index, bool modified) {
        if (documentManager) {
            documentManager->setDocumentModified(index, modified);
        }
    }

    void reorderDocumentsToMatchNotebook() {
        if (documentManager) {
            documentManager->reorderDocumentsToMatchNotebook();
        }
    }

    void showPreferencesDialog();
    void showShortcutConfigurationDialog();

    void update_window_title();
    void update_status_bar();
    void open_file_dialog();
    bool save_file();
    bool reload_current_document();
    bool save_all_documents();
    bool close_all_documents();
    const std::string &getConfigDirectory() const { return configDir; }
    void setConfigDirectory(const std::string &directory) { configDir = directory; }
    const std::string &getConfigFilePath() const { return configFilePath; }
    void setConfigFilePath(const std::string &path) { configFilePath = path; }
    GtkWidget *getRecentMenu() const { return recentMenu; }
    const std::vector<std::string> &getRecentFiles() const { return recentFiles; }
    void clearRecentFiles() { recentFiles.clear(); }
    void appendRecentFileEntry(const std::string &path) { recentFiles.push_back(path); }
    void setRecentFiles(std::vector<std::string> paths) { recentFiles = std::move(paths); }
    bool isRecentFilesEnabled() const { return useRecentFiles; }
    int getMaxRecentFiles() const { return maxRecentFiles; }
    bool isSearchWrapAroundEnabled() const { return searchWrapAround; }
    bool isSearchAllDocumentsEnabled() const { return searchAllDocuments; }
    GtkWidget *getNotebook() const { return notebook; }
    int getNotebookPageCount() const;
    GtkWidget *getNotebookPageAt(int index) const;
    void appendNotebookPage(GtkWidget *page, GtkWidget *tabLabel);
    void removeNotebookPage(int index);
    void setNotebookPageReorderable(GtkWidget *page, bool reorderable);
    void setCurrentNotebookPage(int index);
    void showNotebook();
    void queueNotebookDraw();
    GtkWidget *getEditorPane() const { return editorPane; }
    GtkWidget *getSearchResultsPanel() const { return searchResultsPanel; }
    GtkWidget *getMenuBarWidget() const { return menubar; }
    GtkWidget *getVBoxWidget() const { return vbox; }
    GtkWindow *getDialogParentWindow() const { return GTK_WINDOW(window); }
    GtkAccelGroup *getAccelGroup() const { return accelGroup; }
    int getDocumentCount() const { return static_cast<int>(documents.size()); }
    Document *getDocument(int index);
    const Document *getDocument(int index) const;
    const std::vector<Document> &getDocuments() const { return documents; }
    int addDocument(Document document);
    void removeDocument(int index);
    void replaceDocuments(std::vector<Document> reorderedDocuments);
    int allocateNextBufferId() { return nextBufferId++; }
    const std::string &getDefaultLexerName() const { return defaultLexer; }
    const std::string &getLastFileDialogDir() const { return lastFileDialogDir; }
    void setLastFileDialogDir(const std::string &directory) { lastFileDialogDir = directory; }
    bool shouldOverwriteReadonly() const { return overwriteReadonly; }
    bool shouldTrimTrailingWhitespaceOnSave() const { return trimTrailingWhitespaceOnSave; }
    bool shouldEnsureFinalNewlineOnSave() const { return ensureFinalNewlineOnSave; }
    int getTabSize() const { return tabSize; }
    bool isCtrlMouseWheelZoomEnabled() const { return ctrlMouseWheelZoom; }
    bool isColumnEditorEnabled() const { return columnEditorEnabled; }
    void setColumnEditorEnabled(bool enabled);
    void beginColumnEditorDrag(GtkWidget *scintilla, int x, int y);
    void updateColumnEditorDrag(GtkWidget *scintilla, int x, int y);
    void endColumnEditorDrag(GtkWidget *scintilla, int x, int y);
    bool isColumnEditorDragging() const { return columnEditorDragging; }
    void convertSelectionToUppercase();
    void convertSelectionToLowercase();
    void convertSelectionToTitleCase();
    void trimTrailingWhitespace();
    void convertSpacesToTabs();
    void convertTabsToSpaces();
    bool convertCurrentDocumentEncoding(FileEncoding encoding, bool useBom);
    void zoomIn();
    void zoomOut();
    void resetZoom();
    std::vector<ShortcutBinding> &getShortcutBindings() { return shortcutBindings; }
    const std::vector<ShortcutBinding> &getShortcutBindings() const {
        return shortcutBindings;
    }
    void refreshLineNumberStyle(GtkWidget *scintilla, bool force = false) {
        applyLineNumberStyle(scintilla, force);
    }
    void refreshBraceHighlighting(GtkWidget *scintilla = nullptr) { updateBraceHighlighting(scintilla); }
    void refreshCurrentColumnHighlight(GtkWidget *scintilla = nullptr) {
        updateCurrentColumnHighlight(scintilla);
    }
    void handleSelectedKeywordDoubleClick(int index);
    void refreshDynamicHighlights(int index, int updateFlags);
    void refreshSearchResultsAfterModification();
    void handleMacroScintillaNotification(SCNotification *notification);
    void recordMacroSearchAction(MacroAppCommand command,
                                 const std::string &findPattern,
                                 const std::string &replaceText,
                                 bool caseSensitive,
                                 bool wholeWord,
                                 bool regex);
    void addMacroMenuItems(GtkWidget *macroMenu);
    void startMacroRecording();
    void stopMacroRecording();
    void showMacroPlaybackDialog();
    void promptAndSaveCurrentMacro();
    void showMacroManagementDialog();
    void saveMacros();
    void reloadMacros();
    std::string getMacrosFilePath() const;
    bool executeMacroSearchCommand(MacroAppCommand command,
                                   const std::string &findPattern,
                                   const std::string &replaceText,
                                   bool caseSensitive,
                                   bool wholeWord,
                                   bool regex);
    void showFindDialog();
    void showReplaceDialog();
    void showGoToLineDialog();
    void toggleSearchResultsPanel();
    void createSearchResultsPanel(GtkWidget *parent);
    void applyShortcutBindings(std::vector<ShortcutBinding> bindings);

private:
    void applyLineNumberStyle(GtkWidget *scintilla, bool force = false);
    void applyIndentationSettings(GtkWidget *scintilla = nullptr);
    void applyEditorBehaviorSettings(GtkWidget *scintilla = nullptr);
    void applyColumnEditorSettings(GtkWidget *scintilla = nullptr);
    bool restoreSession();
    void updateBraceHighlighting(GtkWidget *scintilla = nullptr);
    void updateCurrentColumnHighlight(GtkWidget *scintilla = nullptr);
    void updateSelectedKeywordHighlight(GtkWidget *scintilla = nullptr);
    void updateBraceHighlightingForDocument(Document &doc, bool force,
                                            sptr_t currentPos = INVALID_POSITION);
    void updateCurrentColumnHighlightForDocument(Document &doc, bool force,
                                                 sptr_t currentPos = INVALID_POSITION);
    void updateSelectedKeywordHighlightForDocument(Document &doc, bool force,
                                                   bool allowWhenSmartDisabled = false);
    std::unique_ptr<EventHandler> eventHandler;
    std::unique_ptr<WindowInitializer> windowInitializer;
    std::unique_ptr<DocumentManager> documentManager;
    std::unique_ptr<SettingsManager> settingsManager;
    std::unique_ptr<SearchManager> searchManager;
    std::unique_ptr<MacroManager> macroManager;

    GtkWidget *window = nullptr;
    GtkWidget *notebook = nullptr;
    GtkWidget *menubar = nullptr;
    GtkWidget *vbox = nullptr;
    GtkWidget *editorPane = nullptr;
    GtkAccelGroup *accelGroup = nullptr;
    GtkWidget *recentMenu = nullptr;
    GtkWidget *newMenuItem = nullptr;
    GtkWidget *openMenuItem = nullptr;
    GtkWidget *saveMenuItem = nullptr;
    GtkWidget *saveAsMenuItem = nullptr;
    GtkWidget *exitMenuItem = nullptr;
    GtkWidget *undoMenuItem = nullptr;
    GtkWidget *redoMenuItem = nullptr;
    GtkWidget *cutMenuItem = nullptr;
    GtkWidget *copyMenuItem = nullptr;
    GtkWidget *pasteMenuItem = nullptr;
    GtkWidget *findMenuItem = nullptr;
    GtkWidget *replaceMenuItem = nullptr;
    GtkWidget *wordWrapMenuItem = nullptr;
    GtkWidget *fullScreenMenuItem = nullptr;
    GtkWidget *showWhitespaceMenuItem = nullptr;
    GtkWidget *showEolMarkersMenuItem = nullptr;
    GtkWidget *statusBar = nullptr;
    GtkWidget *statusBarLabel = nullptr;
    GtkWidget *preferencesMenuItem = nullptr;
    GtkWidget *shortcutConfigMenuItem = nullptr;
    GtkWidget *searchResultsPanel = nullptr;

    std::vector<std::string> recentFiles;
    std::string configFilePath;
    std::string configDir;
    std::string shortcutsFilePath;
    std::string lastFileDialogDir;
    std::string uiLanguage = "en";
    bool useRecentFiles = true;
    int maxRecentFiles = 5;
    std::string lineWrapMode = "off";
    std::string fontFamily = "Monospace";
    int fontSize = 11;
    bool autoSave = false;
    int autoSaveInterval = 60;
    bool overwriteReadonly = false;
    bool searchWrapAround = true;
    bool searchAllDocuments = false;
    std::string defaultLexer = "null";
    bool syntaxHighlightingEnabled = true;
    int tabSize = 4;
    bool useSpacesForTabs = true;
    bool autoIndent = true;
    bool showWhitespace = false;
    bool showEolMarkers = false;
    bool highlightCurrentLine = true;
    bool showIndentGuides = true;
    int longLineColumn = 120;
    bool scrollPastLastLine = true;
    bool highlightMatchingBraces = true;
    bool showRightMarginBackground = false;
    bool highlightCurrentColumn = false;
    bool ctrlMouseWheelZoom = true;
    bool smartKeywordHighlighting = false;
    bool trimTrailingWhitespaceOnSave = false;
    bool ensureFinalNewlineOnSave = false;
    bool columnEditorEnabled = false;
    bool columnEditorDragging = false;
    bool updatingViewSymbolMenuItems = false;
    sptr_t columnEditorAnchor = 0;
    std::vector<Document> documents;
    int currentDocumentIndex = -1;
    int nextBufferId = 1;
    guint fileStateRefreshTimerId = 0;
    guint autoSaveTimerId = 0;
    std::vector<ShortcutBinding> shortcutBindings;
    bool wordWrapEnabled = false;
    bool fullscreenMode = false;
    bool updatingFullscreenMenuItem = false;
    bool showLineNumbers = true;

    bool open_document_path(const std::string &path);
};
