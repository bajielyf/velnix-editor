#include "ui/EditorWindow.h"
#include "ui/EventHandler.h"
#include "ui/Localization.h"
#include "ui/WindowInitializer.h"
#include "editor/DocumentManager.h"
#include "editor/DocumentIO.h"
#include "editor/SearchManager.h"
#include "editor/MacroManager.h"
#include "config/SettingsManager.h"
#include "config/RecentFilesManager.h"
#include "config/ShortcutManager.h"
#include "config/ConfigPaths.h"
#include "editor/DocumentState.h"
#include "core/Logger.h"
#include "core/UpdateChecker.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>

namespace {

constexpr long kLineNumberForeground = 0x9B9B9B;
constexpr long kLineNumberBackground = 0xF7F7F7;
constexpr int kMinimumLineDigits = 3;
constexpr int kLineNumberPadding = 12;
constexpr int kMinRecentFiles = 1;
constexpr int kMaxRecentFiles = 20;
constexpr int kMinFontSize = 8;
constexpr int kMaxFontSize = 72;
constexpr int kMinAutoSaveInterval = 10;
constexpr int kMaxAutoSaveInterval = 300;
constexpr int kMinLongLineColumn = 0;
constexpr int kMaxLongLineColumn = 240;
constexpr guint kStartupUpdateDelaySeconds = 2;
constexpr long kCaretLineBackground = 0xFFF6D8;
constexpr long kIndentGuideForeground = 0xD0D0D0;
constexpr long kEdgeColumnColor = 0xE7E7E7;
constexpr int kCaretLineAlpha = 80;
constexpr long kBraceHighlightForeground = 0x2B6CB0;
constexpr long kBraceHighlightBackground = 0xDCEEFF;
constexpr long kBraceBadForeground = 0xC53030;
constexpr long kBraceBadBackground = 0xFFE3E3;
constexpr int kSelectedKeywordIndicator = 8;
constexpr long kSelectedKeywordIndicatorColor = 0x80FF00;
constexpr int kSelectedKeywordIndicatorAlpha = 72;
constexpr sptr_t kMaxSelectedKeywordLength = 128;
constexpr int kCustomKeywordIndicatorBase = 16;
constexpr int kCustomKeywordIndicatorCount = 6;
constexpr int kCustomKeywordIndicatorAlpha = 96;
constexpr long kCustomKeywordIndicatorColors[kCustomKeywordIndicatorCount] = {
    0x00FFFF,
    0x8FEA8F,
    0xFFD07A,
    0xD78CFF,
    0x66B8FF,
    0x8e4898,
};
constexpr const char *kCustomKeywordColorKeys[kCustomKeywordIndicatorCount] = {
    "menu.highlight_yellow",
    "menu.highlight_green",
    "menu.highlight_blue",
    "menu.highlight_pink",
    "menu.highlight_orange",
    "menu.highlight_purple",
};

enum class CaseTransform { Uppercase, Lowercase, TitleCase };

enum class WhitespaceTransform {
  TrimTrailingWhitespace,
  SpacesToTabs,
  TabsToSpaces
};

const char *encoding_display_name(FileEncoding encoding, bool useBom) {
  if (encoding == FileEncoding::Ansi) {
    return "ANSI";
  }
  if (encoding == FileEncoding::Utf8) {
    return useBom ? "UTF-8 BOM" : "UTF-8";
  }
  if (encoding == FileEncoding::Utf16LE) {
    return useBom ? "UTF-16 LE BOM" : "UTF-16 LE";
  }
  return useBom ? "UTF-16 BE BOM" : "UTF-16 BE";
}

bool is_brace_character(char ch) {
  switch (ch) {
    case '(':
    case ')':
    case '[':
    case ']':
    case '{':
    case '}':
      return true;
    default:
      return false;
  }
}

bool is_ascii_token_character(char ch) {
  const unsigned char uch = static_cast<unsigned char>(ch);
  return std::isalnum(uch) || ch == '_';
}

bool is_plain_ascii_token(const std::string &text) {
  if (text.empty()) {
    return false;
  }
  return std::all_of(text.begin(), text.end(), is_ascii_token_character);
}

bool is_highlightable_selected_keyword(const std::string &text) {
  if (text.empty() ||
      static_cast<sptr_t>(text.size()) > kMaxSelectedKeywordLength) {
    return false;
  }

  return std::none_of(text.begin(), text.end(), [](char ch) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    return std::isspace(uch) || ch == '\0';
  });
}

GdkRGBA scintilla_color_to_rgba(long color) {
  return GdkRGBA{
      static_cast<double>(color & 0xff) / 255.0,
      static_cast<double>((color >> 8) & 0xff) / 255.0,
      static_cast<double>((color >> 16) & 0xff) / 255.0,
      1.0,
  };
}

gboolean on_highlight_color_swatch_draw(GtkWidget *widget, cairo_t *cr,
                                        gpointer data) {
  (void)data;
  const long color = GPOINTER_TO_INT(
      g_object_get_data(G_OBJECT(widget), "highlight-color"));
  const GdkRGBA rgba = scintilla_color_to_rgba(color);
  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);

  cairo_set_source_rgba(cr, rgba.red, rgba.green, rgba.blue, rgba.alpha);
  cairo_rectangle(cr, 1.5, 1.5, allocation.width - 3.0,
                  allocation.height - 3.0);
  cairo_fill_preserve(cr);
  cairo_set_source_rgba(cr, 0.16, 0.16, 0.16, 0.5);
  cairo_set_line_width(cr, 1.0);
  cairo_stroke(cr);
  return FALSE;
}

GtkWidget *create_highlight_color_menu_item(const char *labelKey,
                                            long color) {
  GtkWidget *item = gtk_menu_item_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *swatch = gtk_drawing_area_new();
  GtkWidget *label = gtk_label_new(Localization::text(labelKey));

  gtk_widget_set_size_request(swatch, 20, 14);
  gtk_label_set_xalign(GTK_LABEL(label), 0.0f);
  gtk_container_set_border_width(GTK_CONTAINER(box), 4);
  gtk_box_pack_start(GTK_BOX(box), swatch, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(item), box);

  g_object_set_data(G_OBJECT(swatch), "highlight-color",
                    GINT_TO_POINTER(static_cast<int>(color)));
  g_signal_connect(swatch, "draw",
                   G_CALLBACK(on_highlight_color_swatch_draw), nullptr);
  Localization::bind_widget_text(label, labelKey);
  return item;
}

void append_custom_highlight_menu_items(GtkWidget *menu,
                                        EditorWindow *editorWindow,
                                        const std::string &selectedText = {},
                                        int documentIndex = -1) {
  for (int i = 0; i < kCustomKeywordIndicatorCount; ++i) {
    GtkWidget *colorItem = create_highlight_color_menu_item(
        kCustomKeywordColorKeys[i], kCustomKeywordIndicatorColors[i]);
    g_object_set_data(G_OBJECT(colorItem), "highlight-color-index",
                      GINT_TO_POINTER(i));
    g_object_set_data(G_OBJECT(colorItem), "highlight-document-index",
                      GINT_TO_POINTER(documentIndex));
    if (!selectedText.empty()) {
      g_object_set_data_full(G_OBJECT(colorItem), "highlight-text",
                             g_strdup(selectedText.c_str()), g_free);
    }
    g_signal_connect(colorItem, "activate",
                     G_CALLBACK(EventHandler::on_custom_highlight_color_activate),
                     editorWindow);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), colorItem);
  }
}

void append_clear_custom_highlight_menu_items(GtkWidget *menu,
                                             EditorWindow *editorWindow,
                                             int documentIndex = -1) {
  for (int i = 0; i < kCustomKeywordIndicatorCount; ++i) {
    GtkWidget *colorItem = create_highlight_color_menu_item(
        kCustomKeywordColorKeys[i], kCustomKeywordIndicatorColors[i]);
    g_object_set_data(G_OBJECT(colorItem), "highlight-color-index",
                      GINT_TO_POINTER(i));
    g_object_set_data(G_OBJECT(colorItem), "highlight-document-index",
                      GINT_TO_POINTER(documentIndex));
    g_signal_connect(
        colorItem, "activate",
        G_CALLBACK(EventHandler::on_clear_custom_highlight_color_activate),
        editorWindow);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), colorItem);
  }

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
  GtkWidget *clearItem =
      gtk_menu_item_new_with_label(Localization::text("menu.clear_highlights"));
  Localization::bind_widget_text(clearItem, "menu.clear_highlights");
  g_object_set_data(G_OBJECT(clearItem), "highlight-document-index",
                    GINT_TO_POINTER(documentIndex));
  g_signal_connect(clearItem, "activate",
                   G_CALLBACK(EventHandler::on_clear_custom_highlights_activate),
                   editorWindow);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), clearItem);
}

std::string get_single_selection_text(GtkWidget *scintilla) {
  if (!scintilla) {
    return {};
  }

  const sptr_t selectionCount =
      scintilla_send_message(SCINTILLA(scintilla), SCI_GETSELECTIONS, 0, 0);
  if (selectionCount != 1) {
    return {};
  }

  sptr_t selectionStart = scintilla_send_message(
      SCINTILLA(scintilla), SCI_GETSELECTIONSTART, 0, 0);
  sptr_t selectionEnd = scintilla_send_message(
      SCINTILLA(scintilla), SCI_GETSELECTIONEND, 0, 0);
  if (selectionStart > selectionEnd) {
    std::swap(selectionStart, selectionEnd);
  }

  const sptr_t selectionLength = selectionEnd - selectionStart;
  if (selectionLength <= 0 || selectionLength > kMaxSelectedKeywordLength) {
    return {};
  }

  std::vector<char> buffer(static_cast<size_t>(selectionLength) + 1, '\0');
  scintilla_send_message(SCINTILLA(scintilla), SCI_GETSELTEXT, 0,
                         reinterpret_cast<sptr_t>(buffer.data()));
  return std::string(buffer.data(), static_cast<size_t>(selectionLength));
}

std::string get_word_at_position(GtkWidget *scintilla, sptr_t position) {
  if (!scintilla || position == INVALID_POSITION) {
    return {};
  }

  const sptr_t textLength =
      scintilla_send_message(SCINTILLA(scintilla), SCI_GETTEXTLENGTH, 0, 0);
  if (textLength <= 0) {
    return {};
  }

  position = std::max<sptr_t>(0, std::min(position, textLength - 1));
  const sptr_t start =
      scintilla_send_message(SCINTILLA(scintilla), SCI_WORDSTARTPOSITION,
                             position, TRUE);
  const sptr_t end =
      scintilla_send_message(SCINTILLA(scintilla), SCI_WORDENDPOSITION,
                             position, TRUE);
  const sptr_t length = end - start;
  if (length <= 0 || length > kMaxSelectedKeywordLength) {
    return {};
  }

  std::vector<char> buffer(static_cast<size_t>(length) + 1, '\0');
  Sci_TextRangeFull range{{start, end}, buffer.data()};
  scintilla_send_message(SCINTILLA(scintilla), SCI_GETTEXTRANGEFULL, 0,
                         reinterpret_cast<sptr_t>(&range));
  return std::string(buffer.data(), static_cast<size_t>(length));
}

int clamp_int(int value, int minValue, int maxValue) {
  return std::max(minValue, std::min(value, maxValue));
}

int calculate_line_number_margin_width(GtkWidget *scintilla, int digits) {
  const std::string sampleText(digits, '9');
  const int textWidth = static_cast<int>(scintilla_send_message(
      SCINTILLA(scintilla), SCI_TEXTWIDTH, STYLE_LINENUMBER,
      reinterpret_cast<sptr_t>(sampleText.c_str())));
  return textWidth + kLineNumberPadding;
}

int calculate_line_number_digits(GtkWidget *scintilla) {
  const int lineCount = static_cast<int>(
      scintilla_send_message(SCINTILLA(scintilla), SCI_GETLINECOUNT, 0, 0));
  const int digits = std::max(
      kMinimumLineDigits,
      static_cast<int>(std::to_string(std::max(1, lineCount)).size()));
  return digits;
}

char ascii_upper(char ch) {
  return static_cast<char>(
      std::toupper(static_cast<unsigned char>(ch)));
}

char ascii_lower(char ch) {
  return static_cast<char>(
      std::tolower(static_cast<unsigned char>(ch)));
}

std::string transform_case_text(const std::string &text,
                                CaseTransform transform) {
  std::string result = text;
  bool atWordStart = true;

  for (char &ch : result) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    switch (transform) {
      case CaseTransform::Uppercase:
        ch = ascii_upper(ch);
        break;
      case CaseTransform::Lowercase:
        ch = ascii_lower(ch);
        break;
      case CaseTransform::TitleCase:
        if (std::isalnum(uch)) {
          ch = atWordStart ? ascii_upper(ch) : ascii_lower(ch);
          atWordStart = false;
        } else {
          atWordStart = true;
        }
        break;
    }
  }

  return result;
}

std::string trim_trailing_whitespace_text(const std::string &text) {
  std::string result;
  result.reserve(text.size());

  size_t lineStart = 0;
  while (lineStart < text.size()) {
    size_t lineEnd = lineStart;
    while (lineEnd < text.size() && text[lineEnd] != '\n' &&
           text[lineEnd] != '\r') {
      ++lineEnd;
    }

    size_t contentEnd = lineEnd;
    while (contentEnd > lineStart &&
           (text[contentEnd - 1] == ' ' || text[contentEnd - 1] == '\t')) {
      --contentEnd;
    }
    result.append(text, lineStart, contentEnd - lineStart);

    if (lineEnd < text.size()) {
      if (text[lineEnd] == '\r' && lineEnd + 1 < text.size() &&
          text[lineEnd + 1] == '\n') {
        result.append("\r\n");
        lineStart = lineEnd + 2;
      } else {
        result.push_back(text[lineEnd]);
        lineStart = lineEnd + 1;
      }
    } else {
      lineStart = lineEnd;
    }
  }

  return result;
}

