# Velnix Editor Project Overview

## Project Positioning

Velnix Editor is a native GTK text editor project built to improve the Linux
desktop editing experience. The project exists because Linux users still lack a
widely adopted editor that combines speed, flexibility, practical power, and the
kind of everyday efficiency many people miss from Notepad++.

The repository currently focuses on preparing the GTK application as an
independently distributable editor under the Velnix Editor brand.

## Current Goal

The current work is focused on preparing the first standalone release:

- stabilize the build and packaging entry points
- complete release-oriented documentation
- clarify the current feature boundary and known limitations
- establish a changelog and repeatable release process for future versions

## Build

Build requirements:

- CMake 3.16+
- a C++17-capable compiler
- GTK 3 development files

```bash
cmake -S . -B build
cmake --build build -j4
```

Main executable:

```bash
./build/VelnixEditor/velnix-editor
```

## Packaging

Build a local Debian package:

```bash
./scripts/build-deb.sh
```

See [docs/INSTALL.md](docs/INSTALL.md) for installation and packaging details.

## Current Feature Scope

- GTK main window, tabbed editing, and basic file operations
- find and replace with search result navigation
- Lexilla syntax highlighting with extension-based lexer matching
- encoding detection, encoding conversion, and EOL status display
- macro recording, playback, and persistence
- preferences, shortcut management, and recent-file storage
- external file change detection and reload prompts

## Code Structure

- `VelnixEditor/src/app/`: application entry point
- `VelnixEditor/src/ui/`: main window, event handling, and UI assembly
- `VelnixEditor/src/editor/`: documents, search, macros, and syntax highlighting
- `VelnixEditor/src/config/`: settings, shortcuts, and recent files
- `VelnixEditor/src/core/`: shared editor data types

## Notes

- `scintilla/` and `lexilla/` provide the editor engine and lexer support and
  are the bundled third-party source components used by the Linux application.
- Test targets and validation steps are documented in [docs/TESTING.md](docs/TESTING.md).
- Encoding behavior is documented in [docs/ENCODING.md](docs/ENCODING.md).
- Current limitations are documented in [docs/KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md).
- License and dependency notes are summarized in [docs/THIRD_PARTY_NOTICES.md](docs/THIRD_PARTY_NOTICES.md).
- The first-release checklist is documented in [docs/RELEASE_CHECKLIST.md](docs/RELEASE_CHECKLIST.md).
- Development status and future work are tracked in [docs/ROADMAP.md](docs/ROADMAP.md).
