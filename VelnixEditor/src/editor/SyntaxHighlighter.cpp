#include "editor/SyntaxHighlighter.h"
#include "editor/DocumentManager.h"
#include <Scintilla.h>
#include <ILexer.h>
#include <SciLexer.h>
#include <Lexilla.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <cctype>
#include <map>

namespace {

// 颜色转换辅助函数
long hexToLong(const std::string& hexColor) {
  if (hexColor.empty() || hexColor[0] != '#') {
    return 0;
  }

  std::string hex = hexColor.substr(1);
  if (hex.length() != 6) {
    return 0;
  }

  long r = std::stoi(hex.substr(0, 2), nullptr, 16);
  long g = std::stoi(hex.substr(2, 2), nullptr, 16);
  long b = std::stoi(hex.substr(4, 2), nullptr, 16);

  return (r << 16) | (g << 8) | b;
}

std::string toLowerCopy(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
  return value;
}

const std::map<std::string, std::vector<std::string>> kKeywordLists = {
  {"cpp", {
      "alignas alignof and and_eq asm auto bitand bitor bool break case catch char char8_t char16_t "
      "char32_t class compl concept const consteval constexpr constinit const_cast continue co_await "
      "co_return co_yield decltype default delete do double dynamic_cast else enum explicit export extern "
      "false float for friend goto if inline int long mutable namespace new noexcept not not_eq nullptr "
      "operator or or_eq private protected public register reinterpret_cast requires return short signed "
      "sizeof static static_assert static_cast struct switch template this thread_local throw true try "
      "typedef typeid typename union unsigned using virtual void volatile wchar_t while xor xor_eq "
      "final override import module",
      "std string vector map unordered_map set unordered_set unique_ptr shared_ptr weak_ptr optional variant "
      "tuple pair size_t nullptr_t cout cin cerr clog endl begin end move forward swap make_shared make_unique"
  }},
  {"python", {
      "and as assert async await break class continue def del elif else except False finally for from global "
      "if import in is lambda None nonlocal not or pass raise return True try while with yield match case",
      ""
  }},
  {"sql", {
      "select from where insert into values update delete create alter drop table index view trigger procedure "
      "function begin end commit rollback join inner left right full outer on group by order having limit offset "
      "distinct union all as and or not null is like in exists between case when then else primary key foreign "
      "references constraint database schema grant revoke",
      ""
  }},
  {"bash", {
      "if then else elif fi for while until do done case esac function select in time coproc return break continue "
      "readonly export local declare typeset unset alias unalias eval exec source test",
      ""
  }},
  {"cmake", {
      "if elseif else endif foreach endforeach function endfunction macro endmacro while endwhile set unset "
      "project add_executable add_library target_link_libraries target_include_directories target_compile_options "
      "target_sources include find_package message option cmake_minimum_required install list string file",
      ""
  }},
  {"powershell", {
      "begin break catch class continue data default do dynamicparam else elseif end enum exit filter finally for "
      "foreach from function hidden if in interface param process return static switch throw trap try until using "
      "var while workflow",
      ""
  }},
  {"rust", {
      "as break const continue crate else enum extern false fn for if impl in let loop match mod move mut pub ref "
      "return self Self static struct super trait true type unsafe use where while async await dyn",
      ""
  }},
  {"json", {"true false null", ""}},
  {"javascript", {
      "break case catch class const continue debugger default delete do else export extends false finally for "
      "function if import in instanceof let new null return super switch this throw true try typeof var void while "
      "with yield async await",
      ""
  }},
  {"typescript", {
      "abstract any as asserts bigint boolean break case catch class const constructor continue debugger declare "
      "default delete do else enum export extends false finally for from function get if implements import in "
      "infer instanceof interface is keyof let module namespace never new null number object package private "
      "protected public override readonly require global return set static string super switch symbol this throw "
      "true try type typeof undefined unique unknown var void while with yield async await",
      ""
  }}
};

}  // namespace

SyntaxHighlighter::SyntaxHighlighter(DocumentManager* docManager)
  : _docManager(docManager), _highlightingEnabled(true) {
  initializeAvailableLexers();
  initializeFileExtensionMap();
}

