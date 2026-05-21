# Velnix Editor Third-Party Notices

This document summarizes project-level license and dependency notes for Velnix
Editor releases. It is not a replacement for the license
headers and license files inside third-party source directories; those notices
must remain with the relevant source.

## Project License

Velnix Editor is distributed under the GNU General Public License version 3.
See the repository [LICENSE](../LICENSE) file for the full license text.

## Bundled Source Components

The repository includes source from these major components:

- `scintilla/`: editing component used by the application
- `lexilla/`: lexer component used for syntax highlighting

The current Linux application build depends on `scintilla/` and `lexilla/`;
the rest of the application source lives under `VelnixEditor/`.

## Notice Preservation

When distributing source or binary packages:

- preserve copyright notices in source files
- preserve applicable third-party license files and documentation
- preserve the repository `LICENSE`
- include or reference this notice summary when practical

## Release Review Items

Before publishing a release, review the release package contents and confirm:

- which third-party files are included in source archives
- which files are installed into binary packages
- whether additional dependency license files should be installed under the
  package documentation directory
- whether the release notes should mention bundled Scintilla and Lexilla source