std::string spaces_to_tabs_text(const std::string &text, int tabWidth) {
  if (tabWidth <= 1) {
    return text;
  }

  std::string result;
  result.reserve(text.size());

  size_t i = 0;
  while (i < text.size()) {
    if (text[i] != ' ') {
      result.push_back(text[i++]);
      continue;
    }

    size_t runEnd = i;
    while (runEnd < text.size() && text[runEnd] == ' ') {
      ++runEnd;
    }

    const size_t runLength = runEnd - i;
    const size_t tabs = runLength / static_cast<size_t>(tabWidth);
    const size_t spaces = runLength % static_cast<size_t>(tabWidth);
    result.append(tabs, '\t');
    result.append(spaces, ' ');
    i = runEnd;
  }

  return result;
}

std::string tabs_to_spaces_text(const std::string &text, int tabWidth) {
  if (tabWidth <= 0) {
    return text;
  }

  std::string result;
  result.reserve(text.size());
  const std::string spaces(static_cast<size_t>(tabWidth), ' ');
  for (char ch : text) {
    if (ch == '\t') {
      result += spaces;
    } else {
      result.push_back(ch);
    }
  }
  return result;
}

void transform_selection_case(EditorWindow *editorWindow,
                              CaseTransform transform) {
  if (!editorWindow) {
    return;
  }

  GtkWidget *scintilla = editorWindow->getCurrentScintilla();
  if (!scintilla) {
    return;
  }

  const std::string documentText =
      editorWindow->getDocumentText(editorWindow->getCurrentDocumentIndex());
  struct SelectionRange {
    sptr_t start;
    sptr_t end;
  };

  std::vector<SelectionRange> ranges;
  const sptr_t selectionCount =
      scintilla_send_message(SCINTILLA(scintilla), SCI_GETSELECTIONS, 0, 0);
  for (sptr_t i = 0; i < selectionCount; ++i) {
    sptr_t start = scintilla_send_message(SCINTILLA(scintilla),
                                          SCI_GETSELECTIONNSTART, i, 0);
    sptr_t end = scintilla_send_message(SCINTILLA(scintilla),
                                        SCI_GETSELECTIONNEND, i, 0);
    if (start == end) {
      continue;
    }
    if (start > end) {
      std::swap(start, end);
    }
    const sptr_t textLength = static_cast<sptr_t>(documentText.size());
    start = std::max<sptr_t>(0, std::min<sptr_t>(start, textLength));
    end = std::max<sptr_t>(0, std::min<sptr_t>(end, textLength));
    if (start < end) {
      ranges.push_back({start, end});
    }
  }

  if (ranges.empty()) {
    return;
  }

  std::sort(ranges.begin(), ranges.end(),
            [](const SelectionRange &left, const SelectionRange &right) {
              return left.start > right.start;
            });

  scintilla_send_message(SCINTILLA(scintilla), SCI_BEGINUNDOACTION, 0, 0);
  for (const SelectionRange &range : ranges) {
    const std::string selected =
        documentText.substr(static_cast<size_t>(range.start),
                            static_cast<size_t>(range.end - range.start));
    const std::string replacement = transform_case_text(selected, transform);
    scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETRANGE,
                           range.start, range.end);
    scintilla_send_message(SCINTILLA(scintilla), SCI_REPLACETARGET,
                           replacement.size(),
                           reinterpret_cast<sptr_t>(replacement.c_str()));
  }
  scintilla_send_message(SCINTILLA(scintilla), SCI_ENDUNDOACTION, 0, 0);
}

void transform_document_whitespace(EditorWindow *editorWindow,
                                   WhitespaceTransform transform) {
  if (!editorWindow) {
    return;
  }

  GtkWidget *scintilla = editorWindow->getCurrentScintilla();
  if (!scintilla) {
    return;
  }

  const int index = editorWindow->getCurrentDocumentIndex();
  const std::string originalText = editorWindow->getDocumentText(index);
  std::string replacement;
  switch (transform) {
    case WhitespaceTransform::TrimTrailingWhitespace:
      replacement = trim_trailing_whitespace_text(originalText);
      break;
    case WhitespaceTransform::SpacesToTabs:
      replacement = spaces_to_tabs_text(originalText,
                                        editorWindow->getTabSize());
      break;
    case WhitespaceTransform::TabsToSpaces:
      replacement = tabs_to_spaces_text(originalText, editorWindow->getTabSize());
      break;
  }

  if (replacement == originalText) {
    return;
  }

  scintilla_send_message(SCINTILLA(scintilla), SCI_BEGINUNDOACTION, 0, 0);
  scintilla_send_message(SCINTILLA(scintilla), SCI_SETTARGETRANGE, 0,
                         originalText.size());
  scintilla_send_message(SCINTILLA(scintilla), SCI_REPLACETARGET,
                         replacement.size(),
                         reinterpret_cast<sptr_t>(replacement.c_str()));
  scintilla_send_message(SCINTILLA(scintilla), SCI_ENDUNDOACTION, 0, 0);
}

uptr_t get_wrap_mode_from_string(const std::string &mode) {
  if (mode == "window") {
    return SC_WRAP_CHAR;
  }
  if (mode == "word") {
    return SC_WRAP_WORD;
  }
  return SC_WRAP_NONE;
}

}  // namespace

static std::string dirname_from_path(const std::string &path) {
  size_t lastSlash = path.find_last_of("/\\");
  return (lastSlash != std::string::npos) ? path.substr(0, lastSlash) : std::string();
}

namespace {

struct SessionDocument {
  std::string kind = "file";
  std::string path;
  std::string title = "Untitled";
  std::string text;
  bool modified = false;
  sptr_t cursor = 0;
  sptr_t anchor = 0;
  sptr_t firstVisibleLine = 0;
  sptr_t xOffset = 0;
};

struct SessionState {
  int currentIndex = 0;
  std::vector<SessionDocument> documents;
};

std::string json_escape(const std::string &text) {
  std::ostringstream stream;
  for (char ch : text) {
    switch (ch) {
      case '\\':
        stream << "\\\\";
        break;
      case '"':
        stream << "\\\"";
        break;
      case '\n':
        stream << "\\n";
        break;
      case '\r':
        stream << "\\r";
        break;
      case '\t':
        stream << "\\t";
        break;
      default:
        stream << ch;
        break;
    }
  }
  return stream.str();
}

std::string json_unescape(const std::string &text) {
  std::string result;
  result.reserve(text.size());
  for (size_t i = 0; i < text.size(); ++i) {
    if (text[i] != '\\' || i + 1 >= text.size()) {
      result.push_back(text[i]);
      continue;
    }

    const char escaped = text[++i];
    switch (escaped) {
      case 'n':
        result.push_back('\n');
        break;
      case 'r':
        result.push_back('\r');
        break;
      case 't':
        result.push_back('\t');
        break;
      default:
        result.push_back(escaped);
        break;
    }
  }
  return result;
}

bool parse_json_int_field(const std::string &text, const std::string &field,
                          sptr_t &value) {
  const std::regex pattern("\"" + field + "\"\\s*:\\s*(-?\\d+)");
  std::smatch match;
  if (!std::regex_search(text, match, pattern)) {
    return false;
  }

  try {
    value = static_cast<sptr_t>(std::stoll(match[1].str()));
    return true;
  } catch (const std::exception &) {
    return false;
  }
}

bool parse_json_string_field(const std::string &text, const std::string &field,
                             std::string &value) {
  const std::regex pattern("\"" + field + "\"\\s*:\\s*\"((?:\\\\.|[^\"])*)\"");
  std::smatch match;
  if (!std::regex_search(text, match, pattern)) {
    return false;
  }
  value = json_unescape(match[1].str());
  return true;
}

bool parse_json_bool_field(const std::string &text, const std::string &field,
                           bool &value) {
  const std::regex pattern("\"" + field + "\"\\s*:\\s*(true|false|1|0)");
  std::smatch match;
  if (!std::regex_search(text, match, pattern)) {
    return false;
  }

  const std::string raw = match[1].str();
  value = raw == "true" || raw == "1";
  return true;
}

std::vector<std::string> extract_document_objects(const std::string &content) {
  std::vector<std::string> objects;
  const size_t documentsKey = content.find("\"documents\"");
  if (documentsKey == std::string::npos) {
    return objects;
  }

  const size_t arrayStart = content.find('[', documentsKey);
  if (arrayStart == std::string::npos) {
    return objects;
  }

  bool inString = false;
  bool escaping = false;
  int depth = 0;
  size_t objectStart = std::string::npos;
  for (size_t i = arrayStart + 1; i < content.size(); ++i) {
    const char ch = content[i];
    if (inString) {
      if (escaping) {
        escaping = false;
      } else if (ch == '\\') {
        escaping = true;
      } else if (ch == '"') {
        inString = false;
      }
      continue;
    }

    if (ch == '"') {
      inString = true;
      continue;
    }
    if (ch == ']') {
      if (depth == 0) {
        break;
      }
      continue;
    }
    if (ch == '{') {
      if (depth == 0) {
        objectStart = i;
      }
      ++depth;
      continue;
    }
    if (ch == '}' && depth > 0) {
      --depth;
      if (depth == 0 && objectStart != std::string::npos) {
        objects.push_back(content.substr(objectStart, i - objectStart + 1));
        objectStart = std::string::npos;
      }
    }
  }

  return objects;
}

std::string get_scintilla_text(GtkWidget *scintilla) {
  if (!scintilla) {
    return "";
  }

  const sptr_t length =
      scintilla_send_message(SCINTILLA(scintilla), SCI_GETTEXTLENGTH, 0, 0);
  if (length <= 0) {
    return "";
  }

  std::string text(static_cast<size_t>(length) + 1, '\0');
  scintilla_send_message(SCINTILLA(scintilla), SCI_GETTEXT, length + 1,
                         reinterpret_cast<sptr_t>(text.data()));
  text.resize(static_cast<size_t>(length));
  return text;
}

SessionState read_session_file(const std::string &path) {
  SessionState session;
  std::ifstream input(path);
  if (!input) {
    return session;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  const std::string content = buffer.str();

  sptr_t currentIndex = 0;
  if (parse_json_int_field(content, "current_index", currentIndex)) {
    session.currentIndex = static_cast<int>(std::max<sptr_t>(0, currentIndex));
  }

  for (const std::string &objectText : extract_document_objects(content)) {
    SessionDocument doc;
    parse_json_string_field(objectText, "kind", doc.kind);
    parse_json_string_field(objectText, "path", doc.path);
    parse_json_string_field(objectText, "title", doc.title);
    parse_json_string_field(objectText, "text", doc.text);
    parse_json_bool_field(objectText, "modified", doc.modified);
    if (doc.kind != "untitled" && doc.path.empty()) {
      continue;
    }
    parse_json_int_field(objectText, "cursor", doc.cursor);
    parse_json_int_field(objectText, "anchor", doc.anchor);
    parse_json_int_field(objectText, "first_visible_line",
                         doc.firstVisibleLine);
    parse_json_int_field(objectText, "x_offset", doc.xOffset);
    session.documents.push_back(doc);
  }

  return session;
}

void apply_session_document_state(GtkWidget *scintilla,
                                  const SessionDocument &sessionDoc) {
  if (!scintilla) {
    return;
  }

  const sptr_t length =
      scintilla_send_message(SCINTILLA(scintilla), SCI_GETTEXTLENGTH, 0, 0);
  const sptr_t cursor = std::max<sptr_t>(0, std::min(sessionDoc.cursor, length));
  const sptr_t anchor = std::max<sptr_t>(0, std::min(sessionDoc.anchor, length));
  scintilla_send_message(SCINTILLA(scintilla), SCI_SETSEL, anchor, cursor);
  scintilla_send_message(SCINTILLA(scintilla), SCI_SETFIRSTVISIBLELINE,
                         std::max<sptr_t>(0, sessionDoc.firstVisibleLine), 0);
  scintilla_send_message(SCINTILLA(scintilla), SCI_SETXOFFSET,
                         std::max<sptr_t>(0, sessionDoc.xOffset), 0);
}

gboolean on_auto_save_timeout(gpointer data) {
  EditorWindow *editorWindow = static_cast<EditorWindow *>(data);
  editorWindow->performAutoSave();
  return G_SOURCE_CONTINUE;
}

std::string current_local_date() {
  GDateTime *now = g_date_time_new_now_local();
  gchar *formatted = g_date_time_format(now, "%F");
  std::string date = formatted ? formatted : "";
  g_free(formatted);
  g_date_time_unref(now);
  return date;
}

}  // namespace

EditorWindow::EditorWindow(const std::vector<std::string> &startupFiles) {
  currentDocumentIndex = -1;
  eventHandler = std::make_unique<EventHandler>(this);
  windowInitializer = std::make_unique<WindowInitializer>(this);
  documentManager = std::make_unique<DocumentManager>(this);
  searchManager = std::make_unique<SearchManager>(this);
  settingsManager = std::make_unique<SettingsManager>(this);
  macroManager = std::make_unique<MacroManager>(this);
  windowInitializer->initialize();
  settingsManager->load_settings();

  if (!restoreSession()) {
    documentManager->createNewDocument();
  }

  if (!startupFiles.empty()) {
    for (const std::string &path : startupFiles) {
      open_document_path(path);
    }
  }

  setLineWrapMode(lineWrapMode);
  setEditorFont(fontFamily, fontSize);
  update_window_title();
  fileStateRefreshTimerId =
      g_timeout_add_seconds(5, EventHandler::on_periodic_file_state_refresh, this);
  updateAutoSaveTimer();
}

EditorWindow::~EditorWindow() {
  updateCheckLifetime.reset();
  if (startupUpdateTimerId != 0) {
    g_source_remove(startupUpdateTimerId);
    startupUpdateTimerId = 0;
  }
  if (fileStateRefreshTimerId != 0) {
    g_source_remove(fileStateRefreshTimerId);
    fileStateRefreshTimerId = 0;
  }
  if (autoSaveTimerId != 0) {
    g_source_remove(autoSaveTimerId);
    autoSaveTimerId = 0;
  }
}

void EditorWindow::show() {
  gtk_widget_show_all(window);
  if (searchManager) {
    searchManager->hide_search_results_panel();
  }
  startupUpdateTimerId = g_timeout_add_seconds(
      kStartupUpdateDelaySeconds,
      [](gpointer data) -> gboolean {
        auto *editorWindow = static_cast<EditorWindow *>(data);
        editorWindow->startupUpdateTimerId = 0;
        editorWindow->checkForUpdates(false);
        return G_SOURCE_REMOVE;
      },
      this);
}

void EditorWindow::checkForUpdates(bool manualCheck) {
  const auto showProgress = [this] {
    if (updateProgressDialog) {
      return;
    }
    updateProgressDialog = gtk_message_dialog_new(
        getDialogParentWindow(),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, "%s",
        Localization::text("update.checking"));
    g_object_add_weak_pointer(
        G_OBJECT(updateProgressDialog),
        reinterpret_cast<gpointer *>(&updateProgressDialog));
    gtk_widget_show(updateProgressDialog);
  };

  if (updateCheckInProgress) {
    if (manualCheck) {
      updateCheckManualResultRequested = true;
      showProgress();
    }
    return;
  }

  const std::string today = current_local_date();
  if (!manualCheck && lastUpdateCheckDate == today) {
    return;
  }

  if (!manualCheck) {
    lastUpdateCheckDate = today;
    if (settingsManager) {
      settingsManager->save_settings();
    }
  } else {
    updateCheckManualResultRequested = true;
    showProgress();
  }

  updateCheckInProgress = true;
  const std::weak_ptr<int> lifetime = updateCheckLifetime;
  UpdateChecker::checkAsync(
      [this, lifetime](UpdateChecker::Result result) {
        if (lifetime.expired()) {
          return;
        }
        const bool showManualResult = updateCheckManualResultRequested;
        updateCheckManualResultRequested = false;
        updateCheckInProgress = false;
        if (updateProgressDialog) {
          gtk_widget_destroy(updateProgressDialog);
        }
        if (!result.success) {
          if (showManualResult) {
            showUpdateError(result.errorMessage);
          }
          return;
        }
        showUpdateResult(result.latestVersion, showManualResult);
      });
}

void EditorWindow::showUpdateResult(const std::string &latestVersion,
                                    bool manualCheck) {
  if (!UpdateChecker::isNewerVersion(latestVersion, VELNIX_EDITOR_VERSION)) {
    if (!manualCheck) {
      return;
    }
    gchar *detail = g_strdup_printf(
        Localization::text("update.up_to_date_detail"), VELNIX_EDITOR_VERSION);
    GtkWidget *dialog = gtk_message_dialog_new(
        getDialogParentWindow(),
        static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL |
                                    GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%s",
        Localization::text("update.up_to_date"));
    gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                             detail);
    g_free(detail);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return;
  }