void SyntaxHighlighter::initializeAvailableLexers() {
  _availableLexers = {
    {"null", "Plain Text", SCLEX_NULL, "txt,text,log"},
    {"cpp", "C/C++", SCLEX_CPP, "c,cpp,cxx,cc,h,hpp,hxx,hh"},
    {"python", "Python", SCLEX_PYTHON, "py,pyw"},
    {"java", "Java", SCLEX_CPP, "java"},
    {"javascript", "JavaScript", SCLEX_CPP, "js,jsx"},
    {"html", "HTML", SCLEX_HTML, "html,htm"},
    {"xml", "XML", SCLEX_XML, "xml"},
    {"css", "CSS", SCLEX_CSS, "css"},
    {"sql", "SQL", SCLEX_SQL, "sql"},
    {"php", "PHP", SCLEX_PHPSCRIPT, "php"},
    {"ruby", "Ruby", SCLEX_RUBY, "rb"},
    {"perl", "Perl", SCLEX_PERL, "pl,pm"},
    {"bash", "Bash", SCLEX_BASH, "sh,bash"},
    {"lua", "Lua", SCLEX_LUA, "lua"},
    {"makefile", "Makefile", SCLEX_MAKEFILE, "mak,mk"},
    {"json", "JSON", SCLEX_JSON, "json"},
    {"yaml", "YAML", SCLEX_YAML, "yaml,yml"},
    {"markdown", "Markdown", SCLEX_MARKDOWN, "md,markdown"},
    {"diff", "Diff/Patch", SCLEX_DIFF, "diff,patch"},
    {"properties", "Properties", SCLEX_PROPERTIES, "properties,ini,cfg"},
    {"batch", "Batch", SCLEX_BATCH, "bat,cmd"},
    {"powershell", "PowerShell", SCLEX_POWERSHELL, "ps1"},
    {"go", "Go", SCLEX_CPP, "go"},
    {"rust", "Rust", SCLEX_RUST, "rs"},
    {"swift", "Swift", SCLEX_CPP, "swift"},
    {"kotlin", "Kotlin", SCLEX_CPP, "kt,kts"},
    {"scala", "Scala", SCLEX_CPP, "scala"},
    {"dart", "Dart", SCLEX_CPP, "dart"},
    {"typescript", "TypeScript", SCLEX_CPP, "ts,tsx"},
    {"vue", "Vue.js", SCLEX_HTML, "vue"},
    {"dockerfile", "Dockerfile", SCLEX_CPP, "dockerfile"},
    {"cmake", "CMake", SCLEX_CMAKE, "cmake"},
    {"tex", "LaTeX", SCLEX_LATEX, "tex,latex"},
    {"matlab", "MATLAB", SCLEX_MATLAB, "m"},
    {"r", "R", SCLEX_CPP, "r,rscript"},
    {"haskell", "Haskell", SCLEX_HASKELL, "hs,lhs"},
    {"erlang", "Erlang", SCLEX_ERLANG, "erl,hrl"},
    {"clojure", "Clojure", SCLEX_LISP, "clj,cljs,cljc"},
    {"scheme", "Scheme", SCLEX_LISP, "scm,ss"},
    {"fortran", "Fortran", SCLEX_FORTRAN, "f,f90,f95,f03,f08"},
    {"pascal", "Pascal", SCLEX_PASCAL, "pas,pp"},
    {"ada", "Ada", SCLEX_ADA, "ada,adb,ads"},
    {"vhdl", "VHDL", SCLEX_VHDL, "vhdl,vhd"},
    {"verilog", "Verilog", SCLEX_VERILOG, "v,vh,sv"},
    {"tcl", "Tcl", SCLEX_TCL, "tcl"},
    {"autoit", "AutoIt", SCLEX_AU3, "au3"},
    {"awk", "AWK", SCLEX_CPP, "awk"},
    {"sed", "Sed", SCLEX_CPP, "sed"}
  };
}

void SyntaxHighlighter::initializeFileExtensionMap() {
  for (const auto& lexer : _availableLexers) {
    std::string extensions = lexer.fileExtensions;
    size_t pos = 0;
    std::string token;
    while ((pos = extensions.find(',')) != std::string::npos) {
      token = extensions.substr(0, pos);
      _fileExtensionMap[token] = lexer.name;
      extensions.erase(0, pos + 1);
    }
    if (!extensions.empty()) {
      _fileExtensionMap[extensions] = lexer.name;
    }
  }
}

const LexerDefinition *SyntaxHighlighter::findLexerDefinition(const std::string& lexerName) const {
  auto it = std::find_if(_availableLexers.begin(), _availableLexers.end(),
                         [&lexerName](const LexerDefinition& def) {
                           return def.name == lexerName;
                         });
  return it != _availableLexers.end() ? &(*it) : nullptr;
}

