#pragma once

#include "core/EditorTypes.h"
#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <map>

class DocumentManager;

struct LexerDefinition {
  std::string name;
  std::string description;
  int lexerId;
  std::string fileExtensions;
};

class SyntaxHighlighter {
public:
  explicit SyntaxHighlighter(DocumentManager* docManager);
  ~SyntaxHighlighter() = default;

  // 语言管理
  void setLexer(const std::string& lexerName);
  void setLexerByFileExtension(const std::string& filePath);
  std::string getCurrentLexer() const;
  void applyCurrentDocumentHighlighting();

  // 样式设置
  void applyDefaultStyles();
  void setStyleColor(int styleId, const std::string& color);
  void setStyleFont(int styleId, const std::string& fontName, int fontSize);

  // 语言检测
  std::string detectLanguageFromContent(const std::string& content);
  std::string detectLanguageFromFileName(const std::string& fileName);

  // 可用语言列表
  const std::vector<LexerDefinition>& getAvailableLexers() const { return _availableLexers; }

  // 语法高亮控制
  void enableHighlighting(bool enable);
  bool isHighlightingEnabled() const { return _highlightingEnabled; }

private:
  DocumentManager* _docManager;
  bool _highlightingEnabled;

  std::vector<LexerDefinition> _availableLexers;
  std::map<std::string, std::string> _fileExtensionMap;

  void initializeAvailableLexers();
  void initializeFileExtensionMap();
  const LexerDefinition *findLexerDefinition(const std::string& lexerName) const;
  void setupLexer(const LexerDefinition& lexer);
  void applyLexerStyles(const LexerDefinition& lexer);
  void applyKeywordLists(const LexerDefinition& lexer);

  // 颜色定义
  static constexpr const char* DEFAULT_BACKGROUND = "#FFFFFF";
  static constexpr const char* DEFAULT_FOREGROUND = "#000000";
  static constexpr const char* COMMENT_COLOR = "#008000";
  static constexpr const char* STRING_COLOR = "#800080";
  static constexpr const char* KEYWORD_COLOR = "#0000FF";
  static constexpr const char* NUMBER_COLOR = "#FF6600";
  static constexpr const char* PREPROCESSOR_COLOR = "#800000";
  static constexpr const char* MARKDOWN_HEADER1_COLOR = "#1F4E79";
  static constexpr const char* MARKDOWN_HEADER2_COLOR = "#2F6F9F";
  static constexpr const char* MARKDOWN_HEADER3_COLOR = "#3F84B8";
  static constexpr const char* MARKDOWN_HEADER4_COLOR = "#4F97C7";
  static constexpr const char* MARKDOWN_HEADER5_COLOR = "#5FA9D4";
  static constexpr const char* MARKDOWN_HEADER6_COLOR = "#6FBADE";
  static constexpr const char* MARKDOWN_LINK_COLOR = "#0B63CE";
  static constexpr const char* MARKDOWN_QUOTE_COLOR = "#5F7F5F";
  static constexpr const char* MARKDOWN_CODE_COLOR = "#A31515";
  static constexpr const char* MARKDOWN_CODE_BACKGROUND = "#F7F3EE";
  static constexpr const char* MARKDOWN_CODEBLOCK_BACKGROUND = "#F3EFE8";
  static constexpr const char* MARKDOWN_RULE_COLOR = "#8A8A8A";
};
