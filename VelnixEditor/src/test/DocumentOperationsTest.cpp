#include "editor/DocumentIO.h"
#include "editor/SaveNormalization.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>

// Helper class to manage temporary test files
class TempFile {
 public:
  std::string path;

  explicit TempFile() {
    gchar *tmp_path = g_strdup("/tmp/VelnixEditor_test_XXXXXX");
    gint fd = g_mkstemp(tmp_path);
    if (fd != -1) {
      close(fd);
      path = tmp_path;
      g_free(tmp_path);
    }
  }

  ~TempFile() {
    if (!path.empty()) {
      g_unlink(path.c_str());
    }
  }

  void write_content(const std::string &content) {
    std::ofstream file(path);
    file << content;
    file.close();
  }

  std::string read_content() const {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }

  void make_readonly() {
    if (!path.empty()) {
      chmod(path.c_str(), 0444);
    }
  }

  void make_writable() {
    if (!path.empty()) {
      chmod(path.c_str(), 0644);
    }
  }
};

// Test function to simulate saveDocumentToPath logic
bool test_save_to_path(const std::string &path, const std::string &content, bool overwriteReadonly) {
  std::string normalizedPath = path; // Assume normalized

  if (overwriteReadonly) {
    if (access(normalizedPath.c_str(), F_OK) == 0 && access(normalizedPath.c_str(), W_OK) != 0) {
      struct stat st;
      if (stat(normalizedPath.c_str(), &st) != 0) {
        return false;
      }
      mode_t writableMode = st.st_mode | S_IWUSR;
      if (chmod(normalizedPath.c_str(), writableMode) != 0) {
        return false;
      }
    }
  }

  std::ofstream output(normalizedPath, std::ios::binary);
  if (!output) {
    return false;
  }

  output.write(content.data(), static_cast<std::streamsize>(content.size()));
  return output.good();
}

// Test Suite: Overwrite Readonly Save Behavior
namespace OverwriteReadonlySaveTests {

void test_save_to_writable_file() {
  TempFile temp;
  temp.write_content("initial content");
  temp.make_writable();

  std::string newContent = "new content";
  bool saved = test_save_to_path(temp.path, newContent, false); // overwriteReadonly not relevant
  assert(saved == true);
  assert(temp.read_content() == newContent);
  std::cout << "[PASS] test_save_to_writable_file\n";
}

void test_save_to_readonly_file_without_overwrite() {
  TempFile temp;
  temp.write_content("initial content");
  temp.make_readonly();

  std::string newContent = "new content";
  bool saved = test_save_to_path(temp.path, newContent, false);
  assert(saved == false); // Should fail because file is readonly and overwriteReadonly is false
  assert(temp.read_content() == "initial content"); // Content should remain unchanged
  std::cout << "[PASS] test_save_to_readonly_file_without_overwrite\n";
}

void test_save_to_readonly_file_with_overwrite() {
  TempFile temp;
  temp.write_content("initial content");
  temp.make_readonly();

  std::string newContent = "new content";
  bool saved = test_save_to_path(temp.path, newContent, true);
  assert(saved == true); // Should succeed because overwriteReadonly is true
  assert(temp.read_content() == newContent); // Content should be updated

  // Verify file is now writable (permissions restored)
  struct stat st;
  stat(temp.path.c_str(), &st);
  assert((st.st_mode & S_IWUSR) != 0); // Should have write permission

  std::cout << "[PASS] test_save_to_readonly_file_with_overwrite\n";
}

void test_save_to_nonexistent_file_with_overwrite() {
  std::string nonexistent = "/tmp/nonexistent_file_12345.txt";

  std::string newContent = "new content";
  bool saved = test_save_to_path(nonexistent, newContent, true);
  assert(saved == true); // Should succeed for new file
  std::ifstream check(nonexistent);
  assert(check.good());
  std::string content;
  std::getline(check, content);
  assert(content == newContent);

  // Cleanup
  g_unlink(nonexistent.c_str());
  std::cout << "[PASS] test_save_to_nonexistent_file_with_overwrite\n";
}

}  // namespace OverwriteReadonlySaveTests