std::string SyntaxHighlighter::getCurrentLexer() const {
  if (!_docManager) {
    return "null";
  }

  Document *doc = _docManager->getCurrentDocument();
  if (!doc || doc->lexerName.empty()) {
    return "null";
  }
  return doc->lexerName;
}

void SyntaxHighlighter::setLexer(const std::string& lexerName) {
  if (!_docManager) {
    return;
  }

  Document *doc = _docManager->getCurrentDocument();
  if (!doc) {
    return;
  }

  const LexerDefinition *lexer = findLexerDefinition(lexerName);
  if (!lexer) {
    std::cout << "Unknown lexer: " << lexerName << std::endl;
    return;
  }

  doc->lexerName = lexer->name;
  if (_highlightingEnabled) {
    setupLexer(*lexer);
  }
  std::cout << "Switched to lexer: " << lexer->name << std::endl;
}

void SyntaxHighlighter::setLexerByFileExtension(const std::string& filePath) {
  const std::string detected = detectLanguageFromFileName(filePath);
  if (detected != "null") {
    setLexer(detected);
    return;
  }

  if (!_docManager) {
    return;
  }

  Document *doc = _docManager->getCurrentDocument();
  if (!doc) {
    return;
  }

  const std::string detectedByContent = detectLanguageFromContent(
      _docManager->getDocumentText(_docManager->getCurrentDocumentIndex()));
  doc->lexerName = detectedByContent;
  if (_highlightingEnabled) {
    if (const LexerDefinition *lexer = findLexerDefinition(detectedByContent)) {
      setupLexer(*lexer);
    }
  }
}

void SyntaxHighlighter::applyCurrentDocumentHighlighting() {
  if (!_docManager) {
    return;
  }

  Document *doc = _docManager->getCurrentDocument();
  if (!doc) {
    return;
  }

  if (!_highlightingEnabled) {
    GtkWidget *sci = _docManager->getCurrentScintilla();
    if (sci) {
      scintilla_send_message(SCINTILLA(sci), SCI_SETILEXER, 0, 0);
      scintilla_send_message(SCINTILLA(sci), SCI_COLOURISE, 0, -1);
    }
    return;
  }

  if (doc->lexerName.empty()) {
    doc->lexerName = doc->filePath.empty()
        ? "null"
        : detectLanguageFromFileName(doc->filePath);
  }

  const LexerDefinition *lexer = findLexerDefinition(doc->lexerName);
  if (!lexer) {
    doc->lexerName = "null";
    lexer = findLexerDefinition("null");
  }

  if (lexer) {
    setupLexer(*lexer);
  }
}

void SyntaxHighlighter::setupLexer(const LexerDefinition& lexer) {
  GtkWidget* sci = _docManager->getCurrentScintilla();
  if (!sci) return;

  applyDefaultStyles();

  Scintilla::ILexer5 *lexerInstance = nullptr;
  if (lexer.name != "null") {
    lexerInstance = ::CreateLexer(lexer.name.c_str());
  }
  scintilla_send_message(SCINTILLA(sci), SCI_SETILEXER, 0,
                         reinterpret_cast<sptr_t>(lexerInstance));

  applyKeywordLists(lexer);
  applyLexerStyles(lexer);

  scintilla_send_message(SCINTILLA(sci), SCI_COLOURISE, 0, -1);
}

void SyntaxHighlighter::applyDefaultStyles() {
  GtkWidget* sci = _docManager->getCurrentScintilla();
  if (!sci) return;

  // 设置默认样式
  scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBACK, STYLE_DEFAULT,
                         hexToLong(DEFAULT_BACKGROUND));
  scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, STYLE_DEFAULT,
                         hexToLong(DEFAULT_FOREGROUND));
  const std::string fontFamily = _docManager->getEditorFontFamily().empty()
      ? "Monospace"
      : _docManager->getEditorFontFamily();
  const int fontSize = std::clamp(_docManager->getEditorFontSize(), 8, 72);
  scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFONT, STYLE_DEFAULT,
                         reinterpret_cast<sptr_t>(fontFamily.c_str()));
  scintilla_send_message(SCINTILLA(sci), SCI_STYLESETSIZE, STYLE_DEFAULT,
                         fontSize);

  // 应用默认样式到所有样式
  scintilla_send_message(SCINTILLA(sci), SCI_STYLECLEARALL, 0, 0);
}

