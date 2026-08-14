# Velnix Editor 0.1.5 Release Notes

Velnix Editor 0.1.5 is a patch release focused on safer legacy-encoding
workflows, quicker document-state changes from the status bar, EOL conversion,
and automated multi-architecture releases.

## Highlights

- Common Windows and East Asian legacy encodings can now be detected
  automatically with ICU-assisted, script-aware validation.
- **Reopen with Encoding** lets you manually choose ANSI, UTF, Windows-125x,
  Windows-874, GB18030, Big5, Shift_JIS, or CP949 when automatic detection is
  not appropriate.
- The status bar now exposes clickable language, EOL, and encoding controls.
- LF, CRLF, and CR conversion is available from the Edit menu, editor context
  menu, and status bar.
- Tagged builds now produce Debian packages for both `amd64` and `arm64` and
  publish them automatically with SHA-256 checksums.
- Fixed-guide visibility now follows its configuration setting consistently.

## Downloads

Download the packages and checksum file from this release page:

- `velnix-editor_0.1.5_amd64.deb`
- `velnix-editor_0.1.5_arm64.deb`
- `SHA256SUMS`

Verify both packages from the release directory with:

```bash
sha256sum -c SHA256SUMS
```

Install the package matching your system architecture with:

```bash
sudo dpkg -i velnix-editor_0.1.5_<arch>.deb
```

If your system reports missing dependencies, repair the installation with:

```bash
sudo apt install -f
```

The package installs the `velnix-editor` executable, desktop integration, and
bundled project documentation.

## Build From Source

Velnix Editor now requires ICU development files in addition to GTK and
libcurl. On Debian or Ubuntu:

```bash
sudo apt install build-essential cmake pkg-config libgtk-3-dev libcurl4-openssl-dev libicu-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/VelnixEditor/velnix-editor [file ...]
```

To build a local Debian package:

```bash
./scripts/build-deb.sh
```

The generated package and `SHA256SUMS` file are written to `dist/`.

## Encoding Notes

Text remains UTF-8 internally while editing. A legacy-encoded document keeps
its detected or manually selected encoding when saved, and saving fails rather
than silently replacing characters that the target encoding cannot represent.
The conversion commands intentionally target UTF-8 and UTF-16; use **Reopen
with Encoding** when the bytes on disk need to be decoded differently.

Legacy encoding detection is heuristic. For short or ambiguous files, verify
the status-bar encoding and use **Reopen with Encoding** if necessary.

## Known Limitations

- Very large files or broad search scopes still need additional performance
  validation and tuning.
- Automatic legacy-encoding detection can be ambiguous for short files and
  similar single-byte encodings.
- Custom keyword highlights are document-local. The current refresh cache may
  miss an immediate refresh after some same-length content replacements.
- Update checks require network access to the project website. Automatic check
  failures remain silent; manual checks report the error.
- Theme customization and plugin support are planned for later releases.
- End-user documentation remains intentionally compact.

## Changes Since 0.1.4

- Added ICU-assisted detection for common legacy encodings.
- Added manual reopen-with-encoding support and regression coverage for
  detection, decoding, lossless saving, and large-file handling.
- Added interactive, localized status-bar language, EOL, and encoding controls.
- Added LF, CRLF, and CR conversion to the Edit and editor context menus.
- Added automated `amd64` and `arm64` GitHub Release packaging.
- Fixed ambiguous Windows-1252 fallback handling.
- Fixed long-line guide visibility so it follows configuration.

## License

Velnix Editor is distributed under the GNU General Public License version 3.
See `LICENSE` and `docs/THIRD_PARTY_NOTICES.md` for project and bundled
third-party source notices.