  gchar *primary = g_strdup_printf(Localization::text("update.available"),
                                   latestVersion.c_str());
  gchar *detail = g_strdup_printf(Localization::text("update.available_detail"),
                                  VELNIX_EDITOR_VERSION);
  GtkWidget *dialog = gtk_message_dialog_new(
      getDialogParentWindow(),
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL |
                                  GTK_DIALOG_DESTROY_WITH_PARENT),
      GTK_MESSAGE_INFO, GTK_BUTTONS_NONE, "%s", primary);
  gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                           detail);
  gtk_dialog_add_button(GTK_DIALOG(dialog), Localization::text("update.later"),
                        GTK_RESPONSE_CANCEL);
  gtk_dialog_add_button(GTK_DIALOG(dialog),
                        Localization::text("update.open_homepage"),
                        GTK_RESPONSE_ACCEPT);
  g_free(primary);
  g_free(detail);

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    GError *error = nullptr;
    if (!gtk_show_uri_on_window(getDialogParentWindow(), VELNIX_HOMEPAGE,
                                GDK_CURRENT_TIME, &error)) {
      const std::string message = error ? error->message : "Unknown error";
      g_clear_error(&error);
      gtk_widget_destroy(dialog);
      showUpdateError(message);
      return;
    }
  }
  gtk_widget_destroy(dialog);
}

void EditorWindow::showUpdateError(const std::string &message) {
  GtkWidget *dialog = gtk_message_dialog_new(
      getDialogParentWindow(),
      static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL |
                                  GTK_DIALOG_DESTROY_WITH_PARENT),
      GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s",
      Localization::text("update.failed"));
  gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s",
                                           message.c_str());
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
}

GtkWidget *EditorWindow::get_window() { return window; }

void EditorWindow::installWindowComponents(
    const WindowComponents &components) {
  window = components.window;
  accelGroup = components.accelGroup;
  menubar = components.menubar;
  notebook = components.notebook;
  vbox = components.vbox;
  editorPane = components.editorPane;
  searchResultsPanel = components.searchResultsPanel;
  recentMenu = components.recentMenu;
  wordWrapMenuItem = components.wordWrapMenuItem;
  fullScreenMenuItem = components.fullScreenMenuItem;
  showWhitespaceMenuItem = components.showWhitespaceMenuItem;
  showEolMarkersMenuItem = components.showEolMarkersMenuItem;
  statusBar = components.statusBar;
  statusBarLabel = components.statusBarLabel;
}

void EditorWindow::registerFileMenuItems(const FileMenuItems &items) {
  newMenuItem = items.newItem;
  openMenuItem = items.openItem;
  saveMenuItem = items.saveItem;
  saveAsMenuItem = items.saveAsItem;
  exitMenuItem = items.exitItem;
}

void EditorWindow::registerSettingsMenuItems(
    const SettingsMenuItems &items) {
  preferencesMenuItem = items.preferencesItem;
  shortcutConfigMenuItem = items.shortcutConfigItem;
}

int EditorWindow::getNotebookPageCount() const {
  return notebook ? gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) : 0;
}

GtkWidget *EditorWindow::getNotebookPageAt(int index) const {
  if (!notebook) {
    return nullptr;
  }
  return gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), index);
}

void EditorWindow::appendNotebookPage(GtkWidget *page, GtkWidget *tabLabel) {
  if (!notebook || !page) {
    return;
  }
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page, tabLabel);
}

void EditorWindow::removeNotebookPage(int index) {
  if (!notebook) {
    return;
  }
  gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), index);
}

void EditorWindow::setNotebookPageReorderable(GtkWidget *page,
                                              bool reorderable) {
  if (!notebook || !page) {
    return;
  }
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), page,
                                   reorderable ? TRUE : FALSE);
}

void EditorWindow::setCurrentNotebookPage(int index) {
  if (!notebook) {
    return;
  }
  gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), index);
}

void EditorWindow::showNotebook() {
  if (notebook) {
    gtk_widget_show(notebook);
  }
}

void EditorWindow::queueNotebookDraw() {
  if (notebook) {
    gtk_widget_queue_draw(notebook);
  }
}

void EditorWindow::activateDocument(int index, bool syncNotebookPage) {
  if (index < 0 || index >= static_cast<int>(documents.size())) {
    return;
  }

  currentDocumentIndex = index;
  if (syncNotebookPage) {
    setCurrentNotebookPage(index);
  }
  update_window_title();
}

Document *EditorWindow::getDocument(int index) {
  if (index < 0 || index >= static_cast<int>(documents.size())) {
    return nullptr;
  }
  return &documents[index];
}

const Document *EditorWindow::getDocument(int index) const {
  if (index < 0 || index >= static_cast<int>(documents.size())) {
    return nullptr;
  }
  return &documents[index];
}

int EditorWindow::addDocument(Document document) {
  documents.push_back(document);
  return static_cast<int>(documents.size()) - 1;
}

void EditorWindow::removeDocument(int index) {
  if (index < 0 || index >= static_cast<int>(documents.size())) {
    return;
  }
  documents.erase(documents.begin() + index);
}

void EditorWindow::replaceDocuments(std::vector<Document> reorderedDocuments) {
  documents = std::move(reorderedDocuments);
}

bool EditorWindow::hasDocument(int index) const {
  return getDocument(index) != nullptr;
}

EditorWindow::DocumentEditorState EditorWindow::getDocumentEditorState(
    int index) const {
  DocumentEditorState state;
  const Document *doc = getDocument(index);
  if (!doc) {
    return state;
  }

  state.scintilla = doc->scintilla;
  state.modified = doc->modified;
  return state;
}

EditorWindow::DocumentFileState EditorWindow::getDocumentFileStateSnapshot(
    int index) const {
  DocumentFileState state;
  const Document *doc = getDocument(index);
  if (!doc) {
    return state;
  }

  state.path = doc->filePath;
  state.fileState = doc->fileState;
  state.readOnly = doc->readOnly;
  state.externallyModified = doc->externallyModified;
  state.lastModifiedTime = doc->lastModifiedTime;
  state.eolMode = doc->eolMode;
  state.encoding = doc->encoding;
  state.useBom = doc->useBom;
  return state;
}

void EditorWindow::applyDocumentFileStateSnapshot(
    int index, const DocumentFileState &state) {
  Document *doc = getDocument(index);
  if (!doc) {
    return;
  }

  doc->filePath = state.path;
  doc->fileState = state.fileState;
  doc->readOnly = state.readOnly;
  doc->externallyModified = state.externallyModified;
  doc->lastModifiedTime = state.lastModifiedTime;
  doc->eolMode = state.eolMode;
  doc->encoding = state.encoding;
  doc->useBom = state.useBom;
}

void EditorWindow::setDocumentModifiedFlag(int index, bool modified) {
  Document *doc = getDocument(index);
  if (!doc) {
    return;
  }

  doc->modified = modified;
}

void EditorWindow::recomputeDocumentStatus(int index) {
  Document *doc = getDocument(index);
  if (!doc) {
    return;
  }

  doc->status = DocumentState::compute_document_status(*doc);
  updateDocumentTitle(index);
}

void EditorWindow::addRecentFile(const std::string &path) {
  if (settingsManager && settingsManager->get_recent_files()) {
    settingsManager->get_recent_files()->add_recent_file(path);
  }
}

void EditorWindow::saveSession() const {
  const std::string sessionPath = ::get_session_file_path(configDir);
  std::ofstream output(sessionPath, std::ios::trunc);
  if (!output) {
    Logger::warning("Unable to open session file '%s' for writing.",
                    sessionPath.c_str());
    return;
  }

  struct SavedSessionDocument {
    std::string kind = "file";
    std::string path;
    std::string title = "Untitled";
    std::string text;
    bool modified = false;
    sptr_t cursor = 0;
    sptr_t anchor = 0;
    sptr_t firstVisibleLine = 0;
    sptr_t xOffset = 0;
  };

  std::vector<SavedSessionDocument> savedDocuments;
  savedDocuments.reserve(documents.size());
  int savedCurrentIndex = 0;

  for (size_t index = 0; index < documents.size(); ++index) {
    const Document &doc = documents[index];
    if (!doc.scintilla) {
      continue;
    }

    SavedSessionDocument saved;
    saved.path = doc.filePath;
    saved.title = doc.title.empty() ? "Untitled" : doc.title;
    saved.modified = doc.modified;
    if (doc.filePath.empty()) {
      saved.kind = "untitled";
    }
    if (saved.kind == "untitled" || saved.modified) {
      saved.text = get_scintilla_text(doc.scintilla);
    }

    if (saved.kind == "untitled" && saved.text.empty() && !saved.modified) {
      continue;
    }

    if (static_cast<int>(index) == currentDocumentIndex) {
      savedCurrentIndex = static_cast<int>(savedDocuments.size());
    }

    saved.cursor = scintilla_send_message(SCINTILLA(doc.scintilla),
                                          SCI_GETCURRENTPOS, 0, 0);
    saved.anchor =
        scintilla_send_message(SCINTILLA(doc.scintilla), SCI_GETANCHOR, 0, 0);
    saved.firstVisibleLine = scintilla_send_message(
        SCINTILLA(doc.scintilla), SCI_GETFIRSTVISIBLELINE, 0, 0);
    saved.xOffset =
        scintilla_send_message(SCINTILLA(doc.scintilla), SCI_GETXOFFSET, 0, 0);
    savedDocuments.push_back(saved);
  }

  if (savedCurrentIndex >= static_cast<int>(savedDocuments.size())) {
    savedCurrentIndex = 0;
  }

  output << "{\n";
  output << "  \"version\": 1,\n";
  output << "  \"current_index\": " << savedCurrentIndex << ",\n";
  output << "  \"documents\": [\n";
  for (size_t index = 0; index < savedDocuments.size(); ++index) {
    const SavedSessionDocument &doc = savedDocuments[index];
    output << "    {\n";
    output << "      \"kind\": \"" << doc.kind << "\",\n";
    output << "      \"path\": \"" << json_escape(doc.path) << "\",\n";
    output << "      \"title\": \"" << json_escape(doc.title) << "\",\n";
    output << "      \"text\": \"" << json_escape(doc.text) << "\",\n";
    output << "      \"modified\": " << (doc.modified ? "true" : "false")
           << ",\n";
    output << "      \"cursor\": " << doc.cursor << ",\n";
    output << "      \"anchor\": " << doc.anchor << ",\n";
    output << "      \"first_visible_line\": " << doc.firstVisibleLine << ",\n";
    output << "      \"x_offset\": " << doc.xOffset << "\n";
    output << "    }" << (index + 1 < savedDocuments.size() ? "," : "") << "\n";
  }
  output << "  ]\n";
  output << "}\n";
}

