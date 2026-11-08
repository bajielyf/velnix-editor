# Velnix Editor 0.1.7 Release Notes

Velnix Editor 0.1.7 is a patch release focused on accurate UTF-8 caret
placement, a clearer Find dialog, and smoother navigation and reordering when
many document tabs are open.

## Highlights

- Clicking either half of a CJK character, accented character, or emoji now
  places the caret on the expected character boundary instead of treating the
  character's individual UTF-8 bytes as separate hit-test positions.
- The Find dialog now has a clearer layout with vertically grouped actions and
  a flat bottom status area.
- A localized **Count** action reports matches in the current document while
  respecting case-sensitive, whole-word, and regular-expression options.
- The tab strip can be scrolled independently with arrow controls or the mouse
  wheel without changing the active document.
- Clicking the editor reveals the active tab when it is outside the visible
  tab-strip viewport.
- Tab dragging now shows a translucent preview, faded source placeholder, and
  insertion marker, and safely restores the original order when cancelled.

## Downloads

Download the package matching your architecture and the checksum file from
the release page:

- `velnix-editor_0.1.7_amd64.deb`
- `velnix-editor_0.1.7_arm64.deb`
- `SHA256SUMS`

Verify the downloaded packages from the release directory:

```bash
sha256sum -c SHA256SUMS
```

Install the package matching your system architecture:

```bash
sudo dpkg -i velnix-editor_0.1.7_<arch>.deb
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

## Changes Since 0.1.6

- Corrected UTF-8 character width measurement used for caret hit testing.
- Redesigned the Find dialog and added current-document match counting.
- Added independent tab-strip arrow and mouse-wheel scrolling.
- Added active-tab reveal and visual feedback for tab reordering.
- Added automated GTK tab-strip interaction regression coverage.

## License

Velnix Editor is distributed under the GNU General Public License version 3.
See `LICENSE` and `docs/THIRD_PARTY_NOTICES.md` for project and bundled
third-party source notices.
