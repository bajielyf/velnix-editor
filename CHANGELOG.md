# Changelog

All notable project-level changes intended for published releases should be recorded in this file.

## [0.1.2] - unreleased

### Added

- Added optional command-line file arguments, opening provided files after any restored session tabs.

### Fixed

- Fixed the issue where the shortcut key was invalid when opening the software for the first time after installing the package.

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
