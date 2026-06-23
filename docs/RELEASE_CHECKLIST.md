# Velnix Editor Release Checklist

Use this checklist when preparing a Velnix Editor release.

## 1. Version Information

- [ ] Confirm the public release version is correct
- [ ] Confirm the version is consistent across build, packaging, website, and docs
- [ ] Record the release content in [CHANGELOG.md](../CHANGELOG.md)
- [ ] Prepare release notes under `docs/RELEASE_NOTES_<version>.md`
- [ ] Update `docs/version.txt` to the release version

## 2. Build and Test

- [ ] Build from source and confirm `velnix-editor` is generated
- [ ] Build `test_gtk`
- [ ] Build `test_document_state`, `test_config_store`, and `test_document_operations`
- [ ] Run available non-GUI test targets documented in [TESTING.md](TESTING.md)
- [ ] Run the key manual test or validation flow

## 3. Packaging and Installation

- [ ] Run `./scripts/build-deb.sh`
- [ ] Confirm the `.deb` package is written to `dist/`
- [ ] Install the generated package in a clean environment
- [ ] Verify the executable, desktop entry, and installed documentation paths

## 4. Core Feature Regression

- [ ] New, open, save, and save as
- [ ] Tab switching and tab closing
- [ ] Find, replace, and search result navigation
- [ ] Syntax highlighting switching and extension-based matching
- [ ] Encoding display and encoding conversion
- [ ] Recent files, preferences, and shortcut configuration
- [ ] Session restore
- [ ] Optional command-line file arguments append after restored session tabs
- [ ] Macro recording, playback, and management
- [ ] External file modification detection and reload

## 5. Documentation and Release Materials

- [ ] README reflects the current feature scope and build entry point
- [ ] Installation and packaging instructions can be followed directly
- [ ] Testing documentation matches the available build targets
- [ ] Encoding documentation matches the current implementation
- [ ] Known limitations match the current implementation
- [ ] License and third-party notices are current
- [ ] Roadmap no longer includes obsolete completed work as future work
- [ ] Release page copy includes a short project description, known limitations,
  and download instructions

## 6. Release Boundary Confirmation

- [ ] Confirm the files included in the release package
- [ ] Confirm how third-party dependency notes and license text are handled
- [ ] Confirm only intended application and dependency files are included in the final release package
- [ ] Confirm `README.md` and `LICENSE` are installed or shipped with the package

## 7. Release Follow-Up

- [ ] Prepare the tag for the release version
- [ ] Prepare release announcement or release notes
- [ ] List known issues and follow-up plans for the next version
