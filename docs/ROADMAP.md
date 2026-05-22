# Velnix Editor Roadmap

## Purpose

This roadmap tracks the current state of Velnix Editor, the remaining work
before the next release, and the likely direction after that release.

Last updated: 2026-05-22

## Current Positioning

Velnix Editor is a native GTK editor built to improve the Linux desktop editing
experience. The current product direction is to provide a fast, flexible, and
capable editor for users who miss the practical efficiency of Notepad++ on
Linux, while keeping Velnix Editor an independent project with its own brand,
package name, executable name, and release process.

## Current Status

- The application builds as a Linux/GTK desktop editor.
- The generated and installed executable is `velnix-editor`.
- The application source is organized around `app / ui / editor / config / core`.
- The build uses `VelnixEditor/`, `scintilla/`, and `lexilla/`; no extra
  historical reference-only source tree is tracked in the repository.
- Debian packaging exists through `scripts/build-deb.sh`.
- README, project overview, installation, testing, encoding, known limitations,
  third-party notices, release checklist, license, and changelog now exist.
- Release preparation is now mainly about package boundary review, release
  validation, release metadata confirmation, and UI/product polish.

## Completed Work

### 1. Project Identity and Release Naming

- Public project name is `Velnix Editor`.
- Runtime configuration path is `~/.config/velnix-editor/`.
- Debian package name defaults to `velnix-editor`.
- Generated and installed binary name is `velnix-editor`.
- Desktop entry launches `velnix-editor %F`.
- Terminal launches support optional file arguments through
  `velnix-editor [file ...]`.
- Public documentation now describes the project as an independent Velnix
  Editor release effort.

### 2. Application Framework and Build

- Top-level CMake includes `scintilla/`, `lexilla/`, and `VelnixEditor/`.
- The main CMake target remains `velnix_editor`, while its output name is
  `velnix-editor`.
- `test_gtk`, `test_document_state`, `test_config_store`, and
  `test_document_operations` are available as build targets.
- Installation rules install the binary, desktop entry, and top-level README.
- A local verification build has confirmed the binary links as `velnix-editor`
  and installs as `bin/velnix-editor`.

### 3. Main Window and Editing Basics

- GTK main window, menu bar, notebook tabs, and base layout are implemented.
- New, open, save, save as, and quit are implemented.
- Multi-document switching, tab closing, and tab reordering are implemented.
- Window titles update dynamically based on the current document state.
- Scintilla is integrated as the editing component.
- Undo, redo, cut, copy, paste, word wrap, line numbers, and basic styling are
  implemented.

### 4. Document Management and File State

- Document creation, open, save, save as, and close are implemented.
- Blank document reuse is implemented.
- Path normalization and duplicate-open prevention by path are implemented.
- Optional command-line file arguments open after any restored session tabs.
- Document modification tracking is implemented.
- Missing files, inaccessible files, and externally modified files are detected.
- File state refresh runs on focus changes and through periodic checks.
- User prompts exist for missing, inaccessible, and externally modified files.
- Documents can be reloaded from disk.
- Session restore reopens previous tabs and restores unsaved text, cursor, and
  scroll state on launch.

### 5. Encoding and EOL Handling

- File encoding, EOL mode, and read-only state handling are implemented.
- UTF-8 BOM, UTF-16 LE BOM, UTF-16 BE BOM, UTF-8 validation, and ANSI fallback
  are documented and implemented.
- ANSI encoding can be selected through `VELNIX_EDITOR_ANSI_ENCODING`, locale
  detection, or the `WINDOWS-1252` fallback.
- Encoding conversion menu supports ANSI, UTF-8, UTF-8 BOM, UTF-16 LE, and
  UTF-16 BE.
- Conversion failures surface user-facing prompts when text cannot be
  represented losslessly.

### 6. Search, Replace, Syntax, and Macros

- Find and replace dialogs are implemented.
- Find next, find previous, find all, replace next, and replace all are
  implemented.
- Search result panel display, grouped interaction, and double-click navigation
  are implemented.
- Find-all result building runs in the background, with cancellation support.
- Search result refresh and auto-refresh controls are implemented.
- Selected-keyword occurrence highlighting is implemented for the active
  document. Normal selection highlighting is controlled by the smart keyword
  highlighting preference, which is disabled by default; double-click word
  selection still highlights matching occurrences.
- Lexilla is integrated for syntax highlighting.
- Lexer list loading, language-menu switching, and extension-based matching are
  implemented.
- Macro recording, playback, save, load, management, rename, and delete are
  implemented.
- Search-related operations can be recorded into macros.

### 7. Configuration and Preferences

- Configuration directory management is implemented.
- Main configuration, shortcut configuration, recent files, and macro files are
  persisted.
- Configuration directory migration logic exists.
- Preferences dialog is implemented.
- Shortcut configuration dialog and basic conflict warning behavior are
  implemented.

### 8. Internal Code Cleanup

- Main project internal naming has moved toward `EditorWindow` terminology.
- Old window-object naming leftovers have been removed from the application
  layer.
