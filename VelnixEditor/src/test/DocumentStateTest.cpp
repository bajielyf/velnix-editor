#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <glib.h>
#include <glib/gstdio.h>

#include "editor/DocumentState.h"

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
      remove_write_permissions();
    }
  }

  void make_writable() {
    if (!path.empty()) {
      g_chmod(path.c_str(), 0644);
    }
  }

 private:
  void remove_write_permissions() {
    g_chmod(path.c_str(), 0444);
  }
};

// Test Suite: File State Query
namespace FileStateQueryTests {

void test_query_valid_file() {
  TempFile temp;
  temp.write_content("test content");

  gint64 modifiedTime = 0;
  bool readOnly = false;
  FileState fileState = FileState::Ok;

  bool result = DocumentState::query_file_state(temp.path, modifiedTime, readOnly,
                                                fileState);

  assert(result == true);
  assert(modifiedTime > 0);
  assert(readOnly == false);
  assert(fileState == FileState::Ok);
  std::cout << "[PASS] test_query_valid_file\n";
}

void test_query_missing_file() {
  std::string non_existent = "/tmp/non_existent_file_12345_abcde.txt";
  gint64 modifiedTime = 0;
  bool readOnly = false;
  FileState fileState = FileState::Ok;

  bool result = DocumentState::query_file_state(non_existent, modifiedTime,
                                                readOnly, fileState);

  assert(result == false);
  assert(fileState == FileState::Missing);
  std::cout << "[PASS] test_query_missing_file\n";
}

void test_query_readonly_file() {
  TempFile temp;
  temp.write_content("readonly content");
  temp.make_readonly();

  gint64 modifiedTime = 0;
  bool readOnly = false;
  FileState fileState = FileState::Ok;

  bool result = DocumentState::query_file_state(temp.path, modifiedTime, readOnly,
                                                fileState);

  temp.make_writable();  // Ensure cleanup

  assert(result == true);
  assert(readOnly == true);
  assert(fileState == FileState::Ok);
  std::cout << "[PASS] test_query_readonly_file\n";
}

void test_query_empty_path() {
  std::string empty_path;
  gint64 modifiedTime = 0;
  bool readOnly = false;
  FileState fileState = FileState::Ok;

  bool result = DocumentState::query_file_state(empty_path, modifiedTime,
                                                readOnly, fileState);

  assert(result == false);
  assert(fileState == FileState::Inaccessible);
  std::cout << "[PASS] test_query_empty_path\n";
}

}  // namespace FileStateQueryTests

// Test Suite: Document Status Computation
namespace DocumentStatusComputeTests {

void test_compute_status_untitled() {
  Document doc{};
  doc.filePath = "";
  doc.modified = false;
  doc.externallyModified = false;
  doc.fileState = FileState::Ok;

  DocumentStatus status = DocumentState::compute_document_status(doc);
  assert(status == DocumentStatus::Untitled);
  std::cout << "[PASS] test_compute_status_untitled\n";
}

void test_compute_status_saved() {
  Document doc{};
  doc.filePath = "/tmp/test.txt";
  doc.modified = false;
  doc.externallyModified = false;
  doc.fileState = FileState::Ok;

  DocumentStatus status = DocumentState::compute_document_status(doc);
  assert(status == DocumentStatus::Saved);
  std::cout << "[PASS] test_compute_status_saved\n";
}

void test_compute_status_modified() {
  Document doc{};
  doc.filePath = "/tmp/test.txt";
  doc.modified = true;
  doc.externallyModified = false;
  doc.fileState = FileState::Ok;

  DocumentStatus status = DocumentState::compute_document_status(doc);
  assert(status == DocumentStatus::Modified);
  std::cout << "[PASS] test_compute_status_modified\n";
}

void test_compute_status_externally_modified() {
  Document doc{};
  doc.filePath = "/tmp/test.txt";
  doc.modified = false;
  doc.externallyModified = true;
  doc.fileState = FileState::Ok;

  DocumentStatus status = DocumentState::compute_document_status(doc);
  assert(status == DocumentStatus::ExternallyModified);
  std::cout << "[PASS] test_compute_status_externally_modified\n";
}

void test_compute_status_missing() {
  Document doc{};
  doc.filePath = "/tmp/test.txt";
  doc.modified = false;
  doc.externallyModified = false;
  doc.fileState = FileState::Missing;

  DocumentStatus status = DocumentState::compute_document_status(doc);
  assert(status == DocumentStatus::Missing);
  std::cout << "[PASS] test_compute_status_missing\n";
}

void test_compute_status_inaccessible() {
  Document doc{};
  doc.filePath = "/tmp/test.txt";
  doc.modified = false;
  doc.externallyModified = false;
  doc.fileState = FileState::Inaccessible;

  DocumentStatus status = DocumentState::compute_document_status(doc);
  assert(status == DocumentStatus::Inaccessible);
  std::cout << "[PASS] test_compute_status_inaccessible\n";
}

void test_compute_status_priority_modified_over_external() {
  // Note: This test checks that fileState::Missing and Inaccessible
  // take priority over modified flags
  Document doc{};
  doc.filePath = "/tmp/test.txt";
  doc.modified = true;
  doc.externallyModified = true;
  doc.fileState = FileState::Missing;

  DocumentStatus status = DocumentState::compute_document_status(doc);
  assert(status == DocumentStatus::Missing);
  std::cout << "[PASS] test_compute_status_priority_missing\n";
}

}  // namespace DocumentStatusComputeTests

