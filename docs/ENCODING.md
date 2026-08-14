# Velnix Editor Encoding Detection and Conversion

This document describes how Velnix Editor detects file encodings when opening
files, how a document can be reopened with an explicit encoding, and how
encoding information is exposed through the UI.

## Opening Files: Detection Order

The current implementation lives in
`VelnixEditor/src/editor/DocumentIO.cpp` in
`DocumentIO::decode_file_content()`. The strategy is: prefer BOM markers,
validate UTF-8 when no BOM is present, try ICU-assisted legacy-encoding
detection, and finally fall back to the configured ANSI encoding.

- **UTF-8 BOM**: if the file starts with `EF BB BF`, decode as **UTF-8** and
  record `useBom=true`
- **UTF-16 LE BOM**: if the file starts with `FF FE`, decode as **UTF-16LE**
  after skipping the BOM and record `useBom=true`
- **UTF-16 BE BOM**: if the file starts with `FE FF`, decode as **UTF-16BE**
  after skipping the BOM and record `useBom=true`
- **UTF-8 validation**: if the full file passes `g_utf8_validate()`, treat it
  as **UTF-8** with `useBom=false`
- **Legacy encoding detection**: if UTF-8 validation fails, use ICU statistical
  detection plus script-aware validation to identify supported Windows and East
  Asian legacy encodings
- **ANSI fallback**: if no supported legacy encoding can be selected reliably,
  convert the file from the configured ANSI encoding to UTF-8 text

Supported legacy encodings are Windows-1250/1251/1252/1253/1254/1255/1256/1258,
Windows-874, GB18030, Big5, Shift_JIS, and CP949. Automatic detection is
heuristic, especially for short files and similar single-byte encodings.

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
- **Language-specific legacy encodings**: the Encoding menu also offers
  Windows-1250/1251/1252/1253/1254/1255/1256/1258, Windows-874, GB18030,
  Big5, Shift_JIS, and CP949. Text remains UTF-8 inside the editor and is
  converted losslessly when saved; unsupported characters cause the conversion
  to fail instead of being replaced silently.

## UI: Display and Conversion Entry Points

- **Display**: the bottom status bar shows the current document `Encoding`,
  `EOL`, language, and read-only state when applicable. The encoding control is
  clickable for writable documents.
- **Conversion**: the **Encoding** menu provides "Convert to ..." actions for:
  - UTF-8
  - UTF-8 BOM
  - UTF-16 LE
  - UTF-16 BE
- **Manual decoding**: **Encoding > Reopen with Encoding** and the encoding
  status control can reopen the current file using ANSI, UTF-8, UTF-16, or any
  supported legacy encoding. Unsaved documents cannot be reopened from disk.

An opened legacy document retains its selected encoding when saved. Conversion
targets are limited to UTF-8 and UTF-16; saving to the current legacy encoding
remains lossless and fails if the edited text cannot be represented.

Note: current UTF-16 conversion menu actions write BOMs by default so the byte
order can be detected reliably when the file is opened later.