bool EditorWindow::restoreSession() {
  if (!documentManager) {
    return false;
  }

  const SessionState session = read_session_file(::get_session_file_path(configDir));
  if (session.documents.empty()) {
    return false;
  }

  int restoredCount = 0;
  for (const SessionDocument &sessionDoc : session.documents) {
    const std::string title =
        sessionDoc.title.empty() ? "Untitled" : sessionDoc.title;
    int targetIndex = documentManager->createNewDocument(title);
    if (targetIndex < 0) {
      continue;
    }

    if (sessionDoc.kind == "untitled") {
      documentManager->setDocumentText(targetIndex, sessionDoc.text);
      setDocumentModifiedFlag(targetIndex, sessionDoc.modified ||
                                               !sessionDoc.text.empty());
      recomputeDocumentStatus(targetIndex);
    } else if (!documentManager->loadDocumentFromPath(targetIndex,
                                                      sessionDoc.path)) {
      Logger::warning("Unable to restore session document '%s': %s",
                      sessionDoc.path.c_str(),
                      documentManager->get_last_error().c_str());
      removeNotebookPage(targetIndex);
      removeDocument(targetIndex);
      currentDocumentIndex = getDocumentCount() - 1;
      continue;
    } else if (sessionDoc.modified) {
      documentManager->setDocumentText(targetIndex, sessionDoc.text);
      setDocumentModifiedFlag(targetIndex, true);
      recomputeDocumentStatus(targetIndex);
    }

    apply_session_document_state(getDocument(targetIndex)
                                     ? getDocument(targetIndex)->scintilla
                                     : nullptr,
                                 sessionDoc);
    ++restoredCount;
  }

  if (restoredCount == 0) {
    currentDocumentIndex = -1;
    return false;
  }

  const int currentIndex =
      std::max(0, std::min(session.currentIndex, getDocumentCount() - 1));
  activateDocument(currentIndex);
  applyCurrentSyntaxHighlighting();
  return true;
}

void EditorWindow::refreshSearchResultsAfterModification() {
  if (searchManager && searchManager->is_auto_refresh_enabled() &&
      searchManager->has_last_search()) {
    searchManager->schedule_search_results_refresh();
  }
}

void EditorWindow::refreshDynamicHighlights(int index, int updateFlags) {
  Document *doc = getDocument(index);
  if (!doc || !doc->scintilla) {
    return;
  }

  const bool contentChanged = (updateFlags & SC_UPDATE_CONTENT) != 0;
  const bool selectionChanged =
      (updateFlags & SC_UPDATE_SELECTION) != 0 || updateFlags == SC_UPDATE_NONE;
  if (!contentChanged && !selectionChanged) {
    return;
  }

  if (contentChanged) {
    doc->cachedBraceCursorPosition = INVALID_POSITION;
  }
  const sptr_t currentPos = scintilla_send_message(
      SCINTILLA(doc->scintilla), SCI_GETCURRENTPOS, 0, 0);
  updateBraceHighlightingForDocument(*doc, false, currentPos);
  if (selectionChanged) {
    updateCurrentColumnHighlightForDocument(*doc, false, currentPos);
  }
  if (contentChanged || selectionChanged) {
    updateSelectedKeywordHighlightForDocument(*doc, contentChanged);
  }
  if (contentChanged) {
    updateCustomKeywordHighlightsForDocument(*doc, true);
  }
}

void EditorWindow::handleSelectedKeywordDoubleClick(int index) {
  Document *doc = getDocument(index);
  if (!doc || !doc->scintilla) {
    return;
  }

  doc->selectedKeywordHighlightFromDoubleClick = true;
  doc->selectedKeywordDoubleClickText.clear();
  updateSelectedKeywordHighlightForDocument(*doc, true, true);
}

void EditorWindow::highlightSelectionWithColor(int colorIndex) {
  Document *doc = getDocument(currentDocumentIndex);
  if (!doc || !doc->scintilla) {
    return;
  }

  const std::string selectedText = get_single_selection_text(doc->scintilla);
  highlightTextWithColor(colorIndex, selectedText);
}

void EditorWindow::highlightTextWithColor(int colorIndex,
                                          const std::string &text) {
  highlightTextInDocumentWithColor(currentDocumentIndex, colorIndex, text);
}

void EditorWindow::highlightTextInDocumentWithColor(int documentIndex,
                                                    int colorIndex,
                                                    const std::string &text) {
  Document *doc = getDocument(documentIndex);
  if (!doc || !doc->scintilla) {
    return;
  }

  colorIndex = clamp_int(colorIndex, 0, kCustomKeywordIndicatorCount - 1);
  const std::string selectedText = text;
  if (!is_highlightable_selected_keyword(selectedText)) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "%s", "Select a single word or token before applying a highlight.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return;
  }

  auto existing = std::find_if(
      doc->customKeywordHighlights.begin(), doc->customKeywordHighlights.end(),
      [&selectedText](const CustomKeywordHighlight &highlight) {
        return highlight.text == selectedText;
      });
  if (existing != doc->customKeywordHighlights.end()) {
    existing->colorIndex = colorIndex;
  } else {
    doc->customKeywordHighlights.push_back({selectedText, colorIndex});
  }

  updateCustomKeywordHighlightsForDocument(*doc, true);
}

void EditorWindow::clearCurrentDocumentCustomHighlights() {
  clearDocumentCustomHighlights(currentDocumentIndex);
}

void EditorWindow::clearDocumentCustomHighlights(int documentIndex) {
  Document *doc = getDocument(documentIndex);
  if (!doc || !doc->scintilla) {
    return;
  }

  doc->customKeywordHighlights.clear();
  updateCustomKeywordHighlightsForDocument(*doc, true);
}

void EditorWindow::clearCurrentDocumentCustomHighlightsForColor(int colorIndex) {
  clearDocumentCustomHighlightsForColor(currentDocumentIndex, colorIndex);
}

void EditorWindow::clearDocumentCustomHighlightsForColor(int documentIndex,
                                                         int colorIndex) {
  Document *doc = getDocument(documentIndex);
  if (!doc || !doc->scintilla) {
    return;
  }

  colorIndex = clamp_int(colorIndex, 0, kCustomKeywordIndicatorCount - 1);
  doc->customKeywordHighlights.erase(
      std::remove_if(doc->customKeywordHighlights.begin(),
                     doc->customKeywordHighlights.end(),
                     [colorIndex](const CustomKeywordHighlight &highlight) {
                       return highlight.colorIndex == colorIndex;
                     }),
      doc->customKeywordHighlights.end());
  updateCustomKeywordHighlightsForDocument(*doc, true);
}

void EditorWindow::showEditorContextMenu(GtkWidget *widget,
                                         GdkEventButton *event) {
  const int contextDocumentIndex = findDocumentIndexByScintilla(widget);
  if (contextDocumentIndex >= 0 && contextDocumentIndex != currentDocumentIndex) {
    activateDocument(contextDocumentIndex);
  }

  std::string contextText = get_single_selection_text(widget);
  if (contextText.empty() && event) {
    const sptr_t position = scintilla_send_message(
        SCINTILLA(widget), SCI_POSITIONFROMPOINTCLOSE,
        static_cast<uptr_t>(event->x), static_cast<sptr_t>(event->y));
    contextText = get_word_at_position(widget, position);
  }

  GtkWidget *menu = gtk_menu_new();
  GtkWidget *highlightItem =
      gtk_menu_item_new_with_label(Localization::text("menu.highlight"));
  Localization::bind_widget_text(highlightItem, "menu.highlight");
  GtkWidget *highlightMenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(highlightItem), highlightMenu);
  append_custom_highlight_menu_items(highlightMenu, this, contextText,
                                     contextDocumentIndex);

  GtkWidget *clearItem = gtk_menu_item_new_with_label(
      Localization::text("menu.clear_custom_highlights"));
  Localization::bind_widget_text(clearItem, "menu.clear_custom_highlights");
  GtkWidget *clearMenu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(clearItem), clearMenu);
  append_clear_custom_highlight_menu_items(clearMenu, this,
                                          contextDocumentIndex);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), highlightItem);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), clearItem);
  g_signal_connect(menu, "selection-done", G_CALLBACK(gtk_widget_destroy),
                   nullptr);
  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer(GTK_MENU(menu),
                            reinterpret_cast<GdkEvent *>(event));
  (void)widget;
}

void EditorWindow::handleMacroScintillaNotification(
    SCNotification *notification) {
  if (macroManager) {
    macroManager->handleScintillaNotification(notification);
  }
}

void EditorWindow::recordMacroSearchAction(MacroAppCommand command,
                                           const std::string &findPattern,
                                           const std::string &replaceText,
                                           bool caseSensitive,
                                           bool wholeWord,
                                           bool regex) {
  if (macroManager) {
    macroManager->recordSearchAction(command, findPattern, replaceText,
                                     caseSensitive, wholeWord, regex);
  }
}

void EditorWindow::addMacroMenuItems(GtkWidget *macroMenu) {
  if (macroManager) {
    macroManager->addMacroMenuItems(macroMenu);
  }
}

void EditorWindow::startMacroRecording() {
  if (macroManager) {
    macroManager->startRecording();
  }
}

void EditorWindow::stopMacroRecording() {
  if (macroManager) {
    macroManager->stopRecording();
  }
}

void EditorWindow::showMacroPlaybackDialog() {
  if (macroManager) {
    macroManager->showPlaybackDialog();
  }
}

void EditorWindow::promptAndSaveCurrentMacro() {
  if (macroManager) {
    macroManager->promptAndSaveCurrentMacro();
  }
}

void EditorWindow::showMacroManagementDialog() {
  if (macroManager) {
    macroManager->showMacroManagementDialog();
  }
}

void EditorWindow::saveMacros() {
  if (macroManager) {
    macroManager->saveMacros();
  }
}

void EditorWindow::reloadMacros() {
  if (macroManager) {
    macroManager->reloadMacros();
  }
}

std::string EditorWindow::getMacrosFilePath() const {
  return ::get_macros_file_path(configDir);
}

bool EditorWindow::executeMacroSearchCommand(MacroAppCommand command,
                                             const std::string &findPattern,
                                             const std::string &replaceText,
                                             bool caseSensitive,
                                             bool wholeWord,
                                             bool regex) {
  if (!searchManager) {
    return false;
  }

  switch (command) {
    case MacroAppCommand::FindNext:
      searchManager->find_next(findPattern, caseSensitive, wholeWord, regex);
      return true;
    case MacroAppCommand::FindPrevious:
      searchManager->find_previous(findPattern, caseSensitive, wholeWord, regex);
      return true;
    case MacroAppCommand::FindAll:
      searchManager->find_all(findPattern, caseSensitive, wholeWord, regex);
      return true;
    case MacroAppCommand::ReplaceNext:
      searchManager->replace_next(findPattern, replaceText, caseSensitive,
                                  wholeWord, regex);
      return true;
    case MacroAppCommand::ReplaceAll:
      searchManager->replace_all(findPattern, replaceText, caseSensitive,
                                 wholeWord, regex);
      return true;
  }

  return false;
}

bool EditorWindow::executeMacroFindAllBatch(
    const std::vector<MacroSearchRequest> &requests) {
  if (!searchManager || requests.empty()) {
    return false;
  }

  searchManager->find_all_batch(requests);
  return true;
}

void EditorWindow::showFindDialog() {
  if (searchManager) {
    searchManager->show_find_dialog();
  }
}

void EditorWindow::showReplaceDialog() {
  if (searchManager) {
    searchManager->show_replace_dialog();
  }
}

void EditorWindow::showGoToLineDialog() {
  if (searchManager) {
    searchManager->show_go_to_line_dialog();
  }
}

void EditorWindow::toggleSearchResultsPanel() {
  if (!searchManager) {
    return;
  }

  if (searchManager->is_search_results_panel_visible()) {
    searchManager->hide_search_results_panel();
  } else {
    searchManager->show_search_results_panel();
  }
}

void EditorWindow::createSearchResultsPanel(GtkWidget *parent) {
  if (searchManager) {
    searchManager->create_search_results_panel(parent);
  }
}

void EditorWindow::applyShortcutBindings(std::vector<ShortcutBinding> bindings) {
  shortcutBindings = std::move(bindings);
  for (auto &binding : shortcutBindings) {
    if (binding.displayTexts.empty() && !binding.displayText.empty()) {
      binding.displayTexts.push_back(binding.displayText);
    }
    if (binding.keys.empty() && binding.key != 0) {
      binding.keys.push_back(binding.key);
      binding.modifiers.push_back(binding.mods);
    }
    if (binding.defaultDisplayTexts.empty()) {
      binding.defaultDisplayTexts = binding.displayTexts;
    }

    binding.key = binding.keys.empty() ? 0 : binding.keys.front();
    binding.mods = binding.modifiers.empty()
                       ? static_cast<GdkModifierType>(0)
                       : binding.modifiers.front();
    binding.displayText =
        binding.displayTexts.empty() ? "" : binding.displayTexts.front();

    for (size_t i = 0; i < binding.keys.size() && i < binding.modifiers.size();
         ++i) {
      if (binding.keys[i] != 0) {
        gtk_widget_add_accelerator(binding.item, "activate", accelGroup,
                                   binding.keys[i], binding.modifiers[i],
                                   GTK_ACCEL_VISIBLE);
      }
    }
  }
}

