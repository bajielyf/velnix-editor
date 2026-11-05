# Changelog

All notable project-level changes intended for published releases should be recorded in this file.

## [0.1.6] - 2026-11-05

Patch release focused on session restore reliability, selection statistics,
font preferences, and clearer search result controls.

### Added

- Added localized selection character counts to the status bar, immediately
  before the EOL indicator, counting Unicode characters across multiple
  selections and hiding the count when nothing is selected.
- Added a system font dropdown with monospaced filtering, live font and size
  preview, and a warning when the configured font is not installed.

### Changed

- Prioritized common programming fonts in the font selector, listing the
  recommended CJK coding fonts after Latin coding fonts.
- Added a spaced downward chevron to the Search Results toggle to indicate
  that clicking it hides the results panel.

### Fixed

- Fixed startup crashes when restoring large unsaved documents from
  `session.json` by replacing recursive regular-expression matching with
  iterative parsing while preserving the full document content.

## [0.1.5] - 2026-08-14

Patch release focused on legacy-encoding workflows, interactive document-state
controls, EOL conversion, and automated multi-architecture releases.

### Added

- Added automatic detection for common Windows, Central and Eastern European,
  Middle Eastern, Thai, Chinese, Japanese, and Korean legacy encodings.
- Added **Reopen with Encoding** for manually decoding a document with UTF or
  supported legacy encodings.
- Added lossless decoding, encoding, and round-trip regression coverage,
  including large legacy-encoded files.
- Added EOL conversion to the Edit menu, editor context menu, and status bar.
- Added interactive status-bar controls for language, EOL mode, and encoding.
- Added automated `amd64` and `arm64` Debian builds and tagged GitHub Releases
  with checksums and release notes generated from this changelog.

### Changed

- Localized status-bar document information and refined its layout.
- Limited encoding conversion targets to UTF-8 and UTF-16; legacy encodings
  remain available for automatic detection and **Reopen with Encoding**, and
  an opened document is saved in its current encoding when representable.
- Added ICU as a build and runtime dependency for legacy-encoding detection.

### Fixed

- Aligned fixed long-line guide visibility with its configuration setting.
- Preferred ICU's result when heuristic evidence is too weak to distinguish
  overlapping legacy encodings.
- Kept the Windows-1252 fallback regression check active in Release builds.
- Included `CHANGELOG.md` in the automated GitHub Release assets as documented.

## [0.1.4] - 2026-07-23

Patch release focused on update discovery, faster access to common editing
actions, and refreshed application branding.

### Added

- Added asynchronous update checks against the published website version
  metadata.
- Added a daily startup update check and a manual **Check for Updates** action
  in the Help menu.
- Added undo, redo, cut, copy, paste, column editing, case conversion, and
  whitespace operations to the editor context menu.
- Added context-sensitive availability for editor context-menu actions based
  on selection, clipboard, read-only, and undo/redo state.

### Changed

- Refreshed the application logo asset.

## [0.1.3] - 2026-06-22

Patch release focused on search and replace usability, per-document custom
keyword highlighting, and dialog placement.

### Added

- Added backwards search controls to the find and replace dialogs.
- Added per-document custom keyword highlighting with six selectable colors.
- Added View-menu and editor context-menu actions to apply highlights and clear
  them by color or all at once.

### Changed

- Changed the find and replace dialogs to non-modal windows so editing can
  continue while they remain open.
- Centered find, replace, preferences, and keyboard shortcut dialogs relative
  to the main editor window when shown.

### Fixed

- Corrected the purple custom-highlight color value.

## [0.1.2] - 2026-05-27

Patch release focused on file-opening workflows, macro playback reliability,
and Debian package release artifacts.

### Added

- Added optional command-line file arguments, opening provided files after any restored session tabs.
- Added drag-and-drop file opening for the main window, editor area, and tab labels.
- Added `SHA256SUMS` generation alongside Debian package output.

### Fixed

- Fixed the issue where the shortcut key was invalid when opening the software for the first time after installing the package.
- Fixed macro playback omissions for recorded search result actions.
- Fixed Debian package maintainer metadata.

## [0.1.1] - 2026-05-22

Patch release focused on selected-keyword occurrence highlighting.

### Added

- Added selected-keyword occurrence highlighting for the active document.
- Double-click word selection highlights matching occurrences immediately.
- Normal text selection can highlight matching occurrences when smart keyword
  highlighting is enabled.

## [0.1.0] - 2026-05-18

Initial public release of Velnix Editor.

### Added

- Native GTK desktop editor shell with menu bar, tabbed document area, and status bar.
- Multi-document text editing with new, open, save, save as, close, and tab switching.
- Basic editing actions: undo, redo, cut, copy, paste, word wrap, and line numbers.
- Find and replace workflows, including search result navigation.
- Syntax highlighting through Scintilla and Lexilla, with language menu support and extension-based lexer matching.
- Encoding detection and conversion for UTF-8, UTF-8 BOM, UTF-16 LE, UTF-16 BE, and ANSI fallback workflows.
- External file state detection for missing, inaccessible, and modified-on-disk files, with reload prompts.
- Macro recording, playback, save, load, rename, and delete support.
- Preferences, shortcut configuration, recent file persistence, and macro persistence.
- Debian packaging script that builds `velnix-editor` packages into `dist/`.
- Desktop entry for launching Velnix Editor from Linux desktop environments.
- Release documentation set covering installation, testing, encoding behavior, known limitations, third-party notices, and release checks.
- Project screenshot and user-facing README for first-time users.

### Release Setup

- Public executable name standardized as `velnix-editor`.
- Project version set to `0.1.0` for the first public release line.
- Top-level CMake project version is now the release version source.
- License preamble prepared for the independent Velnix Editor project while keeping the GNU GPLv3 text.
- Project documentation organized so `README.md` is user-facing and `README_PROJECT.md` is developer-facing.
- Product positioning clarified around a fast, practical, native Linux desktop editing experience.

### Known Limitations

- Search is still primarily synchronous and may need optimization for large files or broad search scopes.
- Theme customization, drag-and-drop file opening, selected-keyword occurrence highlighting, and plugin support are planned for later releases.
- First-release package contents and installed documentation should be reviewed before publishing the final artifact.
- End-user documentation is intentionally minimal in this first release and will expand after the release boundary stabilizes.