void SyntaxHighlighter::applyKeywordLists(const LexerDefinition& lexer) {
  GtkWidget* sci = _docManager->getCurrentScintilla();
  if (!sci) return;

  for (int i = 0; i < 9; ++i) {
    scintilla_send_message(SCINTILLA(sci), SCI_SETKEYWORDS, i,
                           reinterpret_cast<sptr_t>(""));
  }

  auto it = kKeywordLists.find(lexer.name);
  if (it == kKeywordLists.end()) {
    return;
  }

  for (size_t i = 0; i < it->second.size(); ++i) {
    scintilla_send_message(SCINTILLA(sci), SCI_SETKEYWORDS,
                           static_cast<uptr_t>(i),
                           reinterpret_cast<sptr_t>(it->second[i].c_str()));
  }
}

void SyntaxHighlighter::applyLexerStyles(const LexerDefinition& lexer) {
  GtkWidget* sci = _docManager->getCurrentScintilla();
  if (!sci) return;

  switch (lexer.lexerId) {
    case SCLEX_CPP:
    case SCLEX_CPPNOCASE:
      // C/C++ 样式
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_C_COMMENT,
                             hexToLong(COMMENT_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_C_COMMENTLINE,
                             hexToLong(COMMENT_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_C_STRING,
                             hexToLong(STRING_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_C_CHARACTER,
                             hexToLong(STRING_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_C_WORD,
                             hexToLong(KEYWORD_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_C_NUMBER,
                             hexToLong(NUMBER_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_C_PREPROCESSOR,
                             hexToLong(PREPROCESSOR_COLOR));
      break;

    case SCLEX_PYTHON:
      // Python 样式
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_P_COMMENTLINE,
                             hexToLong(COMMENT_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_P_STRING,
                             hexToLong(STRING_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_P_WORD,
                             hexToLong(KEYWORD_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_P_NUMBER,
                             hexToLong(NUMBER_COLOR));
      break;

    case SCLEX_HTML:
      // HTML 样式
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_H_TAG,
                             hexToLong(KEYWORD_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_H_ATTRIBUTE,
                             hexToLong(PREPROCESSOR_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_H_DOUBLESTRING,
                             hexToLong(STRING_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_H_SINGLESTRING,
                             hexToLong(STRING_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_H_COMMENT,
                             hexToLong(COMMENT_COLOR));
      break;

    case SCLEX_CSS:
      // CSS 样式
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_CSS_TAG,
                             hexToLong(KEYWORD_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_CSS_CLASS,
                             hexToLong(PREPROCESSOR_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_CSS_ID,
                             hexToLong(PREPROCESSOR_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_CSS_COMMENT,
                             hexToLong(COMMENT_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_CSS_DOUBLESTRING,
                             hexToLong(STRING_COLOR));
      break;

    case SCLEX_SQL:
      // SQL 样式
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_SQL_COMMENT,
                             hexToLong(COMMENT_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_SQL_COMMENTLINE,
                             hexToLong(COMMENT_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_SQL_WORD,
                             hexToLong(KEYWORD_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_SQL_STRING,
                             hexToLong(STRING_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_SQL_NUMBER,
                             hexToLong(NUMBER_COLOR));
      break;

    case SCLEX_MARKDOWN:
      // Markdown 样式
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_STRONG1,
                             hexToLong(KEYWORD_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_STRONG2,
                             hexToLong(KEYWORD_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBOLD, SCE_MARKDOWN_STRONG1, 1);
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBOLD, SCE_MARKDOWN_STRONG2, 1);

      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_EM1,
                             hexToLong(PREPROCESSOR_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_EM2,
                             hexToLong(PREPROCESSOR_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETITALIC, SCE_MARKDOWN_EM1, 1);
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETITALIC, SCE_MARKDOWN_EM2, 1);

      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_HEADER1,
                             hexToLong(MARKDOWN_HEADER1_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_HEADER2,
                             hexToLong(MARKDOWN_HEADER2_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_HEADER3,
                             hexToLong(MARKDOWN_HEADER3_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_HEADER4,
                             hexToLong(MARKDOWN_HEADER4_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_HEADER5,
                             hexToLong(MARKDOWN_HEADER5_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_HEADER6,
                             hexToLong(MARKDOWN_HEADER6_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBOLD, SCE_MARKDOWN_HEADER1, 1);
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBOLD, SCE_MARKDOWN_HEADER2, 1);
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBOLD, SCE_MARKDOWN_HEADER3, 1);
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBOLD, SCE_MARKDOWN_HEADER4, 1);
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBOLD, SCE_MARKDOWN_HEADER5, 1);
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBOLD, SCE_MARKDOWN_HEADER6, 1);

      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_ULIST_ITEM,
                             hexToLong(KEYWORD_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_OLIST_ITEM,
                             hexToLong(KEYWORD_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_BLOCKQUOTE,
                             hexToLong(MARKDOWN_QUOTE_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETITALIC, SCE_MARKDOWN_BLOCKQUOTE, 1);
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_STRIKEOUT,
                             hexToLong(COMMENT_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_HRULE,
                             hexToLong(MARKDOWN_RULE_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_LINK,
                             hexToLong(MARKDOWN_LINK_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETUNDERLINE, SCE_MARKDOWN_LINK, 1);

      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_CODE,
                             hexToLong(MARKDOWN_CODE_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBACK, SCE_MARKDOWN_CODE,
                             hexToLong(MARKDOWN_CODE_BACKGROUND));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_CODE2,
                             hexToLong(MARKDOWN_CODE_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBACK, SCE_MARKDOWN_CODE2,
                             hexToLong(MARKDOWN_CODE_BACKGROUND));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, SCE_MARKDOWN_CODEBK,
                             hexToLong(MARKDOWN_CODE_COLOR));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETBACK, SCE_MARKDOWN_CODEBK,
                             hexToLong(MARKDOWN_CODEBLOCK_BACKGROUND));
      scintilla_send_message(SCINTILLA(sci), SCI_STYLESETEOLFILLED, SCE_MARKDOWN_CODEBK, 1);
      break;

    default:
      // 对于其他词法分析器，使用默认样式
      break;
  }
}

void SyntaxHighlighter::setStyleColor(int styleId, const std::string& color) {
  GtkWidget* sci = _docManager->getCurrentScintilla();
  if (!sci) return;

  scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFORE, styleId, hexToLong(color));
}

void SyntaxHighlighter::setStyleFont(int styleId, const std::string& fontName, int fontSize) {
  GtkWidget* sci = _docManager->getCurrentScintilla();
  if (!sci) return;

  scintilla_send_message(SCINTILLA(sci), SCI_STYLESETFONT, styleId,
                         reinterpret_cast<sptr_t>(fontName.c_str()));
  scintilla_send_message(SCINTILLA(sci), SCI_STYLESETSIZE, styleId, fontSize);
}

std::string SyntaxHighlighter::detectLanguageFromContent(const std::string& content) {
  // 简单的基于内容的语言检测
  if (content.find("#include") != std::string::npos ||
      content.find("int main") != std::string::npos) {
    return "cpp";
  }

  if (content.find("def ") != std::string::npos ||
      content.find("import ") != std::string::npos ||
      content.find("print(") != std::string::npos) {
    return "python";
  }

  if (content.find("<html") != std::string::npos ||
      content.find("<div") != std::string::npos) {
    return "html";
  }

  if (content.find("function") != std::string::npos ||
      content.find("var ") != std::string::npos ||
      content.find("console.log") != std::string::npos) {
    return "javascript";
  }

  if (content.find("SELECT") != std::string::npos ||
      content.find("FROM") != std::string::npos ||
      content.find("WHERE") != std::string::npos) {
    return "sql";
  }

  return "null";  // 默认纯文本
}

std::string SyntaxHighlighter::detectLanguageFromFileName(const std::string& fileName) {
  std::string normalizedFileName = fileName;
  size_t slashPos = normalizedFileName.find_last_of("/\\");
  if (slashPos != std::string::npos) {
    normalizedFileName = normalizedFileName.substr(slashPos + 1);
  }
  normalizedFileName = toLowerCopy(normalizedFileName);

  if (normalizedFileName == "makefile" || normalizedFileName == "gnumakefile") {
    return "makefile";
  }
  if (normalizedFileName == "cmakelists.txt") {
    return "cmake";
  }
  if (normalizedFileName == "dockerfile") {
    return "dockerfile";
  }

  size_t dotPos = normalizedFileName.find_last_of('.');
  if (dotPos == std::string::npos) {
    return "null";
  }

  std::string extension = normalizedFileName.substr(dotPos + 1);

  auto it = _fileExtensionMap.find(extension);
  return it != _fileExtensionMap.end() ? it->second : "null";
}

void SyntaxHighlighter::enableHighlighting(bool enable) {
  _highlightingEnabled = enable;

  if (enable) {
    applyCurrentDocumentHighlighting();
  } else {
    GtkWidget* sci = _docManager->getCurrentScintilla();
    if (sci) {
      scintilla_send_message(SCINTILLA(sci), SCI_SETILEXER, 0, 0);
      scintilla_send_message(SCINTILLA(sci), SCI_COLOURISE, 0, -1);
    }
  }
}