void EditorWindow::showPreferencesDialog() {
  if (settingsManager) {
    settingsManager->show_preferences_dialog();
  }
}

void EditorWindow::showShortcutConfigurationDialog() {
  if (settingsManager && settingsManager->get_shortcuts()) {
    settingsManager->get_shortcuts()->show_shortcuts_dialog();
  }
}

void EditorWindow::setWordWrapEnabled(bool enabled, bool persist) {
  if (!enabled) {
    lineWrapMode = "off";
  } else if (lineWrapMode == "off") {
    lineWrapMode = "word";
  }

  wordWrapEnabled = enabled;

  if (wordWrapMenuItem &&
      gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(wordWrapMenuItem)) != enabled) {
    g_signal_handlers_block_by_func(
        wordWrapMenuItem,
        reinterpret_cast<gpointer>(EventHandler::on_word_wrap_toggled), this);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wordWrapMenuItem), enabled);
    g_signal_handlers_unblock_by_func(
        wordWrapMenuItem,
        reinterpret_cast<gpointer>(EventHandler::on_word_wrap_toggled), this);
  }

  const uptr_t wrapMode = enabled ? get_wrap_mode_from_string(lineWrapMode)
                                  : SC_WRAP_NONE;
  for (auto &doc : documents) {
    if (doc.scintilla) {
      scintilla_send_message(SCINTILLA(doc.scintilla), SCI_SETWRAPMODE,
                             wrapMode, 0);
    }
  }

  if (persist && settingsManager) {
    settingsManager->save_settings();
  }
}

void EditorWindow::setLineWrapMode(const std::string &mode) {
  if (mode == "off" || mode == "window" || mode == "word") {
    lineWrapMode = mode;
  } else {
    lineWrapMode = "off";
  }
  const bool enabled = lineWrapMode != "off";
  wordWrapEnabled = enabled;
  const uptr_t wrapMode = get_wrap_mode_from_string(lineWrapMode);
  for (auto &doc : documents) {
    if (doc.scintilla) {
      scintilla_send_message(SCINTILLA(doc.scintilla), SCI_SETWRAPMODE,
                             wrapMode, 0);
    }
  }
  if (wordWrapMenuItem &&
      gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(wordWrapMenuItem)) != enabled) {
    g_signal_handlers_block_by_func(
        wordWrapMenuItem,
        reinterpret_cast<gpointer>(EventHandler::on_word_wrap_toggled), this);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(wordWrapMenuItem), enabled);
    g_signal_handlers_unblock_by_func(
        wordWrapMenuItem,
        reinterpret_cast<gpointer>(EventHandler::on_word_wrap_toggled), this);
  }
}

void EditorWindow::setEditorFont(const std::string &family, int size) {
  fontFamily = family;
  fontSize = size;

  for (auto &doc : documents) {
    if (!doc.scintilla) {
      continue;
    }
    scintilla_send_message(SCINTILLA(doc.scintilla), SCI_STYLESETFONT,
                           STYLE_DEFAULT,
                           reinterpret_cast<sptr_t>(fontFamily.c_str()));
    scintilla_send_message(SCINTILLA(doc.scintilla), SCI_STYLESETSIZE,
                           STYLE_DEFAULT, fontSize);
    scintilla_send_message(SCINTILLA(doc.scintilla), SCI_STYLECLEARALL, 0, 0);
    applyLineNumberStyle(doc.scintilla, true);
  }
}

void EditorWindow::toggleWordWrap() { setWordWrapEnabled(!wordWrapEnabled); }

void EditorWindow::setFullscreenMode(bool enabled) {
  fullscreenMode = enabled;

  if (fullScreenMenuItem &&
      gtk_check_menu_item_get_active(
          GTK_CHECK_MENU_ITEM(fullScreenMenuItem)) != enabled) {
    updatingFullscreenMenuItem = true;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(fullScreenMenuItem),
                                   enabled);
    updatingFullscreenMenuItem = false;
  }

  if (!window) {
    return;
  }

  if (enabled) {
    gtk_window_fullscreen(GTK_WINDOW(window));
    if (menubar) {
      gtk_widget_hide(menubar);
    }
    if (statusBar) {
      gtk_widget_hide(statusBar);
    }
  } else {
    gtk_window_unfullscreen(GTK_WINDOW(window));
    if (menubar) {
      gtk_widget_show(menubar);
    }
    if (statusBar) {
      gtk_widget_show(statusBar);
    }
  }

  if (GtkWidget *scintilla = getCurrentScintilla()) {
    gtk_widget_grab_focus(scintilla);
  }
}

void EditorWindow::toggleFullscreenMode() {
  setFullscreenMode(!fullscreenMode);
}

void EditorWindow::applyLineNumberStyle(GtkWidget *scintilla, bool force) {
  if (!scintilla) {
    return;
  }

  Document *doc = nullptr;
  const int index = findDocumentIndexByScintilla(scintilla);
  if (index >= 0) {
    doc = getDocument(index);
  }

  if (showLineNumbers) {
    const int digits = calculate_line_number_digits(scintilla);
    if (!force && doc && doc->cachedLineNumberDigits == digits) {
      return;
    }
    if (doc) {
      doc->cachedLineNumberDigits = digits;
    }
  }

  scintilla_send_message(SCINTILLA(scintilla), SCI_STYLESETFORE,
                         STYLE_LINENUMBER, kLineNumberForeground);
  scintilla_send_message(SCINTILLA(scintilla), SCI_STYLESETBACK,
                         STYLE_LINENUMBER, kLineNumberBackground);
  scintilla_send_message(SCINTILLA(scintilla), SCI_SETMARGINTYPEN, 0,
                         SC_MARGIN_NUMBER);

  int marginWidth = 0;
  if (showLineNumbers) {
    marginWidth = calculate_line_number_margin_width(
        scintilla, doc ? doc->cachedLineNumberDigits
                       : calculate_line_number_digits(scintilla));
  }

  scintilla_send_message(SCINTILLA(scintilla), SCI_SETMARGINWIDTHN, 0,
                         marginWidth);
}

void EditorWindow::setLineNumbersVisible(bool visible) {
  showLineNumbers = visible;

  for (auto &doc : documents) {
    applyLineNumberStyle(doc.scintilla);
  }
}

void EditorWindow::setWhitespaceMarkersVisible(bool visible, bool persist) {
  showWhitespace = visible;
  if (showWhitespaceMenuItem &&
      gtk_check_menu_item_get_active(
          GTK_CHECK_MENU_ITEM(showWhitespaceMenuItem)) != visible) {
    const bool wasUpdating = updatingViewSymbolMenuItems;
    updatingViewSymbolMenuItems = true;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(showWhitespaceMenuItem),
                                   visible);
    updatingViewSymbolMenuItems = wasUpdating;
  }
  applyEditorBehaviorSettings();
  if (persist && !updatingViewSymbolMenuItems && settingsManager) {
    settingsManager->save_settings();
  }
}

void EditorWindow::setEolMarkersVisible(bool visible, bool persist) {
  showEolMarkers = visible;
  if (showEolMarkersMenuItem &&
      gtk_check_menu_item_get_active(
          GTK_CHECK_MENU_ITEM(showEolMarkersMenuItem)) != visible) {
    const bool wasUpdating = updatingViewSymbolMenuItems;
    updatingViewSymbolMenuItems = true;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(showEolMarkersMenuItem),
                                   visible);
    updatingViewSymbolMenuItems = wasUpdating;
  }
  applyEditorBehaviorSettings();
  if (persist && !updatingViewSymbolMenuItems && settingsManager) {
    settingsManager->save_settings();
  }
}

void EditorWindow::applyIndentationSettings(GtkWidget *scintilla) {
  auto apply_to_editor = [this](GtkWidget *target) {
    if (!target) {
      return;
    }

    scintilla_send_message(SCINTILLA(target), SCI_SETTABWIDTH, tabSize, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETINDENT, tabSize, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETUSETABS,
                           useSpacesForTabs ? 0 : 1, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETTABINDENTS,
                           autoIndent ? 1 : 0, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETBACKSPACEUNINDENTS,
                           autoIndent ? 1 : 0, 0);
  };

  if (scintilla) {
    apply_to_editor(scintilla);
    return;
  }

  for (auto &doc : documents) {
    apply_to_editor(doc.scintilla);
  }
}

void EditorWindow::applyEditorBehaviorSettings(GtkWidget *scintilla) {
  auto apply_to_editor = [this](GtkWidget *target) {
    if (!target) {
      return;
    }

    scintilla_send_message(SCINTILLA(target), SCI_SETVIEWWS,
                           showWhitespace ? SCWS_VISIBLEALWAYS : SCWS_INVISIBLE,
                           0);
    scintilla_send_message(SCINTILLA(target), SCI_SETVIEWEOL,
                           showEolMarkers ? 1 : 0, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETCARETLINELAYER,
                           SC_LAYER_UNDER_TEXT, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETCARETLINEVISIBLEALWAYS,
                           0, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETCARETLINEVISIBLE,
                           highlightCurrentLine ? 1 : 0, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETCARETLINEBACK,
                           kCaretLineBackground, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETCARETLINEBACKALPHA,
                           highlightCurrentLine ? kCaretLineAlpha
                                                : SC_ALPHA_TRANSPARENT,
                           0);
    scintilla_send_message(SCINTILLA(target), SCI_SETCARETLINEFRAME, 0, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETINDENTATIONGUIDES,
                           showIndentGuides ? SC_IV_LOOKFORWARD : SC_IV_NONE, 0);
    scintilla_send_message(SCINTILLA(target), SCI_STYLESETFORE,
                           STYLE_INDENTGUIDE, kIndentGuideForeground);
    scintilla_send_message(SCINTILLA(target), SCI_STYLESETFORE,
                           STYLE_BRACELIGHT, kBraceHighlightForeground);
    scintilla_send_message(SCINTILLA(target), SCI_STYLESETBACK,
                           STYLE_BRACELIGHT, kBraceHighlightBackground);
    scintilla_send_message(SCINTILLA(target), SCI_STYLESETFORE,
                           STYLE_BRACEBAD, kBraceBadForeground);
    scintilla_send_message(SCINTILLA(target), SCI_STYLESETBACK,
                           STYLE_BRACEBAD, kBraceBadBackground);
    scintilla_send_message(SCINTILLA(target), SCI_INDICSETSTYLE,
                           kSelectedKeywordIndicator, INDIC_ROUNDBOX);
    scintilla_send_message(SCINTILLA(target), SCI_INDICSETFORE,
                           kSelectedKeywordIndicator,
                           kSelectedKeywordIndicatorColor);
    scintilla_send_message(SCINTILLA(target), SCI_INDICSETALPHA,
                           kSelectedKeywordIndicator,
                           kSelectedKeywordIndicatorAlpha);
    scintilla_send_message(SCINTILLA(target), SCI_INDICSETOUTLINEALPHA,
                           kSelectedKeywordIndicator,
                           kSelectedKeywordIndicatorAlpha + 36);
    for (int i = 0; i < kCustomKeywordIndicatorCount; ++i) {
      const int indicator = kCustomKeywordIndicatorBase + i;
      scintilla_send_message(SCINTILLA(target), SCI_INDICSETSTYLE, indicator,
                             INDIC_ROUNDBOX);
      scintilla_send_message(SCINTILLA(target), SCI_INDICSETFORE, indicator,
                             kCustomKeywordIndicatorColors[i]);
      scintilla_send_message(SCINTILLA(target), SCI_INDICSETALPHA, indicator,
                             kCustomKeywordIndicatorAlpha);
      scintilla_send_message(SCINTILLA(target), SCI_INDICSETOUTLINEALPHA,
                             indicator, kCustomKeywordIndicatorAlpha + 28);
    }
    scintilla_send_message(SCINTILLA(target), SCI_SETENDATLASTLINE,
                           scrollPastLastLine ? 0 : 1, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETEDGECOLOUR,
                           kEdgeColumnColor, 0);

    if (highlightCurrentColumn) {
      // Scintilla only supports one edge rendering mode at a time, so when the
      // current column guide is enabled we prefer visible guide lines over the
      // background fill variant of the long-line marker.
      scintilla_send_message(SCINTILLA(target), SCI_SETEDGEMODE,
                             EDGE_MULTILINE, 0);
      scintilla_send_message(SCINTILLA(target), SCI_SETEDGECOLUMN,
                             longLineColumn, 0);
      scintilla_send_message(SCINTILLA(target), SCI_SETHIGHLIGHTGUIDE, 0, 0);
      scintilla_send_message(SCINTILLA(target), SCI_MULTIEDGECLEARALL, 0, 0);
      if (longLineColumn > 0) {
        scintilla_send_message(SCINTILLA(target), SCI_MULTIEDGEADDLINE,
                               longLineColumn, kEdgeColumnColor);
      }
      updateCurrentColumnHighlight(target);
    } else {
      scintilla_send_message(SCINTILLA(target), SCI_MULTIEDGECLEARALL, 0, 0);
      scintilla_send_message(SCINTILLA(target), SCI_SETHIGHLIGHTGUIDE, 0, 0);
      scintilla_send_message(SCINTILLA(target), SCI_SETEDGEMODE,
                             longLineColumn > 0
                                 ? (showRightMarginBackground ? EDGE_BACKGROUND
                                                              : EDGE_LINE)
                                 : EDGE_NONE,
                             0);
      scintilla_send_message(SCINTILLA(target), SCI_SETEDGECOLUMN,
                             longLineColumn, 0);
      if (highlightCurrentColumn) {
        updateCurrentColumnHighlight(target);
      }
    }

    updateBraceHighlighting(target);
    updateSelectedKeywordHighlight(target);
    updateCustomKeywordHighlights(target, true);
  };

  if (scintilla) {
    apply_to_editor(scintilla);
    return;
  }

  for (auto &doc : documents) {
    apply_to_editor(doc.scintilla);
  }
}

