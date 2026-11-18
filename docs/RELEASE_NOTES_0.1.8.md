# Velnix Editor 0.1.8 Release Notes

Velnix Editor 0.1.8 is a patch release focused on more flexible custom
highlighting and small interaction improvements in the Find dialog and editor
scrollbar.

## Highlights

- Custom highlighting now accepts arbitrary non-empty selections, including
  phrases with spaces, punctuation, multiline text, and selections longer than
  128 bytes.
- Manually highlighted text is matched literally without whole-word
  restrictions. Automatic keyword highlighting keeps its existing behavior.
- The editor's vertical scrollbar expands when the pointer moves over it and
  stays expanded while it is being dragged.
- The Find dialog's status area now stays flush with the bottom edge of the
  window without an unused GTK action area below it.

## Downloads

Download the package matching your architecture and the checksum file from
the release page:

- `velnix-editor_0.1.8_amd64.deb`
- `velnix-editor_0.1.8_arm64.deb`
- `SHA256SUMS`

Verify the downloaded packages from the release directory:

```bash
sha256sum -c SHA256SUMS
```

Install the package matching your system architecture:

```bash
sudo dpkg -i velnix-editor_0.1.8_<arch>.deb
```

If dependencies are missing:

```bash
sudo apt install -f
```

## Build From Source

On Debian or Ubuntu:

```bash
sudo apt install build-essential cmake pkg-config libgtk-3-dev libcurl4-openssl-dev libicu-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
./build/VelnixEditor/velnix-editor [file ...]
```

To build a local Debian package and checksum file in `dist/`:

```bash
./scripts/build-deb.sh
```

Tagged builds publish `amd64` and `arm64` packages, `SHA256SUMS`, and
`CHANGELOG.md` through the GitHub Actions release workflow.

## Known Limitations

- Very large files and broad multi-document searches still need further
  performance tuning.
- Font availability and fallback depend on fonts installed on the system.
- Legacy encoding detection can be ambiguous for short files and similar
  single-byte encodings; use **Reopen with Encoding** when needed.
- Debug validation on Linux with ICU 74.2 still fails the short GB18030
  auto-detection regression assertion documented in `docs/KNOWN_ISSUES.md`.
- Custom keyword highlighting may not immediately refresh after some
  same-length content replacements.
- Update checks require network access. Automatic failures remain silent;
  manual checks report errors.
- Theme customization, plugin support, and a fuller user manual remain planned.

## Changes Since 0.1.7

- Allowed arbitrary selected strings in custom highlighting actions.
- Preserved full selections through editor context-menu highlight actions.
- Expanded the editor vertical scrollbar on hover and while dragging.
- Anchored the Find dialog status area to the bottom edge.

## License

Velnix Editor is distributed under the GNU General Public License version 3.
See `LICENSE` and `docs/THIRD_PARTY_NOTICES.md` for project and bundled
third-party source notices.
