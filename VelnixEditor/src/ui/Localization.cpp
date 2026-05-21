#include "ui/Localization.h"

#include <algorithm>
#include <vector>

namespace Localization {
namespace {

struct TranslationRow {
  const char *key;
  const char *en;
  const char *ja;
  const char *ko;
  const char *es;
  const char *pt;
  const char *de;
  const char *fr;
  const char *ar;
  const char *yue;
  const char *zhHans;
  const char *zhHant;
};

std::string activeLanguage = "en";
std::vector<GtkWidget *> boundWidgets;

constexpr const char *kGplNotice =
    "This program is free software: you can redistribute it and/or modify it "
    "under the terms of the GNU General Public License as published by the "
    "Free Software Foundation, either version 3 of the License, or (at your "
    "option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful, but "
    "WITHOUT ANY WARRANTY; without even the implied warranty of "
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU "
    "General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License along "
    "with this program.  If not, see <https://www.gnu.org/licenses/>.";

const std::vector<TranslationRow> kTranslations{
    {"app.name", "Velnix Editor", "Velnix Editor", "Velnix Editor", "Velnix Editor", "Velnix Editor", "Velnix Editor", "Velnix Editor", "Velnix Editor", "Velnix Editor", "Velnix Editor", "Velnix Editor"},
    {"menu.file", "File", "ファイル", "파일", "Archivo", "Arquivo", "Datei", "Fichier", "ملف", "檔案", "文件", "檔案"},
    {"menu.new", "New", "新規", "새로 만들기", "Nuevo", "Novo", "Neu", "Nouveau", "جديد", "新增", "新建", "新增"},
    {"menu.open", "Open", "開く", "열기", "Abrir", "Abrir", "Öffnen", "Ouvrir", "فتح", "開啟", "打开", "開啟"},
    {"menu.reload", "Reload", "再読み込み", "다시 불러오기", "Recargar", "Recarregar", "Neu laden", "Recharger", "إعادة تحميل", "重新載入", "重新加载", "重新載入"},
    {"menu.save", "Save", "保存", "저장", "Guardar", "Salvar", "Speichern", "Enregistrer", "حفظ", "儲存", "保存", "儲存"},
    {"menu.save_as", "Save As...", "名前を付けて保存...", "다른 이름으로 저장...", "Guardar como...", "Salvar como...", "Speichern unter...", "Enregistrer sous...", "حفظ باسم...", "另存新檔...", "另存为...", "另存新檔..."},
    {"menu.save_all", "Save All", "すべて保存", "모두 저장", "Guardar todo", "Salvar tudo", "Alle speichern", "Tout enregistrer", "حفظ الكل", "全部儲存", "全部保存", "全部儲存"},
    {"menu.close_all", "Close All", "すべて閉じる", "모두 닫기", "Cerrar todo", "Fechar tudo", "Alle schließen", "Tout fermer", "إغلاق الكل", "全部關閉", "全部关闭", "全部關閉"},
    {"menu.exit", "Exit", "終了", "종료", "Salir", "Sair", "Beenden", "Quitter", "خروج", "退出", "退出", "結束"},
    {"menu.recent_files", "Recent Files", "最近使ったファイル", "최근 파일", "Archivos recientes", "Arquivos recentes", "Zuletzt geöffnete Dateien", "Fichiers récents", "الملفات الأخيرة", "最近檔案", "最近文件", "最近檔案"},
    {"menu.recent_files_disabled", "Recent files disabled", "最近使ったファイルは無効です", "최근 파일이 꺼져 있습니다", "Archivos recientes desactivados", "Arquivos recentes desativados", "Zuletzt geöffnete Dateien deaktiviert", "Fichiers récents désactivés", "الملفات الأخيرة معطلة", "最近檔案已停用", "最近文件已禁用", "最近檔案已停用"},
    {"menu.no_recent_files", "(No recent files)", "(最近使ったファイルはありません)", "(최근 파일 없음)", "(Sin archivos recientes)", "(Sem arquivos recentes)", "(Keine zuletzt geöffneten Dateien)", "(Aucun fichier récent)", "(لا توجد ملفات أخيرة)", "(冇最近檔案)", "(无最近文件)", "(無最近檔案)"},
    {"menu.edit", "Edit", "編集", "편집", "Editar", "Editar", "Bearbeiten", "Edition", "تحرير", "編輯", "编辑", "編輯"},
    {"menu.undo", "Undo", "元に戻す", "실행 취소", "Deshacer", "Desfazer", "Rückgängig", "Annuler", "تراجع", "復原", "撤销", "復原"},
    {"menu.redo", "Redo", "やり直す", "다시 실행", "Rehacer", "Refazer", "Wiederholen", "Rétablir", "إعادة", "重做", "重做", "重做"},
    {"menu.cut", "Cut", "切り取り", "잘라내기", "Cortar", "Recortar", "Ausschneiden", "Couper", "قص", "剪下", "剪切", "剪下"},
    {"menu.copy", "Copy", "コピー", "복사", "Copiar", "Copiar", "Kopieren", "Copier", "نسخ", "複製", "复制", "複製"},
    {"menu.paste", "Paste", "貼り付け", "붙여넣기", "Pegar", "Colar", "Einfügen", "Coller", "لصق", "貼上", "粘贴", "貼上"},
    {"menu.column_editor", "Column Editor", "列エディター", "열 편집기", "Editor de columnas", "Editor de colunas", "Spalteneditor", "Editeur de colonnes", "محرر الأعمدة", "欄位編輯器", "列编辑器", "欄位編輯器"},
    {"menu.convert_case", "Convert Case", "大文字/小文字変換", "대소문자 변환", "Convertir mayúsculas/minúsculas", "Converter maiúsculas/minúsculas", "Groß-/Kleinschreibung ändern", "Changer la casse", "تغيير حالة الأحرف", "轉換大小寫", "转换大小写", "轉換大小寫"},
    {"menu.uppercase", "Uppercase", "大文字", "대문자", "Mayúsculas", "Maiúsculas", "Großbuchstaben", "Majuscules", "أحرف كبيرة", "大寫", "大写", "大寫"},
    {"menu.lowercase", "Lowercase", "小文字", "소문자", "Minúsculas", "Minúsculas", "Kleinbuchstaben", "Minuscules", "أحرف صغيرة", "細寫", "小写", "小寫"},
    {"menu.title_case", "Title Case", "タイトルケース", "제목식 대소문자", "Tipo título", "Formato de título", "Titel-Schreibweise", "Casse de titre", "حالة العنوان", "標題大小寫", "标题式大小写", "標題大小寫"},
    {"menu.blank_operations", "Blank Operations", "空白処理", "공백 작업", "Operaciones de espacios", "Operações de espaços", "Leerzeichen-Aktionen", "Opérations d'espaces", "عمليات الفراغات", "空白處理", "空白处理", "空白處理"},
    {"menu.trim_trailing_whitespace", "Trim Trailing Whitespace", "行末の空白を削除", "줄 끝 공백 제거", "Recortar espacios finales", "Remover espaços finais", "Nachgestellte Leerzeichen entfernen", "Supprimer les espaces finaux", "إزالة الفراغات النهائية", "移除行尾空白", "移除行尾空白", "移除行尾空白"},
    {"menu.spaces_to_tabs", "Spaces to Tabs", "スペースをタブに変換", "공백을 탭으로", "Espacios a tabuladores", "Espaços para tabulações", "Leerzeichen zu Tabs", "Espaces en tabulations", "مسافات إلى علامات تبويب", "空格轉 Tab", "空格转制表符", "空格轉定位字元"},
    {"menu.tabs_to_spaces", "Tabs to Spaces", "タブをスペースに変換", "탭을 공백으로", "Tabuladores a espacios", "Tabulações para espaços", "Tabs zu Leerzeichen", "Tabulations en espaces", "علامات تبويب إلى مسافات", "Tab 轉空格", "制表符转空格", "定位字元轉空格"},
    {"menu.search", "Search", "検索", "검색", "Buscar", "Pesquisar", "Suchen", "Rechercher", "بحث", "搜尋", "搜索", "搜尋"},
    {"menu.find", "Find", "検索", "찾기", "Buscar", "Localizar", "Suchen", "Rechercher", "بحث", "尋找", "查找", "尋找"},
    {"menu.replace", "Replace", "置換", "바꾸기", "Reemplazar", "Substituir", "Ersetzen", "Remplacer", "استبدال", "取代", "替换", "取代"},
    {"menu.go_to", "Go To...", "移動...", "이동...", "Ir a...", "Ir para...", "Gehe zu...", "Aller à...", "انتقال إلى...", "前往...", "转到...", "前往..."},
    {"menu.view", "View", "表示", "보기", "Ver", "Exibir", "Ansicht", "Affichage", "عرض", "檢視", "视图", "檢視"},
    {"menu.word_wrap", "Word Wrap", "折り返し", "자동 줄 바꿈", "Ajuste de línea", "Quebra de linha", "Zeilenumbruch", "Retour à la ligne", "التفاف النص", "自動換行", "自动换行", "自動換行"},
    {"menu.full_screen", "Full Screen Mode", "全画面モード", "전체 화면 모드", "Modo pantalla completa", "Modo tela cheia", "Vollbildmodus", "Mode plein écran", "وضع ملء الشاشة", "全螢幕模式", "全屏模式", "全螢幕模式"},
    {"menu.show_symbols", "Show Symbols", "記号を表示", "기호 표시", "Mostrar símbolos", "Mostrar símbolos", "Symbole anzeigen", "Afficher les symboles", "إظهار الرموز", "顯示符號", "显示符号", "顯示符號"},
    {"menu.show_spaces_tabs", "Show Spaces and Tabs", "スペースとタブを表示", "공백과 탭 표시", "Mostrar espacios y tabuladores", "Mostrar espaços e tabulações", "Leerzeichen und Tabs anzeigen", "Afficher espaces et tabulations", "إظهار المسافات وعلامات التبويب", "顯示空格同 Tab", "显示空格和制表符", "顯示空格和定位字元"},
    {"menu.show_eol", "Show End-of-Line (CRLF)", "改行記号を表示 (CRLF)", "줄 끝 표시 (CRLF)", "Mostrar fin de línea (CRLF)", "Mostrar fim de linha (CRLF)", "Zeilenende anzeigen (CRLF)", "Afficher les fins de ligne (CRLF)", "إظهار نهايات الأسطر (CRLF)", "顯示行尾 (CRLF)", "显示行尾 (CRLF)", "顯示行尾 (CRLF)"},
    {"menu.zoom", "Zoom", "ズーム", "확대/축소", "Zoom", "Zoom", "Zoom", "Zoom", "تكبير", "縮放", "缩放", "縮放"},
    {"menu.zoom_in", "Zoom In", "拡大", "확대", "Acercar", "Ampliar", "Vergrößern", "Zoom avant", "تكبير", "放大", "放大", "放大"},
    {"menu.zoom_out", "Zoom Out", "縮小", "축소", "Alejar", "Reduzir", "Verkleinern", "Zoom arrière", "تصغير", "縮細", "缩小", "縮小"},
    {"menu.reset_zoom", "Reset Zoom", "ズームをリセット", "확대/축소 초기화", "Restablecer zoom", "Redefinir zoom", "Zoom zurücksetzen", "Réinitialiser le zoom", "إعادة ضبط التكبير", "重設縮放", "重置缩放", "重設縮放"},
    {"menu.search_results", "Search Results", "検索結果", "검색 결과", "Resultados de búsqueda", "Resultados da pesquisa", "Suchergebnisse", "Résultats de recherche", "نتائج البحث", "搜尋結果", "搜索结果", "搜尋結果"},
    {"menu.encoding", "Encoding", "エンコーディング", "인코딩", "Codificación", "Codificação", "Kodierung", "Encodage", "الترميز", "編碼", "编码", "編碼"},
    {"menu.convert_ansi", "Convert to ANSI", "ANSI に変換", "ANSI로 변환", "Convertir a ANSI", "Converter para ANSI", "In ANSI konvertieren", "Convertir en ANSI", "تحويل إلى ANSI", "轉換為 ANSI", "转换为 ANSI", "轉換為 ANSI"},
    {"menu.convert_utf8", "Convert to UTF-8", "UTF-8 に変換", "UTF-8로 변환", "Convertir a UTF-8", "Converter para UTF-8", "In UTF-8 konvertieren", "Convertir en UTF-8", "تحويل إلى UTF-8", "轉換為 UTF-8", "转换为 UTF-8", "轉換為 UTF-8"},
    {"menu.convert_utf8_bom", "Convert to UTF-8 BOM", "UTF-8 BOM に変換", "UTF-8 BOM으로 변환", "Convertir a UTF-8 BOM", "Converter para UTF-8 BOM", "In UTF-8 BOM konvertieren", "Convertir en UTF-8 BOM", "تحويل إلى UTF-8 BOM", "轉換為 UTF-8 BOM", "转换为 UTF-8 BOM", "轉換為 UTF-8 BOM"},
    {"menu.convert_utf16_le", "Convert to UTF-16 LE", "UTF-16 LE に変換", "UTF-16 LE로 변환", "Convertir a UTF-16 LE", "Converter para UTF-16 LE", "In UTF-16 LE konvertieren", "Convertir en UTF-16 LE", "تحويل إلى UTF-16 LE", "轉換為 UTF-16 LE", "转换为 UTF-16 LE", "轉換為 UTF-16 LE"},
    {"menu.convert_utf16_be", "Convert to UTF-16 BE", "UTF-16 BE に変換", "UTF-16 BE로 변환", "Convertir a UTF-16 BE", "Converter para UTF-16 BE", "In UTF-16 BE konvertieren", "Convertir en UTF-16 BE", "تحويل إلى UTF-16 BE", "轉換為 UTF-16 BE", "转换为 UTF-16 BE", "轉換為 UTF-16 BE"},
    {"menu.language", "Language", "言語", "언어", "Lenguaje", "Linguagem", "Sprache", "Langage", "اللغة", "語言", "语言", "語言"},
    {"menu.settings", "Settings", "設定", "설정", "Configuración", "Configurações", "Einstellungen", "Paramètres", "الإعدادات", "設定", "设置", "設定"},
    {"menu.preferences", "Preferences...", "環境設定...", "환경 설정...", "Preferencias...", "Preferências...", "Einstellungen...", "Préférences...", "التفضيلات...", "偏好設定...", "偏好设置...", "偏好設定..."},
    {"menu.keyboard_shortcuts", "Keyboard Shortcuts...", "キーボードショートカット...", "키보드 단축키...", "Atajos de teclado...", "Atalhos de teclado...", "Tastenkürzel...", "Raccourcis clavier...", "اختصارات لوحة المفاتيح...", "鍵盤快捷鍵...", "键盘快捷键...", "鍵盤快速鍵..."},
    {"menu.macro", "Macro", "マクロ", "매크로", "Macro", "Macro", "Makro", "Macro", "ماكرو", "巨集", "宏", "巨集"},
    {"menu.start_recording", "Start Recording", "記録開始", "기록 시작", "Iniciar grabación", "Iniciar gravação", "Aufzeichnung starten", "Démarrer l'enregistrement", "بدء التسجيل", "開始錄製", "开始录制", "開始錄製"},
    {"menu.stop_recording", "Stop Recording", "記録停止", "기록 중지", "Detener grabación", "Parar gravação", "Aufzeichnung stoppen", "Arrêter l'enregistrement", "إيقاف التسجيل", "停止錄製", "停止录制", "停止錄製"},
    {"menu.play_macro", "Play Macro...", "マクロを再生...", "매크로 실행...", "Reproducir macro...", "Executar macro...", "Makro abspielen...", "Lire la macro...", "تشغيل ماكرو...", "播放巨集...", "播放宏...", "播放巨集..."},
    {"menu.macro_management", "Macro Management...", "マクロ管理...", "매크로 관리...", "Gestión de macros...", "Gerenciamento de macros...", "Makroverwaltung...", "Gestion des macros...", "إدارة وحدات الماكرو...", "巨集管理...", "宏管理...", "巨集管理..."},
    {"menu.save_current_macro", "Save Current Macro", "現在のマクロを保存", "현재 매크로 저장", "Guardar macro actual", "Salvar macro atual", "Aktuelles Makro speichern", "Enregistrer la macro actuelle", "حفظ الماكرو الحالي", "儲存目前巨集", "保存当前宏", "儲存目前巨集"},
    {"menu.plugins", "Plugins", "プラグイン", "플러그인", "Plugins", "Plugins", "Plugins", "Extensions", "الإضافات", "外掛", "插件", "外掛"},
    {"menu.help", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?", "?"},
    {"menu.about", "About Velnix Editor", "Velnix Editor について", "Velnix Editor 정보", "Acerca de Velnix Editor", "Sobre Velnix Editor", "Info zu Velnix Editor", "À propos de Velnix Editor", "حول Velnix Editor", "關於 Velnix Editor", "关于 Velnix Editor", "關於 Velnix Editor"},
    {"about.build", "Build:", "ビルド:", "빌드:", "Compilación:", "Build:", "Build:", "Compilation :", "البناء:", "建置：", "构建：", "建置："},
    {"about.version", "Version", "バージョン", "버전", "Versión", "Versão", "Version", "Version", "الإصدار", "版本", "版本", "版本"},
    {"about.home", "Home:", "ホーム:", "홈:", "Inicio:", "Início:", "Startseite:", "Accueil :", "الرئيسية:", "主頁：", "主页：", "首頁："},
    {"about.license_title", "GNU General Public Licence", "GNU General Public Licence", "GNU General Public Licence", "GNU General Public Licence", "GNU General Public Licence", "GNU General Public Licence", "GNU General Public Licence", "GNU General Public Licence", "GNU General Public Licence", "GNU General Public Licence", "GNU General Public Licence"},
    {"about.gpl_notice", kGplNotice, kGplNotice, kGplNotice, kGplNotice, kGplNotice, kGplNotice, kGplNotice, kGplNotice, kGplNotice, kGplNotice, kGplNotice},
    {"tab.display", "Display", "表示", "显示", "Pantalla", "Exibição", "Anzeige", "Affichage", "العرض", "顯示", "显示", "顯示"},
    {"tab.syntax", "Syntax", "構文", "구문", "Sintaxis", "Sintaxe", "Syntax", "Syntaxe", "الصياغة", "語法", "语法", "語法"},
    {"tab.files", "Files", "ファイル", "파일", "Archivos", "Arquivos", "Dateien", "Fichiers", "الملفات", "檔案", "文件", "檔案"},
    {"tab.editor", "Editor", "エディター", "편집기", "Editor", "Editor", "Editor", "Editeur", "المحرر", "編輯器", "编辑器", "編輯器"},
    {"tab.indentation", "Indentation", "インデント", "들여쓰기", "Sangría", "Indentação", "Einrückung", "Indentation", "المسافة البادئة", "縮排", "缩进", "縮排"},
    {"tab.search", "Search", "検索", "검색", "Buscar", "Pesquisar", "Suchen", "Rechercher", "بحث", "搜尋", "搜索", "搜尋"},
    {"tab.config", "Config", "設定", "설정", "Configuración", "Configuração", "Konfiguration", "Configuration", "التكوين", "設定", "配置", "設定"},
    {"pref.enable_recent_files", "Enable Recent Files", "最近使ったファイルを有効にする", "최근 파일 사용", "Activar archivos recientes", "Ativar arquivos recentes", "Zuletzt geöffnete Dateien aktivieren", "Activer les fichiers récents", "تفعيل الملفات الأخيرة", "啟用最近檔案", "启用最近文件", "啟用最近檔案"},
    {"pref.interface_language", "Interface language:", "インターフェイス言語:", "인터페이스 언어:", "Idioma de la interfaz:", "Idioma da interface:", "Oberflächensprache:", "Langue de l'interface :", "لغة الواجهة:", "介面語言：", "界面语言：", "介面語言："},
    {"pref.show_line_numbers", "Show Line Numbers", "行番号を表示", "줄 번호 표시", "Mostrar números de línea", "Mostrar números de linha", "Zeilennummern anzeigen", "Afficher les numéros de ligne", "إظهار أرقام الأسطر", "顯示行號", "显示行号", "顯示行號"},
    {"pref.recent_files_count", "Recent files count:", "最近使ったファイル数:", "최근 파일 수:", "Cantidad de archivos recientes:", "Quantidade de arquivos recentes:", "Anzahl zuletzt geöffneter Dateien:", "Nombre de fichiers récents :", "عدد الملفات الأخيرة:", "最近檔案數量：", "最近文件数量：", "最近檔案數量："},
    {"pref.config_directory", "Config directory:", "設定ディレクトリ:", "설정 디렉터리:", "Directorio de configuración:", "Diretório de configuração:", "Konfigurationsordner:", "Dossier de configuration :", "مجلد الإعدادات:", "設定目錄：", "配置目录：", "設定目錄："},
    {"pref.enable_word_wrap", "Enable Word Wrap", "折り返しを有効にする", "자동 줄 바꿈 사용", "Activar ajuste de línea", "Ativar quebra de linha", "Zeilenumbruch aktivieren", "Activer le retour à la ligne", "تفعيل التفاف النص", "啟用自動換行", "启用自动换行", "啟用自動換行"},
    {"pref.line_wrap_mode", "Line wrap mode:", "折り返しモード:", "줄 바꿈 모드:", "Modo de ajuste:", "Modo de quebra:", "Zeilenumbruchmodus:", "Mode de retour à la ligne :", "وضع التفاف السطر:", "換行模式：", "换行模式：", "換行模式："},
    {"pref.wrap_off", "Off", "オフ", "끄기", "Desactivado", "Desativado", "Aus", "Désactivé", "إيقاف", "關閉", "关闭", "關閉"},
    {"pref.wrap_window", "Window", "ウィンドウ", "창", "Ventana", "Janela", "Fenster", "Fenêtre", "النافذة", "視窗", "窗口", "視窗"},
    {"pref.wrap_word", "Word", "単語", "단어", "Palabra", "Palavra", "Wort", "Mot", "كلمة", "單詞", "单词", "單字"},
    {"pref.font_family", "Font family:", "フォントファミリー:", "글꼴:", "Familia de fuente:", "Família da fonte:", "Schriftfamilie:", "Famille de police :", "عائلة الخط:", "字型：", "字体：", "字型："},
    {"pref.font_size", "Font size:", "フォントサイズ:", "글꼴 크기:", "Tamaño de fuente:", "Tamanho da fonte:", "Schriftgröße:", "Taille de police :", "حجم الخط:", "字體大小：", "字体大小：", "字體大小："},
    {"pref.show_whitespace", "Show whitespace markers", "空白記号を表示", "공백 표시", "Mostrar marcas de espacios", "Mostrar marcadores de espaços", "Leerzeichenmarkierungen anzeigen", "Afficher les marques d'espaces", "إظهار علامات الفراغ", "顯示空白標記", "显示空白标记", "顯示空白標記"},
    {"pref.show_eol", "Show end-of-line markers", "改行記号を表示", "줄 끝 표시", "Mostrar marcas de fin de línea", "Mostrar marcadores de fim de linha", "Zeilenendemarkierungen anzeigen", "Afficher les marques de fin de ligne", "إظهار علامات نهاية السطر", "顯示行尾標記", "显示行尾标记", "顯示行尾標記"},
    {"pref.highlight_current_line", "Highlight current line", "現在行を強調表示", "현재 줄 강조", "Resaltar línea actual", "Realçar linha atual", "Aktuelle Zeile hervorheben", "Surligner la ligne actuelle", "تمييز السطر الحالي", "醒目顯示目前行", "高亮当前行", "醒目顯示目前行"},
    {"pref.show_current_column", "Show current column guide", "現在列ガイドを表示", "현재 열 가이드 표시", "Mostrar guía de columna actual", "Mostrar guia da coluna atual", "Aktuelle Spaltenführung anzeigen", "Afficher le guide de colonne actuelle", "إظهار دليل العمود الحالي", "顯示目前欄位參考線", "显示当前列参考线", "顯示目前欄位參考線"},
    {"pref.enable_ctrl_wheel_zoom", "Enable Ctrl+mouse wheel zoom", "Ctrl+マウスホイールズームを有効にする", "Ctrl+마우스 휠 확대/축소 사용", "Activar zoom con Ctrl+rueda", "Ativar zoom com Ctrl+roda", "Strg+Mausrad-Zoom aktivieren", "Activer le zoom Ctrl+molette", "تفعيل التكبير عبر Ctrl+عجلة الفأرة", "啟用 Ctrl+滾輪縮放", "启用 Ctrl+滚轮缩放", "啟用 Ctrl+滾輪縮放"},
    {"pref.smart_keyword_highlighting", "Enable smart keyword highlighting", "スマートキーワード強調表示を有効にする", "스마트 키워드 강조 사용", "Activar resaltado inteligente de palabras clave", "Ativar realce inteligente de palavras-chave", "Intelligente Schlüsselworthervorhebung aktivieren", "Activer la mise en évidence intelligente des mots-clés", "تفعيل تمييز الكلمات المفتاحية الذكي", "啟用智能關鍵字醒目提示", "启用智能关键词高亮", "啟用智慧關鍵字醒目提示"},
    {"pref.trim_trailing_whitespace_on_save", "Trim trailing whitespace on save", "保存時に行末の空白を削除", "저장 시 줄 끝 공백 제거", "Recortar espacios finales al guardar", "Remover espaços finais ao salvar", "Nachgestellte Leerzeichen beim Speichern entfernen", "Supprimer les espaces finaux à l'enregistrement", "إزالة الفراغات النهائية عند الحفظ", "儲存時移除行尾空白", "保存时移除行尾空白", "儲存時移除行尾空白"},
    {"pref.ensure_final_newline", "Ensure final newline on save", "保存時に末尾改行を保証", "저장 시 마지막 줄바꿈 보장", "Asegurar salto final al guardar", "Garantir nova linha final ao salvar", "Abschließenden Zeilenumbruch beim Speichern sicherstellen", "Assurer une ligne finale à l'enregistrement", "ضمان سطر جديد أخير عند الحفظ", "儲存時確保結尾換行", "保存时确保末尾换行", "儲存時確保結尾換行"},
    {"pref.show_indent_guides", "Show indent guides", "インデントガイドを表示", "들여쓰기 가이드 표시", "Mostrar guías de sangría", "Mostrar guias de indentação", "Einrückungshilfen anzeigen", "Afficher les guides d'indentation", "إظهار أدلة المسافة البادئة", "顯示縮排參考線", "显示缩进参考线", "顯示縮排參考線"},
    {"pref.long_line_column", "Long line guide column:", "長い行のガイド列:", "긴 줄 가이드 열:", "Columna guía de línea larga:", "Coluna guia de linha longa:", "Spalte für lange Zeilen:", "Colonne guide de ligne longue :", "عمود دليل السطر الطويل:", "長行參考欄：", "长行参考列：", "長行參考欄："},
    {"pref.highlight_right_margin", "Highlight text beyond right margin", "右マージンを超えたテキストを強調表示", "오른쪽 여백 밖 텍스트 강조", "Resaltar texto más allá del margen derecho", "Realçar texto além da margem direita", "Text hinter rechtem Rand hervorheben", "Surligner le texte au-delà de la marge droite", "تمييز النص بعد الهامش الأيمن", "醒目顯示超出右邊界文字", "高亮超出右边距的文本", "醒目顯示超出右邊界文字"},
    {"pref.scroll_past_last_line", "Scroll past last line", "最終行を越えてスクロール", "마지막 줄 이후 스크롤", "Desplazar más allá de la última línea", "Rolar além da última linha", "Über die letzte Zeile hinaus scrollen", "Défiler après la dernière ligne", "التمرير بعد السطر الأخير", "可捲過最後一行", "可滚动超过最后一行", "可捲過最後一行"},
    {"pref.highlight_matching_braces", "Highlight matching braces", "対応する括弧を強調表示", "짝이 맞는 괄호 강조", "Resaltar llaves coincidentes", "Realçar chaves correspondentes", "Passende Klammern hervorheben", "Surligner les accolades correspondantes", "تمييز الأقواس المتطابقة", "醒目顯示配對括號", "高亮匹配括号", "醒目顯示配對括號"},
    {"pref.enable_syntax_highlighting", "Enable syntax highlighting globally", "構文強調を全体で有効にする", "구문 강조 전역 사용", "Activar resaltado de sintaxis globalmente", "Ativar realce de sintaxe globalmente", "Syntaxhervorhebung global aktivieren", "Activer la coloration syntaxique globalement", "تفعيل تمييز الصياغة عامًّا", "全域啟用語法醒目提示", "全局启用语法高亮", "全域啟用語法醒目提示"},
    {"pref.default_lexer", "Default lexer for new documents:", "新規文書の既定 lexer:", "새 문서 기본 렉서:", "Lexer predeterminado para documentos nuevos:", "Lexer padrão para novos documentos:", "Standard-Lexer für neue Dokumente:", "Lexer par défaut pour les nouveaux documents :", "المحلل الافتراضي للمستندات الجديدة:", "新文件預設 lexer：", "新文档默认 lexer：", "新文件預設 lexer："},
    {"pref.default_lexer_note", "Applies only to new documents and untitled buffers.", "新規文書と無題バッファーにのみ適用されます。", "새 문서와 제목 없는 버퍼에만 적용됩니다.", "Solo se aplica a documentos nuevos y búferes sin título.", "Aplica-se apenas a novos documentos e buffers sem título.", "Gilt nur für neue Dokumente und unbenannte Puffer.", "S'applique uniquement aux nouveaux documents et tampons sans titre.", "ينطبق فقط على المستندات الجديدة والمخازن المؤقتة بلا عنوان.", "只適用於新文件同未命名緩衝區。", "仅适用于新文档和未命名缓冲区。", "只適用於新文件與未命名緩衝區。"},
    {"pref.enable_auto_save", "Enable auto-save", "自動保存を有効にする", "자동 저장 사용", "Activar guardado automático", "Ativar salvamento automático", "Automatisches Speichern aktivieren", "Activer l'enregistrement automatique", "تفعيل الحفظ التلقائي", "啟用自動儲存", "启用自动保存", "啟用自動儲存"},
    {"pref.auto_save_interval", "Auto-save interval (s):", "自動保存間隔 (秒):", "자동 저장 간격(초):", "Intervalo de guardado automático (s):", "Intervalo de salvamento automático (s):", "Intervall für automatisches Speichern (s):", "Intervalle d'enregistrement auto (s) :", "فاصل الحفظ التلقائي (ث):", "自動儲存間隔（秒）：", "自动保存间隔（秒）：", "自動儲存間隔（秒）："},
    {"pref.overwrite_readonly", "Overwrite read-only files", "読み取り専用ファイルを上書き", "읽기 전용 파일 덮어쓰기", "Sobrescribir archivos de solo lectura", "Sobrescrever arquivos somente leitura", "Schreibgeschützte Dateien überschreiben", "Écraser les fichiers en lecture seule", "استبدال الملفات للقراءة فقط", "覆寫唯讀檔案", "覆盖只读文件", "覆寫唯讀檔案"},
    {"pref.tab_size", "Tab size:", "タブ幅:", "탭 크기:", "Tamaño de tabulación:", "Tamanho da tabulação:", "Tabulatorgröße:", "Taille des tabulations :", "حجم علامة التبويب:", "Tab 大小：", "制表符大小：", "定位字元大小："},
    {"pref.use_spaces_for_tabs", "Use spaces for tabs", "タブにスペースを使用", "탭 대신 공백 사용", "Usar espacios para tabuladores", "Usar espaços para tabulações", "Leerzeichen statt Tabs verwenden", "Utiliser des espaces pour les tabulations", "استخدام مسافات بدلاً من علامات التبويب", "用空格代替 Tab", "使用空格代替制表符", "使用空格代替定位字元"},
    {"pref.auto_indent", "Auto indent", "自動インデント", "자동 들여쓰기", "Sangría automática", "Indentação automática", "Automatische Einrückung", "Indentation automatique", "مسافة بادئة تلقائية", "自動縮排", "自动缩进", "自動縮排"},
    {"pref.search_wrap_around", "Search wrap around", "折り返して検索", "순환 검색", "Buscar con vuelta", "Pesquisa circular", "Suche umbrechen", "Recherche circulaire", "البحث من البداية عند النهاية", "循環搜尋", "循环搜索", "循環搜尋"},
    {"pref.search_all_documents", "Search all open documents", "開いているすべての文書を検索", "열린 모든 문서 검색", "Buscar en todos los documentos abiertos", "Pesquisar em todos os documentos abertos", "Alle geöffneten Dokumente durchsuchen", "Rechercher dans tous les documents ouverts", "البحث في كل المستندات المفتوحة", "搜尋所有開啟文件", "搜索所有打开的文档", "搜尋所有開啟文件"},
    {"pref.shortcuts_file", "Shortcuts file:", "ショートカットファイル:", "단축키 파일:", "Archivo de atajos:", "Arquivo de atalhos:", "Tastenkürzeldatei:", "Fichier des raccourcis :", "ملف الاختصارات:", "快捷鍵檔案：", "快捷键文件：", "快速鍵檔案："},
    {"pref.recent_files_file", "Recent files file:", "最近使ったファイル一覧:", "최근 파일 파일:", "Archivo de recientes:", "Arquivo de recentes:", "Datei zuletzt geöffneter Dateien:", "Fichier des fichiers récents :", "ملف الملفات الأخيرة:", "最近檔案列表：", "最近文件列表：", "最近檔案列表："},
    {"dialog.preferences", "Preferences", "環境設定", "환경 설정", "Preferencias", "Preferências", "Einstellungen", "Préférences", "التفضيلات", "偏好設定", "偏好设置", "偏好設定"},
    {"dialog.cancel", "_Cancel", "_キャンセル", "_취소", "_Cancelar", "_Cancelar", "_Abbrechen", "_Annuler", "_إلغاء", "_取消", "_取消", "_取消"},
    {"dialog.ok", "_OK", "_OK", "_확인", "_Aceptar", "_OK", "_OK", "_OK", "_موافق", "_確定", "_确定", "_確定"},
    {"tooltip.close", "Close", "閉じる", "닫기", "Cerrar", "Fechar", "Schließen", "Fermer", "إغلاق", "關閉", "关闭", "關閉"},
    {"tooltip.maximize", "Maximize", "最大化", "최대화", "Maximizar", "Maximizar", "Maximieren", "Agrandir", "تكبير", "最大化", "最大化", "最大化"},
    {"tooltip.restore", "Restore", "元に戻す", "복원", "Restaurar", "Restaurar", "Wiederherstellen", "Restaurer", "استعادة", "還原", "还原", "還原"},
    {"about.comments", "Velnix Editor is a lightweight Linux code editor built with GTK and Scintilla.", "Velnix Editor は GTK と Scintilla で作られた軽量 Linux コードエディターです。", "Velnix Editor는 GTK와 Scintilla로 만든 가벼운 Linux 코드 편집기입니다.", "Velnix Editor es un editor de código ligero para Linux creado con GTK y Scintilla.", "Velnix Editor é um editor de código Linux leve criado com GTK e Scintilla.", "Velnix Editor ist ein leichter Linux-Codeeditor, gebaut mit GTK und Scintilla.", "Velnix Editor est un éditeur de code Linux léger créé avec GTK et Scintilla.", "Velnix Editor هو محرر شيفرة خفيف للينكس مبني باستخدام GTK و Scintilla.", "Velnix Editor 係用 GTK 同 Scintilla 製作嘅輕量 Linux 程式碼編輯器。", "Velnix Editor 是使用 GTK 和 Scintilla 构建的轻量级 Linux 代码编辑器。", "Velnix Editor 是使用 GTK 與 Scintilla 建置的輕量級 Linux 程式碼編輯器。"},
    {"dialog.close", "_Close", "_閉じる", "_닫기", "_Cerrar", "_Fechar", "_Schließen", "_Fermer", "_إغلاق", "_關閉", "_关闭", "_關閉"},
    {"dialog.save", "_Save", "_保存", "_저장", "_Guardar", "_Salvar", "_Speichern", "_Enregistrer", "_حفظ", "_儲存", "_保存", "_儲存"},
    {"dialog.open", "_Open", "_開く", "_열기", "_Abrir", "_Abrir", "_Öffnen", "_Ouvrir", "_فتح", "_開啟", "_打开", "_開啟"},
    {"dialog.go", "_Go", "_移動", "_이동", "_Ir", "_Ir", "_Gehe zu", "_Aller", "_انتقال", "_前往", "_转到", "_前往"},
    {"dialog.find_next", "_Find Next", "_次を検索", "_다음 찾기", "_Buscar siguiente", "_Localizar próximo", "_Weitersuchen", "_Suivant", "_بحث التالي", "_尋找下一個", "_查找下一个", "_尋找下一個"},
    {"dialog.find_all", "Find All", "すべて検索", "모두 찾기", "Buscar todo", "Localizar tudo", "Alle suchen", "Tout rechercher", "بحث الكل", "尋找全部", "查找全部", "尋找全部"},
    {"dialog.replace", "_Replace", "_置換", "_바꾸기", "_Reemplazar", "_Substituir", "_Ersetzen", "_Remplacer", "_استبدال", "_取代", "_替换", "_取代"},
    {"dialog.replace_all", "Replace _All", "すべて置換", "모두 바꾸기", "Reemplazar _todo", "Substituir _tudo", "_Alle ersetzen", "Tout _remplacer", "استبدال _الكل", "全部_取代", "全部_替换", "全部_取代"},
    {"dialog.dont_save", "_Don't Save", "_保存しない", "_저장 안 함", "_No guardar", "_Não salvar", "_Nicht speichern", "Ne _pas enregistrer", "_عدم الحفظ", "_不儲存", "_不保存", "_不儲存"},
    {"dialog.reload", "_Reload", "_再読み込み", "_다시 불러오기", "_Recargar", "_Recarregar", "_Neu laden", "_Recharger", "_إعادة تحميل", "_重新載入", "_重新加载", "_重新載入"},
    {"dialog.ignore", "_Ignore", "_無視", "_무시", "_Ignorar", "_Ignorar", "_Ignorieren", "_Ignorer", "_تجاهل", "_忽略", "_忽略", "_忽略"},
    {"dialog.keep_open", "_Keep Open", "_開いたまま", "_열어 두기", "_Mantener abierto", "_Manter aberto", "_Offen halten", "_Garder ouvert", "_إبقاء مفتوحًا", "_保持開啟", "_保持打开", "_保持開啟"},
    {"dialog.close_buffer", "_Close Buffer", "_バッファーを閉じる", "_버퍼 닫기", "_Cerrar búfer", "_Fechar buffer", "_Puffer schließen", "_Fermer le tampon", "_إغلاق المخزن", "_關閉緩衝區", "_关闭缓冲区", "_關閉緩衝區"},
    {"search.go_to_line", "Go To Line", "行へ移動", "줄로 이동", "Ir a línea", "Ir para linha", "Gehe zu Zeile", "Aller à la ligne", "انتقال إلى السطر", "前往行", "转到行", "前往行"},
    {"search.line_number", "Line number:", "行番号:", "줄 번호:", "Número de línea:", "Número da linha:", "Zeilennummer:", "Numéro de ligne :", "رقم السطر:", "行號：", "行号：", "行號："},
    {"search.find_title", "Find", "検索", "찾기", "Buscar", "Localizar", "Suchen", "Rechercher", "بحث", "尋找", "查找", "尋找"},
    {"search.replace_title", "Replace", "置換", "바꾸기", "Reemplazar", "Substituir", "Ersetzen", "Remplacer", "استبدال", "取代", "替换", "取代"},
    {"search.find_what", "Find what:", "検索する文字列:", "찾을 내용:", "Buscar:", "Localizar:", "Suchen nach:", "Rechercher :", "البحث عن:", "尋找：", "查找内容：", "尋找："},
    {"search.replace_with", "Replace with:", "置換後:", "바꿀 내용:", "Reemplazar con:", "Substituir por:", "Ersetzen durch:", "Remplacer par :", "استبدال بـ:", "取代為：", "替换为：", "取代為："},
    {"search.case_sensitive", "Case sensitive", "大文字/小文字を区別", "대소문자 구분", "Distinguir mayúsculas", "Diferenciar maiúsculas", "Groß-/Kleinschreibung beachten", "Respecter la casse", "مطابقة حالة الأحرف", "區分大小寫", "区分大小写", "區分大小寫"},
    {"search.whole_word", "Whole word", "単語単位", "전체 단어", "Palabra completa", "Palavra inteira", "Ganzes Wort", "Mot entier", "كلمة كاملة", "全字匹配", "全字匹配", "全字匹配"},
    {"search.regex", "Regular expression", "正規表現", "정규식", "Expresión regular", "Expressão regular", "Regulärer Ausdruck", "Expression régulière", "تعبير نمطي", "正則表達式", "正则表达式", "正規表示式"},
    {"search.wrap_around", "Wrap around", "折り返す", "순환", "Dar la vuelta", "Voltar ao início", "Umbrechen", "Recherche circulaire", "التفاف البحث", "循環", "循环", "循環"},
    {"search.all_open_documents", "Search all open documents", "開いているすべての文書を検索", "열린 모든 문서 검색", "Buscar en todos los documentos abiertos", "Pesquisar em todos os documentos abertos", "Alle geöffneten Dokumente durchsuchen", "Rechercher dans tous les documents ouverts", "البحث في كل المستندات المفتوحة", "搜尋所有開啟文件", "搜索所有打开的文档", "搜尋所有開啟文件"},
    {"search.auto_refresh", "Auto Refresh", "自動更新", "자동 새로고침", "Actualización automática", "Atualização automática", "Automatisch aktualisieren", "Actualisation auto", "تحديث تلقائي", "自動更新", "自动刷新", "自動重新整理"},
    {"search.refresh", "Refresh", "更新", "새로고침", "Actualizar", "Atualizar", "Aktualisieren", "Actualiser", "تحديث", "更新", "刷新", "重新整理"},
    {"search.expand", "Expand", "展開", "펼치기", "Expandir", "Expandir", "Erweitern", "Développer", "توسيع", "展開", "展开", "展開"},
    {"search.collapse", "Collapse", "折りたたむ", "접기", "Contraer", "Recolher", "Einklappen", "Réduire", "طي", "收合", "折叠", "收合"},
    {"search.clear", "Clear", "クリア", "지우기", "Limpiar", "Limpar", "Leeren", "Effacer", "مسح", "清除", "清除", "清除"},
    {"search.results_cleared", "Search results cleared.", "検索結果をクリアしました。", "검색 결과가 지워졌습니다.", "Resultados de búsqueda borrados.", "Resultados da pesquisa limpos.", "Suchergebnisse gelöscht.", "Résultats effacés.", "تم مسح نتائج البحث.", "搜尋結果已清除。", "搜索结果已清除。", "搜尋結果已清除。"},
    {"search.no_refresh", "No search results to refresh.", "更新する検索結果はありません。", "새로고칠 검색 결과가 없습니다.", "No hay resultados para actualizar.", "Não há resultados para atualizar.", "Keine Suchergebnisse zum Aktualisieren.", "Aucun résultat à actualiser.", "لا توجد نتائج لتحديثها.", "冇搜尋結果可更新。", "没有可刷新的搜索结果。", "沒有可重新整理的搜尋結果。"},
    {"search.no_query", "No search query to run.", "実行する検索クエリはありません。", "실행할 검색어가 없습니다.", "No hay búsqueda para ejecutar.", "Não há pesquisa para executar.", "Keine Suchanfrage vorhanden.", "Aucune recherche à lancer.", "لا يوجد استعلام بحث.", "冇搜尋查詢可執行。", "没有可执行的搜索查询。", "沒有可執行的搜尋查詢。"},
    {"search.no_document", "No document available.", "利用可能な文書がありません。", "사용 가능한 문서가 없습니다.", "No hay documento disponible.", "Nenhum documento disponível.", "Kein Dokument verfügbar.", "Aucun document disponible.", "لا يوجد مستند متاح.", "冇可用文件。", "没有可用文档。", "沒有可用文件。"},
    {"search.searching", "Searching in background...", "バックグラウンドで検索中...", "백그라운드에서 검색 중...", "Buscando en segundo plano...", "Pesquisando em segundo plano...", "Suche im Hintergrund...", "Recherche en arrière-plan...", "جار البحث في الخلفية...", "背景搜尋中...", "正在后台搜索...", "背景搜尋中..."},
    {"search.refreshing", "Refreshing search groups in background...", "検索グループをバックグラウンドで更新中...", "백그라운드에서 검색 그룹 새로고침 중...", "Actualizando grupos en segundo plano...", "Atualizando grupos em segundo plano...", "Suchgruppen werden aktualisiert...", "Actualisation des groupes en arrière-plan...", "جار تحديث مجموعات البحث...", "背景更新搜尋群組...", "正在后台刷新搜索组...", "背景重新整理搜尋群組..."},
    {"search.removed_prefix", "Removed ", "削除しました: ", "제거됨: ", "Eliminados ", "Removidos ", "Entfernt: ", "Supprimé : ", "تمت إزالة ", "已移除 ", "已移除 ", "已移除 "},
    {"search.removed_suffix", " selected item(s).", " 個の選択項目。", "개 선택 항목.", " elemento(s) seleccionados.", " item(ns) selecionado(s).", " ausgewählte Elemente.", " élément(s) sélectionné(s).", " عنصرًا محددًا.", " 個選取項目。", " 个选中项。", " 個選取項目。"},
    {"search.none_removed", "No search result removed.", "削除された検索結果はありません。", "제거된 검색 결과가 없습니다.", "No se eliminó ningún resultado.", "Nenhum resultado removido.", "Kein Suchergebnis entfernt.", "Aucun résultat supprimé.", "لم تتم إزالة أي نتيجة.", "未移除搜尋結果。", "未移除搜索结果。", "未移除搜尋結果。"},
    {"search.invalid_regex", "Invalid regular expression: ", "無効な正規表現: ", "잘못된 정규식: ", "Expresión regular inválida: ", "Expressão regular inválida: ", "Ungültiger regulärer Ausdruck: ", "Expression régulière invalide : ", "تعبير نمطي غير صالح: ", "無效正則表達式：", "无效正则表达式：", "無效正規表示式："},
    {"search.refreshed_prefix", "Refreshed ", "更新しました: ", "새로고침됨: ", "Actualizados ", "Atualizados ", "Aktualisiert: ", "Actualisé : ", "تم تحديث ", "已更新 ", "已刷新 ", "已重新整理 "},
    {"search.refreshed_suffix", " search groups.", " 個の検索グループ。", "개 검색 그룹.", " grupos de búsqueda.", " grupos de pesquisa.", " Suchgruppen.", " groupes de recherche.", " مجموعات بحث.", " 個搜尋群組。", " 个搜索组。", " 個搜尋群組。"},
    {"search.match_one", " match)", " 件)", "개 일치)", " coincidencia)", " ocorrência)", " Treffer)", " correspondance)", " تطابق)", " 個符合項）", " 个匹配）", " 個符合項）"},
    {"search.match_many", " matches)", " 件)", "개 일치)", " coincidencias)", " ocorrências)", " Treffer)", " correspondances)", " تطابقات)", " 個符合項）", " 个匹配）", " 個符合項）"},
    {"search.line_prefix", "Line ", "行 ", "줄 ", "Línea ", "Linha ", "Zeile ", "Ligne ", "السطر ", "第 ", "第 ", "第 "},
    {"search.line_separator", ": ", ": ", ": ", ": ", ": ", ": ", " : ", ": ", " 行：", " 行：", " 行："},
    {"search.label_prefix", "Search \"", "検索 \"", "검색 \"", "Búsqueda \"", "Pesquisa \"", "Suche \"", "Recherche \"", "بحث \"", "搜尋「", "搜索“", "搜尋「"},
    {"search.label_middle_one", "\" (", "\" (", "\" (", "\" (", "\" (", "\" (", "\" (", "\" (", "」（", "”（", "」（"},
    {"search.hit_one", " hit in ", " 件 / ", "개 결과, 파일 ", " resultado en ", " resultado em ", " Treffer in ", " résultat dans ", " نتيجة في ", " 個結果，位於 ", " 个结果，位于 ", " 個結果，位於 "},
    {"search.hit_many", " hits in ", " 件 / ", "개 결과, 파일 ", " resultados en ", " resultados em ", " Treffer in ", " résultats dans ", " نتائج في ", " 個結果，位於 ", " 个结果，位于 ", " 個結果，位於 "},
    {"search.file_one", " file)", " ファイル)", "개 파일)", " archivo)", " arquivo)", " Datei)", " fichier)", " ملف)", " 個檔案）", " 个文件）", " 個檔案）"},
    {"search.file_many", " files)", " ファイル)", "개 파일)", " archivos)", " arquivos)", " Dateien)", " fichiers)", " ملفات)", " 個檔案）", " 个文件）", " 個檔案）"},
    {"search.showing_prefix", "Showing ", "表示中: ", "표시 중: ", "Mostrando ", "Mostrando ", "Anzeigen: ", "Affichage : ", "عرض ", "顯示中：", "正在显示：", "顯示中："},
    {"macro.no_saved", "No saved macros available.", "保存済みマクロはありません。", "저장된 매크로가 없습니다.", "No hay macros guardadas.", "Nenhuma macro salva.", "Keine gespeicherten Makros verfügbar.", "Aucune macro enregistrée.", "لا توجد وحدات ماكرو محفوظة.", "冇已儲存巨集。", "没有已保存的宏。", "沒有已儲存巨集。"},
    {"macro.save_current", "Save Current Macro", "現在のマクロを保存", "현재 매크로 저장", "Guardar macro actual", "Salvar macro atual", "Aktuelles Makro speichern", "Enregistrer la macro actuelle", "حفظ الماكرو الحالي", "儲存目前巨集", "保存当前宏", "儲存目前巨集"},
    {"macro.name", "Macro name:", "マクロ名:", "매크로 이름:", "Nombre de macro:", "Nome da macro:", "Makroname:", "Nom de la macro :", "اسم الماكرو:", "巨集名稱：", "宏名称：", "巨集名稱："},
    {"macro.default_name", "Macro ", "マクロ ", "매크로 ", "Macro ", "Macro ", "Makro ", "Macro ", "ماكرو ", "巨集 ", "宏 ", "巨集 "},
    {"macro.play", "Play Macro", "マクロを再生", "매크로 실행", "Reproducir macro", "Executar macro", "Makro abspielen", "Lire la macro", "تشغيل ماكرو", "播放巨集", "播放宏", "播放巨集"},
    {"macro.play_button", "_Play", "_再生", "_실행", "_Reproducir", "_Executar", "_Abspielen", "_Lire", "_تشغيل", "_播放", "_播放", "_播放"},
    {"macro.select_to_play", "Select a macro to play.", "再生するマクロを選択してください。", "실행할 매크로를 선택하세요.", "Seleccione una macro para reproducir.", "Selecione uma macro para executar.", "Wählen Sie ein Makro zum Abspielen aus.", "Sélectionnez une macro à lire.", "حدد ماكرو لتشغيله.", "請選擇要播放嘅巨集。", "请选择要播放的宏。", "請選擇要播放的巨集。"},
    {"macro.management", "Macro Management", "マクロ管理", "매크로 관리", "Gestión de macros", "Gerenciamento de macros", "Makroverwaltung", "Gestion des macros", "إدارة وحدات الماكرو", "巨集管理", "宏管理", "巨集管理"},
    {"macro.management_hint", "Select a macro to rename or delete:", "名前変更または削除するマクロを選択:", "이름을 바꾸거나 삭제할 매크로를 선택하세요:", "Seleccione una macro para renombrar o eliminar:", "Selecione uma macro para renomear ou excluir:", "Wählen Sie ein Makro zum Umbenennen oder Löschen:", "Sélectionnez une macro à renommer ou supprimer :", "حدد ماكرو لإعادة تسميته أو حذفه:", "選擇要重新命名或刪除嘅巨集：", "选择要重命名或删除的宏：", "選擇要重新命名或刪除的巨集："},
    {"macro.rename", "Rename", "名前変更", "이름 바꾸기", "Renombrar", "Renomear", "Umbenennen", "Renommer", "إعادة تسمية", "重新命名", "重命名", "重新命名"},
    {"macro.delete", "Delete", "削除", "삭제", "Eliminar", "Excluir", "Löschen", "Supprimer", "حذف", "刪除", "删除", "刪除"},
    {"macro.rename_title", "Rename Macro", "マクロ名を変更", "매크로 이름 바꾸기", "Renombrar macro", "Renomear macro", "Makro umbenennen", "Renommer la macro", "إعادة تسمية الماكرو", "重新命名巨集", "重命名宏", "重新命名巨集"},
    {"macro.delete_title", "Delete Macro", "マクロを削除", "매크로 삭제", "Eliminar macro", "Excluir macro", "Makro löschen", "Supprimer la macro", "حذف الماكرو", "刪除巨集", "删除宏", "刪除巨集"},
    {"macro.select_to_rename", "Select a macro to rename.", "名前変更するマクロを選択してください。", "이름을 바꿀 매크로를 선택하세요.", "Seleccione una macro para renombrar.", "Selecione uma macro para renomear.", "Wählen Sie ein Makro zum Umbenennen aus.", "Sélectionnez une macro à renommer.", "حدد ماكرو لإعادة تسميته.", "請選擇要重新命名嘅巨集。", "请选择要重命名的宏。", "請選擇要重新命名的巨集。"},
    {"macro.select_to_delete", "Select a macro to delete.", "削除するマクロを選択してください。", "삭제할 매크로를 선택하세요.", "Seleccione una macro para eliminar.", "Selecione uma macro para excluir.", "Wählen Sie ein Makro zum Löschen aus.", "Sélectionnez une macro à supprimer.", "حدد ماكرو لحذفه.", "請選擇要刪除嘅巨集。", "请选择要删除的宏。", "請選擇要刪除的巨集。"},
    {"macro.not_found", "The selected macro could not be found.", "選択したマクロが見つかりません。", "선택한 매크로를 찾을 수 없습니다.", "No se encontró la macro seleccionada.", "A macro selecionada não foi encontrada.", "Das ausgewählte Makro wurde nicht gefunden.", "La macro sélectionnée est introuvable.", "تعذر العثور على الماكرو المحدد.", "搵唔到選取嘅巨集。", "找不到选中的宏。", "找不到選取的巨集。"},
    {"macro.current_name", "Current name:", "現在の名前:", "현재 이름:", "Nombre actual:", "Nome atual:", "Aktueller Name:", "Nom actuel :", "الاسم الحالي:", "目前名稱：", "当前名称：", "目前名稱："},
    {"macro.new_name", "New name:", "新しい名前:", "새 이름:", "Nuevo nombre:", "Novo nome:", "Neuer Name:", "Nouveau nom :", "الاسم الجديد:", "新名稱：", "新名称：", "新名稱："},
    {"macro.rename_button", "_Rename", "_名前変更", "_이름 바꾸기", "_Renombrar", "_Renomear", "_Umbenennen", "_Renommer", "_إعادة تسمية", "_重新命名", "_重命名", "_重新命名"},
    {"macro.empty_name", "Macro name cannot be empty.", "マクロ名は空にできません。", "매크로 이름은 비워둘 수 없습니다.", "El nombre no puede estar vacío.", "O nome não pode ficar vazio.", "Makroname darf nicht leer sein.", "Le nom ne peut pas être vide.", "لا يمكن أن يكون اسم الماكرو فارغًا.", "巨集名稱唔可以空白。", "宏名称不能为空。", "巨集名稱不可空白。"},
    {"macro.rename_failed", "Rename failed. The name may already exist.", "名前変更に失敗しました。名前が既に存在する可能性があります。", "이름 바꾸기에 실패했습니다. 이름이 이미 있을 수 있습니다.", "Error al renombrar. El nombre puede existir.", "Falha ao renomear. O nome pode já existir.", "Umbenennen fehlgeschlagen. Der Name existiert vielleicht schon.", "Échec du renommage. Le nom existe peut-être déjà.", "فشلت إعادة التسمية. قد يكون الاسم موجودًا.", "重新命名失敗，名稱可能已存在。", "重命名失败，名称可能已存在。", "重新命名失敗，名稱可能已存在。"},
    {"macro.delete_question_prefix", "Delete macro \"", "マクロ \"", "매크로 \"", "¿Eliminar macro \"", "Excluir macro \"", "Makro \"", "Supprimer la macro \"", "حذف الماكرو \"", "刪除巨集「", "删除宏“", "刪除巨集「"},
    {"macro.delete_question_suffix", "\"?", "\" を削除しますか?", "\" 삭제?", "\"?", "\"?", "\" löschen?", "\" ?", "\"؟", "」？", "”？", "」？"},
    {"macro.delete_warning", "This action cannot be undone.", "この操作は元に戻せません。", "이 작업은 되돌릴 수 없습니다.", "Esta acción no se puede deshacer.", "Esta ação não pode ser desfeita.", "Diese Aktion kann nicht rückgängig gemacht werden.", "Cette action est irréversible.", "لا يمكن التراجع عن هذا الإجراء.", "呢個動作無法復原。", "此操作无法撤销。", "此操作無法復原。"},
    {"shortcut.title", "Keyboard Shortcut Settings", "キーボードショートカット設定", "키보드 단축키 설정", "Configuración de atajos", "Configurações de atalhos", "Tastenkürzel-Einstellungen", "Paramètres des raccourcis", "إعدادات اختصارات لوحة المفاتيح", "鍵盤快捷鍵設定", "键盘快捷键设置", "鍵盤快速鍵設定"},
    {"shortcut.search_placeholder", "Search shortcuts...", "ショートカットを検索...", "단축키 검색...", "Buscar atajos...", "Pesquisar atalhos...", "Tastenkürzel suchen...", "Rechercher des raccourcis...", "البحث عن اختصارات...", "搜尋快捷鍵...", "搜索快捷键...", "搜尋快速鍵..."},
    {"shortcut.import", "Import", "インポート", "가져오기", "Importar", "Importar", "Importieren", "Importer", "استيراد", "匯入", "导入", "匯入"},
    {"shortcut.export", "Export", "エクスポート", "내보내기", "Exportar", "Exportar", "Exportieren", "Exporter", "تصدير", "匯出", "导出", "匯出"},
    {"shortcut.import_tooltip", "Import shortcuts from JSON", "JSON からショートカットをインポート", "JSON에서 단축키 가져오기", "Importar atajos desde JSON", "Importar atalhos de JSON", "Tastenkürzel aus JSON importieren", "Importer des raccourcis depuis JSON", "استيراد الاختصارات من JSON", "從 JSON 匯入快捷鍵", "从 JSON 导入快捷键", "從 JSON 匯入快速鍵"},
    {"shortcut.export_tooltip", "Export shortcuts to JSON", "ショートカットを JSON にエクスポート", "단축키를 JSON으로 내보내기", "Exportar atajos a JSON", "Exportar atalhos para JSON", "Tastenkürzel nach JSON exportieren", "Exporter les raccourcis en JSON", "تصدير الاختصارات إلى JSON", "匯出快捷鍵到 JSON", "导出快捷键到 JSON", "匯出快速鍵到 JSON"},
    {"shortcut.command", "Command", "コマンド", "명령", "Comando", "Comando", "Befehl", "Commande", "الأمر", "命令", "命令", "命令"},
    {"shortcut.shortcut1", "Shortcut 1", "ショートカット 1", "단축키 1", "Atajo 1", "Atalho 1", "Tastenkürzel 1", "Raccourci 1", "الاختصار 1", "快捷鍵 1", "快捷键 1", "快速鍵 1"},
    {"shortcut.shortcut2", "Shortcut 2", "ショートカット 2", "단축키 2", "Atajo 2", "Atalho 2", "Tastenkürzel 2", "Raccourci 2", "الاختصار 2", "快捷鍵 2", "快捷键 2", "快速鍵 2"},
    {"shortcut.status", "Status", "状態", "상태", "Estado", "Status", "Status", "État", "الحالة", "狀態", "状态", "狀態"},
    {"shortcut.reset", "Reset", "リセット", "초기화", "Restablecer", "Redefinir", "Zurücksetzen", "Réinitialiser", "إعادة ضبط", "重設", "重置", "重設"},
    {"shortcut.help", "Double-click a shortcut cell to edit. Changes apply immediately.", "ショートカットセルをダブルクリックして編集します。変更はすぐに適用されます。", "단축키 셀을 두 번 클릭해 편집하세요. 변경 사항은 즉시 적용됩니다.", "Doble clic en una celda para editar. Los cambios se aplican de inmediato.", "Clique duas vezes em uma célula para editar. As alterações são aplicadas imediatamente.", "Doppelklicken Sie eine Tastenkürzel-Zelle zum Bearbeiten. Änderungen gelten sofort.", "Double-cliquez sur une cellule pour modifier. Les changements s'appliquent immédiatement.", "انقر نقرًا مزدوجًا على خلية اختصار للتعديل. تطبق التغييرات فورًا.", "雙擊快捷鍵儲存格即可編輯，變更即時生效。", "双击快捷键单元格即可编辑，更改立即生效。", "雙擊快速鍵儲存格即可編輯，變更立即生效。"},
    {"shortcut.json_files", "Shortcut JSON files", "ショートカット JSON ファイル", "단축키 JSON 파일", "Archivos JSON de atajos", "Arquivos JSON de atalhos", "Tastenkürzel-JSON-Dateien", "Fichiers JSON de raccourcis", "ملفات JSON للاختصارات", "快捷鍵 JSON 檔案", "快捷键 JSON 文件", "快速鍵 JSON 檔案"},
    {"shortcut.export_title", "Export Shortcuts", "ショートカットをエクスポート", "단축키 내보내기", "Exportar atajos", "Exportar atalhos", "Tastenkürzel exportieren", "Exporter les raccourcis", "تصدير الاختصارات", "匯出快捷鍵", "导出快捷键", "匯出快速鍵"},
    {"shortcut.import_title", "Import Shortcuts", "ショートカットをインポート", "단축키 가져오기", "Importar atajos", "Importar atalhos", "Tastenkürzel importieren", "Importer les raccourcis", "استيراد الاختصارات", "匯入快捷鍵", "导入快捷键", "匯入快速鍵"},
    {"shortcut.export_button", "_Export", "_エクスポート", "_내보내기", "_Exportar", "_Exportar", "_Exportieren", "_Exporter", "_تصدير", "_匯出", "_导出", "_匯出"},
    {"shortcut.import_button", "_Import", "_インポート", "_가져오기", "_Importar", "_Importar", "_Importieren", "_Importer", "_استيراد", "_匯入", "_导入", "_匯入"},
    {"shortcut.exported", "Shortcut configuration exported.", "ショートカット設定をエクスポートしました。", "단축키 설정을 내보냈습니다.", "Configuración exportada.", "Configuração exportada.", "Tastenkürzel-Konfiguration exportiert.", "Configuration exportée.", "تم تصدير إعدادات الاختصارات.", "快捷鍵設定已匯出。", "快捷键配置已导出。", "快速鍵設定已匯出。"},
    {"shortcut.imported", "Shortcut configuration imported and applied.", "ショートカット設定をインポートして適用しました。", "단축키 설정을 가져와 적용했습니다.", "Configuración importada y aplicada.", "Configuração importada e aplicada.", "Tastenkürzel-Konfiguration importiert und angewendet.", "Configuration importée et appliquée.", "تم استيراد إعدادات الاختصارات وتطبيقها.", "快捷鍵設定已匯入並套用。", "快捷键配置已导入并应用。", "快速鍵設定已匯入並套用。"},
    {"shortcut.invalid_format", "Invalid format. Use Ctrl+S or Alt+F4.", "形式が無効です。Ctrl+S または Alt+F4 を使用してください。", "형식이 잘못되었습니다. Ctrl+S 또는 Alt+F4를 사용하세요.", "Formato inválido. Use Ctrl+S o Alt+F4.", "Formato inválido. Use Ctrl+S ou Alt+F4.", "Ungültiges Format. Verwenden Sie Ctrl+S oder Alt+F4.", "Format invalide. Utilisez Ctrl+S ou Alt+F4.", "تنسيق غير صالح. استخدم Ctrl+S أو Alt+F4.", "格式無效。請用 Ctrl+S 或 Alt+F4。", "格式无效。请使用 Ctrl+S 或 Alt+F4。", "格式無效。請使用 Ctrl+S 或 Alt+F4。"},
    {"shortcut.not_changed_invalid", "Shortcut was not changed: invalid format.", "ショートカットは変更されませんでした: 形式が無効です。", "단축키가 변경되지 않았습니다: 형식 오류.", "El atajo no cambió: formato inválido.", "Atalho não alterado: formato inválido.", "Tastenkürzel nicht geändert: ungültiges Format.", "Raccourci inchangé : format invalide.", "لم يتغير الاختصار: تنسيق غير صالح.", "快捷鍵未變更：格式無效。", "快捷键未更改：格式无效。", "快速鍵未變更：格式無效。"},
    {"shortcut.conflict_prefix", "Conflict with ", "競合: ", "충돌: ", "Conflicto con ", "Conflito com ", "Konflikt mit ", "Conflit avec ", "تعارض مع ", "衝突：", "冲突：", "衝突："},
    {"shortcut.unable_apply", "Unable to apply shortcut.", "ショートカットを適用できません。", "단축키를 적용할 수 없습니다.", "No se puede aplicar el atajo.", "Não foi possível aplicar o atalho.", "Tastenkürzel kann nicht angewendet werden.", "Impossible d'appliquer le raccourci.", "تعذر تطبيق الاختصار.", "無法套用快捷鍵。", "无法应用快捷键。", "無法套用快速鍵。"},
    {"shortcut.not_changed", "Shortcut was not changed.", "ショートカットは変更されませんでした。", "단축키가 변경되지 않았습니다.", "El atajo no cambió.", "Atalho não alterado.", "Tastenkürzel wurde nicht geändert.", "Raccourci inchangé.", "لم يتغير الاختصار.", "快捷鍵未變更。", "快捷键未更改。", "快速鍵未變更。"},
    {"shortcut.active", "Active", "有効", "활성", "Activo", "Ativo", "Aktiv", "Actif", "نشط", "啟用", "启用", "啟用"},
    {"shortcut.default", "Default", "既定", "기본", "Predeterminado", "Padrão", "Standard", "Par défaut", "افتراضي", "預設", "默认", "預設"},
    {"shortcut.export_no_destination", "Export failed: no destination file selected.", "エクスポート失敗: 保存先ファイルが選択されていません。", "내보내기 실패: 대상 파일이 선택되지 않았습니다.", "Error al exportar: no se seleccionó destino.", "Falha ao exportar: nenhum destino selecionado.", "Export fehlgeschlagen: keine Zieldatei ausgewählt.", "Échec de l'export : aucune destination sélectionnée.", "فشل التصدير: لم يتم تحديد ملف وجهة.", "匯出失敗：未選擇目的檔案。", "导出失败：未选择目标文件。", "匯出失敗：未選擇目的檔案。"},
    {"shortcut.export_create_dir_prefix", "Export failed: could not create destination directory: ", "エクスポート失敗: 保存先ディレクトリを作成できません: ", "내보내기 실패: 대상 디렉터리를 만들 수 없습니다: ", "Error al exportar: no se pudo crear el directorio: ", "Falha ao exportar: não foi possível criar o diretório: ", "Export fehlgeschlagen: Zielordner konnte nicht erstellt werden: ", "Échec de l'export : impossible de créer le dossier : ", "فشل التصدير: تعذر إنشاء مجلد الوجهة: ", "匯出失敗：無法建立目的目錄：", "导出失败：无法创建目标目录：", "匯出失敗：無法建立目的目錄："},
    {"shortcut.export_failed_prefix", "Export failed: ", "エクスポート失敗: ", "내보내기 실패: ", "Error al exportar: ", "Falha ao exportar: ", "Export fehlgeschlagen: ", "Échec de l'export : ", "فشل التصدير: ", "匯出失敗：", "导出失败：", "匯出失敗："},
    {"shortcut.import_no_source", "Import failed: no source file selected.", "インポート失敗: ソースファイルが選択されていません。", "가져오기 실패: 원본 파일이 선택되지 않았습니다.", "Error al importar: no se seleccionó origen.", "Falha ao importar: nenhum arquivo selecionado.", "Import fehlgeschlagen: keine Quelldatei ausgewählt.", "Échec de l'import : aucun fichier source sélectionné.", "فشل الاستيراد: لم يتم تحديد ملف مصدر.", "匯入失敗：未選擇來源檔案。", "导入失败：未选择源文件。", "匯入失敗：未選擇來源檔案。"},
    {"shortcut.import_open_failed", "Import failed: could not open selected file.", "インポート失敗: 選択したファイルを開けません。", "가져오기 실패: 선택한 파일을 열 수 없습니다.", "Error al importar: no se pudo abrir el archivo.", "Falha ao importar: não foi possível abrir o arquivo.", "Import fehlgeschlagen: Datei konnte nicht geöffnet werden.", "Échec de l'import : impossible d'ouvrir le fichier.", "فشل الاستيراد: تعذر فتح الملف المحدد.", "匯入失敗：無法開啟選取檔案。", "导入失败：无法打开所选文件。", "匯入失敗：無法開啟選取檔案。"},
    {"shortcut.import_empty", "Import failed: selected file does not contain shortcuts.", "インポート失敗: 選択したファイルにショートカットがありません。", "가져오기 실패: 선택한 파일에 단축키가 없습니다.", "Error al importar: el archivo no contiene atajos.", "Falha ao importar: o arquivo não contém atalhos.", "Import fehlgeschlagen: Datei enthält keine Tastenkürzel.", "Échec de l'import : le fichier ne contient aucun raccourci.", "فشل الاستيراد: الملف لا يحتوي على اختصارات.", "匯入失敗：選取檔案沒有快捷鍵。", "导入失败：所选文件不包含快捷键。", "匯入失敗：選取檔案沒有快速鍵。"},
    {"shortcut.import_create_dir_prefix", "Import failed: could not create shortcuts directory: ", "インポート失敗: ショートカットディレクトリを作成できません: ", "가져오기 실패: 단축키 디렉터리를 만들 수 없습니다: ", "Error al importar: no se pudo crear el directorio: ", "Falha ao importar: não foi possível criar o diretório: ", "Import fehlgeschlagen: Tastenkürzel-Ordner konnte nicht erstellt werden: ", "Échec de l'import : impossible de créer le dossier : ", "فشل الاستيراد: تعذر إنشاء مجلد الاختصارات: ", "匯入失敗：無法建立快捷鍵目錄：", "导入失败：无法创建快捷键目录：", "匯入失敗：無法建立快速鍵目錄："},
    {"shortcut.import_failed_prefix", "Import failed: ", "インポート失敗: ", "가져오기 실패: ", "Error al importar: ", "Falha ao importar: ", "Import fehlgeschlagen: ", "Échec de l'import : ", "فشل الاستيراد: ", "匯入失敗：", "导入失败：", "匯入失敗："},
    {"error.encoding_failed", "Encoding conversion failed.", "エンコーディング変換に失敗しました。", "인코딩 변환에 실패했습니다.", "Error al convertir codificación.", "Falha na conversão de codificação.", "Kodierungskonvertierung fehlgeschlagen.", "Échec de la conversion d'encodage.", "فشل تحويل الترميز.", "編碼轉換失敗。", "编码转换失败。", "編碼轉換失敗。"},
    {"error.encoding_lossless_prefix", "The current text cannot be represented losslessly as ", "現在のテキストはロスレスで表現できません: ", "현재 텍스트를 무손실로 표현할 수 없습니다: ", "El texto actual no puede representarse sin pérdida como ", "O texto atual não pode ser representado sem perdas como ", "Der aktuelle Text kann nicht verlustfrei dargestellt werden als ", "Le texte actuel ne peut pas être représenté sans perte en ", "لا يمكن تمثيل النص الحالي دون فقدان كـ ", "目前文字無法無損表示為 ", "当前文本无法无损表示为 ", "目前文字無法無損表示為 "},
    {"error.unable_reload", "Unable to reload file.", "ファイルを再読み込みできません。", "파일을 다시 불러올 수 없습니다.", "No se puede recargar el archivo.", "Não foi possível recarregar o arquivo.", "Datei kann nicht neu geladen werden.", "Impossible de recharger le fichier.", "تعذر إعادة تحميل الملف.", "無法重新載入檔案。", "无法重新加载文件。", "無法重新載入檔案。"},
    {"error.unsaved_reload_detail", "The current document has not been saved to disk.", "現在の文書はディスクに保存されていません。", "현재 문서가 디스크에 저장되지 않았습니다.", "El documento actual no se ha guardado en disco.", "O documento atual não foi salvo em disco.", "Das aktuelle Dokument wurde nicht gespeichert.", "Le document actuel n'a pas été enregistré.", "لم يتم حفظ المستند الحالي على القرص.", "目前文件未儲存到磁碟。", "当前文档尚未保存到磁盘。", "目前文件尚未儲存到磁碟。"},
    {"error.unable_open", "Unable to open file.", "ファイルを開けません。", "파일을 열 수 없습니다.", "No se puede abrir el archivo.", "Não foi possível abrir o arquivo.", "Datei kann nicht geöffnet werden.", "Impossible d'ouvrir le fichier.", "تعذر فتح الملف.", "無法開啟檔案。", "无法打开文件。", "無法開啟檔案。"},
    {"error.missing_file", "The file is no longer available on disk.", "ファイルはディスク上にありません。", "파일이 더 이상 디스크에 없습니다.", "El archivo ya no está disponible en disco.", "O arquivo não está mais disponível no disco.", "Die Datei ist nicht mehr auf dem Datenträger verfügbar.", "Le fichier n'est plus disponible sur le disque.", "لم يعد الملف متاحًا على القرص.", "檔案已不在磁碟上。", "文件已不在磁盘上。", "檔案已不在磁碟上。"},
    {"error.inaccessible_file", "The file could not be accessed.", "ファイルにアクセスできません。", "파일에 접근할 수 없습니다.", "No se pudo acceder al archivo.", "Não foi possível acessar o arquivo.", "Auf die Datei konnte nicht zugegriffen werden.", "Impossible d'accéder au fichier.", "تعذر الوصول إلى الملف.", "無法存取檔案。", "无法访问文件。", "無法存取檔案。"},
    {"error.changed_on_disk", "The file has changed on disk.", "ファイルはディスク上で変更されました。", "파일이 디스크에서 변경되었습니다.", "El archivo cambió en disco.", "O arquivo foi alterado no disco.", "Die Datei wurde auf dem Datenträger geändert.", "Le fichier a changé sur le disque.", "تغير الملف على القرص.", "檔案喺磁碟上已變更。", "文件已在磁盘上更改。", "檔案已在磁碟上變更。"},
    {"prompt.keep_buffer_prefix", "Keep the buffer for ", "次のバッファーを開いたままにしますか: ", "다음 버퍼를 열어 둘까요: ", "¿Mantener abierto el búfer de ", "Manter aberto o buffer de ", "Puffer geöffnet halten für ", "Garder ouvert le tampon de ", "إبقاء المخزن مفتوحًا لـ ", "保持緩衝區開啟：", "保持缓冲区打开：", "保持緩衝區開啟："},
    {"prompt.keep_buffer_suffix", " open?", "?", "?", "?", "?", "?", " ?", "؟", "？", "？", "？"},
    {"prompt.keep_try_again_suffix", " open and try again later?", " を開いたままにして後で再試行しますか?", " 열어 두고 나중에 다시 시도할까요?", " abierto e intentar más tarde?", " aberto e tentar novamente mais tarde?", " offen halten und später erneut versuchen?", " ouvert et réessayer plus tard ?", " مفتوحًا والمحاولة لاحقًا؟", "，稍後再試？", "，稍后重试？", "，稍後再試？"},
    {"prompt.reload_prefix", "Reload ", "次を再読み込みしますか: ", "다시 불러올까요: ", "¿Recargar ", "Recarregar ", "", "Recharger ", "إعادة تحميل ", "重新載入 ", "重新加载 ", "重新載入 "},
    {"prompt.reload_suffix", " from disk?", "?", "?", " desde disco?", " do disco?", " von Datenträger neu laden?", " depuis le disque ?", " من القرص؟", "？", "？", "？"},
    {"prompt.unsaved_changes", "The document has unsaved changes.", "文書に未保存の変更があります。", "문서에 저장되지 않은 변경 사항이 있습니다.", "El documento tiene cambios sin guardar.", "O documento tem alterações não salvas.", "Das Dokument hat ungespeicherte Änderungen.", "Le document contient des modifications non enregistrées.", "يحتوي المستند على تغييرات غير محفوظة.", "文件有未儲存變更。", "文档有未保存的更改。", "文件有未儲存變更。"},
    {"prompt.save_changes_prefix", "Save changes to ", "閉じる前に次の変更を保存しますか: ", "닫기 전에 변경 사항을 저장할까요: ", "¿Guardar cambios en ", "Salvar alterações em ", "Änderungen speichern an ", "Enregistrer les modifications de ", "حفظ التغييرات في ", "關閉前儲存變更：", "关闭前保存更改：", "關閉前儲存變更："},
    {"prompt.save_changes_suffix", " before closing?", "?", "?", " antes de cerrar?", " antes de fechar?", " vor dem Schließen?", " avant de fermer ?", " قبل الإغلاق؟", "？", "？", "？"},
    {"prompt.save_before_closing", "Do you want to save changes before closing?", "閉じる前に変更を保存しますか?", "닫기 전에 변경 사항을 저장할까요?", "¿Desea guardar los cambios antes de cerrar?", "Deseja salvar as alterações antes de fechar?", "Möchten Sie die Änderungen vor dem Schließen speichern?", "Voulez-vous enregistrer avant de fermer ?", "هل تريد حفظ التغييرات قبل الإغلاق؟", "關閉前要儲存變更嗎？", "关闭前要保存更改吗？", "關閉前要儲存變更嗎？"},
    {"prompt.reload_file", "Reload this file from disk?", "このファイルをディスクから再読み込みしますか?", "이 파일을 디스크에서 다시 불러올까요?", "¿Recargar este archivo desde disco?", "Recarregar este arquivo do disco?", "Diese Datei vom Datenträger neu laden?", "Recharger ce fichier depuis le disque ?", "إعادة تحميل هذا الملف من القرص؟", "從磁碟重新載入此檔案？", "从磁盘重新加载此文件？", "從磁碟重新載入此檔案？"},
    {"prompt.discard_unsaved", "Unsaved changes in this tab will be discarded.", "このタブの未保存の変更は破棄されます。", "이 탭의 저장되지 않은 변경 사항이 버려집니다.", "Los cambios sin guardar se descartarán.", "Alterações não salvas serão descartadas.", "Ungespeicherte Änderungen in diesem Tab werden verworfen.", "Les modifications non enregistrées seront perdues.", "سيتم تجاهل التغييرات غير المحفوظة.", "此分頁未儲存變更將被捨棄。", "此标签页未保存的更改将被丢弃。", "此分頁未儲存變更將被捨棄。"},
};

const TranslationRow *find_row(const char *key) {
  auto it = std::find_if(kTranslations.begin(), kTranslations.end(),
                         [key](const TranslationRow &row) {
                           return std::string(row.key) == key;
                         });
  return it == kTranslations.end() ? nullptr : &*it;
}

const char *value_for_language(const TranslationRow &row,
                               const std::string &language) {
  if (language == "ja") return row.ja;
  if (language == "ko") return row.ko;
  if (language == "es") return row.es;
  if (language == "pt") return row.pt;
  if (language == "de") return row.de;
  if (language == "fr") return row.fr;
  if (language == "ar") return row.ar;
  if (language == "yue") return row.yue;
  if (language == "zh-Hans") return row.zhHans;
  if (language == "zh-Hant") return row.zhHant;
  return row.en;
}

void set_widget_text(GtkWidget *widget, const char *text) {
  if (GTK_IS_MENU_ITEM(widget)) {
    gtk_menu_item_set_label(GTK_MENU_ITEM(widget), text);
  } else if (GTK_IS_FRAME(widget)) {
    gtk_frame_set_label(GTK_FRAME(widget), text);
  } else if (GTK_IS_BUTTON(widget)) {
    gtk_button_set_label(GTK_BUTTON(widget), text);
  } else if (GTK_IS_LABEL(widget)) {
    gtk_label_set_text(GTK_LABEL(widget), text);
  }
}

void unregister_bound_widget(gpointer data, GObject *) {
  GtkWidget *widget = GTK_WIDGET(data);
  boundWidgets.erase(std::remove(boundWidgets.begin(), boundWidgets.end(),
                                 widget),
                     boundWidgets.end());
}

void register_bound_widget(GtkWidget *widget) {
  if (!widget) {
    return;
  }
  if (std::find(boundWidgets.begin(), boundWidgets.end(), widget) !=
      boundWidgets.end()) {
    return;
  }
  boundWidgets.push_back(widget);
  g_object_weak_ref(G_OBJECT(widget), unregister_bound_widget, widget);
}

void refresh_bound_widget(GtkWidget *widget) {
  const char *key = static_cast<const char *>(
      g_object_get_data(G_OBJECT(widget), "i18n-key"));
  if (key) {
    set_widget_text(widget, text(key));
  }

  const char *titleKey = static_cast<const char *>(
      g_object_get_data(G_OBJECT(widget), "i18n-title-key"));
  if (titleKey && GTK_IS_WINDOW(widget)) {
    gtk_window_set_title(GTK_WINDOW(widget), text(titleKey));
  }

  const char *placeholderKey = static_cast<const char *>(
      g_object_get_data(G_OBJECT(widget), "i18n-placeholder-key"));
  if (placeholderKey && GTK_IS_ENTRY(widget)) {
    gtk_entry_set_placeholder_text(GTK_ENTRY(widget), text(placeholderKey));
  }
}

void refresh_about_dialog(GtkWidget *widget) {
  if (!GTK_IS_ABOUT_DIALOG(widget)) {
    return;
  }

  gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(widget),
                                    text("app.name"));
  gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(widget),
                                text("about.comments"));
}

void refresh_child(GtkWidget *widget, gpointer) {
  refresh_widget_tree(widget);
}

}  // namespace