// Test Suite: File State Change Detection
namespace FileStateChangeTests {

void test_detect_file_deletion() {
  TempFile temp;
  temp.write_content("initial content");

  gint64 modifiedTime1 = 0;
  bool readOnly1 = false;
  FileState fileState1 = FileState::Ok;

  DocumentState::query_file_state(temp.path, modifiedTime1, readOnly1, fileState1);
  assert(fileState1 == FileState::Ok);

  // Manually delete the file
  g_unlink(temp.path.c_str());

  gint64 modifiedTime2 = 0;
  bool readOnly2 = false;
  FileState fileState2 = FileState::Ok;

  DocumentState::query_file_state(temp.path, modifiedTime2, readOnly2, fileState2);
  assert(fileState2 == FileState::Missing);

  std::cout << "[PASS] test_detect_file_deletion\n";
}

void test_detect_permission_change() {
  TempFile temp;
  temp.write_content("content");
  temp.make_writable();

  gint64 modifiedTime1 = 0;
  bool readOnly1 = false;
  FileState fileState1 = FileState::Ok;

  DocumentState::query_file_state(temp.path, modifiedTime1, readOnly1, fileState1);
  assert(readOnly1 == false);

  // Change permissions to readonly
  temp.make_readonly();

  gint64 modifiedTime2 = 0;
  bool readOnly2 = false;
  FileState fileState2 = FileState::Ok;

  DocumentState::query_file_state(temp.path, modifiedTime2, readOnly2, fileState2);
  assert(readOnly2 == true);

  temp.make_writable();  // Cleanup
  std::cout << "[PASS] test_detect_permission_change\n";
}

}  // namespace FileStateChangeTests

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  std::cout << "\n===== DocumentState Test Suite =====\n\n";

  // File State Query Tests
  std::cout << "--- FileState Query Tests ---\n";
  FileStateQueryTests::test_query_valid_file();
  FileStateQueryTests::test_query_missing_file();
  FileStateQueryTests::test_query_readonly_file();
  FileStateQueryTests::test_query_empty_path();

  // Document Status Computation Tests
  std::cout << "\n--- Document Status Computation Tests ---\n";
  DocumentStatusComputeTests::test_compute_status_untitled();
  DocumentStatusComputeTests::test_compute_status_saved();
  DocumentStatusComputeTests::test_compute_status_modified();
  DocumentStatusComputeTests::test_compute_status_externally_modified();
  DocumentStatusComputeTests::test_compute_status_missing();
  DocumentStatusComputeTests::test_compute_status_inaccessible();
  DocumentStatusComputeTests::test_compute_status_priority_modified_over_external();

  // File State Change Detection Tests
  std::cout << "\n--- File State Change Detection Tests ---\n";
  FileStateChangeTests::test_detect_file_deletion();
  FileStateChangeTests::test_detect_permission_change();

  std::cout << "\n===== All Tests Passed =====\n\n";
  return 0;
}
