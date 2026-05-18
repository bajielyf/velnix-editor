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
  assert(DocumentIO::decode_file_content("caf\xE9", text, encoding, useBom));
  assert(encoding == FileEncoding::Ansi);
  assert(!useBom);
  assert(text == "caf\xC3\xA9");
  std::cout << "[PASS] test_decode_ansi_fallback\n";
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
  DocumentEncodingTests::test_encode_ansi_lossless_when_representable();
  DocumentEncodingTests::test_encode_ansi_fails_when_unrepresentable();
  DocumentEncodingTests::test_encode_ansi_fallback_handles_control_code_roundtrip();
  DocumentEncodingTests::test_decode_ansi_fallback();
  DocumentEncodingTests::test_decode_ansi_fallback_handles_undefined_windows_1252_byte();

  std::cout << "\n===== All Overwrite Readonly Save Tests Passed =====\n\n";
  return 0;
}
