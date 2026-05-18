# Velnix Editor First Release Checklist

Use this checklist when preparing the first standalone Velnix Editor release.

## 1. Version Information

- [Y] Confirm the public release version is `0.1.0`
- [Y] Confirm the version is consistent across build, packaging, and docs
- [Y] Record the release content in [CHANGELOG.md](../CHANGELOG.md)

## 2. Build and Test

- [Y] Build from source and confirm `velnix-editor` is generated
- [Y] Build `test_gtk`
- [Y] Build `test_document_state`, `test_config_store`, and `test_document_operations`
- [Y] Run available non-GUI test targets documented in [TESTING.md](TESTING.md)
- [Y] Run the key manual test or validation flow

## 3. Packaging and Installation

- [Y] Run `./scripts/build-deb.sh`
- [Y] Confirm the `.deb` package is written to `dist/`
- [Y] Install the generated package in a clean environment
- [Y] Verify the executable, desktop entry, and installed documentation paths

## 4. Core Feature Regression

- [Y] New, open, save, and save as
- [Y] Tab switching and tab closing
- [Y] Find, replace, and search result navigation
- [Y] Syntax highlighting switching and extension-based matching
- [Y] Encoding display and encoding conversion
- [Y] Recent files, preferences, and shortcut configuration
- [Y] External file modification detection and reload

## 5. Documentation and Release Materials

- [Y] README reflects the current feature scope and build entry point
- [Y] Installation and packaging instructions can be followed directly
- [Y] Testing documentation matches the available build targets
- [Y] Encoding documentation matches the current implementation
- [Y] Known limitations match the current implementation
- [Y] License and third-party notices are current
- [Y] Roadmap no longer includes obsolete migration-only content
- [Y] Release page copy includes a short project description, known limitations,
  and download instructions

## 6. Release Boundary Confirmation

- [Y] Confirm the files included in the release package
- [Y] Confirm how third-party dependency notes and license text are handled
- [Y] Confirm only intended application and dependency files are included in the final release package
- [Y] Confirm `README.md` and `LICENSE` are installed or shipped with the package

## 7. First-Release Follow-Up

- [Y] Prepare the tag for the release version
- [Y] Prepare release announcement or release notes
- [Y] List known issues and follow-up plans for the next version