- `EditorWindow` and `DocumentManager` core component ownership uses
  `std::unique_ptr`.
- `WindowInitializer` and `EditorWindow` interaction has been narrowed through
  structured component and menu item APIs.
- `DocumentOperations`, `DocumentRefresh`, and `DocumentPrompt` now interact
  with `EditorWindow` through tighter public boundaries.

### 9. Release Documentation and Legal Notes

- Top-level README has been rewritten as the public project entry point.
- `README_PROJECT.md` describes project positioning, build, packaging, feature
  scope, and code structure.
- `docs/INSTALL.md` documents build, run, package, and install steps.
- `docs/TESTING.md` documents available test targets and smoke tests.
- `docs/ENCODING.md` documents encoding detection and conversion behavior.
- `docs/KNOWN_ISSUES.md` records current release limitations.
- `docs/THIRD_PARTY_NOTICES.md` summarizes bundled third-party source notes.
- `docs/RELEASE_CHECKLIST.md` tracks release verification.
- `CHANGELOG.md` exists for release-facing changes.
- `LICENSE` has a Velnix Editor project preamble and keeps the GPLv3 text.

## Release Blockers

### 1. Release Metadata Confirmation

- Top-level `CMakeLists.txt` is the release version source.
- `VelnixEditor/CMakeLists.txt` uses the top-level version during normal
  repository builds and only declares a fallback version for standalone
  subproject builds.
- `scripts/build-deb.sh` reads the top-level project version for package
  metadata.
- Before release, confirm the package metadata, changelog, README, release
  notes, and release tag all match this version.

### 2. Package Boundary Review

- Confirm exactly which files should be included in source archives.
- Confirm exactly which files should be installed by the Debian package.
- Decide whether additional license or third-party documentation should be
  installed under `share/doc/velnix-editor`.
- Confirm Scintilla and Lexilla bundled docs/history files are handled
  intentionally.

### 3. Release Validation

- Run the documented source build.
- Build the explicit test targets.
- Run non-GUI test binaries from `docs/TESTING.md`.
- Build the Debian package through `scripts/build-deb.sh`.
- Install the generated package in a clean or disposable environment.
- Verify desktop entry launch, basic editing, save/reload, search/replace,
  syntax highlighting, encoding conversion, preferences, shortcuts, and recent
  file behavior.
- Verify terminal launch both without arguments and with multiple file
  arguments, including the restored-session-then-arguments tab order.

### 4. Product Polish for Release

- Add or finalize application icon assets.
- Confirm desktop entry metadata and package description.
- Review about dialog text and public website/repository URL.
- Confirm startup behavior and first-run experience.
- Prepare release notes with known limitations.

## Post-Release Priorities

### High Priority

- Consolidate configuration schema definitions to reduce loose coupling between
  UI options and stored fields.
- Add clearer error feedback and logging behavior.

### Medium Priority

- Add drag-and-drop file opening for the main window and tab area.
- Continue improving syntax highlighting configuration. Global syntax
  highlighting toggle and default lexer selection are already implemented; the
  default lexer currently applies only to new documents.
- Continue adding editor behavior settings. The first batch already includes
  whitespace display, EOL markers, current line highlight, indentation guides,
  long-line guide, and indentation behavior.
- Design and implement a theme system for editor colors, UI appearance, and
  user-selectable presets.
- Improve macro system discoverability and usability.
- Expand shortcut conflict detection and shortcut management ergonomics.
- Improve performance in large-file and many-tab scenarios.
- Improve encoding workflows, including status-bar or document-property access
  for encoding display and switching.

### Low Priority

- Improve code consistency across the project, including member access style and
  defensive argument validation in macro recording paths.
- Add help pages or an about/help documentation entry.
- Write more complete end-user documentation.
- Plan a plugin system for future extensibility, including extension loading,
  capability boundaries, configuration, and packaging expectations.

## Milestones

### M1: 0.1.1 Patch Release Cleanup

- Confirm release metadata for version `0.1.1`.
- Confirm package contents and installed files.
- Complete build, test, package, and install validation.
- Finish release notes and known limitations.

### M2: Patch Release, 0.1.1

- Publish package artifact.
- Publish changelog and known issues.
- Tag the release.
- Keep source documentation and package metadata aligned.

### M3: Editing Experience Expansion

- Improve search responsiveness and search result workflows.
- Improve preferences and shortcut configuration experience.
- Add drag-and-drop file opening.
- Add first-pass theme customization.
- Add more complete editing behavior controls.

### M4: Extensibility Foundation

- Define plugin system goals and non-goals.
- Design plugin loading, security boundaries, and compatibility policy.
- Prototype a minimal plugin API after the core editor experience stabilizes.

## Maintenance Guidance

- Update this roadmap when a feature moves between planned, in progress, and
  completed.
- Keep release blockers short, concrete, and testable.
- Keep completed work factual and tied to behavior that exists in the current
  repository.
- Remove obsolete migration goals instead of keeping them as permanent
  placeholders.
