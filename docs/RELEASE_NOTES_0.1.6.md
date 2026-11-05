# Velnix Editor 0.1.6 Release Notes

Velnix Editor 0.1.6 is a patch release focused on reliable session restoration,
selection statistics, font selection, and clearer search result controls.

## Highlights

- Restoring a session with large unsaved documents no longer crashes because
  of recursive regular-expression parsing. Complete unsaved content is retained.
- Selected character counts appear immediately to the left of the EOL indicator
  in the status bar. Counts use Unicode characters, include multiple selections,
  and disappear when no text is selected. The text follows the interface language.
- Preferences now offer a system font dropdown that opens when clicking the
  text area or arrow, with monospaced filtering and live font/size preview.
- Common programming fonts are prioritized; recommended CJK coding fonts follow
  Latin coding fonts. Missing configured fonts remain visible with a warning.
- A spaced downward chevron beside Search Results indicates that clicking the
  button hides the results panel.

## Downloads

Download the package matching your architecture and the checksum file from
the release page:

- `velnix-editor_0.1.6_amd64.deb`
- `velnix-editor_0.1.6_arm64.deb`
- `SHA256SUMS`

Verify the downloaded packages from the release directory:

```bash
sha256sum -c SHA256SUMS
```

Install the package matching your system architecture:

```bash
sudo dpkg -i velnix-editor_0.1.6_<arch>.deb
```

If dependencies are missing:

```bash
sudo apt install -f
```

The package installs the `velnix-editor` executable, desktop integration, and
project documentation. Fonts are selected from those installed on the system;
this release does not bundle additional fonts.

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
`CHANGELOG.md` through the existing GitHub Actions release workflow.

## Known Limitations

- Very large files and broad searches still need further performance tuning;
  the session parsing fix does not eliminate memory costs for large sessions.
- Font availability and fallback depend on the system's installed fonts.
- Legacy encoding detection can be ambiguous for short files and similar
  single-byte encodings; use **Reopen with Encoding** when needed.
- Pre-release Debug validation on Linux with ICU 74.2 currently fails the short
  GB18030 auto-detection regression assertion. This must be investigated before
  publishing; see `docs/KNOWN_ISSUES.md`.
- Custom keyword highlighting may not immediately refresh after some
  same-length content replacements.
- Update checks require network access. Automatic failures remain silent;
  manual checks report errors.
- Theme customization, plugin support, and a fuller user manual remain planned.

## Changes Since 0.1.5

- Replaced recursive regex-based session field parsing with iterative scanning.
- Added localized Unicode selection character counts to the status bar.
- Added system font selection, filtering, preview, and missing-font feedback.
- Added a downward chevron and spacing to the Search Results toggle.

## License

Velnix Editor is distributed under the GNU General Public License version 3.
See `LICENSE` and `docs/THIRD_PARTY_NOTICES.md` for project and bundled
third-party source notices.