void EditorWindow::applyColumnEditorSettings(GtkWidget *scintilla) {
  auto apply_to_editor = [this](GtkWidget *target) {
    if (!target) {
      return;
    }

    scintilla_send_message(SCINTILLA(target),
                           SCI_SETMOUSESELECTIONRECTANGULARSWITCH, 1, 0);
    scintilla_send_message(SCINTILLA(target),
                           SCI_SETRECTANGULARSELECTIONMODIFIER, SCMOD_ALT, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETMULTIPLESELECTION, 1, 0);
    scintilla_send_message(SCINTILLA(target),
                           SCI_SETADDITIONALSELECTIONTYPING, 1, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETMULTIPASTE,
                           SC_MULTIPASTE_EACH, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETVIRTUALSPACEOPTIONS,
                           SCVS_RECTANGULARSELECTION, 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETSELECTIONMODE,
                           columnEditorEnabled ? SC_SEL_RECTANGLE
                                               : SC_SEL_STREAM,
                           0);
  };

  if (scintilla) {
    apply_to_editor(scintilla);
    return;
  }

  for (auto &doc : documents) {
    apply_to_editor(doc.scintilla);
  }
}

void EditorWindow::setColumnEditorEnabled(bool enabled) {
  columnEditorEnabled = enabled;
  columnEditorDragging = false;
  applyColumnEditorSettings();
}

void EditorWindow::beginColumnEditorDrag(GtkWidget *scintilla, int x, int y) {
  if (!scintilla || !columnEditorEnabled) {
    return;
  }

  gtk_widget_grab_focus(scintilla);
  columnEditorDragging = true;
  columnEditorAnchor = scintilla_send_message(SCINTILLA(scintilla),
                                              SCI_POSITIONFROMPOINT, x, y);
  scintilla_send_message(SCINTILLA(scintilla), SCI_SETSELECTIONMODE,
                         SC_SEL_RECTANGLE, 0);
  scintilla_send_message(SCINTILLA(scintilla),
                         SCI_SETRECTANGULARSELECTIONANCHOR,
                         columnEditorAnchor, 0);
  scintilla_send_message(SCINTILLA(scintilla),
                         SCI_SETRECTANGULARSELECTIONANCHORVIRTUALSPACE, 0, 0);
  updateColumnEditorDrag(scintilla, x, y);
}

void EditorWindow::updateColumnEditorDrag(GtkWidget *scintilla, int x, int y) {
  if (!scintilla || !columnEditorDragging) {
    return;
  }

  const sptr_t caret = scintilla_send_message(SCINTILLA(scintilla),
                                              SCI_POSITIONFROMPOINT, x, y);
  scintilla_send_message(SCINTILLA(scintilla),
                         SCI_SETRECTANGULARSELECTIONANCHOR,
                         columnEditorAnchor, 0);
  scintilla_send_message(SCINTILLA(scintilla),
                         SCI_SETRECTANGULARSELECTIONCARET, caret, 0);
  scintilla_send_message(SCINTILLA(scintilla),
                         SCI_SETRECTANGULARSELECTIONCARETVIRTUALSPACE, 0, 0);
  scintilla_send_message(SCINTILLA(scintilla), SCI_SCROLLCARET, 0, 0);
}

void EditorWindow::endColumnEditorDrag(GtkWidget *scintilla, int x, int y) {
  if (!columnEditorDragging) {
    return;
  }

  updateColumnEditorDrag(scintilla, x, y);
  columnEditorDragging = false;
}

void EditorWindow::updateBraceHighlighting(GtkWidget *scintilla) {
  if (scintilla) {
    int index = findDocumentIndexByScintilla(scintilla);
    if (Document *doc = getDocument(index)) {
      updateBraceHighlightingForDocument(*doc, true);
    }
    return;
  }

  for (auto &doc : documents) {
    updateBraceHighlightingForDocument(doc, true);
  }
}

void EditorWindow::updateCurrentColumnHighlight(GtkWidget *scintilla) {
  if (scintilla) {
    int index = findDocumentIndexByScintilla(scintilla);
    if (Document *doc = getDocument(index)) {
      updateCurrentColumnHighlightForDocument(*doc, true);
    }
    return;
  }

  for (auto &doc : documents) {
    updateCurrentColumnHighlightForDocument(doc, true);
  }
}

void EditorWindow::updateSelectedKeywordHighlight(GtkWidget *scintilla) {
  if (scintilla) {
    int index = findDocumentIndexByScintilla(scintilla);
    if (Document *doc = getDocument(index)) {
      updateSelectedKeywordHighlightForDocument(*doc, true);
    }
    return;
  }

  for (auto &doc : documents) {
    updateSelectedKeywordHighlightForDocument(doc, true);
  }
}

void EditorWindow::updateCustomKeywordHighlights(GtkWidget *scintilla,
                                                 bool force) {
  if (scintilla) {
    int index = findDocumentIndexByScintilla(scintilla);
    if (Document *doc = getDocument(index)) {
      updateCustomKeywordHighlightsForDocument(*doc, force);
    }
    return;
  }

  for (auto &doc : documents) {
    updateCustomKeywordHighlightsForDocument(doc, force);
  }
}

void EditorWindow::updateBraceHighlightingForDocument(Document &doc, bool force,
                                                      sptr_t currentPos) {
  GtkWidget *target = doc.scintilla;
  if (!target) {
    return;
  }

  if (!highlightMatchingBraces) {
    if (force || doc.cachedBraceHighlightStart != INVALID_POSITION ||
        doc.cachedBraceHighlightEnd != INVALID_POSITION ||
        doc.cachedBraceBadLightPosition != INVALID_POSITION) {
      scintilla_send_message(SCINTILLA(target), SCI_BRACEHIGHLIGHT,
                             INVALID_POSITION, INVALID_POSITION);
      scintilla_send_message(SCINTILLA(target), SCI_BRACEBADLIGHT,
                             INVALID_POSITION, 0);
    }
    doc.cachedBraceCursorPosition = INVALID_POSITION;
    doc.cachedBraceHighlightStart = INVALID_POSITION;
    doc.cachedBraceHighlightEnd = INVALID_POSITION;
    doc.cachedBraceBadLightPosition = INVALID_POSITION;
    return;
  }

  if (currentPos == INVALID_POSITION) {
    currentPos = scintilla_send_message(SCINTILLA(target), SCI_GETCURRENTPOS,
                                        0, 0);
  }
  if (!force && doc.cachedBraceCursorPosition == currentPos) {
    return;
  }

  sptr_t bracePos = INVALID_POSITION;
  if (currentPos > 0) {
    const char beforeChar = static_cast<char>(scintilla_send_message(
        SCINTILLA(target), SCI_GETCHARAT, currentPos - 1, 0));
    if (is_brace_character(beforeChar)) {
      bracePos = currentPos - 1;
    }
  }

  if (bracePos == INVALID_POSITION) {
    const char currentChar = static_cast<char>(scintilla_send_message(
        SCINTILLA(target), SCI_GETCHARAT, currentPos, 0));
    if (is_brace_character(currentChar)) {
      bracePos = currentPos;
    }
  }

  sptr_t highlightStart = INVALID_POSITION;
  sptr_t highlightEnd = INVALID_POSITION;
  sptr_t badLightPosition = INVALID_POSITION;

  if (bracePos != INVALID_POSITION) {
    const sptr_t matchPos = scintilla_send_message(
        SCINTILLA(target), SCI_BRACEMATCH, bracePos, 0);
    if (matchPos != INVALID_POSITION) {
      highlightStart = bracePos;
      highlightEnd = matchPos;
    } else {
      badLightPosition = bracePos;
    }
  }

  if (force || doc.cachedBraceHighlightStart != highlightStart ||
      doc.cachedBraceHighlightEnd != highlightEnd) {
    scintilla_send_message(SCINTILLA(target), SCI_BRACEHIGHLIGHT,
                           highlightStart, highlightEnd);
  }
  if (force || doc.cachedBraceBadLightPosition != badLightPosition) {
    scintilla_send_message(SCINTILLA(target), SCI_BRACEBADLIGHT,
                           badLightPosition, 0);
  }

  doc.cachedBraceCursorPosition = currentPos;
  doc.cachedBraceHighlightStart = highlightStart;
  doc.cachedBraceHighlightEnd = highlightEnd;
  doc.cachedBraceBadLightPosition = badLightPosition;
}

void EditorWindow::updateCurrentColumnHighlightForDocument(Document &doc,
                                                           bool force,
                                                           sptr_t currentPos) {
  GtkWidget *target = doc.scintilla;
  if (!target) {
    return;
  }

  if (!highlightCurrentColumn) {
    if (force || doc.cachedCurrentColumnHighlightEnabled) {
      scintilla_send_message(SCINTILLA(target), SCI_SETHIGHLIGHTGUIDE, 0, 0);
    }
    doc.cachedCurrentColumnHighlight = -1;
    doc.cachedCurrentColumnHighlightEnabled = false;
    return;
  }

  if (currentPos == INVALID_POSITION) {
    currentPos = scintilla_send_message(SCINTILLA(target), SCI_GETCURRENTPOS,
                                        0, 0);
  }
  const int currentColumn = static_cast<int>(scintilla_send_message(
      SCINTILLA(target), SCI_GETCOLUMN, currentPos, 0));
  if (!force && doc.cachedCurrentColumnHighlightEnabled &&
      doc.cachedCurrentColumnHighlight == currentColumn) {
    return;
  }

  scintilla_send_message(SCINTILLA(target), SCI_MULTIEDGECLEARALL, 0, 0);
  if (longLineColumn > 0) {
    scintilla_send_message(SCINTILLA(target), SCI_MULTIEDGEADDLINE,
                           longLineColumn, kEdgeColumnColor);
  }
  scintilla_send_message(SCINTILLA(target), SCI_SETHIGHLIGHTGUIDE, 0, 0);
  if (currentColumn >= 0 && currentColumn != longLineColumn) {
    scintilla_send_message(SCINTILLA(target), SCI_MULTIEDGEADDLINE,
                           currentColumn, kEdgeColumnColor);
  }

  doc.cachedCurrentColumnHighlight = currentColumn;
  doc.cachedCurrentColumnHighlightEnabled = true;
}

void EditorWindow::updateSelectedKeywordHighlightForDocument(Document &doc,
                                                             bool force,
                                                             bool allowWhenSmartDisabled) {
  GtkWidget *target = doc.scintilla;
  if (!target) {
    return;
  }

  const sptr_t textLength =
      scintilla_send_message(SCINTILLA(target), SCI_GETTEXTLENGTH, 0, 0);
  const std::string selectedText = get_single_selection_text(target);

  if (!is_highlightable_selected_keyword(selectedText)) {
    if (force || !doc.cachedSelectedKeywordHighlight.empty()) {
      scintilla_send_message(SCINTILLA(target), SCI_SETINDICATORCURRENT,
                             kSelectedKeywordIndicator, 0);
      scintilla_send_message(SCINTILLA(target), SCI_INDICATORCLEARRANGE, 0,
                             textLength);
    }
    doc.cachedSelectedKeywordHighlight.clear();
    doc.cachedSelectedKeywordDocumentLength = textLength;
    if (!allowWhenSmartDisabled) {
      doc.selectedKeywordHighlightFromDoubleClick = false;
      doc.selectedKeywordDoubleClickText.clear();
    }
    return;
  }

  if (doc.selectedKeywordHighlightFromDoubleClick &&
      doc.selectedKeywordDoubleClickText.empty()) {
    doc.selectedKeywordDoubleClickText = selectedText;
  }

  if (!smartKeywordHighlighting) {
    const bool allowDoubleClickHighlight =
        doc.selectedKeywordHighlightFromDoubleClick &&
        selectedText == doc.selectedKeywordDoubleClickText;
    if (!allowDoubleClickHighlight) {
      if (force || !doc.cachedSelectedKeywordHighlight.empty()) {
        scintilla_send_message(SCINTILLA(target), SCI_SETINDICATORCURRENT,
                               kSelectedKeywordIndicator, 0);
        scintilla_send_message(SCINTILLA(target), SCI_INDICATORCLEARRANGE, 0,
                               textLength);
      }
      doc.cachedSelectedKeywordHighlight.clear();
      doc.cachedSelectedKeywordDocumentLength = textLength;
      doc.selectedKeywordHighlightFromDoubleClick = false;
      doc.selectedKeywordDoubleClickText.clear();
      return;
    }
  } else if (!allowWhenSmartDisabled) {
    doc.selectedKeywordHighlightFromDoubleClick = false;
    doc.selectedKeywordDoubleClickText.clear();
  }

  if (!force && doc.cachedSelectedKeywordHighlight == selectedText &&
      doc.cachedSelectedKeywordDocumentLength == textLength) {
    return;
  }

  scintilla_send_message(SCINTILLA(target), SCI_SETINDICATORCURRENT,
                         kSelectedKeywordIndicator, 0);
  scintilla_send_message(SCINTILLA(target), SCI_INDICATORCLEARRANGE, 0,
                         textLength);

  const sptr_t selectedLength = static_cast<sptr_t>(selectedText.size());
  sptr_t searchStart = 0;
  const int searchFlags = SCFIND_MATCHCASE |
                          (is_plain_ascii_token(selectedText)
                               ? SCFIND_WHOLEWORD
                               : 0);
  scintilla_send_message(SCINTILLA(target), SCI_SETSEARCHFLAGS, searchFlags, 0);

  while (searchStart <= textLength - selectedLength) {
    scintilla_send_message(SCINTILLA(target), SCI_SETTARGETRANGE, searchStart,
                           textLength);
    const sptr_t matchStart = scintilla_send_message(
        SCINTILLA(target), SCI_SEARCHINTARGET, selectedLength,
        reinterpret_cast<sptr_t>(selectedText.c_str()));
    if (matchStart == INVALID_POSITION) {
      break;
    }
    const sptr_t matchEnd =
        scintilla_send_message(SCINTILLA(target), SCI_GETTARGETEND, 0, 0);
    if (matchEnd <= matchStart) {
      break;
    }
    scintilla_send_message(SCINTILLA(target), SCI_INDICATORFILLRANGE,
                           matchStart, matchEnd - matchStart);
    searchStart = matchEnd;
  }

  doc.cachedSelectedKeywordHighlight = selectedText;
  doc.cachedSelectedKeywordDocumentLength = textLength;
}

