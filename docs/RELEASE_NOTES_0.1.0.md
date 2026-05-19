# Velnix Editor 0.1.0 Release Notes

Velnix Editor 0.1.0 is the first public release of Velnix Editor, a lightweight
native text editor for Linux developers.

This release establishes the core desktop editing experience: a GTK application
shell, tabbed documents, Scintilla-based editing, syntax highlighting, search
and replace, encoding handling, macro support, preferences, shortcuts, recent
files, session restore, and Debian packaging.

## Highlights

- Native GTK desktop editor with menu bar, tabbed document area, and status bar.
- Multi-document editing with new, open, save, save as, close, and tab
  switching workflows.
- Core editing actions including undo, redo, cut, copy, paste, word wrap, and
  line numbers.
- Find and replace workflows with next, previous, replace, replace all, and
  search result navigation.
- Syntax highlighting through Scintilla and Lexilla, with language menu support
  and extension-based lexer matching.
- Encoding detection and conversion for UTF-8, UTF-8 BOM, UTF-16 LE, UTF-16 BE,
  and ANSI fallback workflows.
- External file state detection for missing, inaccessible, and modified-on-disk
  files, with reload prompts.
- Session restore for previous tabs, unsaved text, cursor, and scroll state.
- Macro recording, playback, save, load, rename, and delete support.
- Preferences, shortcut configuration, recent file persistence, and macro
  persistence.
- Local Debian package generation through `./scripts/build-deb.sh`.

## Downloads

Download the Debian package from this release page:

- `velnix-editor_0.1.0_amd64.deb`

Install it with:

```bash
sudo dpkg -i velnix-editor_0.1.0_amd64.deb
```

If your system reports missing dependencies, repair the installation with:

```bash
sudo apt install -f
```

The package installs:

- the `velnix-editor` executable
- a desktop launcher entry
- bundled project documentation

## Build From Source

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/VelnixEditor/velnix-editor
```

To build a local Debian package:

```bash
./scripts/build-deb.sh
```

The generated package is written to `dist/`.

## Known Limitations

- Very large files or broad search scopes still need additional performance
  validation and tuning.
- Theme customization, drag-and-drop file opening, selected-keyword occurrence
  highlighting, and plugin support are planned for later releases.
- End-user documentation is intentionally minimal in this first release and
  will expand after the release boundary stabilizes.

## License

Velnix Editor is distributed under the GNU General Public License version 3.
See `LICENSE` and `docs/THIRD_PARTY_NOTICES.md` for project and bundled
third-party source notices.