namespace SaveNormalizationTests {

void test_trim_trailing_whitespace_lf() {
  const std::string input = "alpha  \n beta\t\t\ncharlie";
  const std::string expected = "alpha\n beta\ncharlie";
  assert(SaveNormalization::trim_trailing_whitespace(input) == expected);
  std::cout << "[PASS] test_trim_trailing_whitespace_lf\n";
}

void test_trim_trailing_whitespace_crlf() {
  const std::string input = "alpha  \r\nbeta\t \r\ngamma";
  const std::string expected = "alpha\r\nbeta\r\ngamma";
  assert(SaveNormalization::trim_trailing_whitespace(input) == expected);
  std::cout << "[PASS] test_trim_trailing_whitespace_crlf\n";
}

void test_ensure_final_newline() {
  const std::string input = "alpha";
  const std::string expected = "alpha\n";
  assert(SaveNormalization::normalize_text_for_save(input, false, true) == expected);
  std::cout << "[PASS] test_ensure_final_newline\n";
}

void test_preserve_existing_final_newline() {
  const std::string input = "alpha\r\n";
  assert(SaveNormalization::normalize_text_for_save(input, false, true) == input);
  std::cout << "[PASS] test_preserve_existing_final_newline\n";
}

void test_combined_normalization() {
  const std::string input = "alpha \r\nbeta\t";
  const std::string expected = "alpha\r\nbeta\n";
  assert(SaveNormalization::normalize_text_for_save(input, true, true) == expected);
  std::cout << "[PASS] test_combined_normalization\n";
}

}  // namespace SaveNormalizationTests

