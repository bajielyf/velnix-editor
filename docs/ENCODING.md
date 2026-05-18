# Velnix Editor Encoding Detection and Conversion

This document describes how Velnix Editor detects file encodings when opening
files, how ANSI fallback is selected, and how encoding information is exposed
through the UI.

## Opening Files: Detection Order

The current implementation lives in
`VelnixEditor/src/editor/DocumentIO.cpp` in
`DocumentIO::decode_file_content()`. The strategy is: prefer BOM markers,
validate UTF-8 when no BOM is present, and fall back to ANSI when validation
fails.

- **UTF-8 BOM**: if the file starts with `EF BB BF`, decode as **UTF-8** and
  record `useBom=true`
- **UTF-16 LE BOM**: if the file starts with `FF FE`, decode as **UTF-16LE**
  after skipping the BOM and record `useBom=true`
- **UTF-16 BE BOM**: if the file starts with `FE FF`, decode as **UTF-16BE**
  after skipping the BOM and record `useBom=true`
- **UTF-8 validation**: if the full file passes `g_utf8_validate()`, treat it
  as **UTF-8** with `useBom=false`
- **ANSI fallback**: if none of the above applies, convert the file from the
  selected ANSI encoding to UTF-8 text

## ANSI Fallback: Encoding Name Selection

When ANSI fallback is needed, or when a file is saved as ANSI, Velnix Editor
selects an ANSI encoding name for `g_convert()`:

- if **`VELNIX_EDITOR_ANSI_ENCODING`** is set and non-empty, use it directly
- otherwise, read the system locale character set; if it is available and not
  UTF-8, use that character set
- if no clear character set is available, default to `WINDOWS-1252`

## Saving Files: Output Rules

The current implementation lives in `DocumentIO::encode_document_text()`:

- **UTF-8**: write UTF-8 bytes directly; if `useBom=true`, prepend `EF BB BF`
- **UTF-16LE/UTF-16BE**: convert UTF-8 text to the target UTF-16 encoding, then
  prepend the matching BOM when `useBom=true` (`FF FE` for LE, `FE FF` for BE)
- **ANSI**: convert UTF-8 text to the selected ANSI encoding; conversion fails
  if the text cannot be represented losslessly in that target encoding

## UI: Display and Conversion Entry Points

- **Display**: the bottom status bar shows the current document `Encoding`,
  `EOL`, and read-only state when applicable
- **Conversion**: the **Encoding** menu provides "Convert to ..." actions for:
  - ANSI
  - UTF-8
  - UTF-8 BOM
  - UTF-16 LE
  - UTF-16 BE

Note: current UTF-16 conversion menu actions write BOMs by default so the byte
order can be detected reliably when the file is opened later.