void EditorWindow::updateCustomKeywordHighlightsForDocument(Document &doc,
                                                            bool force) {
  GtkWidget *target = doc.scintilla;
  if (!target) {
    return;
  }

  const sptr_t textLength =
      scintilla_send_message(SCINTILLA(target), SCI_GETTEXTLENGTH, 0, 0);
  if (!force && doc.cachedCustomKeywordDocumentLength == textLength) {  // TODO. Judging whether to update based solely on length may miss changes in content.
    return;
  }

  for (int i = 0; i < kCustomKeywordIndicatorCount; ++i) {
    scintilla_send_message(SCINTILLA(target), SCI_SETINDICATORCURRENT,
                           kCustomKeywordIndicatorBase + i, 0);
    scintilla_send_message(SCINTILLA(target), SCI_INDICATORCLEARRANGE, 0,
                           textLength);
  }

  for (const auto &highlight : doc.customKeywordHighlights) {
    if (!is_highlightable_selected_keyword(highlight.text)) {
      continue;
    }

    const int colorIndex =
        clamp_int(highlight.colorIndex, 0, kCustomKeywordIndicatorCount - 1);
    const sptr_t selectedLength =
        static_cast<sptr_t>(highlight.text.size());
    sptr_t searchStart = 0;
    const int searchFlags =
        SCFIND_MATCHCASE |
        (is_plain_ascii_token(highlight.text) ? SCFIND_WHOLEWORD : 0);
    scintilla_send_message(SCINTILLA(target), SCI_SETSEARCHFLAGS, searchFlags,
                           0);
    scintilla_send_message(SCINTILLA(target), SCI_SETINDICATORCURRENT,
                           kCustomKeywordIndicatorBase + colorIndex, 0);

    while (searchStart <= textLength - selectedLength) {
      scintilla_send_message(SCINTILLA(target), SCI_SETTARGETRANGE,
                             searchStart, textLength);
      const sptr_t matchStart = scintilla_send_message(
          SCINTILLA(target), SCI_SEARCHINTARGET, selectedLength,
          reinterpret_cast<sptr_t>(highlight.text.c_str()));
      if (matchStart == INVALID_POSITION) {
        break;
      }
      const sptr_t matchEnd =
          scintilla_send_message(SCINTILLA(target), SCI_GETTARGETEND, 0, 0);
      if (matchEnd <= matchStart) {
        break;
      }
      scintilla_send_message(SCINTILLA(target), SCI_INDICATORFILLRANGE,
                             matchStart, matchEnd - matchStart);
      searchStart = matchEnd;
    }
  }

  doc.cachedCustomKeywordDocumentLength = textLength;
}

bool EditorWindow::save_file() { return documentManager->saveDocument(currentDocumentIndex); }

void EditorWindow::convertSelectionToUppercase() {
  transform_selection_case(this, CaseTransform::Uppercase);
}

void EditorWindow::convertSelectionToLowercase() {
  transform_selection_case(this, CaseTransform::Lowercase);
}

void EditorWindow::convertSelectionToTitleCase() {
  transform_selection_case(this, CaseTransform::TitleCase);
}

void EditorWindow::trimTrailingWhitespace() {
  transform_document_whitespace(this, WhitespaceTransform::TrimTrailingWhitespace);
}

void EditorWindow::convertSpacesToTabs() {
  transform_document_whitespace(this, WhitespaceTransform::SpacesToTabs);
}

void EditorWindow::convertTabsToSpaces() {
  transform_document_whitespace(this, WhitespaceTransform::TabsToSpaces);
}

bool EditorWindow::convertCurrentDocumentEncoding(FileEncoding encoding,
                                                  bool useBom) {
  Document *doc = getDocument(currentDocumentIndex);
  if (!doc) {
    return false;
  }

  if (encoding == FileEncoding::Ansi) {
    useBom = false;
  }

  if (doc->encoding == encoding && doc->useBom == useBom) {
    return true;
  }

  Document conversionProbe = *doc;
  conversionProbe.encoding = encoding;
  conversionProbe.useBom = useBom;
  std::string rawBytes;
  if (!DocumentIO::encode_document_text(
          conversionProbe, getDocumentText(currentDocumentIndex), rawBytes)) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
        GTK_BUTTONS_OK, "%s", Localization::text("error.encoding_failed"));
    const std::string secondary =
        std::string(Localization::text("error.encoding_lossless_prefix")) +
        encoding_display_name(encoding, useBom) + ".";
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog), "%s", secondary.c_str());
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    return false;
  }

  doc->encoding = encoding;
  doc->useBom = useBom;
  setDocumentModified(currentDocumentIndex, true);
  update_window_title();
  return true;
}

void EditorWindow::update_status_bar() {
  if (!statusBarLabel) {
    return;
  }

  Document *doc = documentManager ? documentManager->getCurrentDocument() : nullptr;
  const char *encodingText =
      doc ? encoding_display_name(doc->encoding, doc->useBom) : "Unknown";
  const char *eolText = "LF";
  if (doc) {
    switch (doc->eolMode) {
      case EolMode::CrLf:
        eolText = "CRLF";
        break;
      case EolMode::Cr:
        eolText = "CR";
        break;
      case EolMode::Lf:
      default:
        eolText = "LF";
        break;
    }
  }

  std::string status = "Encoding: ";
  status += encodingText;
  status += "    EOL: ";
  status += eolText;
  if (doc && doc->readOnly) {
    status += "    [Read Only]";
  }
  gtk_label_set_text(GTK_LABEL(statusBarLabel), status.c_str());
}

void EditorWindow::zoomIn() {
  if (GtkWidget *scintilla = getCurrentScintilla()) {
    scintilla_send_message(SCINTILLA(scintilla), SCI_ZOOMIN, 0, 0);
  }
}

void EditorWindow::zoomOut() {
  if (GtkWidget *scintilla = getCurrentScintilla()) {
    scintilla_send_message(SCINTILLA(scintilla), SCI_ZOOMOUT, 0, 0);
  }
}

void EditorWindow::resetZoom() {
  if (GtkWidget *scintilla = getCurrentScintilla()) {
    scintilla_send_message(SCINTILLA(scintilla), SCI_SETZOOM, 0, 0);
  }
}

bool EditorWindow::reload_current_document() {
  if (!hasDocument(currentDocumentIndex)) {
    return false;
  }

  const DocumentFileState fileState =
      getDocumentFileStateSnapshot(currentDocumentIndex);
  if (!fileState.hasPath()) {
    documentManager->show_error_dialog(
        Localization::text("error.unable_reload"),
        Localization::text("error.unsaved_reload_detail"));
    return false;
  }

  const DocumentEditorState editorState =
      getDocumentEditorState(currentDocumentIndex);
  if (editorState.modified) {
    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
        GTK_BUTTONS_NONE, "%s", Localization::text("prompt.reload_file"));
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog), "%s",
        Localization::text("prompt.discard_unsaved"));
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           Localization::text("dialog.cancel"),
                           GTK_RESPONSE_CANCEL,
                           Localization::text("dialog.reload"),
                           GTK_RESPONSE_ACCEPT, NULL);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (response != GTK_RESPONSE_ACCEPT) {
      return false;
    }
  }

  if (!reloadDocumentFromDisk(currentDocumentIndex)) {
    documentManager->show_error_dialog(Localization::text("error.unable_reload"),
                                       fileState.path);
    return false;
  }

  applyCurrentSyntaxHighlighting();
  return true;
}

bool EditorWindow::save_all_documents() {
  for (int i = 0; i < getDocumentCount(); ++i) {
    const DocumentEditorState editorState = getDocumentEditorState(i);
    if (!editorState.modified) {
      continue;
    }

    documentManager->switchToDocument(i);
    if (!documentManager->saveDocument(i)) {
      return false;
    }
  }
  return true;
}

bool EditorWindow::close_all_documents() {
  if (!offer_save_if_modified()) {
    return false;
  }

  for (int i = getDocumentCount() - 1; i >= 0; --i) {
    removeNotebookPage(i);
    removeDocument(i);
  }

  currentDocumentIndex = -1;
  documentManager->createNewDocument();
  update_window_title();
  return true;
}

bool EditorWindow::open_document_path(const std::string &path) {
  std::string normalizedPath = DocumentManager::normalize_path(path);
  int existingIndex = documentManager->findDocumentIndexByPath(normalizedPath);
  if (existingIndex >= 0) {
    documentManager->switchToDocument(existingIndex);
    return true;
  }

  int targetIndex = documentManager->isDocumentBlank(currentDocumentIndex) ? currentDocumentIndex
                                                          : documentManager->createNewDocument();

  if (!documentManager->loadDocumentFromPath(targetIndex, normalizedPath)) {
    const std::string detail = documentManager->get_last_error();
    documentManager->show_error_dialog(
        Localization::text("error.unable_open"),
        detail.empty() ? normalizedPath : (normalizedPath + "\n\n" + detail));
    if (documents.size() > 1 && documentManager->isDocumentBlank(targetIndex)) {
      documentManager->closeDocument(targetIndex);
    }
    return false;
  }

  lastFileDialogDir = dirname_from_path(normalizedPath);
  settingsManager->get_recent_files()->add_recent_file(normalizedPath);
  documentManager->switchToDocument(targetIndex);
  return true;
}

void EditorWindow::open_file_dialog() {
  GtkWidget *dialog = gtk_file_chooser_dialog_new(
      Localization::text("menu.open"), GTK_WINDOW(window),
      GTK_FILE_CHOOSER_ACTION_OPEN, Localization::text("dialog.cancel"),
      GTK_RESPONSE_CANCEL, Localization::text("dialog.open"),
      GTK_RESPONSE_ACCEPT, NULL);

  if (!lastFileDialogDir.empty()) {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                        lastFileDialogDir.c_str());
  } else {
    Document *currentDoc = documentManager->getCurrentDocument();
    if (currentDoc && !currentDoc->filePath.empty()) {
      std::string currentDir = dirname_from_path(currentDoc->filePath);
      if (!currentDir.empty()) {
        gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),
                                            currentDir.c_str());
      }
    }
  }

  if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
    char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    if (filename) {
      std::string path = filename;
      g_free(filename);
      open_document_path(path);
    }
  }

  gtk_widget_destroy(dialog);
}

bool EditorWindow::offer_save_if_modified() {
  for (int i = static_cast<int>(documents.size()) - 1; i >= 0; --i) {
    if (!documents[i].modified) {
      continue;
    }

    documentManager->switchToDocument(i);
    std::string displayName = documents[i].filePath.empty()
                                  ? documents[i].title
                                  : DocumentManager::basename_from_path(documents[i].filePath);

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(window), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
        GTK_BUTTONS_NONE, "%s", Localization::text("prompt.unsaved_changes"));
    const std::string secondary =
        std::string(Localization::text("prompt.save_changes_prefix")) +
        displayName + Localization::text("prompt.save_changes_suffix");
    gtk_message_dialog_format_secondary_text(
        GTK_MESSAGE_DIALOG(dialog), "%s", secondary.c_str());
    gtk_dialog_add_buttons(GTK_DIALOG(dialog),
                           Localization::text("dialog.cancel"), GTK_RESPONSE_CANCEL,
                           Localization::text("dialog.dont_save"), GTK_RESPONSE_NO,
                           Localization::text("dialog.save"),
                           GTK_RESPONSE_YES, NULL);

    int response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response == GTK_RESPONSE_YES) {
      if (!save_file()) {
        return false;
      }
    } else if (response == GTK_RESPONSE_CANCEL) {
      return false;
    }
  }
  return true;
}

void EditorWindow::update_window_title() {
  std::string title = Localization::text("app.name");
  Document *doc = documentManager->getCurrentDocument();
  if (doc && !doc->filePath.empty()) {
    title += " - ";
    title += DocumentManager::basename_from_path(doc->filePath);
  }
  if (doc && doc->status == DocumentStatus::Missing) {
    title += " [Missing]";
  } else if (doc && doc->status == DocumentStatus::Inaccessible) {
    title += " [Offline]";
  } else if (doc && doc->status == DocumentStatus::ExternallyModified) {
    title += " [Changed on Disk]";
  }
  if (doc && doc->readOnly) {
    title += " [Read Only]";
  }
  if (doc && doc->status == DocumentStatus::Modified) {
    title += " *";
  }
  gtk_window_set_title(GTK_WINDOW(window), title.c_str());
  update_status_bar();
}