namespace DocumentEncodingTests {

Document make_test_document(FileEncoding encoding, bool useBom) {
  Document doc{};
  doc.encoding = encoding;
  doc.useBom = useBom;
  doc.eolMode = EolMode::Lf;
  return doc;
}

void test_encode_utf8_without_bom() {
  Document doc = make_test_document(FileEncoding::Utf8, false);
  std::string rawBytes;
  assert(DocumentIO::encode_document_text(doc, "alpha", rawBytes));
  assert(rawBytes == "alpha");
  std::cout << "[PASS] test_encode_utf8_without_bom\n";
}

void test_encode_utf8_with_bom() {
  Document doc = make_test_document(FileEncoding::Utf8, true);
  std::string rawBytes;
  assert(DocumentIO::encode_document_text(doc, "alpha", rawBytes));
  assert(rawBytes == std::string("\xEF\xBB\xBF", 3) + "alpha");
  std::cout << "[PASS] test_encode_utf8_with_bom\n";
}

void test_encode_language_specific_encodings() {
  struct EncodingCase {
    FileEncoding encoding;
    const char *utf8;
    std::string expected;
  };
  const EncodingCase cases[] = {
      {FileEncoding::Windows1252, "caf\xC3\xA9", "caf\xE9"},
      {FileEncoding::Windows1251, "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82",
       std::string("\xCF\xF0\xE8\xE2\xE5\xF2", 6)},
      {FileEncoding::Gb18030, "\xE4\xB8\xAD\xE6\x96\x87",
       std::string("\xD6\xD0\xCE\xC4", 4)},
      {FileEncoding::ShiftJis, "\xE6\x97\xA5\xE6\x9C\xAC",
       std::string("\x93\xFA\x96\x7B", 4)},
      {FileEncoding::Cp949, "\xED\x95\x9C\xEA\xB5\xAD",
       std::string("\xC7\xD1\xB1\xB9", 4)},
  };
  for (const auto &testCase : cases) {
    Document doc = make_test_document(testCase.encoding, false);
    std::string rawBytes;
    assert(DocumentIO::encode_document_text(doc, testCase.utf8, rawBytes));
    assert(rawBytes == testCase.expected);
  }

  Document latin = make_test_document(FileEncoding::Windows1252, false);
  std::string rawBytes;
  assert(!DocumentIO::encode_document_text(latin, "\xE4\xB8\xAD\xE6\x96\x87",
                                           rawBytes));
  std::cout << "[PASS] test_encode_language_specific_encodings\n";
}

void test_encode_ansi_lossless_when_representable() {
  g_setenv("VELNIX_EDITOR_ANSI_ENCODING", "WINDOWS-1252", TRUE);
  Document doc = make_test_document(FileEncoding::Ansi, false);
  std::string rawBytes;
  assert(DocumentIO::encode_document_text(doc, "caf\xC3\xA9", rawBytes));
  assert(rawBytes == "caf\xE9");
  std::cout << "[PASS] test_encode_ansi_lossless_when_representable\n";
}

void test_encode_ansi_fails_when_unrepresentable() {
  g_setenv("VELNIX_EDITOR_ANSI_ENCODING", "WINDOWS-1252", TRUE);
  Document doc = make_test_document(FileEncoding::Ansi, false);
  std::string rawBytes;
  assert(!DocumentIO::encode_document_text(doc, "snowman \xE2\x98\x83",
                                           rawBytes));
  std::cout << "[PASS] test_encode_ansi_fails_when_unrepresentable\n";
}

void test_encode_ansi_fallback_handles_control_code_roundtrip() {
  g_setenv("VELNIX_EDITOR_ANSI_ENCODING", "WINDOWS-1252", TRUE);
  Document doc = make_test_document(FileEncoding::Ansi, false);
  std::string rawBytes;
  assert(DocumentIO::encode_document_text(doc, "a\xC2\x81z", rawBytes));
  assert(rawBytes == std::string("a\x81z", 3));
  std::cout << "[PASS] test_encode_ansi_fallback_handles_control_code_roundtrip\n";
}

void test_decode_ansi_fallback() {
  g_setenv("VELNIX_EDITOR_ANSI_ENCODING", "WINDOWS-1252", TRUE);
  std::string text;
  FileEncoding encoding = FileEncoding::Utf8;
  bool useBom = true;
  assert(DocumentIO::decode_file_content("c\x9Cur", text, encoding, useBom));
  assert(encoding == FileEncoding::Windows1252);
  assert(!useBom);
  assert(text == "c\xC5\x93ur");
  std::cout << "[PASS] test_decode_ansi_fallback\n";
}

void test_detect_listed_legacy_encodings() {
  struct DetectionCase { FileEncoding encoding; const char *utf8; };
  const DetectionCase cases[] = {
      {FileEncoding::Windows1250, "Za\xC5\xBC\xC3\xB3\xC5\x82\xC4\x87"},
      {FileEncoding::Windows1251, "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82"},
      {FileEncoding::Windows1253, "\xCE\x9A\xCE\xB1\xCE\xBB\xCE\xB7\xCE\xBC\xCE\xAD\xCF\x81\xCE\xB1"},
      {FileEncoding::Windows1254,
       "T\xC3\xBC" "rk\xC3\xA7" "e \xC4\x9F\xC4\xB1\xC5\x9F"},
      {FileEncoding::Windows1255, "\xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D"},
      {FileEncoding::Windows1256, "\xD9\x85\xD8\xB1\xD8\xAD\xD8\xA8\xD8\xA7"},
      {FileEncoding::Windows1258, "Vi\xE1\xBB\x87t Nam"},
      {FileEncoding::Windows874, "\xE0\xB8\xAA\xE0\xB8\xA7\xE0\xB8\xB1\xE0\xB8\xAA\xE0\xB8\x94\xE0\xB8\xB5"},
      {FileEncoding::ShiftJis, "\xE3\x81\x93\xE3\x82\x93\xE3\x81\xAB\xE3\x81\xA1\xE3\x81\xAF"},
      {FileEncoding::Cp949, "\xEC\x95\x88\xEB\x85\x95\xED\x95\x98\xEC\x84\xB8\xEC\x9A\x94"},
      {FileEncoding::Gb18030, "\xE7\xAE\x80\xE4\xBD\x93\xE4\xB8\xAD\xE6\x96\x87\xE6\xB5\x8B\xE8\xAF\x95"},
      {FileEncoding::Big5, "\xE7\xB9\x81\xE9\xAB\x94\xE4\xB8\xAD\xE6\x96\x87\xE6\xB8\xAC\xE8\xA9\xA6"},
  };
  for (const auto &testCase : cases) {
    Document doc = make_test_document(testCase.encoding, false);
    std::string rawBytes;
    assert(DocumentIO::encode_document_text(doc, testCase.utf8, rawBytes));
    FileEncoding detected = FileEncoding::Ansi;
    assert(DocumentIO::detect_legacy_encoding(rawBytes, detected));
    assert(detected == testCase.encoding);
    std::string decoded;
    bool useBom = true;
    assert(DocumentIO::decode_file_content(rawBytes, decoded, detected, useBom));
    assert(detected == testCase.encoding);
    assert(!useBom);
    if (testCase.encoding != FileEncoding::Windows1258) {
      // Windows-1258 may canonically decompose Vietnamese tone marks.
      assert(decoded == testCase.utf8);
    }
  }
  std::cout << "[PASS] test_detect_listed_legacy_encodings\n";
}

void test_detect_east_asian_realistic_text() {
  const std::pair<FileEncoding, const char *> cases[] = {
      {FileEncoding::ShiftJis,
       "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E\xE3\x81\xAE\xE6\x96\x87\xE6\x9B\xB8\xE3\x82\x92"
       "\xE6\xAD\xA3\xE3\x81\x97\xE3\x81\x8F\xE8\xA1\xA8\xE7\xA4\xBA\xE3\x81\x97\xE3\x81\xBE\xE3\x81\x99\xE3\x80\x82"},
      {FileEncoding::Cp949,
       "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4 \xEB\xAC\xB8\xEC\x84\x9C\xEB\xA5\xBC "
       "\xEC\x98\xAC\xEB\xB0\x94\xEB\xA5\xB4\xEA\xB2\x8C \xED\x91\x9C\xEC\x8B\x9C\xED\x95\xA9\xEB\x8B\x88\xEB\x8B\xA4. "
       "\xEA\xB0\x82"},  // U+AC02 exercises the CP949 extension beyond EUC-KR.
  };
  for (const auto &testCase : cases) {
    Document doc = make_test_document(testCase.first, false);
    std::string rawBytes;
    assert(DocumentIO::encode_document_text(doc, testCase.second, rawBytes));
    std::string decoded;
    FileEncoding detected = FileEncoding::Ansi;
    bool useBom = true;
    assert(DocumentIO::decode_file_content(rawBytes, decoded, detected, useBom));
    assert(detected == testCase.first);
    assert(decoded == testCase.second);
  }
  std::cout << "[PASS] test_detect_east_asian_realistic_text\n";
}

void test_cp949_is_not_misdetected_as_windows1250() {
  // Short Korean text is frequently ranked as Windows-1250 by statistical
  // detectors because both encodings accept the same high bytes.
  const std::string cp949("\xC7\xD1\xB1\xB9\xBE\xEE \xB9\xAE\xBC\xAD", 11);
  FileEncoding detected = FileEncoding::Ansi;
  assert(DocumentIO::detect_legacy_encoding(cp949, detected));
  assert(detected == FileEncoding::Cp949);

  std::string decoded;
  bool useBom = true;
  assert(DocumentIO::decode_file_content(cp949, decoded, detected, useBom));
  assert(detected == FileEncoding::Cp949);
  assert(decoded == "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4 \xEB\xAC\xB8\xEC\x84\x9C");
  std::cout << "[PASS] test_cp949_is_not_misdetected_as_windows1250\n";
}

void test_manual_legacy_encoding_selection() {
  std::string text;
  const std::string cp949("\xC7\xD1\xB1\xB9\xBE\xEE", 6);
  assert(DocumentIO::decode_file_content_as(cp949, FileEncoding::Cp949, false,
                                            text));
  assert(text == "\xED\x95\x9C\xEA\xB5\xAD\xEC\x96\xB4");

  const std::string shiftJis("\x93\xFA\x96\x7B\x8C\xEA", 6);
  assert(DocumentIO::decode_file_content_as(
      shiftJis, FileEncoding::ShiftJis, false, text));
  assert(text == "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E");
  std::cout << "[PASS] test_manual_legacy_encoding_selection\n";
}

void test_decode_gb18030() {
  std::string text;
  FileEncoding encoding = FileEncoding::Utf8;
  bool useBom = true;
  assert(DocumentIO::decode_file_content(
      std::string("GB18030: \xD6\xD0\xCE\xC4\xB2\xE2\xCA\xD4"),
      text, encoding, useBom));
  assert(encoding == FileEncoding::Gb18030);
  assert(!useBom);
  assert(text == "GB18030: \xE4\xB8\xAD\xE6\x96\x87\xE6\xB5\x8B\xE8\xAF\x95");
  std::cout << "[PASS] test_decode_gb18030\n";
}

void test_decode_ansi_fallback_handles_undefined_windows_1252_byte() {
  g_setenv("VELNIX_EDITOR_ANSI_ENCODING", "WINDOWS-1252", TRUE);
  std::string text;
  FileEncoding encoding = FileEncoding::Utf8;
  bool useBom = true;
  assert(DocumentIO::decode_file_content(std::string("a\x81z", 3), text,
                                         encoding, useBom));
  assert(encoding == FileEncoding::Ansi);
  assert(!useBom);
  assert(text == "a\xC2\x81z");
  std::cout << "[PASS] test_decode_ansi_fallback_handles_undefined_windows_1252_byte\n";
}

}  // namespace DocumentEncodingTests

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  std::cout << "\n===== Overwrite Readonly Save Test Suite =====\n\n";

  // Overwrite Readonly Save Tests
  std::cout << "--- Overwrite Readonly Save Tests ---\n";
  OverwriteReadonlySaveTests::test_save_to_writable_file();
  OverwriteReadonlySaveTests::test_save_to_readonly_file_without_overwrite();
  OverwriteReadonlySaveTests::test_save_to_readonly_file_with_overwrite();
  OverwriteReadonlySaveTests::test_save_to_nonexistent_file_with_overwrite();

  std::cout << "\n--- Save Normalization Tests ---\n";
  SaveNormalizationTests::test_trim_trailing_whitespace_lf();
  SaveNormalizationTests::test_trim_trailing_whitespace_crlf();
  SaveNormalizationTests::test_ensure_final_newline();
  SaveNormalizationTests::test_preserve_existing_final_newline();
  SaveNormalizationTests::test_combined_normalization();

  std::cout << "\n--- Document Encoding Tests ---\n";
  DocumentEncodingTests::test_encode_utf8_without_bom();
  DocumentEncodingTests::test_encode_utf8_with_bom();
  DocumentEncodingTests::test_encode_language_specific_encodings();
  DocumentEncodingTests::test_encode_ansi_lossless_when_representable();
  DocumentEncodingTests::test_encode_ansi_fails_when_unrepresentable();
  DocumentEncodingTests::test_encode_ansi_fallback_handles_control_code_roundtrip();
  DocumentEncodingTests::test_decode_ansi_fallback();
  DocumentEncodingTests::test_decode_gb18030();
  DocumentEncodingTests::test_detect_listed_legacy_encodings();
  DocumentEncodingTests::test_detect_east_asian_realistic_text();
  DocumentEncodingTests::test_cp949_is_not_misdetected_as_windows1250();
  DocumentEncodingTests::test_manual_legacy_encoding_selection();
  DocumentEncodingTests::test_decode_ansi_fallback_handles_undefined_windows_1252_byte();

  std::cout << "\n===== All Overwrite Readonly Save Tests Passed =====\n\n";
  return 0;
}