void set_language(const std::string &language) {
  activeLanguage = language.empty() ? "en" : language;
  gtk_widget_set_default_direction(activeLanguage == "ar" ? GTK_TEXT_DIR_RTL
                                                          : GTK_TEXT_DIR_LTR);
}

const std::string &current_language() {
  return activeLanguage;
}

const char *text(const char *key) {
  return text_for_language(activeLanguage, key);
}

const char *text_for_language(const std::string &language, const char *key) {
  const TranslationRow *row = find_row(key);
  if (!row) {
    return key;
  }
  return value_for_language(*row, language);
}

void bind_widget_text(GtkWidget *widget, const char *key) {
  register_bound_widget(widget);
  g_object_set_data_full(G_OBJECT(widget), "i18n-key", g_strdup(key), g_free);
  set_widget_text(widget, text(key));
}

void bind_window_title(GtkWidget *widget, const char *key) {
  register_bound_widget(widget);
  g_object_set_data_full(G_OBJECT(widget), "i18n-title-key", g_strdup(key),
                         g_free);
  if (GTK_IS_WINDOW(widget)) {
    gtk_window_set_title(GTK_WINDOW(widget), text(key));
  }
}

void bind_entry_placeholder(GtkWidget *widget, const char *key) {
  register_bound_widget(widget);
  g_object_set_data_full(G_OBJECT(widget), "i18n-placeholder-key",
                         g_strdup(key), g_free);
  if (GTK_IS_ENTRY(widget)) {
    gtk_entry_set_placeholder_text(GTK_ENTRY(widget), text(key));
  }
}