EditorWindow::ConfigState EditorWindow::getConfigState() const {
  ConfigState config;
  config.useRecentFiles = useRecentFiles;
  config.maxRecentFiles = maxRecentFiles;
  config.showLineNumbers = showLineNumbers;
  config.uiLanguage = uiLanguage;
  config.lineWrapMode = lineWrapMode;
  config.fontFamily = fontFamily;
  config.fontSize = fontSize;
  config.autoSave = autoSave;
  config.autoSaveInterval = autoSaveInterval;
  config.overwriteReadonly = overwriteReadonly;
  config.searchWrapAround = searchWrapAround;
  config.searchAllDocuments = searchAllDocuments;
  config.lastFileDialogDir = lastFileDialogDir;
  config.configDir = configDir;
  config.defaultLexer = defaultLexer;
  config.syntaxHighlightingEnabled = syntaxHighlightingEnabled;
  config.tabSize = tabSize;
  config.useSpacesForTabs = useSpacesForTabs;
  config.autoIndent = autoIndent;
  config.showWhitespace = showWhitespace;
  config.showEolMarkers = showEolMarkers;
  config.highlightCurrentLine = highlightCurrentLine;
  config.highlightCurrentColumn = highlightCurrentColumn;
  config.ctrlMouseWheelZoom = ctrlMouseWheelZoom;
  config.smartKeywordHighlighting = smartKeywordHighlighting;
  config.trimTrailingWhitespaceOnSave = trimTrailingWhitespaceOnSave;
  config.ensureFinalNewlineOnSave = ensureFinalNewlineOnSave;
  config.showIndentGuides = showIndentGuides;
  config.longLineColumn = longLineColumn;
  config.scrollPastLastLine = scrollPastLastLine;
  config.highlightMatchingBraces = highlightMatchingBraces;
  config.showRightMarginBackground = showRightMarginBackground;
  return config;
}

void EditorWindow::applyConfigState(const ConfigState &config) {
  useRecentFiles = config.useRecentFiles;
  maxRecentFiles =
      clamp_int(config.maxRecentFiles, kMinRecentFiles, kMaxRecentFiles);
  if (recentFiles.size() > static_cast<size_t>(maxRecentFiles)) {
    recentFiles.resize(maxRecentFiles);
  }

  setLineNumbersVisible(config.showLineNumbers);
  uiLanguage = config.uiLanguage.empty() ? "en" : config.uiLanguage;
  Localization::set_language(uiLanguage);
  Localization::refresh_open_windows();
  if (searchManager) {
    searchManager->refresh_localized_ui();
  }
  if (settingsManager && settingsManager->get_recent_files()) {
    settingsManager->get_recent_files()->refresh_recent_menu();
  }
  setLineWrapMode(config.lineWrapMode);
  setEditorFont(config.fontFamily.empty() ? "Monospace" : config.fontFamily,
                clamp_int(config.fontSize, kMinFontSize, kMaxFontSize));

  autoSave = config.autoSave;
  autoSaveInterval = clamp_int(config.autoSaveInterval, kMinAutoSaveInterval,
                               kMaxAutoSaveInterval);
  overwriteReadonly = config.overwriteReadonly;
  searchWrapAround = config.searchWrapAround;
  searchAllDocuments = config.searchAllDocuments;
  lastFileDialogDir = config.lastFileDialogDir;
  configDir = config.configDir;
  defaultLexer = config.defaultLexer.empty() ? "null" : config.defaultLexer;
  syntaxHighlightingEnabled = config.syntaxHighlightingEnabled;
  tabSize = clamp_int(config.tabSize, 2, 8);
  useSpacesForTabs = config.useSpacesForTabs;
  autoIndent = config.autoIndent;
  setWhitespaceMarkersVisible(config.showWhitespace, false);
  setEolMarkersVisible(config.showEolMarkers, false);
  highlightCurrentLine = config.highlightCurrentLine;
  highlightCurrentColumn = config.highlightCurrentColumn;
  ctrlMouseWheelZoom = config.ctrlMouseWheelZoom;
  smartKeywordHighlighting = config.smartKeywordHighlighting;
  trimTrailingWhitespaceOnSave = config.trimTrailingWhitespaceOnSave;
  ensureFinalNewlineOnSave = config.ensureFinalNewlineOnSave;
  showIndentGuides = config.showIndentGuides;
  longLineColumn = clamp_int(config.longLineColumn, kMinLongLineColumn,
                             kMaxLongLineColumn);
  scrollPastLastLine = config.scrollPastLastLine;
  highlightMatchingBraces = config.highlightMatchingBraces;
  showRightMarginBackground = config.showRightMarginBackground;

  // Apply syntax highlighting settings
  if (documentManager) {
    documentManager->enableSyntaxHighlighting(syntaxHighlightingEnabled);
    applyCurrentSyntaxHighlighting();
  }

  // Apply indentation settings to all documents
  applyIndentationSettings();
  applyEditorBehaviorSettings();

  update_window_title();
  updateAutoSaveTimer();
}

void EditorWindow::loadConfigFromStore(const ConfigStore &store) {
  ConfigState config = getConfigState();
  config.useRecentFiles = store.get_bool(ConfigSchema::Items::UseRecentFiles.key);
  config.maxRecentFiles = store.get_int(ConfigSchema::Items::MaxRecentFiles.key);
  config.showLineNumbers = store.get_bool(ConfigSchema::Items::ShowLineNumbers.key);
  config.uiLanguage = store.get_string(ConfigSchema::Items::UiLanguage.key);
  config.lineWrapMode = store.get_string(ConfigSchema::Items::LineWrapMode.key);
  config.fontFamily = store.get_string(ConfigSchema::Items::FontFamily.key);
  config.fontSize = store.get_int(ConfigSchema::Items::FontSize.key);
  config.autoSave = store.get_bool(ConfigSchema::Items::AutoSave.key);
  config.autoSaveInterval = store.get_int(ConfigSchema::Items::AutoSaveInterval.key);
  config.overwriteReadonly = store.get_bool(ConfigSchema::Items::OverwriteReadonly.key);
  config.searchWrapAround = store.get_bool(ConfigSchema::Items::SearchWrapAround.key);
  config.searchAllDocuments = store.get_bool(ConfigSchema::Items::SearchAllDocuments.key);
  config.lastFileDialogDir = store.get_string(ConfigSchema::Items::LastFileDialogDir.key);
  config.defaultLexer = store.get_string(ConfigSchema::Items::DefaultLexer.key);
  config.syntaxHighlightingEnabled = store.get_bool(ConfigSchema::Items::SyntaxHighlightingEnabled.key);
  config.tabSize = store.get_int(ConfigSchema::Items::TabSize.key);
  config.useSpacesForTabs = store.get_bool(ConfigSchema::Items::UseSpacesForTabs.key);
  config.autoIndent = store.get_bool(ConfigSchema::Items::AutoIndent.key);
  config.showWhitespace = store.get_bool(ConfigSchema::Items::ShowWhitespace.key);
  config.showEolMarkers = store.get_bool(ConfigSchema::Items::ShowEolMarkers.key);
  config.highlightCurrentLine = store.get_bool(ConfigSchema::Items::HighlightCurrentLine.key);
  config.highlightCurrentColumn = store.get_bool(ConfigSchema::Items::HighlightCurrentColumn.key);
  config.ctrlMouseWheelZoom = store.get_bool(ConfigSchema::Items::CtrlMouseWheelZoom.key);
  config.smartKeywordHighlighting =
      store.get_bool(ConfigSchema::Items::SmartKeywordHighlighting.key);
  config.trimTrailingWhitespaceOnSave = store.get_bool(ConfigSchema::Items::TrimTrailingWhitespaceOnSave.key);
  config.ensureFinalNewlineOnSave =
      store.get_bool(ConfigSchema::Items::EnsureFinalNewlineOnSave.key);
  config.showIndentGuides = store.get_bool(ConfigSchema::Items::ShowIndentGuides.key);
  config.longLineColumn = store.get_int(ConfigSchema::Items::LongLineColumn.key);
  config.scrollPastLastLine = store.get_bool(ConfigSchema::Items::ScrollPastLastLine.key);
  config.highlightMatchingBraces =
      store.get_bool(ConfigSchema::Items::HighlightMatchingBraces.key);
  config.showRightMarginBackground =
      store.get_bool(ConfigSchema::Items::ShowRightMarginBackground.key);
  const std::string configuredConfigDir =
      store.get_string(ConfigSchema::Items::ConfigDirectory.key);
  if (!configuredConfigDir.empty()) {
    config.configDir = configuredConfigDir;
  }
  lastUpdateCheckDate =
      store.get_string(ConfigSchema::Items::LastUpdateCheckDate.key);
  applyConfigState(config);
}

void EditorWindow::saveConfigToStore(ConfigStore &store) const {
  const ConfigState config = getConfigState();
  store.set_string(ConfigSchema::Items::LastUpdateCheckDate.key,
                   lastUpdateCheckDate);
  store.set_bool(ConfigSchema::Items::UseRecentFiles.key, config.useRecentFiles);
  store.set_int(ConfigSchema::Items::MaxRecentFiles.key, config.maxRecentFiles);
  store.set_bool(ConfigSchema::Items::ShowLineNumbers.key, config.showLineNumbers);
  store.set_string(ConfigSchema::Items::UiLanguage.key, config.uiLanguage);
  store.set_string(ConfigSchema::Items::LineWrapMode.key, config.lineWrapMode);
  store.set_string(ConfigSchema::Items::FontFamily.key, config.fontFamily);
  store.set_int(ConfigSchema::Items::FontSize.key, config.fontSize);
  store.set_bool(ConfigSchema::Items::AutoSave.key, config.autoSave);
  store.set_int(ConfigSchema::Items::AutoSaveInterval.key,
                config.autoSaveInterval);
  store.set_bool(ConfigSchema::Items::OverwriteReadonly.key,
                 config.overwriteReadonly);
  store.set_bool(ConfigSchema::Items::SearchWrapAround.key,
                 config.searchWrapAround);
  store.set_bool(ConfigSchema::Items::SearchAllDocuments.key,
                 config.searchAllDocuments);
  store.set_string(ConfigSchema::Items::LastFileDialogDir.key,
                   config.lastFileDialogDir);
  store.set_string(ConfigSchema::Items::DefaultLexer.key, config.defaultLexer);
  store.set_bool(ConfigSchema::Items::SyntaxHighlightingEnabled.key,
                 config.syntaxHighlightingEnabled);
  store.set_int(ConfigSchema::Items::TabSize.key, config.tabSize);
  store.set_bool(ConfigSchema::Items::UseSpacesForTabs.key, config.useSpacesForTabs);
  store.set_bool(ConfigSchema::Items::AutoIndent.key, config.autoIndent);
  store.set_bool(ConfigSchema::Items::ShowWhitespace.key, config.showWhitespace);
  store.set_bool(ConfigSchema::Items::ShowEolMarkers.key, config.showEolMarkers);
  store.set_bool(ConfigSchema::Items::HighlightCurrentLine.key,
                 config.highlightCurrentLine);
  store.set_bool(ConfigSchema::Items::HighlightCurrentColumn.key,
                 config.highlightCurrentColumn);
  store.set_bool(ConfigSchema::Items::CtrlMouseWheelZoom.key,
                 config.ctrlMouseWheelZoom);
  store.set_bool(ConfigSchema::Items::SmartKeywordHighlighting.key,
                 config.smartKeywordHighlighting);
  store.set_bool(ConfigSchema::Items::TrimTrailingWhitespaceOnSave.key,
                 config.trimTrailingWhitespaceOnSave);
  store.set_bool(ConfigSchema::Items::EnsureFinalNewlineOnSave.key,
                 config.ensureFinalNewlineOnSave);
  store.set_bool(ConfigSchema::Items::ShowIndentGuides.key,
                 config.showIndentGuides);
  store.set_int(ConfigSchema::Items::LongLineColumn.key, config.longLineColumn);
  store.set_bool(ConfigSchema::Items::ScrollPastLastLine.key,
                 config.scrollPastLastLine);
  store.set_bool(ConfigSchema::Items::HighlightMatchingBraces.key,
                 config.highlightMatchingBraces);
  store.set_bool(ConfigSchema::Items::ShowRightMarginBackground.key,
                 config.showRightMarginBackground);
  store.set_string(ConfigSchema::Items::ConfigDirectory.key, config.configDir);
}

void EditorWindow::updateAutoSaveTimer() {
  if (autoSaveTimerId != 0) {
    g_source_remove(autoSaveTimerId);
    autoSaveTimerId = 0;
  }

  if (!autoSave || autoSaveInterval <= 0) {
    return;
  }

  autoSaveTimerId = g_timeout_add_seconds(autoSaveInterval, on_auto_save_timeout,
                                          this);
}

bool EditorWindow::performAutoSave() {
  if (!autoSave || autoSaveInterval <= 0 || !documentManager) {
    return false;
  }

  bool anySaved = false;
  for (int index = 0; index < static_cast<int>(documents.size()); ++index) {
    Document &doc = documents[index];
    if (!doc.modified || doc.filePath.empty()) {
      continue;
    }
    if (documentManager->saveDocument(index)) {
      anySaved = true;
    }
  }
  return anySaved;
}