void refresh_widget_tree(GtkWidget *widget) {
  if (!widget) {
    return;
  }
  gtk_widget_set_direction(widget, activeLanguage == "ar" ? GTK_TEXT_DIR_RTL
                                                          : GTK_TEXT_DIR_LTR);
  refresh_bound_widget(widget);
  refresh_about_dialog(widget);
  if (GTK_IS_CONTAINER(widget)) {
    gtk_container_foreach(GTK_CONTAINER(widget), refresh_child, nullptr);
  }
  if (GTK_IS_MENU_ITEM(widget)) {
    GtkWidget *submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));
    if (submenu) {
      refresh_widget_tree(submenu);
    }
  }
}

void refresh_open_windows() {
  GList *windows = gtk_window_list_toplevels();
  for (GList *node = windows; node != nullptr; node = node->next) {
    refresh_widget_tree(GTK_WIDGET(node->data));
  }
  g_list_free(windows);

  std::vector<GtkWidget *> widgets = boundWidgets;
  for (GtkWidget *widget : widgets) {
    refresh_bound_widget(widget);
    gtk_widget_set_direction(widget, activeLanguage == "ar" ? GTK_TEXT_DIR_RTL
                                                            : GTK_TEXT_DIR_LTR);
  }
}

}  // namespace Localization
