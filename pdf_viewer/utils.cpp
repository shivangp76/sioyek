//#include <Windows.h>
#include <cwctype>

#ifdef SIOYEK_MOBILE
#include <unistd.h>
#endif



#include <cmath>
#include <complex>
#include <cassert>
#include "utils.h"
#include <optional>
#include <functional>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <string>
#include <qclipboard.h>
#include <qguiapplication.h>
#include <qprocess.h>
#include <qdesktopservices.h>
#include <qurl.h>
#include <qmessagebox.h>
#include <qbytearray.h>
#include <qdatastream.h>
#include <qstringlist.h>
#include <qcommandlineparser.h>
#include <qdir.h>
#include <qurl.h>
#include <qnetworkaccessmanager.h>
#include <qnetworkrequest.h>
#include <qnetworkreply.h>
#include <qscreen.h>
#include <qjsonarray.h>
#include <quuid.h>
#include <qjsondocument.h>
#include <qrandom.h>
#include <qmenu.h>
#include <qmenubar.h>
#include "path.h"
#include <qtextdocument.h>
#include <qabstracttextdocumentlayout.h>
#include <qtextcursor.h>
#include <qlistview.h>
#include <QTimer>

#include <cstdint>

#if defined(_WIN32)
    #include <windows.h>
#elif defined(Q_OS_MACOS)
    #include <ApplicationServices/ApplicationServices.h>
#endif

#ifdef SIOYEK_ANDROID
#include <QtCore/private/qandroidextras_p.h>
#include <qjniobject.h>
#endif

#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
#include <signal.h>
#endif

#ifdef SIOYEK_ADVANCED_AUDIO
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#undef MINIAUDIO_IMPLEMENTATION
#endif


#include <mupdf/pdf.h>
#include "synctex/synctex_parser.h"

extern std::ofstream LOG_FILE;
extern int STATUS_BAR_FONT_SIZE;
extern float STATUS_BAR_COLOR[3];
extern float STATUS_BAR_TEXT_COLOR[3];
extern float UI_SELECTED_TEXT_COLOR[3];
extern float UI_SELECTED_BACKGROUND_COLOR[3];
extern bool NUMERIC_TAGS;
extern float DARK_MODE_CONTRAST;
extern float CUSTOM_BACKGROUND_COLOR[3];
extern float CUSTOM_TEXT_COLOR[3];
extern float CUSTOM_COLOR_CONTRAST;
extern int CHAT_FONT_SIZE;
extern int DOCUMENTATION_FONT_SIZE;

extern float TTS_RATE;
extern bool ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE;
extern bool ALWAYS_COPY_SELECTED_TEXT;

extern std::wstring CHAT_FONT_FACE_NAME;
extern QString EPUB_TEMPLATE;
extern float EPUB_LINE_SPACING;
extern float EPUB_WIDTH;
extern float EPUB_HEIGHT;
extern float EPUB_FONT_SIZE;
extern std::wstring EPUB_CSS;
extern float HIGHLIGHT_COLORS[26 * 3];
extern float BLACK_COLOR[3];

extern std::wstring EXTERNAL_TEXT_EDITOR_COMMAND;

extern Path last_opened_file_address_path;

extern float UI_BACKGROUND_COLOR[3];
extern float UI_TEXT_COLOR[3];

// extern std::wstring PAPER_SEARCH_URL_PATH;
extern std::wstring PAPER_SEARCH_TILE_PATH;
extern std::wstring PAPER_SEARCH_CONTRIB_PATH;
extern std::wstring UI_FONT_FACE_NAME;
extern std::wstring STATUS_FONT_FACE_NAME;

extern bool VERBOSE;

extern QString global_font_family;

#ifdef Q_OS_WIN
#include <windows.h>
#include <io.h>
#endif


std::wstring to_lower(const std::wstring& inp) {
    std::wstring res;
    for (char c : inp) {
        res.push_back(::tolower(c));
    }
    return res;
}

std::wstring get_path_extras_file_name(const std::wstring& path_) {
    Path path = Path(path_);
    QString filename = QString::fromStdWString(path.filename().value());
    QString drawing_file_name = filename + ".sioyek.extras";
    std::wstring extras_file_path = path.file_parent().slash(drawing_file_name.toStdWString()).get_path();
    return extras_file_path;
}

void get_path_epub_size(const std::wstring& path, float* out_width, float* out_height) {
    QString extras_file_path = QString::fromStdWString(get_path_extras_file_name(path));
    QFileInfo extras_file_info(extras_file_path);

    float width = EPUB_WIDTH;
    float height = EPUB_HEIGHT;

    if (extras_file_info.exists()) {
        QFile extras_file(extras_file_path);
        if (extras_file.open(QIODevice::ReadOnly)) {
            QByteArray data = extras_file.readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            QJsonObject object = doc.object();
            if (object.find("epub_width") != object.end()) {
                width = object["epub_width"].toDouble();
                height = object["epub_height"].toDouble();

            }
            extras_file.close();
        }
    }

    *out_width = width;
    *out_height = height;
}

void get_flat_toc(const std::vector<TocNode*>& roots, std::vector<std::wstring>& output, std::vector<DocumentPos>& pages) {
    // Enumerate ToC nodes in DFS order

    for (const auto& root : roots) {
        output.push_back(root->title);
        DocumentPos pos;
        pos.page = root->page;
        pos.x = 0;
        pos.y = root->y;
        // if pos.y is nan, set it to 0
        if (std::isnan(pos.y)) {
            pos.y = 0;
        }
        pages.push_back(pos);
        get_flat_toc(root->children, output, pages);
    }
}

TocNode* get_toc_node_from_indices_helper(const std::vector<TocNode*>& roots, const std::vector<int>& indices, int pointer) {
    assert(pointer >= 0);

    if (pointer == 0) {
        return roots[indices[pointer]];
    }

    return get_toc_node_from_indices_helper(roots[indices[pointer]]->children, indices, pointer - 1);
}

TocNode* get_toc_node_from_indices(const std::vector<TocNode*>& roots, const std::vector<int>& indices) {
    // Get a table of contents item by recursively indexing using `indices`
    return get_toc_node_from_indices_helper(roots, indices, indices.size() - 1);
}


QStandardItem* get_item_tree_from_toc_helper(const std::vector<TocNode*>& children, QStandardItem* parent) {

    for (const auto* child : children) {
        QStandardItem* child_item = new QStandardItem(QString::fromStdWString(child->title));
        QStandardItem* page_item = new QStandardItem("[ " + QString::number(child->page) + " ]");
        child_item->setData(child->page);
        page_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);

        get_item_tree_from_toc_helper(child->children, child_item);
        parent->appendRow(QList<QStandardItem*>() << child_item << page_item);
    }
    return parent;
}


QStandardItemModel* get_model_from_toc(const std::vector<TocNode*>& roots) {

    QStandardItemModel* model = new QStandardItemModel();
    get_item_tree_from_toc_helper(roots, model->invisibleRootItem());
    return model;
}


int mod(int a, int b)
{
    // compute a mod b handling negative numbers "correctly"
    return (a % b + b) % b;
}

ParsedUri parse_uri(fz_context* mupdf_context, fz_document* document, std::string uri) {
    fz_link_dest dest = fz_resolve_link_dest(mupdf_context, document, uri.c_str());
    int target_page = fz_page_number_from_location(mupdf_context, document, dest.loc) + 1;

    if (std::isnan(dest.x)) {
        dest.x = 0;
    }

    ParsedUri res;
    if (dest.type == FZ_LINK_DEST_FIT_H) {
        res = { target_page, dest.x, dest.y };
    }
    else if (dest.type == FZ_LINK_DEST_FIT_R) {
        // this looks weird but it works for the one document I have with this reference type
        // (programming massively parallel processors) it may be wrong though
        res = { target_page+1, dest.x, -dest.y };
    }
    else if (dest.type != FZ_LINK_DEST_XYZ) {
        float x = dest.x + dest.w / 2;;
        float y = dest.y + dest.h / 2;
        res = { target_page, x, y };
    }
    else {
     res = { target_page, dest.x, dest.y };
    }

    if (std::isnan(res.x)) {
        res.x = 0;
    }
    if (std::isnan(res.y)) {
        res.y = 0;
    }
    return res;
}

char get_symbol(int key, bool is_shift_pressed, const std::vector<char>& special_symbols) {

    if (key >= 'A' && key <= 'Z') {
        if (is_shift_pressed) {
            return key;
        }
        else {
            return key + 'a' - 'A';
        }
    }

    if (key >= '0' && key <= '9') {
        return key;
    }

    if (std::find(special_symbols.begin(), special_symbols.end(), key) != special_symbols.end()) {
        return key;
    }

    return 0;
}

void rect_to_quad(fz_rect rect, float quad[8]) {
    quad[0] = rect.x0;
    quad[1] = rect.y0;
    quad[2] = rect.x1;
    quad[3] = rect.y0;
    quad[4] = rect.x0;
    quad[5] = rect.y1;
    quad[6] = rect.x1;
    quad[7] = rect.y1;
}

void copy_to_clipboard(const std::wstring& text, bool selection) {
    auto clipboard = QGuiApplication::clipboard();
    auto qtext = QString::fromStdWString(text);
    if (!selection) {
        clipboard->setText(qtext);
    }
    else {
        if (ALWAYS_COPY_SELECTED_TEXT) {
            clipboard->setText(qtext);
        }
        else {
            clipboard->setText(qtext, QClipboard::Selection);
        }
    }
}

#define OPEN_KEY(parent, name, ptr) \
    RegCreateKeyExA(parent, name, 0, 0, 0, KEY_WRITE, 0, &ptr, 0)

#define SET_KEY(parent, name, value) \
    RegSetValueExA(parent, name, 0, REG_SZ, (const BYTE *)(value), (DWORD)strlen(value) + 1)

void install_app(const char* argv0)
{
#ifdef Q_OS_WIN
    char buf[512];
    HKEY software, classes, testpdf, dotpdf;
    HKEY shell, open, command, supported_types;
    HKEY pdf_progids;

    OPEN_KEY(HKEY_CURRENT_USER, "Software", software);
    OPEN_KEY(software, "Classes", classes);
    OPEN_KEY(classes, ".pdf", dotpdf);
    OPEN_KEY(dotpdf, "OpenWithProgids", pdf_progids);
    OPEN_KEY(classes, "Sioyek", testpdf);
    OPEN_KEY(testpdf, "SupportedTypes", supported_types);
    OPEN_KEY(testpdf, "shell", shell);
    OPEN_KEY(shell, "open", open);
    OPEN_KEY(open, "command", command);

    sprintf(buf, "\"%s\" \"%%1\"", argv0);

    SET_KEY(open, "FriendlyAppName", "Sioyek");
    SET_KEY(command, "", buf);
    SET_KEY(supported_types, ".pdf", "");
    SET_KEY(pdf_progids, "sioyek", "");

    RegCloseKey(dotpdf);
    RegCloseKey(testpdf);
    RegCloseKey(classes);
    RegCloseKey(software);
#endif
}

int get_f_key(std::wstring name) {
    if (name[0] == '<') {
        name = name.substr(1, name.size() - 2);
    }
    if (name[0] != 'f') {
        return 0;
    }
    name = name.substr(1, name.size() - 1);
    if (!isdigit(name[0])) {
        return 0;
    }

    int num;
    std::wstringstream ss(name);
    ss >> num;
    return  num;
}

void show_error_message(const std::wstring& error_message) {
    QMessageBox msgBox;
    msgBox.setText(QString::fromStdWString(error_message));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
}

int show_option_buttons(const std::wstring& error_message, const std::vector<std::wstring>& buttons) {
    QMessageBox msgBox;
    msgBox.setText(QString::fromStdWString(error_message));
    for (auto& btn_text : buttons) {
        msgBox.addButton(QString::fromStdWString(btn_text), QMessageBox::ActionRole);
    }
    //msgBox.setStandardButtons(QMessageBox::Ok);
    //msgBox.setDefaultButton(QMessageBox::Ok);
    return msgBox.exec();
}

std::wstring utf8_decode(const std::string& encoded_str) {
    std::wstring res;
    utf8::utf8to32(encoded_str.begin(), encoded_str.end(), std::back_inserter(res));
    return res;
}

std::string utf8_encode(const std::wstring& decoded_str) {
    std::string res;
    utf8::utf32to8(decoded_str.begin(), decoded_str.end(), std::back_inserter(res));
    return res;
}

bool is_rtl(int c) {
    if (
        (c == 0x05BE) || (c == 0x05C0) || (c == 0x05C3) || (c == 0x05C6) ||
        ((c >= 0x05D0) && (c <= 0x05F4)) ||
        (c == 0x0608) || (c == 0x060B) || (c == 0x060D) ||
        ((c >= 0x061B) && (c <= 0x064A)) ||
        ((c >= 0x066D) && (c <= 0x066F)) ||
        ((c >= 0x0671) && (c <= 0x06D5)) ||
        ((c >= 0x06E5) && (c <= 0x06E6)) ||
        ((c >= 0x06EE) && (c <= 0x06EF)) ||
        ((c >= 0x06FA) && (c <= 0x0710)) ||
        ((c >= 0x0712) && (c <= 0x072F)) ||
        ((c >= 0x074D) && (c <= 0x07A5)) ||
        ((c >= 0x07B1) && (c <= 0x07EA)) ||
        ((c >= 0x07F4) && (c <= 0x07F5)) ||
        ((c >= 0x07FA) && (c <= 0x0815)) ||
        (c == 0x081A) || (c == 0x0824) || (c == 0x0828) ||
        ((c >= 0x0830) && (c <= 0x0858)) ||
        ((c >= 0x085E) && (c <= 0x08AC)) ||
        (c == 0x200F) || (c == 0xFB1D) ||
        ((c >= 0xFB1F) && (c <= 0xFB28)) ||
        ((c >= 0xFB2A) && (c <= 0xFD3D)) ||
        ((c >= 0xFD50) && (c <= 0xFDFC)) ||
        ((c >= 0xFE70) && (c <= 0xFEFC)) ||
        ((c >= 0x10800) && (c <= 0x1091B)) ||
        ((c >= 0x10920) && (c <= 0x10A00)) ||
        ((c >= 0x10A10) && (c <= 0x10A33)) ||
        ((c >= 0x10A40) && (c <= 0x10B35)) ||
        ((c >= 0x10B40) && (c <= 0x10C48)) ||
        ((c >= 0x1EE00) && (c <= 0x1EEBB))
        ) return true;
    return false;
}

std::wstring reverse_wstring(const std::wstring& inp) {
    std::wstring res;
    for (int i = inp.size() - 1; i >= 0; i--) {
        res.push_back(inp[i]);
    }
    return res;
}

bool parse_search_command(const std::wstring& search_command, int* out_begin, int* out_end, std::wstring* search_text) {
    std::wstringstream ss(search_command);
    if (search_command[0] == '<') {
        wchar_t dummy;
        ss >> dummy;
        ss >> *out_begin;
        ss >> dummy;
        ss >> *out_end;
        ss >> dummy;
        std::getline(ss, *search_text);
        return true;
    }
    else {
        *search_text = ss.str();
        return false;
    }
}

float dist_squared(fz_point p1, fz_point p2) {
    return (p1.x - p2.x) * (p1.x - p2.x) + 100 * (p1.y - p2.y) * (p1.y - p2.y);
}

bool is_text_rtl(const std::wstring& text) {
    int score = 0;
    for (int chr : text) {
        if (is_rtl(chr)) {
            score += 1;
        }
        else {
            score -= 1;
        }
    }
    return score > 0;
}

bool is_stext_line_rtl(fz_stext_line* line) {

    float rtl_count = 0.0f;
    float total_count = 0.0f;
    LL_ITER(ch, line->first_char) {
        if (is_rtl(ch->c)) {
            rtl_count += 1.0f;
        }
        total_count += 1.0f;
    }
    return ((rtl_count / total_count) > 0.5f);
}
bool is_stext_page_rtl(fz_stext_page* stext_page) {

    float rtl_count = 0.0f;
    float total_count = 0.0f;

    LL_ITER(block, stext_page->first_block) {
        if (block->type == FZ_STEXT_BLOCK_TEXT) {
            LL_ITER(line, block->u.t.first_line) {
                LL_ITER(ch, line->first_char) {
                    if (is_rtl(ch->c)) {
                        rtl_count += 1.0f;
                    }
                    total_count += 1.0f;
                }
            }
        }
    }
    return ((rtl_count / total_count) > 0.5f);
}

std::vector<fz_stext_char*> reorder_mixed_stext_line(fz_stext_line* line) {


    bool rtl = is_stext_line_rtl(line);
    std::vector<fz_stext_char*> chars = reorder_stext_line(line);
    auto should_be_reversed = [&](int c) {
        if (rtl) {
            return !is_rtl(c);
        }
        else {
            return is_rtl(c);
        }
    };

    std::vector<std::pair<int, int>> ltr_spans;

    std::optional<int> range_begin = {};

    for (int i = 0; i < chars.size(); i++) {
        if (chars[i]->c <= 64) continue;

        if (should_be_reversed(chars[i]->c)) {
            if (!range_begin.has_value()) range_begin = i;
        }
        else {
            if (range_begin.has_value()) {
                ltr_spans.push_back(std::make_pair(range_begin.value(), i - 1));
                range_begin = {};
            }
        }
    }
    if (range_begin.has_value()) {
        ltr_spans.push_back(std::make_pair(range_begin.value(), chars.size() - 1));
    }

    for (auto [begin, end] : ltr_spans) {
        std::reverse(chars.begin() + begin, chars.begin() + end + 1);
    }
    return chars;
}

std::vector<fz_stext_char*> reorder_stext_line(fz_stext_line* line) {

    std::vector<fz_stext_char*> reordered_chars;

    bool rtl = is_stext_line_rtl(line);

    LL_ITER(ch, line->first_char) {
        reordered_chars.push_back(ch);
    }

    if (rtl) {
        std::sort(reordered_chars.begin(), reordered_chars.end(), [](fz_stext_char* lhs, fz_stext_char* rhs) {
            return lhs->quad.lr.x > rhs->quad.lr.x;
            });
    }
    else {
        std::sort(reordered_chars.begin(), reordered_chars.end(), [](fz_stext_char* lhs, fz_stext_char* rhs) {
            return (lhs->quad.lr.x <= rhs->quad.lr.x) && (lhs->quad.ll.x < rhs->quad.ll.x);
            });
    }
    return reordered_chars;
}

void get_flat_chars_from_block(fz_stext_block* block, std::vector<fz_stext_char*>& flat_chars, bool dehyphenate) {
    if (block->type == FZ_STEXT_BLOCK_TEXT) {
        LL_ITER(line, block->u.t.first_line) {
            std::vector<fz_stext_char*> reordered_chars = reorder_stext_line(line);
            for (auto ch : reordered_chars) {
                if (ch->c == 65533) {
                    // unicode replacement character https://www.fileformat.info/info/unicode/char/fffd/index.htm
                    ch->c = ' ';
                }

                if (dehyphenate) {
                    if (ch->c == '-' && (ch->next == nullptr)) {
                        continue;
                    }
                }

                flat_chars.push_back(ch);
            }
        }
    }
}

bool is_index_reverse_reference_number(const std::vector<fz_stext_char*>& flat_chars, int index, int* range_begin, int* range_end) {

    if (flat_chars[index]->next != nullptr) return false;
    if (flat_chars[index]->c > 128) return false;
    if (!std::isdigit(flat_chars[index]->c)) return false;

    std::vector<char> chars_between_last_dot_and_index;

    int current_index = index-1;
    bool reached_dot = false;

    while (current_index >= 0) {
        if ((flat_chars[current_index]->c == '.') || (flat_chars[current_index]->next == nullptr)) {
            break;
        }
        chars_between_last_dot_and_index.push_back(flat_chars[current_index]->c);
        current_index--;
    }
    int n_chars = chars_between_last_dot_and_index.size();
    if (n_chars > 0 && n_chars < 20) {
        for (int i = 0; i < n_chars; i++) {
            if ((chars_between_last_dot_and_index[i] > 0) && (chars_between_last_dot_and_index[i] < 128) && std::isalpha(chars_between_last_dot_and_index[i])) {
                return false;
            }
        }
        *range_begin = current_index;
        *range_end = index;
        return true;
    }
    return false;

    //int last_dot_index

}


void get_flat_chars_from_stext_page_for_bib_detection(fz_stext_page* stext_page, std::vector<DocumentCharacter>& flat_chars) {
    LL_ITER(block, stext_page->first_block) {
        if (block->type == FZ_STEXT_BLOCK_TEXT) {
            LL_ITER(line, block->u.t.first_line) {
                std::vector<fz_stext_char*> current_line_chars;
                std::optional<DocumentCharacter> phantom_space = {};

                LL_ITER(chr, line->first_char) {
                    current_line_chars.push_back(chr);
                }

                //if (current_line_chars.size() < 5) {
                //    bool found_dot = false;
                //    for (auto c : current_line_chars) {
                //        if (c->c == '.') {
                //            found_dot = true;
                //            break;
                //        }
                //    }
                //    if (!found_dot) {
                //        continue;
                //    }
                //}
                if (current_line_chars.size() > 0) { // remove inverse references from end of lines
                    if (current_line_chars.back()->c <= 128 && (std::isdigit(current_line_chars.back()->c))) {
                        while (current_line_chars.size() > 0 && current_line_chars.back()->c != '.') {
                            current_line_chars.pop_back();
                        }
                    }
                }

                if (current_line_chars.size() > 0) { //dehyphenate
                    if (current_line_chars.back()->c == '-') {
                        current_line_chars.pop_back();
                    }
                    else {
                        DocumentCharacter ps;
                        ps.c = ' ';
                        ps.rect = fz_rect_from_quad(current_line_chars.back()->quad);
                        ps.stext_block = block;
                        ps.stext_line = line;
                        ps.stext_char = nullptr;
                        ps.is_final = true;
                        phantom_space = ps;
                    }
                }
                for (int i = 0; i < current_line_chars.size(); i++) {
                    DocumentCharacter dc;
                    dc.c = current_line_chars[i]->c;
                    dc.rect = fz_rect_from_quad(current_line_chars[i]->quad);
                    dc.stext_block = block;
                    dc.stext_line = line;
                    dc.stext_char = current_line_chars[i];
                    if (i == current_line_chars.size() - 1) {
                        if (!phantom_space.has_value()) {
                            dc.is_final = true;
                        }
                    }
                    flat_chars.push_back(dc);
                }
                if (phantom_space) {
                    flat_chars.push_back(phantom_space.value());
                }
            }
        }
    }
    //std::vector<fz_stext_char*> temp_flat_chars;
    //get_flat_chars_from_stext_page(stext_page, temp_flat_chars, true);
    //std::vector<std::pair<int, int>> ranges_to_remove;

    //for (int i = 0; i < temp_flat_chars.size(); i++) {
    //    int begin_index = -1;
    //    int end_index = -1;
    //    if (is_index_reverse_reference_number(temp_flat_chars, i, &begin_index, &end_index)) {
    //        ranges_to_remove.push_back(std::make_pair(begin_index, end_index));
    //    }
    //}

    //int current_range_index = -1;
    //if (ranges_to_remove.size() > 0) current_range_index = 0;

    //for (int i = 0; i < temp_flat_chars.size(); i++) {
    //    if ((current_range_index < ranges_to_remove.size() - 1) && (i > ranges_to_remove[current_range_index].second)) {
    //        current_range_index += 1;
    //    }

    //    if ((current_range_index == -1) || !(i > ranges_to_remove[current_range_index].first && i <= ranges_to_remove[current_range_index].second)) {
    //        flat_chars.push_back(temp_flat_chars[i]);
    //    }

    //}


}

void get_flat_chars_from_stext_page(fz_stext_page* stext_page, std::vector<fz_stext_char*>& flat_chars, bool dehyphenate) {

    LL_ITER(block, stext_page->first_block) {
        get_flat_chars_from_block(block, flat_chars, dehyphenate);
    }
}

bool is_delimeter(int c) {
    std::vector<char> delimeters = { ' ', '\n', ';', ',' };
    return std::find(delimeters.begin(), delimeters.end(), c) != delimeters.end();
}

float get_character_height(fz_stext_char* c) {
    return std::abs(c->quad.ul.y - c->quad.ll.y);
}

float get_character_width(fz_stext_char* c) {
    return std::abs(c->quad.ul.x - c->quad.ur.x);
}

bool is_start_of_new_line(fz_stext_char* prev_char, fz_stext_char* current_char) {
    float height = get_character_height(current_char);
    float threshold = height / 2;
    if (std::abs(prev_char->quad.ll.y - current_char->quad.ll.y) > threshold) {
        return true;
    }
    return false;
}
bool is_start_of_new_word(fz_stext_char* prev_char, fz_stext_char* current_char) {
    if (is_delimeter(prev_char->c)) {
        return true;
    }

    return is_start_of_new_line(prev_char, current_char);
}

PagelessDocumentRect create_word_rect(const std::vector<PagelessDocumentRect>& chars) {
    PagelessDocumentRect res;
    res.x0 = res.x1 = res.y0 = res.y1 = 0;
    if (chars.size() == 0) return res;
    res = chars[0];

    for (size_t i = 1; i < chars.size(); i++) {
        if (res.x0 > chars[i].x0) res.x0 = chars[i].x0;
        if (res.x1 < chars[i].x1) res.x1 = chars[i].x1;
        if (res.y0 > chars[i].y0) res.y0 = chars[i].y0;
        if (res.y1 < chars[i].y1) res.y1 = chars[i].y1;
    }

    return res;
}

std::vector<PagelessDocumentRect> create_word_rects_multiline(const std::vector<PagelessDocumentRect>& chars) {
    std::vector<PagelessDocumentRect> res;
    std::vector<PagelessDocumentRect> current_line_chars;

    if (chars.size() == 0) return res;
    current_line_chars.push_back(chars[0]);

    for (size_t i = 1; i < chars.size(); i++) {
        if (chars[i].x0 < chars[i - 1].x0) { // a new line has begun
            res.push_back(create_word_rect(current_line_chars));
            current_line_chars.clear();
            current_line_chars.push_back(chars[i]);
        }
        else {
            current_line_chars.push_back(chars[i]);
        }
    }
    if (current_line_chars.size() > 0) {
        res.push_back(create_word_rect(current_line_chars));
    }
    return res;
}

PagelessDocumentRect create_word_rect(const std::vector<fz_stext_char*>& chars) {
    PagelessDocumentRect res;
    res.x0 = res.x1 = res.y0 = res.y1 = 0;
    if (chars.size() == 0) return res;
    res = fz_rect_from_quad(chars[0]->quad);

    for (size_t i = 1; i < chars.size(); i++) {
        PagelessDocumentRect current_char_rect = fz_rect_from_quad(chars[i]->quad);
        if (res.x0 > current_char_rect.x0) res.x0 = current_char_rect.x0;
        if (res.x1 < current_char_rect.x1) res.x1 = current_char_rect.x1;
        if (res.y0 > current_char_rect.y0) res.y0 = current_char_rect.y0;
        if (res.y1 < current_char_rect.y1) res.y1 = current_char_rect.y1;
    }

    return res;
}

void get_flat_words_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars, std::vector<PagelessDocumentRect>& flat_word_rects, std::vector<std::vector<PagelessDocumentRect>>* out_char_rects) {

    if (flat_chars.size() == 0) return;

    std::vector<std::wstring> res;
    std::vector<fz_stext_char*> pending_word;
    pending_word.push_back(flat_chars[0]);

    for (size_t i = 1; i < flat_chars.size(); i++) {
        if (is_start_of_new_word(flat_chars[i - 1], flat_chars[i])) {
            flat_word_rects.push_back(create_word_rect(pending_word));
            if (out_char_rects != nullptr) {
                std::vector<PagelessDocumentRect> chars;
                for (auto c : pending_word) {
                    chars.push_back(fz_rect_from_quad(c->quad));
                }
                out_char_rects->push_back(chars);
            }
            if (is_start_of_new_line(flat_chars[i - 1], flat_chars[i])) {
                fz_rect new_rect = fz_rect_from_quad(flat_chars[i - 1]->quad);

                new_rect.x0 = (new_rect.x0 + 2 * new_rect.x1) / 3;
                flat_word_rects.push_back(new_rect);
                if (out_char_rects != nullptr) {
                    out_char_rects->push_back({ new_rect });
                }
            }
            pending_word.clear();
            pending_word.push_back(flat_chars[i]);
        }
        else {
            pending_word.push_back(flat_chars[i]);
        }
    }
}

void get_word_rect_list_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars,
    std::vector<std::wstring>& words,
    std::vector<std::vector<PagelessDocumentRect>>& flat_word_rects) {

    if (flat_chars.size() == 0) return;

    std::vector<fz_stext_char*> pending_word;
    pending_word.push_back(flat_chars[0]);

    auto get_rects = [&]() {
        std::vector<PagelessDocumentRect> res;
        for (auto chr : pending_word) {
            res.push_back(fz_rect_from_quad(chr->quad));
        }
        return res;
    };

    auto get_word = [&]() {
        std::wstring res;
        for (auto chr : pending_word) {
            res.push_back(chr->c);
        }
        return res;
    };

    for (size_t i = 1; i < flat_chars.size(); i++) {
        if (is_start_of_new_word(flat_chars[i - 1], flat_chars[i])) {
            flat_word_rects.push_back(get_rects());
            words.push_back(get_word());

            pending_word.clear();
            pending_word.push_back(flat_chars[i]);
        }
        else {
            pending_word.push_back(flat_chars[i]);
        }
    }
}

int get_num_tag_digits(int n) {
    int base = NUMERIC_TAGS ? 10 : 26;
    int res = 1;
    while (n > base) {
        n = n / base;
        res++;
    }
    return res;
}

std::string get_aplph_tag(int n, int max_n) {
    int n_digits = get_num_tag_digits(max_n);
    std::string tag;
    for (int i = 0; i < n_digits; i++) {
        tag.push_back('a' + (n % 26));
        n = n / 26;
    }
    std::reverse(tag.begin(), tag.end());
    return tag;
}

std::vector<std::string> get_tags(int n) {
    std::vector<std::string> res;
    int n_digits = get_num_tag_digits(n);
    for (int i = 0; i < n; i++) {
        int current_n = i;
        std::string tag;
        if (!NUMERIC_TAGS) {
            for (int i = 0; i < n_digits; i++) {
                tag.push_back('a' + (current_n % 26));
                current_n = current_n / 26;
            }
        }
        else {
            tag = std::to_string(i);
            for (int i = tag.size(); i < n_digits; i++){
                tag = "0" + tag;
            }
        }
        res.push_back(tag);
    }
    return res;
}

int get_index_from_tag(std::string tag, bool reversed) {
    if (reversed) {
        std::reverse(tag.begin(), tag.end());
    }

    int res = 0;
    int mult = 1;

    if (!NUMERIC_TAGS) {
        for (size_t i = 0; i < tag.size(); i++) {
            res += (tag[i] - 'a') * mult;
            mult = mult * 26;
        }
    }
    else {
        res = std::stoi(tag);
    }
    return res;
}

fz_stext_line* find_closest_line_to_document_point(fz_stext_page* page, fz_point document_point, int* out_index){
    float min_distance = 10000000;
    fz_stext_line* result = nullptr;
    int index = 0;
    LL_ITER(block, page->first_block) {
        if (block->type == FZ_STEXT_BLOCK_TEXT) {
            LL_ITER(line, block->u.t.first_line) {
                index++;
                if (fz_is_point_inside_rect(document_point, line->bbox)) {
                    *out_index = index;
                    return line;
                }
                float mid_y = (line->first_char->quad.ll.y + line->first_char->quad.ul.y) / 2;
                float mid_x = (line->first_char->quad.ll.x + line->first_char->quad.lr.x) / 2;
                float distance = std::abs(mid_y - document_point.y) + std::abs(mid_x - document_point.x);
                if (distance < min_distance) {
                    min_distance = distance;
                    result = line;
                    *out_index = index;
                }
            }
        }
    }
    return result;
}

fz_stext_char* find_closest_char_to_document_point(const std::vector<fz_stext_char*> flat_chars, fz_point document_point, int* location_index) {
    float min_distance = std::numeric_limits<float>::infinity();
    fz_stext_char* res = nullptr;

    int index = 0;
    for (auto current_char : flat_chars) {

        fz_point quad_center;

        quad_center.x = (current_char->quad.ll.x + current_char->quad.lr.x) / 2;
        quad_center.y = (current_char->quad.ll.y + current_char->quad.ul.y) / 2;

        float distance = dist_squared(document_point, quad_center);
        if (distance < min_distance) {
            min_distance = distance;
            res = current_char;
            *location_index = index;
        }
        index++;
    }

    return res;
}

bool is_line_separator(fz_stext_char* last_char, fz_stext_char* current_char) {
    if (last_char == nullptr) {
        return false;
    }
    float dist = abs(last_char->quad.ll.y - current_char->quad.ll.y);
    if (dist > 1.0f) {
        return true;
    }
    return false;
}

bool is_separator(fz_stext_char* last_char, fz_stext_char* current_char) {
    if (last_char == nullptr) {
        return false;
    }

    if (current_char->c == ' ') {
        return true;
    }
    float dist = abs(last_char->quad.ll.y - current_char->quad.ll.y);
    if (dist > 1.0f) {
        return true;
    }
    return false;
}


std::wstring get_string_from_stext_block(fz_stext_block* block, bool handle_rtl, bool dehyphenate) {
    if (block->type == FZ_STEXT_BLOCK_TEXT) {
        std::wstring res;
        LL_ITER(line, block->u.t.first_line) {
            res += get_string_from_stext_line(line, handle_rtl);
            if (line->next && res.size() > 0) {
                if (dehyphenate) {
                    if (res.back() == '-') {
                        res.pop_back();
                    }
                    else {
                        res.push_back(' ');
                    }
                }
            }
        }
        return res;
    }
    else {
        return L"";
    }
}
std::wstring get_string_from_stext_line(fz_stext_line* line, bool handle_rtl) {

    if (!handle_rtl) {
        std::wstring res;
        LL_ITER(ch, line->first_char) {
            res.push_back(ch->c);
        }
        return res;
    }
    else {
        std::vector<fz_stext_char*> chars = reorder_mixed_stext_line(line);
        std::wstring res;
        for (auto ch : chars) {
            res.push_back(ch->c);
        }
        return res;
    }
}

std::vector<PagelessDocumentRect> get_char_rects_from_stext_line(fz_stext_line* line) {
    std::vector<PagelessDocumentRect> res;
    LL_ITER(ch, line->first_char) {
        res.push_back(fz_rect_from_quad(ch->quad));
    }
    return res;
}

bool is_consequtive(fz_rect rect1, fz_rect rect2) {

    float xdist = abs(rect1.x1 - rect2.x1);
    float ydist1 = abs(rect1.y0 - rect2.y0);
    float ydist2 = abs(rect1.y1 - rect2.y1);
    float ydist = std::min(ydist1, ydist2);

    float rect1_width = rect1.x1 - rect1.x0;
    float rect2_width = rect2.x1 - rect2.x0;
    float average_width = (rect1_width + rect2_width) / 2.0f;

    float rect1_height = rect1.y1 - rect1.y0;
    float rect2_height = rect2.y1 - rect2.y0;
    float average_height = (rect1_height + rect2_height) / 2.0f;

    if (xdist < 3 * average_width && ydist < 2 * average_height) {
        return true;
    }

    return false;

}

fz_rect bound_rects(const std::vector<fz_rect>& rects) {
    // find the bounding box of some rects

    fz_rect res = rects[0];

    float average_y0 = 0.0f;
    float average_y1 = 0.0f;

    for (auto rect : rects) {
        if (res.x1 < rect.x1) {
            res.x1 = rect.x1;
        }
        if (res.x0 > rect.x0) {
            res.x0 = rect.x0;
        }

        average_y0 += rect.y0;
        average_y1 += rect.y1;
    }

    average_y0 /= rects.size();
    average_y1 /= rects.size();

    res.y0 = average_y0;
    res.y1 = average_y1;

    return res;

}


int next_path_separator_position(const std::wstring& path) {
    wchar_t sep1 = '/';
    wchar_t sep2 = '\\';
    int index1 = path.find(sep1);
    int index2 = path.find(sep2);
    if (index2 == -1) {
        return index1;
    }

    if (index1 == -1) {
        return index2;
    }
    return std::min(index1, index2);
}

void split_path(std::wstring path, std::vector<std::wstring>& res) {

    size_t loc = -1;
    // overflows
    while ((loc = next_path_separator_position(path)) != (size_t)-1) {

        int skiplen = loc + 1;
        if (loc != 0) {
            std::wstring part = path.substr(0, loc);
            res.push_back(part);
        }
        path = path.substr(skiplen, path.size() - skiplen);
    }
    if (path.size() > 0) {
        res.push_back(path);
    }
}


std::vector<std::wstring> split_whitespace(std::wstring const& input) {
    std::wistringstream buffer(input);
    std::vector<std::wstring> ret((std::istream_iterator<std::wstring, wchar_t>(buffer)),
        std::istream_iterator<std::wstring, wchar_t>());
    return ret;
}

void split_key_string(std::wstring haystack, const std::wstring& needle, std::vector<std::wstring>& res) {
    //todo: we can significantly reduce string allocations in this function if it turns out to be a
    //performance bottleneck.

    if (haystack == needle) {
        res.push_back(L"-");
        return;
    }

    size_t loc = -1;
    size_t needle_size = needle.size();
    while ((loc = haystack.find(needle)) != (size_t)-1) {


        int skiplen = loc + needle_size;
        if (loc != 0) {
            std::wstring part = haystack.substr(0, loc);
            res.push_back(part);
        }
        if ((loc < (haystack.size() - 1)) && (haystack.substr(needle.size(), needle.size()) == needle)) {
            // if needle is repeated, one of them is added as a token for example
            // <C-->
            // means [C, -]
            res.push_back(needle);
        }
        haystack = haystack.substr(skiplen, haystack.size() - skiplen);
    }
    if (haystack.size() > 0) {
        res.push_back(haystack);
    }
}


void run_command(std::wstring command, QStringList parameters, bool wait) {
// ios does not have QProcess
#ifndef SIOYEK_MOBILE


#ifdef Q_OS_WIN
    std::wstring parameters_string = parameters.join(" ").toStdWString();
    SHELLEXECUTEINFO ShExecInfo = { 0 };
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    if (wait) {
        ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE;
    }
    else {
        ShExecInfo.fMask = SEE_MASK_ASYNCOK | SEE_MASK_NO_CONSOLE;
    }

    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = NULL;
    ShExecInfo.lpFile = command.c_str();
    ShExecInfo.lpParameters = NULL;
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_HIDE;
    ShExecInfo.hInstApp = NULL;
    ShExecInfo.lpParameters = parameters_string.c_str();

    ShellExecuteExW(&ShExecInfo);
    if (wait) {
        WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
        CloseHandle(ShExecInfo.hProcess);
    }

#else
    // todo: use setProcessChanellMode to use the same console as the parent process
    QProcess* process = new QProcess;
    QString qcommand = QString::fromStdWString(command);
    QStringList qparameters;

    QObject::connect(process, &QProcess::errorOccurred, [process](QProcess::ProcessError error) {
        auto msg = process->errorString().toStdWString();
        show_error_message(msg);
        });

    QObject::connect(process, qOverload<int, QProcess::ExitStatus >(&QProcess::finished), [process](int exit_code, QProcess::ExitStatus stat) {
        process->deleteLater();
        });

    for (int i = 0; i < parameters.size(); i++) {
        qparameters.append(parameters[i]);
    }
    //qparameters.append(QString::fromStdWString(parameters));

    if (wait) {
        process->start(qcommand, qparameters);
        process->waitForFinished();
    }
    else {
        process->startDetached(qcommand, qparameters);
    }
#endif
#endif


}


void open_file_url(const QString& url_string, bool show_fail_message) {
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(url_string))) {
        show_error_message(("Could not open address: " + url_string).toStdWString());
    }
}

void open_file_url(const std::wstring& url_string, bool show_fail_message) {
    QString qurl_string = QString::fromStdWString(url_string);
    open_file_url(qurl_string, show_fail_message);
}

void open_web_url(const QString& url_string) {
    QDesktopServices::openUrl(QUrl(url_string));
}

void open_web_url(const std::wstring& url_string) {
    QString qurl_string = QString::fromStdWString(url_string);
    open_web_url(qurl_string);
}


void search_custom_engine(const std::wstring& search_string, const std::wstring& custom_engine_url) {

    if (search_string.size() > 0) {
        QString qurl_string = QString::fromStdWString(custom_engine_url + search_string);
        open_web_url(qurl_string);
    }
}


//void open_url(const std::wstring& url_string) {
//
//	if (url_string.size() > 0) {
//		QString qurl_string = QString::fromStdWString(url_string);
//		open_url(qurl_string);
//	}
//}
//
void create_file_if_not_exists(const std::wstring& path) {
    std::string path_utf8 = utf8_encode(path);
    if (!QFile::exists(QString::fromStdWString(path))) {
        std::ofstream outfile(path_utf8);
        outfile << "";
        outfile.close();
    }
}


void open_file(const std::wstring& path, bool show_fail_message) {
    std::wstring canon_path = get_canonical_path(path);
    open_file_url(canon_path, show_fail_message);

}

void get_text_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars, std::wstring& string_res, std::vector<int>& indices) {

    string_res.clear();
    indices.clear();

    for (size_t i = 0; i < flat_chars.size(); i++) {
        fz_stext_char* ch = flat_chars[i];

        if (ch->next == nullptr) { // add a space after the last character in a line, igonre hyphenated characters
            if (ch->c != '-') {
                string_res.push_back(ch->c);
                indices.push_back(i);

                string_res.push_back(' ');
                indices.push_back(-1);
            }
            continue;
        }
        string_res.push_back(ch->c);
        indices.push_back(i);
    }
}


std::wstring find_first_regex_match(const std::wstring& haystack, const std::wstring& regex_string) {
    std::wregex regex(regex_string);
    std::wsmatch match;
    if (std::regex_search(haystack, match, regex)) {
        return match.str();
    }
    return L"";
}

std::vector<std::wstring> find_all_regex_matches(std::wstring haystack,
    const std::wstring& regex_string,
    std::vector<std::pair<int, int>>* match_ranges) {

    std::wregex regex(regex_string);
    std::wsmatch match;
    std::vector<std::wstring> res;
    int skipped_length = 0;

    while (std::regex_search(haystack, match, regex)) {
        for (size_t i = 0; i < match.size(); i++) {
            if (match[i].matched) {
                res.push_back(match[i].str());
                if (match_ranges) {
                    int begin_index = match[i].first - haystack.begin();
                    int match_length = match[i].length();
                    match_ranges->push_back(std::make_pair(skipped_length + begin_index, skipped_length + begin_index + match_length-1));
                }
            }
        }
        skipped_length += match.prefix().length() + match.length();
        haystack = match.suffix();
    }
    return res;

}

void find_regex_matches_in_stext_page(const std::vector<fz_stext_char*>& flat_chars,
    const std::wregex& regex,
    std::vector<std::pair<int, int>>& match_ranges, std::vector<std::wstring>& match_texts) {

    std::wstring page_string;
    std::vector<int> indices;

    get_text_from_flat_chars(flat_chars, page_string, indices);

    std::wsmatch match;

    int offset = 0;
    while (std::regex_search(page_string, match, regex)) {
        int start_index = offset + match.position();
        int end_index = start_index + match.length() - 1;
        match_ranges.push_back(std::make_pair(indices[start_index], indices[end_index]));
        match_texts.push_back(match.str());

        int old_length = page_string.size();
        page_string = match.suffix();
        int new_length = page_string.size();

        offset += (old_length - new_length);
    }
}

bool are_stext_chars_far_enough_for_equation(fz_stext_char* first, fz_stext_char* second) {
    float second_width = second->quad.lr.x - second->quad.ll.x;

    if (second_width < 0) {
        return false;
    }

    return (second->origin.x - first->origin.x) > (5 * second_width);
}

bool is_whitespace(int chr) {
    if ((chr == ' ') || (chr == '\n') || (chr == '\t')) {
        return true;
    }
    return false;
}

std::wstring strip_string(std::wstring& input_string) {

    std::wstring result;
    int start_index = 0;
    int end_index = input_string.size() - 1;
    if (input_string.size() == 0) {
        return L"";
    }

    while (is_whitespace(input_string[start_index])) {
        start_index++;
        if ((size_t)start_index == input_string.size()) {
            return L"";
        }
    }

    while (is_whitespace(input_string[end_index])) {
        end_index--;
    }
    return input_string.substr(start_index, end_index - start_index + 1);

}

void index_generic(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::vector<IndexedData>& indices) {

    std::wstring page_string;
    std::vector<std::optional<fz_rect>> page_rects;

    for (auto ch : flat_chars) {
        page_string.push_back(ch->c);
        page_rects.push_back(fz_rect_from_quad(ch->quad));
        if (ch->next == nullptr) {
            page_string.push_back('\n');
            page_rects.push_back({});
        }
    }

    std::wregex index_dst_regex(L"(^|\n)[A-Z][a-zA-Z]{2,}\\.?[ \t]+[0-9]+((\\.|\\-)[0-9]+)*");
    //std::wregex index_dst_regex(L"(^|\n)[A-Z][a-zA-Z]{2,}[ \t]+[0-9]+(\-[0-9]+)*");
    //std::wregex index_src_regex(L"[a-zA-Z]{3,}[ \t]+[0-9]+(\.[0-9]+)*");
    std::wsmatch match;


    int offset = 0;
    while (std::regex_search(page_string, match, index_dst_regex)) {

        IndexedData new_data;
        new_data.page = page_number;
        std::wstring match_string = match.str();
        new_data.text = strip_string(match_string);
        new_data.y_offset = 0.0f;

        int match_start_index = match.position();
        int match_size = match_string.size();
        for (int i = 0; i < match_size; i++) {
            int index = offset + match_start_index + i;
            if (page_rects[index]) {
                new_data.y_offset = page_rects[index].value().y0;
                break;
            }
        }
        offset += match_start_index + match_size;
        page_string = match.suffix();

        indices.push_back(new_data);
    }
}

void index_equations(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::map<std::wstring, std::vector<IndexedData>>& indices) {
    std::wregex regex(L"\\([0-9]+(\\.[0-9]+)*\\)");
    std::vector<std::pair<int, int>> match_ranges;
    std::vector<std::wstring> match_texts;

    find_regex_matches_in_stext_page(flat_chars, regex, match_ranges, match_texts);

    for (size_t i = 0; i < match_ranges.size(); i++) {
        auto [start_index, end_index] = match_ranges[i];
        if (start_index == -1 || end_index == -1) {
            break;
        }


        // we expect the equation reference to be sufficiently separated from the rest of the text
        if (((start_index > 0) && are_stext_chars_far_enough_for_equation(flat_chars[start_index - 1], flat_chars[start_index]))) {

            std::wstring match_text = match_texts[i].substr(1, match_texts[i].size() - 2);
            IndexedData indexed_equation;
            indexed_equation.page = page_number;
            indexed_equation.text = match_text;
            indexed_equation.y_offset = flat_chars[start_index]->quad.ll.y;
            if (indices.find(match_text) == indices.end()) {
                indices[match_text] = std::vector<IndexedData>();
                indices[match_text].push_back(indexed_equation);
            }
            else {
                indices[match_text].push_back(indexed_equation);
            }
        }
    }

}

void index_references(fz_stext_page* page, int page_number, std::map<std::wstring, IndexedData>& indices) {

    char start_char = '[';
    char end_char = ']';
    char delim_char = ',';

    bool started = false;
    std::vector<IndexedData> temp_indices;
    std::wstring current_text = L"";

    LL_ITER(block, page->first_block) {
        if (block->type != FZ_STEXT_BLOCK_TEXT) continue;

        LL_ITER(line, block->u.t.first_line) {
            int chars_in_line = 0;
            LL_ITER(ch, line->first_char) {
                chars_in_line++;
                if (ch->c == ' ') {
                    continue;
                }
                //if (ch->c == '.') {
                //	started = false;
                //	temp_indices.clear();
                //	current_text.clear();
                //}

                // references are usually at the beginning of the line, we consider the possibility
                // of up to 3 extra characters before the reference (e.g. numbers, extra spaces, etc.)
                if ((ch->c == start_char) && (chars_in_line < 4)) {
                    temp_indices.clear();
                    current_text.clear();
                    started = true;
                    continue;
                }
                if (ch->c == end_char) {
                    started = false;

                    IndexedData index_data;
                    index_data.page = page_number;
                    index_data.y_offset = ch->quad.ll.y;
                    index_data.text = current_text;

                    temp_indices.push_back(index_data);
                    //indices[text] = index_data;

                    for (auto index : temp_indices) {
                        indices[index.text] = index;
                    }
                    current_text.clear();
                    temp_indices.clear();
                    continue;
                }
                if (started && (ch->c == delim_char)) {
                    IndexedData index_data;
                    index_data.page = page_number;
                    index_data.y_offset = ch->quad.ll.y;
                    index_data.text = current_text;
                    current_text.clear();
                    temp_indices.push_back(index_data);
                    continue;
                }
                if (started) {
                    current_text.push_back(ch->c);
                }
            }

        }
    }
}

void sleep_ms(unsigned int ms) {
#ifdef Q_OS_WIN
    Sleep(ms);
#else
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
#endif
}

void get_pixmap_pixel(fz_pixmap* pixmap, int x, int y, unsigned char* r, unsigned char* g, unsigned char* b) {
    if (
        (x < 0) ||
        (y < 0) ||
        (x >= pixmap->w) ||
        (y >= pixmap->h)
        ) {
        *r = 0;
        *g = 0;
        *b = 0;
        return;
    }

    (*r) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 0];
    (*g) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 1];
    (*b) = pixmap->samples[y * pixmap->w * 3 + x * 3 + 2];
}

int find_max_horizontal_line_length_at_pos(fz_pixmap* pixmap, int pos_x, int pos_y) {
    int min_x = pos_x;
    int max_x = pos_x;

    while (true) {
        unsigned char r, g, b;
        get_pixmap_pixel(pixmap, min_x, pos_y, &r, &g, &b);
        if ((r != 255) || (g != 255) || (b != 255)) {
            break;
        }
        else {
            min_x--;
        }
    }
    while (true) {
        unsigned char r, g, b;
        get_pixmap_pixel(pixmap, max_x, pos_y, &r, &g, &b);
        if ((r != 255) || (g != 255) || (b != 255)) {
            break;
        }
        else {
            max_x++;
        }
    }
    return max_x - min_x;
}

bool largest_contigous_ones(std::vector<int>& arr, int* start_index, int* end_index) {
    arr.push_back(0);

    bool has_at_least_one_one = false;

    for (auto x : arr) {
        if (x == 1) {
            has_at_least_one_one = true;
        }
    }
    if (!has_at_least_one_one) {
        return false;
    }


    int max_count = 0;
    int max_start_index = -1;
    int max_end_index = -1;

    int count = 0;

    for (size_t i = 0; i < arr.size(); i++) {
        if (arr[i] == 1) {
            count++;
        }
        else {
            if (count > max_count) {
                max_count = count;
                max_end_index = i - 1;
                max_start_index = i - count;
            }
            count = 0;
        }
    }

    *start_index = max_start_index;
    *end_index = max_end_index;
    return true;
}

std::vector<unsigned int> get_max_width_histogram_from_pixmap(fz_pixmap* pixmap) {
    std::vector<unsigned int> res;

    for (int j = 0; j < pixmap->h; j++) {
        unsigned int x_value = 0;
        for (int i = 0; i < pixmap->w; i++) {
            unsigned char r, g, b;
            get_pixmap_pixel(pixmap, i, j, &r, &g, &b);
            float lightness = (static_cast<float>(r) + static_cast<float>(g) + static_cast<float>(b)) / 3.0f;

            if (lightness > 150) {
                x_value += 1;
            }
        }
        res.push_back(x_value);
    }

    return res;
}

template<typename T>
float average_value(std::vector<T> values) {
    T sum = 0;
    for (auto x : values) {
        sum += x;
    }
    return static_cast<float>(sum) / values.size();
}

template<typename T>
float standard_deviation(std::vector<T> values, float average_value) {
    T sum = 0;
    for (auto x : values) {
        sum += (x - average_value) * (x - average_value);
    }
    return std::sqrt(static_cast<float>(sum) / values.size());
}

//std::vector<unsigned int> get_line_ends_from_histogram(std::vector<unsigned int> histogram) {
void get_line_begins_and_ends_from_histogram(std::vector<unsigned int> histogram, std::vector<unsigned int>& res_begins, std::vector<unsigned int>& res) {

    float mean_width = average_value(histogram);
    float std = standard_deviation(histogram, mean_width);

    std::vector<float> normalized_histogram;

    if (std < 0.00001f) {
        return;
    }

    for (auto x : histogram) {
        normalized_histogram.push_back((x - mean_width) / std);
    }

    size_t i = 0;

    while (i < histogram.size()) {

        while ((i < histogram.size()) && (normalized_histogram[i] > 0.2f)) i++;
        res_begins.push_back(i);
        while ((i < histogram.size()) && (normalized_histogram[i] <= 0.21f)) i++;
        if (i == histogram.size()) break;
        res.push_back(i);
    }

    while (res_begins.size() > res.size()) {
        res_begins.pop_back();
    }

    float additional_distance = 0.0f;

    if (res.size() > 5) {
        std::vector<float> line_distances;

        for (size_t i = 0; i < res.size() - 1; i++) {
            line_distances.push_back(res[i + 1] - res[i]);
        }
        std::nth_element(line_distances.begin(), line_distances.begin() + line_distances.size() / 2, line_distances.end());
        additional_distance = line_distances[line_distances.size() / 2];

        for (size_t i = 0; i < res.size(); i++) {
            res[i] += static_cast<unsigned int>(additional_distance / 5.0f);
            res_begins[i] -= static_cast<unsigned int>(additional_distance / 5.0f);
        }

    }

}

int find_best_vertical_line_location(fz_pixmap* pixmap, int doc_x, int doc_y) {


    int search_height = 5;

    float min_candid_y = doc_y;
    float max_candid_y = doc_y + search_height;

    std::vector<int> max_possible_widths;

    for (int candid_y = min_candid_y; candid_y <= max_candid_y; candid_y++) {
        int current_width = find_max_horizontal_line_length_at_pos(pixmap, doc_x, candid_y);
        max_possible_widths.push_back(current_width);
    }

    int max_width_value = -1;

    for (auto w : max_possible_widths) {
        if (w > max_width_value) {
            max_width_value = w;
        }
    }
    std::vector<int> is_max_list;

    for (auto x : max_possible_widths) {
        if (x == max_width_value) {
            is_max_list.push_back(1);
        }
        else {
            is_max_list.push_back(0);
        }
    }

    int start_index, end_index;
    largest_contigous_ones(is_max_list, &start_index, &end_index);

    //return doc_y + (start_index + end_index) / 2;
    return doc_y + start_index;
}

bool is_string_numeric_(const std::wstring& str) {
    if (str.size() == 0) {
        return false;
    }
    for (auto ch : str) {
        if (!std::isdigit(ch)) {
            return false;
        }
    }
    return true;
}

bool is_string_numeric(const std::wstring& str) {
    if (str.size() == 0) {
        return false;
    }

    if (str[0] == '-') {
        return is_string_numeric_(str.substr(1, str.length() - 1));
    }
    else {
        return is_string_numeric_(str);
    }
}


bool is_string_numeric_float(const std::wstring& str) {
    if (str.size() == 0) {
        return false;
    }
    int dot_count = 0;

    for (size_t i = 0; i < str.size(); i++) {
        if (i == 0) {
            if (str[i] == '-') continue;
        }
        else {
            if (str[i] == '.') {
                dot_count++;
                if (dot_count >= 2) return false;
            }
            else if (!std::isdigit(str[i])) {
                return false;
            }
        }

    }
    return true;
}

QByteArray serialize_string_array(const QStringList& string_list) {
    QByteArray result;
    QDataStream stream(&result, QIODevice::WriteOnly);
    stream << static_cast<int>(string_list.size());
    for (int i = 0; i < string_list.size(); i++) {
        stream << string_list.at(i);
    }
    return result;
}


QStringList deserialize_string_array(const QByteArray& byte_array) {
    QStringList result;
    QDataStream stream(byte_array);

    int size;
    stream >> size;

    for (int i = 0; i < size; i++) {
        QString string;
        stream >> string;
        result.append(string);
    }

    return result;
}




char* get_argv_value(int argc, char** argv, std::string key) {
    for (int i = 0; i < argc - 1; i++) {
        if (key == argv[i]) {
            return argv[i + 1];
        }
    }
    return nullptr;
}

bool has_arg(int argc, char** argv, std::string key) {
    for (int i = 0; i < argc; i++) {
        if (key == argv[i]) {
            return true;
        }
    }
    return false;
}
bool should_reuse_instance(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        if (std::strcmp(argv[i], "--reuse-instance") == 0) return true;

    }
    return false;
}

bool should_new_instance(int argc, char** argv) {
    for (int i = 0; i < argc; i++) {
        if (std::strcmp(argv[i], "--new-instance") == 0) return true;

    }
    return false;
}

QCommandLineParser* get_command_line_parser() {

    QCommandLineParser* parser = new QCommandLineParser();

    parser->setApplicationDescription("Sioyek is a PDF reader designed for reading research papers and technical books.");
    //parser->addVersionOption();

    //QCommandLineOption reuse_instance_option("reuse-instance");
    //reuse_instance_option.setDescription("When opening a new file, reuse the previous instance of sioyek instead of opening a new window.");
    //parser->addOption(reuse_instance_option);

    QCommandLineOption new_instance_option("new-instance");
    new_instance_option.setDescription("When opening a new file, create a new instance of sioyek.");
    parser->addOption(new_instance_option);

    QCommandLineOption instance_name_option("instance-name", "Select a specific sioyek instance by name.", "name");
    parser->addOption(instance_name_option);

    QCommandLineOption new_window_option("new-window");
    new_window_option.setDescription("Open the file in a new window but within the same sioyek instance (reuses the previous window if a sioyek window with the same file already exists).");
    parser->addOption(new_window_option);

    QCommandLineOption reuse_window_option("reuse-window");
    reuse_window_option.setDescription("Force sioyek to reuse the current window even when should_launch_new_window is set.");
    parser->addOption(reuse_window_option);

    QCommandLineOption nofocus_option("nofocus");
    nofocus_option.setDescription("Do not bring the sioyek instance to foreground.");
    parser->addOption(nofocus_option);

    QCommandLineOption version_option("version");
    version_option.setDescription("Print sioyek version.");
    parser->addOption(version_option);


    QCommandLineOption page_option("page", "Which page to open.", "page");
    parser->addOption(page_option);

    QCommandLineOption focus_option("focus-text", "Set a visual mark on line including <text>.", "focus-text");
    parser->addOption(focus_option);

    QCommandLineOption focus_page_option("focus-text-page", "Specifies the page which is used for focus-text", "focus-text-page");
    parser->addOption(focus_page_option);


    QCommandLineOption inverse_search_option("inverse-search", "The command to execute when performing inverse search.\
 In <command>, %1 is filled with the file name and %2 is filled with the line number.", "command");
    parser->addOption(inverse_search_option);

    QCommandLineOption command_option("execute-command", "The command to execute on running instance of sioyek", "execute-command");
    parser->addOption(command_option);

    QCommandLineOption command_data_option("execute-command-data", "Optional data for execute-command command", "execute-command-data");
    parser->addOption(command_data_option);

    QCommandLineOption forward_search_file_option("forward-search-file", "Perform forward search on file <file>. You must also include --forward-search-line to specify the line", "file");
    parser->addOption(forward_search_file_option);

    QCommandLineOption forward_search_line_option("forward-search-line", "Perform forward search on line <line>. You must also include --forward-search-file to specify the file", "line");
    parser->addOption(forward_search_line_option);

    QCommandLineOption forward_search_column_option("forward-search-column", "Perform forward search on column <column>. You must also include --forward-search-file to specify the file", "column");
    parser->addOption(forward_search_column_option);

    QCommandLineOption zoom_level_option("zoom", "Set zoom level to <zoom>.", "zoom");
    parser->addOption(zoom_level_option);

    QCommandLineOption file_path_option("file-path", "The pdf file path.", "file");
    parser->addOption(file_path_option);

    QCommandLineOption xloc_option("xloc", "Set x position within page to <xloc>.", "xloc");
    parser->addOption(xloc_option);

    QCommandLineOption yloc_option("yloc", "Set y position within page to <yloc>.", "yloc");
    parser->addOption(yloc_option);

    QCommandLineOption window_id_option("window-id", "Apply command to window with id <window-id>", "window-id");
    parser->addOption(window_id_option);

    QCommandLineOption shared_database_path_option("shared-database-path", "Specify which file to use for shared data (bookmarks, highlights, etc.)", "path");
    parser->addOption(shared_database_path_option);

    QCommandLineOption local_database_path_option("local-database-path", "Specify which file to use for local data", "path");
    parser->addOption(local_database_path_option);

    QCommandLineOption last_file_path_option("last-file-path", "Specify which file to use for last file location.", "path");
    parser->addOption(last_file_path_option);

    QCommandLineOption force_drawings_path_option("force-drawing-path", "Specify the path of drawings file to use (this is mainly used for testing and documentation, probably not very useful for end-users)", "path");
    parser->addOption(force_drawings_path_option);

    QCommandLineOption force_annotations_option("force-annotations-path", "Specify the path of annotations file to use (this is mainly used for testing and documentation, probably not very useful for end-users)", "path");
    parser->addOption(force_annotations_option);

    QCommandLineOption verbose_option("verbose", "Print extra information in commnad line.");
    parser->addOption(verbose_option);

    QCommandLineOption wait_for_response_option("wait-for-response", "Wait for the command to finish before returning.");
    parser->addOption(wait_for_response_option);

    QCommandLineOption no_auto_config_option("no-auto-config", "Disables all config files except the ones next to the executable. Used mainly for testing.");
    parser->addOption(no_auto_config_option);

    parser->addHelpOption();

    return parser;
}


std::wstring concatenate_path(const std::wstring& prefix, const std::wstring& suffix) {
    std::wstring result = prefix;
#ifdef Q_OS_WIN
    wchar_t separator = '\\';
#else
    wchar_t separator = '/';
#endif
    if (prefix == L"") {
        return suffix;
    }

    if (result[result.size() - 1] != separator) {
        result.push_back(separator);
    }
    result.append(suffix);
    return result;
}

std::wstring get_canonical_path(const std::wstring& path) {
#ifdef SIOYEK_MOBILE
    if (path.size() > 0) {
        if (path[0] == ':') { // it is a resouce file
            return path;
        }
        if (path.substr(0, 9) == L"content:/") {
            return path;
        }
        else {
            QDir dir(QString::fromStdWString(path));
            return std::move(dir.absolutePath().toStdWString());
        }
    }
    else {
        return L"";
    }
#else
    QDir dir(QString::fromStdWString(path));
    //return std::move(dir.canonicalPath().toStdWString());
    return std::move(dir.absolutePath().toStdWString());
#endif

}

std::wstring add_redundant_dot_to_path(const std::wstring& path) {
    std::vector<std::wstring> parts;
    split_path(path, parts);

    std::wstring last = parts[parts.size() - 1];
    parts.erase(parts.begin() + parts.size() - 1);
    parts.push_back(L".");
    parts.push_back(last);

    std::wstring res = L"";
    if (path[0] == '/') {
        res = L"/";
    }

    for (size_t i = 0; i < parts.size(); i++) {
        res.append(parts[i]);
        if (i < parts.size() - 1) {
            res.append(L"/");
        }
    }
    return res;
}

float manhattan_distance(float x1, float y1, float x2, float y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

float manhattan_distance(fvec2 v1, fvec2 v2) {
    return manhattan_distance(v1.x(), v1.y(), v2.x(), v2.y());
}

QWidget* get_top_level_widget(QWidget* widget) {
    while (widget->parent() != nullptr) {
        widget = widget->parentWidget();
    }
    return widget;
}

float type_name_similarity_score(std::wstring name1, std::wstring name2) {
    name1 = to_lower(name1);
    name2 = to_lower(name2);
    size_t common_prefix_index = 0;

    while (name1[common_prefix_index] == name2[common_prefix_index]) {
        common_prefix_index++;
        if ((common_prefix_index == name1.size()) || (common_prefix_index == name2.size())) {
            return common_prefix_index;
        }
    }
    return common_prefix_index;
}

void check_for_updates(QWidget* parent, std::string current_version) {

    return;
    //QString url = "https://github.com/ahrm/sioyek/releases/latest";
    //QNetworkAccessManager* manager = new QNetworkAccessManager;

    //QObject::connect(manager, &QNetworkAccessManager::finished, [=](QNetworkReply *reply) {
    //	std::string response_text = reply->readAll().toStdString();
    //	int first_index = response_text.find("\"");
    //	int last_index = response_text.rfind("\"");
    //	std::string url_string = response_text.substr(first_index + 1, last_index - first_index - 1);

    //	std::vector<std::wstring> parts;
    //	split_path(utf8_decode(url_string), parts);
    //	if (parts.size() > 0) {
    //		std::string version_string = utf8_encode(parts.back().substr(1, parts.back().size() - 1));

    //		if (version_string != current_version) {
    //			int ret = QMessageBox::information(parent, "Update", QString::fromStdString("Do you want to update from " + current_version + " to " + version_string + "?"),
    //				QMessageBox::Ok | QMessageBox::Cancel,
    //				QMessageBox::Cancel);
    //			if (ret == QMessageBox::Ok) {
    //				open_web_url(url);
    //			}
    //		}

    //	}
    //	});
    //manager->get(QNetworkRequest(QUrl(url)));
}

QString expand_home_dir(QString path) {
    if (path.size() > 0) {
        if (path.at(0) == '~') {
            return QDir::homePath() + QDir::separator() + path.remove(0, 2);
        }
    }
    return path;
}

void split_root_file(QString path, QString& out_root, QString& out_partial) {

    QChar sep = QDir::separator();
    if (path.indexOf(sep) == -1) {
        sep = '/';
    }

    QStringList parts = path.split(sep);

    if (path.size() > 0) {
        if (path.at(path.size() - 1) == sep) {
            out_root = parts.join(sep);
        }
        else {
            if ((parts.size() == 2) && (path.at(0) == '/')) {
                out_root = "/";
                out_partial = parts.at(1);
            }
            else {
                out_partial = parts.at(parts.size() - 1);
                parts.pop_back();
                out_root = parts.join(QDir::separator());
            }
        }
    }
    else {
        out_partial = "";
        out_root = "";
    }
}


std::wstring lowercase(const std::wstring& input) {

    std::wstring res;
    for (int i = 0; i < input.size(); i++) {
        if ((input[i] >= 'A') && (input[i] <= 'Z')) {
            res.push_back(input[i] + 'a' - 'A');
        }
        else {
            res.push_back(input[i]);
        }
    }
    return res;
}

int hex2int(int hex) {
    if (hex >= '0' && hex <= '9') {
        return hex - '0';
    }
    else {
        return (hex - 'a') + 10;
    }
}
float get_color_component_from_hex(std::wstring hexcolor) {
    hexcolor = lowercase(hexcolor);

    if (hexcolor.size() < 2) {
        return 0;
    }
    return static_cast<float>(hex2int(hexcolor[0]) * 16 + hex2int(hexcolor[1])) / 255.0f;
}

QString get_color_hexadecimal(float color) {
    QString hex_map[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c", "d", "e", "f" };
    int val = static_cast<int>(color * 255);
    int high = val / 16;
    int low = val % 16;
    return QString("%1%2").arg(hex_map[high], hex_map[low]);

}

QString get_color_qml_string(float r, float g, float b) {
    QString res = QString("#%1%2%3").arg(get_color_hexadecimal(r), get_color_hexadecimal(g), get_color_hexadecimal(b));
    return res;
}


void hexademical_to_normalized_color(std::wstring color_string, float* color, int n_components) {
    if (color_string[0] == '#') {
        color_string = color_string.substr(1, color_string.size() - 1);
    }

    for (int i = 0; i < n_components; i++) {
        *(color + i) = get_color_component_from_hex(color_string.substr(i * 2, 2));
    }
}

void copy_file(std::wstring src_path, std::wstring dst_path) {
    std::ifstream  src(utf8_encode(src_path), std::ios::binary);
    std::ofstream  dst(utf8_encode(dst_path), std::ios::binary);

    dst << src.rdbuf();
}

fz_quad quad_from_rect(fz_rect r)
{
    fz_quad q;
    q.ul = fz_make_point(r.x0, r.y0);
    q.ur = fz_make_point(r.x1, r.y0);
    q.ll = fz_make_point(r.x0, r.y1);
    q.lr = fz_make_point(r.x1, r.y1);
    return q;
}


std::wifstream open_wifstream(const std::wstring& file_name) {

#ifdef Q_OS_WIN
    return std::move(std::wifstream(file_name));
#else
    std::string encoded_file_name = utf8_encode(file_name);
    return std::move(std::wifstream(encoded_file_name.c_str()));
#endif
}

std::wofstream open_wofstream(const std::wstring& file_name) {

#ifdef Q_OS_WIN
    return std::move(std::wofstream(file_name));
#else
    std::string encoded_file_name = utf8_encode(file_name);
    return std::move(std::wofstream(encoded_file_name.c_str()));
#endif
}

std::wstring truncate_string(const std::wstring& inp, int size) {
    if (inp.size() <= (size_t)size) {
        return inp;
    }
    else {
        return inp.substr(0, size - 3) + L"...";
    }

}

std::wstring get_page_formatted_string(int page) {
    std::wstringstream ss;
    ss << L"[ " << page << L" ]";
    return ss.str();
}

bool is_string_titlish(const std::wstring& str) {
    if (str.size() <= 5 || str.size() >= 60) {
        return false;
    }
    std::wregex regex(L"([0-9IVXC]+\\.)+([0-9IVXC]+)*");
    std::wsmatch match;


    std::regex_search(str, match, regex);
    int pos = match.position();
    int size = match.length();
    int numeric_count = 0;

    // the above regex can match numeric values like 0.975, here we use a heuristic
    // to check if the string is a title by checking if it has less than 20% of numeric values
    for (int i = 0; i < str.size(); i++) {
        if ((str[i] < 128) && std::isdigit(str[i])) {
            numeric_count++;
        }
    }
    int max_numeric_count = static_cast<int>(str.size() * 0.2f);

    return (size > 0) && (pos == 0) && (numeric_count <= max_numeric_count);
}

bool is_title_parent_of(const std::wstring& parent_title, const std::wstring& child_title, bool* are_same) {
    int count = std::min(parent_title.size(), child_title.size());

    *are_same = false;

    for (int i = 0; i < count; i++) {
        if (parent_title.at(i) == ' ') {
            if (child_title.at(i) == ' ') {
                *are_same = true;
                return false;
            }
            else {
                return true;
            }
        }
        if (child_title.at(i) != parent_title.at(i)) {
            return false;
        }
    }

    return true;
}

struct Range {
    float begin;
    float end;

    float size() {
        return end - begin;
    }
};

struct Range merge_range(struct Range range1, struct Range range2) {
    struct Range res;
    res.begin = std::min(range1.begin, range2.begin);
    res.end = std::max(range1.end, range2.end);
    return res;
}

float line_num_penalty(int num) {
    return 1.0f;
    //if (num == 1) {
    //	return 1.0f;
    //}
    //return 1.0f + static_cast<float>(num) / 5.0f;
}

float height_increase_penalty(float ratio) {
    return  50 * ratio;
}

float width_increase_bonus(float ratio) {
    return -50 * ratio;
}

int find_best_merge_index_for_line_index(const std::vector<fz_stext_line*>& lines,
    const std::vector<PagelessDocumentRect>& line_rects,
    const std::vector<int> char_counts,
    int index) {

    return index;
    int max_merged_lines = 40;
    //Range current_range = { lines[index]->bbox.y0, lines[index]->bbox.y1 };
    //Range current_range_x = { lines[index]->bbox.x0, lines[index]->bbox.x1 };
    struct Range current_range = { line_rects[index].y0, line_rects[index].y1 };
    struct Range current_range_x = { line_rects[index].x0, line_rects[index].x1 };
    float maximum_height = current_range.size();
    float maximum_width = current_range_x.size();
    float min_cost = current_range.size() * line_num_penalty(1) / current_range_x.size();
    int min_index = index;

    for (size_t j = index + 1; (j < lines.size()) && ((j - index) < (size_t)max_merged_lines); j++) {
        float line_height = line_rects[j].y1 - line_rects[j].y0;
        float line_width = line_rects[j].x1 - line_rects[j].x0;
        if (line_height > maximum_height) {
            maximum_height = line_height;
        }
        if (line_width > maximum_width) {
            maximum_width = line_width;
        }
        if (char_counts[j] > 10) {
            current_range = merge_range(current_range, { line_rects[j].y0, line_rects[j].y1 });
        }

        current_range_x = merge_range(current_range, { line_rects[j].x0, line_rects[j].x1 });

        float cost = current_range.size() / (j - index + 1) +
            line_num_penalty(j - index + 1) / current_range_x.size() +
            height_increase_penalty(current_range.size() / maximum_height) +
            width_increase_bonus(current_range_x.size() / maximum_width);
        if (cost < min_cost) {
            min_cost = cost;
            min_index = j;
        }
    }
    return min_index;
}


fz_rect get_line_rect(fz_stext_line* line) {
    fz_rect res;
    res.x0 = res.x1 = res.y0 = res.y1 = 0;

    if (line == nullptr || line->first_char == nullptr) {
        return res;
    }

    res = fz_rect_from_quad(line->first_char->quad);

    std::vector<float> char_x_begins;
    std::vector<float> char_x_ends;
    std::vector<float> char_y_begins;
    std::vector<float> char_y_ends;

    int num_chars = 0;
    LL_ITER(chr, line->first_char) {
        fz_rect char_rect = fz_union_rect(res, fz_rect_from_quad(chr->quad));
        char_x_ends.push_back(char_rect.x1);
        char_y_begins.push_back(char_rect.y0);
        char_y_ends.push_back(char_rect.y1);

        if (char_rect.x0 > 0) {
            char_x_begins.push_back(char_rect.x0);
        }
        num_chars++;
    }


    int percentile_index = static_cast<int>(0.5f * num_chars);
    int first_percentile_index = percentile_index;
    int last_percentile_index = num_chars - percentile_index;

    if (last_percentile_index >= num_chars) {
        last_percentile_index = num_chars - 1;
    }

    if (char_x_begins.size() > 0) {
        res.x0 = *std::min_element(char_x_begins.begin(), char_x_begins.end());
        res.x1 = *std::max_element(char_x_ends.begin(), char_x_ends.end());
    }
    if (char_y_begins.size() > 0) {
        std::nth_element(char_y_begins.begin(), char_y_begins.begin() + first_percentile_index, char_y_begins.end());
        std::nth_element(char_y_ends.begin(), char_y_ends.begin() + last_percentile_index, char_y_ends.end());
        res.y0 = *(char_y_begins.begin() + first_percentile_index);
        res.y1 = *(char_y_ends.begin() + last_percentile_index);
    }

    return res;
}

int line_num_chars(fz_stext_line* line) {
    int res = 0;
    LL_ITER(chr, line->first_char) {
        res++;
    }
    return res;
}


//struct PageMergedLinesInfo {
//    std::vector<PagelessDocumentRect> merged_line_rects;
//    std::vector<std::wstring> merged_line_texts;
//    std::vector<std::vector<PagelessDocumentRect>> merged_line_chars;
//    std::vector<std::vector<int>> merged_line_indices;
//};

PageMergedLinesInfo merge_lines2(const std::vector<fz_stext_line*>& lines) {

    PageMergedLinesInfo res;

    std::vector<PagelessDocumentRect> temp_rects;
    std::vector<std::wstring> temp_texts;
    std::vector<std::vector<PagelessDocumentRect>> temp_line_chars;

    std::vector<PagelessDocumentRect> custom_line_rects;
    std::vector<int> char_counts;

    std::vector<int> indices_to_delete;
    for (size_t i = 0; i < lines.size(); i++) {
        if (line_num_chars(lines[i]) < 5) {
            indices_to_delete.push_back(i);
        }
    }

    //for (int i = indices_to_delete.size() - 1; i >= 0; i--) {
    //    lines.erase(lines.begin() + indices_to_delete[i]);
    //}

    for (auto line : lines) {
        custom_line_rects.push_back(get_line_rect(line));
        char_counts.push_back(line_num_chars(line));
    }

    for (size_t i = 0; i < lines.size(); i++) {
        PagelessDocumentRect rect = custom_line_rects[i];
        int best_index = find_best_merge_index_for_line_index(lines, custom_line_rects, char_counts, i);
        std::wstring text = get_string_from_stext_line(lines[i]);
        std::vector<PagelessDocumentRect> line_chars = get_char_rects_from_stext_line(lines[i]);

        for (int j = i + 1; j <= best_index; j++) {
            rect = fz_union_rect(rect, lines[j]->bbox);
            text = text + get_string_from_stext_line(lines[j]);
            std::vector<PagelessDocumentRect> merged_line_chars = get_char_rects_from_stext_line(lines[j]);
            line_chars.insert(line_chars.end(), merged_line_chars.begin(), merged_line_chars.end());
        }

        temp_rects.push_back(rect);
        temp_texts.push_back(text);
        temp_line_chars.push_back(line_chars);
        i = best_index;
    }

    for (size_t i = 0; i < temp_rects.size(); i++) {

        if (std::binary_search(indices_to_delete.begin(), indices_to_delete.end(), i)) {
            continue;
        }

        if (i > 0 && res.merged_line_rects.size() > 0) {
            fz_rect prev_rect = res.merged_line_rects.back();
            fz_rect current_rect = temp_rects[i];
            if ((std::abs(prev_rect.y0 - current_rect.y0) < 1.0f) || (std::abs(prev_rect.y1 - current_rect.y1) < 1.0f)) {
                res.merged_line_rects.back().x0 = std::min(prev_rect.x0, current_rect.x0);
                res.merged_line_rects.back().x1 = std::max(prev_rect.x1, current_rect.x1);

                res.merged_line_rects.back().y0 = std::min(prev_rect.y0, current_rect.y0);
                res.merged_line_rects.back().y1 = std::max(prev_rect.y1, current_rect.y1);
                res.merged_line_texts.back() = res.merged_line_texts.back() + temp_texts[i];
                res.merged_line_chars.back().insert(
                    res.merged_line_chars.back().end(), temp_line_chars[i].begin(), temp_line_chars[i].end()
                );
                res.merged_line_indices.back().push_back(i);
                continue;
            }
        }
        res.merged_line_rects.push_back(temp_rects[i]);
        res.merged_line_texts.push_back(temp_texts[i]);
        res.merged_line_indices.push_back({(int)i});

        res.merged_line_chars.push_back(temp_line_chars[i]);
    }

    return res;
}
//void merge_lines(
//    std::vector<fz_stext_line*> lines,
//    std::vector<PagelessDocumentRect>& out_rects,
//    std::vector<std::wstring>& out_texts,
//    std::vector<std::vector<PagelessDocumentRect>>* out_line_chars,
//    std::vector<PagelessDocumentRect>* out_next_rects) {
//
//    std::vector<PagelessDocumentRect> temp_rects;
//    std::vector<std::wstring> temp_texts;
//    std::vector<std::vector<PagelessDocumentRect>> temp_line_chars;
//
//    std::vector<PagelessDocumentRect> custom_line_rects;
//    std::vector<int> char_counts;
//
//    std::vector<int> indices_to_delete;
//    for (size_t i = 0; i < lines.size(); i++) {
//        if (line_num_chars(lines[i]) < 5) {
//            indices_to_delete.push_back(i);
//        }
//    }
//
//    for (int i = indices_to_delete.size() - 1; i >= 0; i--) {
//        lines.erase(lines.begin() + indices_to_delete[i]);
//    }
//
//    for (auto line : lines) {
//        custom_line_rects.push_back(get_line_rect(line));
//        char_counts.push_back(line_num_chars(line));
//    }
//
//    for (size_t i = 0; i < lines.size(); i++) {
//        PagelessDocumentRect rect = custom_line_rects[i];
//        int best_index = find_best_merge_index_for_line_index(lines, custom_line_rects, char_counts, i);
//        std::wstring text = get_string_from_stext_line(lines[i]);
//        std::vector<PagelessDocumentRect> line_chars;
//        if (out_line_chars) {
//            line_chars = get_char_rects_from_stext_line(lines[i]);
//        }
//
//        for (int j = i + 1; j <= best_index; j++) {
//            rect = fz_union_rect(rect, lines[j]->bbox);
//            text = text + get_string_from_stext_line(lines[j]);
//            if (out_line_chars) {
//                std::vector<PagelessDocumentRect> merged_line_chars = get_char_rects_from_stext_line(lines[j]);
//                line_chars.insert(line_chars.end(), merged_line_chars.begin(), merged_line_chars.end());
//            }
//        }
//
//        temp_rects.push_back(rect);
//        temp_texts.push_back(text);
//        if (out_line_chars) {
//            temp_line_chars.push_back(line_chars);
//        }
//        i = best_index;
//    }
//    for (size_t i = 0; i < temp_rects.size(); i++) {
//        if (i > 0 && out_rects.size() > 0) {
//            fz_rect prev_rect = out_rects[out_rects.size() - 1];
//            fz_rect current_rect = temp_rects[i];
//            if ((std::abs(prev_rect.y0 - current_rect.y0) < 1.0f) || (std::abs(prev_rect.y1 - current_rect.y1) < 1.0f)) {
//                out_rects[out_rects.size() - 1].x0 = std::min(prev_rect.x0, current_rect.x0);
//                out_rects[out_rects.size() - 1].x1 = std::max(prev_rect.x1, current_rect.x1);
//
//                out_rects[out_rects.size() - 1].y0 = std::min(prev_rect.y0, current_rect.y0);
//                out_rects[out_rects.size() - 1].y1 = std::max(prev_rect.y1, current_rect.y1);
//                out_texts[out_texts.size() - 1] = out_texts[out_texts.size() - 1] + temp_texts[i];
//                if (out_line_chars) {
//                    (*out_line_chars)[out_line_chars->size()-1].insert((*out_line_chars)[out_line_chars->size()-1].end(), temp_line_chars[i].begin(), temp_line_chars[i].end());
//                }
//                continue;
//            }
//        }
//        out_rects.push_back(temp_rects[i]);
//        out_texts.push_back(temp_texts[i]);
//
//        if (out_rects.size() > 1) {
//            if (out_next_rects != nullptr) {
//                out_next_rects->push_back(out_rects[out_rects.size()-2]);
//            }
//        }
//
//        if (out_line_chars) {
//            out_line_chars->push_back(temp_line_chars[i]);
//        }
//    }
//
//    if (out_rects.size() > 0) {
//        if (out_next_rects) {
//            out_next_rects->push_back(out_rects.back());
//            out_next_rects->push_back(out_rects.back());
//        }
//    }
//}


//int lcs(const char* X, const char* Y, int m, int n)
//{
//    std::vector<std::vector<int>> L;
//    L.reserve(m + 1);
//    for (int i = 0; i < m + 1; i++) {
//        L.push_back(std::vector<int>(n + 1));
//    }
//
//    int i, j;
//
//    /* Following steps build L[m+1][n+1] in bottom up fashion. Note
//      that L[i][j] contains length of LCS of X[0..i-1] and Y[0..j-1] */
//    for (i = 0; i <= m; i++) {
//        for (j = 0; j <= n; j++) {
//            if (i == 0 || j == 0)
//                L[i][j] = 0;
//
//            else if (X[i - 1] == Y[j - 1])
//                L[i][j] = L[i - 1][j - 1] + 1;
//
//            else
//                L[i][j] = std::max(L[i - 1][j], L[i][j - 1]);
//        }
//    }
//
//    /* L[m][n] contains length of LCS for X[0..n-1] and Y[0..m-1] */
//    return L[m][n];
//}

//int lcs_small_optimized(const char* X, const char* Y, int m, int n)
//{
//    const int SIZE = 128;
//    thread_local int L[SIZE * SIZE];
//
//    if (((m + 1) * (n + 1)) > SIZE * SIZE) {
//        return lcs(X, Y, m, n);
//    }
//
//    int i, j;
//
//    for (i = 0; i <= m; i++) {
//        for (j = 0; j <= n; j++) {
//            if (i == 0 || j == 0) {
//                L[i * SIZE + j] = 0;
//            }
//            else if (X[i - 1] == Y[j - 1]) {
//                L[i * SIZE + j] = L[(i - 1) * SIZE + j - 1] + 1;
//            }
//            else {
//                L[i * SIZE + j] = std::max(L[(i - 1) * SIZE + j], L[i* SIZE + j - 1]);
//            }
//        }
//    }
//
//    return L[m * SIZE + n];
//}

bool command_requires_text(const std::wstring& command) {
    if ((command.find(L"%5") != -1) || (command.find(L"command_text") != -1)) {
        return true;
    }
    return false;
}

bool command_requires_rect(const std::wstring& command) {
    if (command.find(L"%{selected_rect}") != -1) {
        return true;
    }
    return false;
}

void parse_command_string(std::wstring command_string, std::string& command_name, std::wstring& command_data) {
    int lindex = command_string.find(L"(");
    int rindex = command_string.rfind(L")");
    if (lindex < rindex) {
        command_name = utf8_encode(command_string.substr(0, lindex));
        command_data = command_string.substr(lindex + 1, rindex - lindex - 1);
    }
    else {
        command_data = L"";
        command_name = utf8_encode(command_string);
    }
}

void parse_color(std::wstring color_string, float* out_color, int n_components) {
    if (color_string.size() > 0) {
        if (color_string[0] == '#') {
            hexademical_to_normalized_color(color_string, out_color, n_components);
        }
        else {
            std::wstringstream ss(color_string);

            for (int i = 0; i < n_components; i++) {
                ss >> *(out_color + i);
            }
        }
    }
}

int get_status_bar_height() {
    if (STATUS_BAR_FONT_SIZE > 0) {
        return STATUS_BAR_FONT_SIZE + 5;
    }
    else {
#ifdef SIOYEK_IOS
        return 20 * 3;
#else
        return 20;
#endif
    }
}

void flat_char_prism(const std::vector<fz_stext_char*>& chars, int page, std::wstring& output_text, std::vector<int>& pages, std::vector<PagelessDocumentRect>& rects) {
    fz_stext_char* last_char = nullptr;

    for (int j = 0; j < chars.size(); j++) {
        if (is_line_separator(last_char, chars[j])) {
            if (last_char->c == '-') {
                pages.pop_back();
                rects.pop_back();
                output_text.pop_back();
            }
            else {
                pages.push_back(page);
                rects.push_back(fz_rect_from_quad(chars[j]->quad));
                output_text.push_back(' ');
            }
        }
        pages.push_back(page);
        rects.push_back(fz_rect_from_quad(chars[j]->quad));
        output_text.push_back(chars[j]->c);
        last_char = chars[j];
    }
}

void flat_char_prism2(const std::vector<fz_stext_char*>& chars, int page, std::wstring& output_text, std::vector<int>& page_begin_indices){
    fz_stext_char* last_char = nullptr;

    page_begin_indices.push_back(output_text.size());

    for (int j = 0; j < chars.size(); j++) {
        if (is_line_separator(last_char, chars[j])) {
            if (last_char->c == '-') {
                output_text.pop_back();
            }
            else {
                output_text.push_back(' ');
            }
        }
        output_text.push_back(chars[j]->c);
        last_char = chars[j];
    }
}

QString get_color_stylesheet(float* bg_color, float* text_color, bool nofont, int font_size) {
    if ((!nofont) && (STATUS_BAR_FONT_SIZE > -1 || font_size > -1)) {
        int size = font_size > 0 ? font_size : STATUS_BAR_FONT_SIZE;
        QString	font_size_stylesheet = QString("font-size: %1px").arg(size);
        return QString("background-color: %1; color: %2; border: 0; %3;").arg(
            get_color_qml_string(bg_color[0], bg_color[1], bg_color[2]),
            get_color_qml_string(text_color[0], text_color[1], text_color[2]),
            font_size_stylesheet
        );
    }
    else {
        return QString("background-color: %1; color: %2; border: 0;").arg(
            get_color_qml_string(bg_color[0], bg_color[1], bg_color[2]),
            get_color_qml_string(text_color[0], text_color[1], text_color[2])
        );
    }

}

QString get_ui_stylesheet(bool nofont, int font_size) {
    return get_color_stylesheet(UI_BACKGROUND_COLOR, UI_TEXT_COLOR, nofont, font_size) + "background-color: transparent;";
}

QString get_status_stylesheet(bool nofont, int font_size) {
    return get_color_stylesheet(STATUS_BAR_COLOR, STATUS_BAR_TEXT_COLOR, nofont, font_size);
}

QString get_status_button_stylesheet(bool nofont, int font_size) {
    return get_color_stylesheet(STATUS_BAR_TEXT_COLOR, STATUS_BAR_COLOR, nofont, font_size) + "padding-left: 5px; padding-right: 5px;";
}

QString get_list_item_stylesheet() {
    return QString("background-color: red; padding-bottom: 20px; padding-top: 20px;");
}

QString get_scrollbar_stylesheet(){

    QColor ui_background_color = QColor::fromRgbF(UI_BACKGROUND_COLOR[0], UI_BACKGROUND_COLOR[1], UI_BACKGROUND_COLOR[2]);
    QColor handle_color;
    QColor handle_color_hover;
    // if color is black
    if (ui_background_color.red() == 0 && ui_background_color.green() == 0 && ui_background_color.blue() == 0) {
        handle_color = QColor::fromRgbF(0.15, 0.15, 0.15);
        handle_color_hover = QColor::fromRgbF(0.25, 0.25, 0.25);
    }
    else {
        handle_color = ui_background_color.lighter();
        handle_color_hover = handle_color.lighter();
    }

    QString handle_color_string = get_color_qml_string(handle_color.redF(), handle_color.greenF(), handle_color.blueF());
    QString handle_color_hover_string = get_color_qml_string(handle_color_hover.redF(), handle_color_hover.greenF(), handle_color_hover.blueF());

    return QString(R"(

        QScrollBar:horizontal {
            border: none;
            background: transparent;
            min-height: 8px;
            margin: 0px 20px 0 20px;
        }

        QScrollBar::handle:horizontal {
            background: %1;
            min-width: 8px;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            background: none;
            width: 0px;
            subcontrol-position: left;
            subcontrol-origin: margin;
        }

        QScrollBar:vertical {
            border: none;
            background: transparent;
            margin: 0 0 0 0;
        }

        QScrollBar::handle:vertical {
            background: %1;
            min-height: 20px;
        }

        QScrollBar::handle:vertical:hover {
            background: %2;
        }

        QScrollBar::handle:horizontal:hover {
            background: %2;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            background: none;
            height: 0px;
            subcontrol-position: top;
            subcontrol-origin: margin;
        }
    )").arg(handle_color_string, handle_color_hover_string);
}
QString get_selected_stylesheet(bool nofont) {
    if ((!nofont) && STATUS_BAR_FONT_SIZE > -1) {
        QString	font_size_stylesheet = QString("font-size: %1px").arg(STATUS_BAR_FONT_SIZE);
        return QString("background-color: %1; color: %2; border: 0; %3;").arg(
            get_color_qml_string(UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2]),
            get_color_qml_string(UI_SELECTED_TEXT_COLOR[0], UI_SELECTED_TEXT_COLOR[1], UI_SELECTED_TEXT_COLOR[2]),
            font_size_stylesheet
        );
    }
    else {
        return QString("background-color: %1; color: %2; border: 0;").arg(
            get_color_qml_string(UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2]),
            get_color_qml_string(UI_SELECTED_TEXT_COLOR[0], UI_SELECTED_TEXT_COLOR[1], UI_SELECTED_TEXT_COLOR[2])
        );
    }
}

void convert_color4(const float* in_color, int* out_color) {
    out_color[0] = (int)(in_color[0] * 255);
    out_color[1] = (int)(in_color[1] * 255);
    out_color[2] = (int)(in_color[2] * 255);
    out_color[3] = (int)(in_color[3] * 255);
}

void convert_color3(const float* in_color, int* out_color) {
    out_color[0] = (int)(in_color[0] * 255);
    out_color[1] = (int)(in_color[1] * 255);
    out_color[2] = (int)(in_color[2] * 255);
}

#ifdef SIOYEK_ANDROID

std::optional<std::function<void(int, int)>> android_global_word_callback = {};
std::optional<std::function<void(QString)>> android_global_state_change_callback = {};
std::optional<std::function<void(QString)>> android_global_external_state_change_callback = {};
std::optional<std::function<QString()>> android_global_on_android_app_pause_callback = {};
std::optional<std::function<void(bool, bool, int)>> android_global_resume_state_callback = {};

QJniObject parseUriString(const QString& uriString) {
    return QJniObject::callStaticObjectMethod
    ("android/net/Uri", "parse",
        "(Ljava/lang/String;)Landroid/net/Uri;",
        QJniObject::fromString(uriString).object());
}

QString android_file_uri_from_content_uri(QString uri) {

    //    mainActivityObj = QtAndroid::androidActivity();
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject uri_object = parseUriString(uri);

    QJniObject file_uri_object = QJniObject::callStaticObjectMethod("info/sioyek/sioyek/SioyekActivity",
        "getPathFromUri",
        "(Landroid/content/Context;Landroid/net/Uri;)Ljava/lang/String;", activity.object(), uri_object.object());
    return file_uri_object.toString();
}

int android_tts_get_max_text_size(){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    return activity.callMethod<int>("ttsGetMaxTextSize", "()I");
}

void android_tts_say(QString text, int start_offset) {

    QJniObject text_jni = QJniObject::fromString(text);
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    //QJniObject contentResolverObj = activity.callObjectMethod("saySomethingElse", "(Ljava/lang/String;)V", text_jni.object<jstring>());
    // activity.callMethod<void>("ttsSay", "(Ljava/lang/String;)V", text_jni.object<jstring>());
    activity.callMethod<void>("ttsSay", "(Ljava/lang/String;I)V", text_jni.object<jstring>(), start_offset);
}

void android_tts_pause(){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<void>("ttsPause");
}

void android_tts_stop(){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<void>("ttsStop");
}

void android_tts_set_rate(float rate){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<void>("ttsSetRate", "(F)V", rate);
}

void android_brightness_set(float brightness){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<void>("setScreenBrightness", "(F)V", brightness);
}

float android_brightness_get(){
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    return activity.callMethod<float>("getScreenBrightness", "()F");
}

// void android_tts_stop_service(){
//     QJniObject activity = QNativeInterface::QAndroidApplication::context();
//     activity.callMethod<void>("stopTtsService", "()V");
// }

#endif

fz_document* open_document_with_file_name(fz_context* context, std::wstring file_name) {

    bool sioyek_mobile = false;
#ifdef SIOYEK_MOBILE
    sioyek_mobile = true;
#endif

    QString file_name_qstring = QString::fromStdWString(file_name);
    if (sioyek_mobile || file_name_qstring.startsWith(":/")) {

        QFile pdf_qfile(QString::fromStdWString(file_name));

        pdf_qfile.open(QIODeviceBase::ReadOnly);
        int qfile_handle = pdf_qfile.handle();
        fz_stream* stream = nullptr;


        if (qfile_handle != -1) {

#ifdef Q_OS_WIN
            FILE* file_ptr = fdopen(qfile_handle, "rb");
            //FILE* file_ptr = fdopen(_dup(qfile_handle), "rb");
#else
            FILE* file_ptr = fdopen(dup(qfile_handle), "rb");
#endif
            stream = fz_open_file_ptr_no_close(context, file_ptr);
        }
        else {
            QByteArray bytes = pdf_qfile.readAll();
            int size = bytes.size();
            unsigned char* new_buffer = new unsigned char[size];
            memcpy(new_buffer, bytes.data(), bytes.size());
            stream = fz_open_memory(context, new_buffer, bytes.size());
        }

        //return fz_open_document_with_stream(context, "application/pdf", stream);
        std::string file_name_str = utf8_encode(file_name);
        return fz_open_document_with_stream(context, file_name_str.c_str(), stream);
    }
    else {
#ifndef SIOYEK_MOBILE


        float epub_width, epub_height;
        get_path_epub_size(file_name, &epub_width, &epub_height);

        fz_document* doc = fz_open_document(context, utf8_encode(file_name).c_str());
        if (fz_is_document_reflowable(context, doc)) {

            if (EPUB_CSS.size() > 0) {
                std::string css = utf8_encode(EPUB_CSS);
                fz_set_user_css(context, css.c_str());
            }

            fz_layout_document(context, doc, epub_width, epub_height, EPUB_FONT_SIZE);

            //int a = 2;
        }
        return doc;
#endif
    }
    return nullptr;
}

void convert_qcolor_to_float3(const QColor& color, float* out_floats) {
    *(out_floats + 0) = static_cast<float>(color.red()) / 255.0f;
    *(out_floats + 1) = static_cast<float>(color.green()) / 255.0f;
    *(out_floats + 2) = static_cast<float>(color.blue()) / 255.0f;
}

//QColor convert_float3_to_qcolor(const float* floats) {
//    return QColor(get_color_qml_string(floats[0], floats[1], floats[2]));
//}

QColor convert_float3_to_qcolor(const float* floats) {
    return QColor::fromRgbF(floats[0], floats[1], floats[2]);
}

QColor convert_float4_to_qcolor(const float* floats) {
    int colors[4];
    convert_color4(floats, colors);
    return QColor(colors[0], colors[1], colors[2], colors[3]);
}

void convert_qcolor_to_float4(const QColor& color, float* out_floats) {
    *(out_floats + 0) = static_cast<float>(color.red()) / 255.0f;
    *(out_floats + 1) = static_cast<float>(color.green()) / 255.0f;
    *(out_floats + 2) = static_cast<float>(color.blue()) / 255.0f;
    *(out_floats + 3) = static_cast<float>(color.alpha()) / 255.0f;
}

#ifdef SIOYEK_ANDROID

#include "main_widget.h"
extern std::vector<MainWidget*> windows;

// modified from https://github.com/mahdize/CrossQFile/blob/main/CrossQFile.cpp


void log_d(QString text){
    // call the activitie's myLogD method with text
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    activity.callMethod<void>("myLogD", "(Ljava/lang/String;)V", QJniObject::fromString(text).object<jstring>());

}

QString android_file_name_from_uri(QString uri) {

    //    mainActivityObj = QtAndroid::androidActivity();
    QJniObject activity = QNativeInterface::QAndroidApplication::context();

    QJniObject contentResolverObj = activity.callObjectMethod
    ("getContentResolver", "()Landroid/content/ContentResolver;");


    //	QAndroidJniObject cursorObj {contentResolverObj.callObjectMethod
    //		("query",
    //		 "(Landroid/net/Uri;[Ljava/lang/String;Landroid/os/Bundle;Landroid/os/CancellationSignal;)Landroid/database/Cursor;",
    //		 parseUriString(fileName()).object(), QAndroidJniObject().object(), QAndroidJniObject().object(),
    //		 QAndroidJniObject().object(), QAndroidJniObject().object())};

    QJniObject cursorObj{ contentResolverObj.callObjectMethod
        ("query",
         "(Landroid/net/Uri;[Ljava/lang/String;Landroid/os/Bundle;Landroid/os/CancellationSignal;)Landroid/database/Cursor;",
         parseUriString(uri).object(), QJniObject().object(), QJniObject().object(),
         QJniObject().object(), QJniObject().object()) };

    cursorObj.callMethod<jboolean>("moveToFirst");

    QJniObject retObj{ cursorObj.callObjectMethod
        ("getString","(I)Ljava/lang/String;", cursorObj.callMethod<jint>
         ("getColumnIndex","(Ljava/lang/String;)I",
          QJniObject::getStaticObjectField<jstring>
          ("android/provider/OpenableColumns","DISPLAY_NAME").object())) };

    QString ret{ retObj.toString() };
    return ret;
}

void check_pending_intents(const QString workingDirPath)
{
    //    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (activity.isValid()) {
        // create a Java String for the Working Dir Path
        QJniObject jniWorkingDir = QJniObject::fromString(workingDirPath);
        if (!jniWorkingDir.isValid()) {
            //            emit shareError(0, tr("Share: an Error occured\nWorkingDir not valid"));
            return;
        }
        activity.callMethod<void>("checkPendingIntents", "(Ljava/lang/String;)V", jniWorkingDir.object<jstring>());
        return;
    }
}


void setFileUrlReceived(const QString& url)
{
    if (windows.size() > 0) {
        windows[0]->open_document(url.toStdWString());
    }
}

void on_android_tts(int begin, int end){
    if (android_global_word_callback){
        android_global_word_callback.value()(begin, end - begin);
    }
    // qDebug() << "ttsed from " << begin << " to " << end;
}

void on_android_pause_global(){
    for (auto window : windows){
        QMetaObject::invokeMethod(window, "on_android_pause", Qt::QueuedConnection);
    }
}

void on_android_resume_global(){
    for (auto window : windows){
        QMetaObject::invokeMethod(window, "on_android_resume", Qt::QueuedConnection);
    }
}

void on_android_state_change(QString new_state){
    if (android_global_state_change_callback){
        android_global_state_change_callback.value()(new_state);
    }
}

void on_android_external_state_change(QString new_state){
    if (android_global_external_state_change_callback){
        android_global_external_state_change_callback.value()(new_state);
    }
}

bool on_android_resume_state(bool is_playing, bool is_on_rest, int offset){
    if (windows.size() > 0){
        for (auto window : windows){
            QMetaObject::invokeMethod(window, "handle_app_tts_resume", Qt::QueuedConnection,
                                      Q_ARG(bool, is_playing), Q_ARG(bool, is_on_rest), Q_ARG(int, offset));
        }
        return true;
    }
    return false;

}

QString on_android_get_rest_on_pause(){
    if (android_global_on_android_app_pause_callback){
        QString res = android_global_on_android_app_pause_callback.value()();
        return res;
    }
    return "";
}

extern "C" {
    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_setFileUrlReceived(JNIEnv* env,
            jobject obj,
            jstring url)
    {
        const char* urlStr = env->GetStringUTFChars(url, NULL);
        Q_UNUSED(obj)
            setFileUrlReceived(urlStr);
        env->ReleaseStringUTFChars(url, urlStr);
        return;
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_qDebug(JNIEnv* env,
            jobject obj,
            jstring url)
    {
        const char* urlStr = env->GetStringUTFChars(url, NULL);
        qDebug() << urlStr;
        Q_UNUSED(obj)
            env->ReleaseStringUTFChars(url, urlStr);
        return;
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onTts(JNIEnv* env,
            jobject obj,
            jint begin, jint end)
    {
        on_android_tts((int)begin, (int)end);
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onAndroidPause(JNIEnv* env,
                                                          jobject obj)
    {
        on_android_pause_global();
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onAndroidResume(JNIEnv* env,
                                                          jobject obj)
    {
        on_android_resume_global();
    }

    // JNIEXPORT void JNICALL
    //     Java_info_sioyek_sioyek_SioyekActivity_onAndroidPause(JNIEnv* env,
    //                                                       jobject obj)
    // {
    //     on_android_pause
    // }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onTtsStateChange(JNIEnv* env,
            jobject obj,
            jstring new_state)
    {

        const char* state_str = env->GetStringUTFChars(new_state, NULL);
        Q_UNUSED(obj)
            env->ReleaseStringUTFChars(new_state, state_str);
        on_android_state_change(state_str);
    }

    JNIEXPORT void JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onExternalTtsStateChange(JNIEnv* env,
            jobject obj,
            jstring new_state)
    {

        const char* state_str = env->GetStringUTFChars(new_state, NULL);
        Q_UNUSED(obj)
            env->ReleaseStringUTFChars(new_state, state_str);
        on_android_external_state_change(state_str);
    }

    JNIEXPORT bool JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_onResumeState(JNIEnv* env,
            jobject obj,
            jboolean is_playing,
            jboolean reading_rest,
            jint offset)
    {

        Q_UNUSED(obj)
        return on_android_resume_state(is_playing, reading_rest, offset);
    }

    JNIEXPORT jstring JNICALL
        Java_info_sioyek_sioyek_SioyekActivity_getRestOnPause(JNIEnv* env,
              jobject obj)
    {

        Q_UNUSED(obj)
        QString res = on_android_get_rest_on_pause();
        std::string res_std = res.toStdString();
        return env->NewStringUTF(res_std.c_str());
    }

}
#endif

float dampen_velocity(float v, float dt) {
    if (v == 0) return 0;
    dt = -dt;

    float accel = 3000.0f;
    if (v > 0) {
        v -= accel * dt;
        if (v < 0) {
            v = 0;
        }
    }
    else {
        v += accel * dt;
        if (v > 0) {
            v = 0;
        }
    }
    return v;
}

fz_irect get_index_irect(fz_rect original, int index, fz_matrix transform, int num_h_slices, int num_v_slices) {
    fz_rect transformed = fz_transform_rect(original, transform);
    fz_irect rounded = fz_round_rect(transformed);

    int vi = index / num_h_slices;
    int hi = index % num_h_slices;

    int slice_width = (rounded.x1 - rounded.x0) / num_h_slices;
    int slice_height = (rounded.y1 - rounded.y0) / num_v_slices;

    int x0 = hi * slice_width;
    int y0 = vi * slice_height;

    int x1 = (hi == (num_h_slices - 1)) ? rounded.x1 : (hi + 1) * slice_width;
    int y1 = (vi == (num_v_slices - 1)) ? rounded.y1 : (vi + 1) * slice_height;

    fz_irect res;
    res.x0 = x0;
    res.x1 = x1;

    res.y0 = y0;
    res.y1 = y1;

    return res;


}

void translate_index(int index, int* h_index, int* v_index, int num_h_slices, int num_v_slices) {
    *v_index = index / num_h_slices;
    *h_index = index % num_h_slices;
}

void get_slice_size(float* width, float* height, fz_rect original, int num_h_slices, int num_v_slices) {
    *width = (original.x1 - original.x0) / num_h_slices;
    *height = (original.y1 - original.y0) / num_v_slices;
}

fz_rect get_index_rect(fz_rect original, int index, int num_h_slices, int num_v_slices) {

    int h_index, v_index;
    translate_index(index, &h_index, &v_index, num_h_slices, num_v_slices);

    float slice_height, slice_width;
    get_slice_size(&slice_width, &slice_height, original, num_h_slices, num_v_slices);

    fz_rect new_rect;
    new_rect.x0 = original.x0 + h_index * slice_width;
    new_rect.x1 = original.x0 + (h_index + 1) * slice_width;
    new_rect.y0 = original.y0 + v_index * slice_height;
    new_rect.y1 = original.y0 + (v_index + 1) * slice_height;
    return new_rect;
}

QStandardItemModel* create_table_model(const std::vector<std::vector<std::wstring>> column_texts) {
    QStandardItemModel* model = new QStandardItemModel();
    if (column_texts.size() == 0) {
        return model;
    }
    int num_rows = column_texts[0].size();
    for (int i = 1; i < column_texts.size(); i++) {
        assert(column_texts[i].size() == num_rows);
    }

    for (int i = 0; i < num_rows; i++) {
        QList<QStandardItem*> items;
        for (int j = 0; j < column_texts.size(); j++) {
            QStandardItem* item = new QStandardItem(QString::fromStdWString(column_texts[j][i]));

            if (j == (column_texts.size() - 1)) {
                item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
            }
            else {
                item->setTextAlignment(Qt::AlignVCenter);
            }
            items.append(item);
        }
        model->appendRow(items);
    }
    return model;
}


QStandardItemModel* create_table_model(std::vector<std::wstring> lefts, std::vector<std::wstring> rights) {
    QStandardItemModel* model = new QStandardItemModel();

    assert(lefts.size() == rights.size());

    for (size_t i = 0; i < lefts.size(); i++) {
        QStandardItem* name_item = new QStandardItem(QString::fromStdWString(lefts[i]));
        QStandardItem* key_item = new QStandardItem(QString::fromStdWString(rights[i]));
        key_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        model->appendRow(QList<QStandardItem*>() << name_item << key_item);
    }
    return model;
}

float vec3_distance_squared(float* v1, float* v2) {
    float distance = 0;

    float dx = *(v1 + 0) - *(v2 + 0);
    float dy = *(v1 + 1) - *(v2 + 1);
    float dz = *(v1 + 2) - *(v2 + 2);

    return dx * dx + dy * dy + dz * dz;
}

float highlight_color_distance(float color1[3], float color2[3]) {
    QColor c1 = QColor::fromRgbF(color1[0], color1[1], color1[2]);
    QColor c2 = QColor::fromRgbF(color2[0], color2[1], color2[2]);
    return std::abs(c1.hslHueF() - c2.hslHueF());
}

char get_highlight_color_type(float color[3]) {
    float min_distance = 1000;
    int min_index = -1;

    for (int i = 0; i < 26; i++) {
        //float dist = vec3_distance_squared(color, &HIGHLIGHT_COLORS[i * 3]);
        float dist = highlight_color_distance(color, &HIGHLIGHT_COLORS[i * 3]);
        if (dist < min_distance) {
            min_distance = dist;
            min_index = i;
        }
    }

    return 'a' + min_index;
}

float* get_highlight_type_color(char type) {
    if (type == '_') {
        return &BLACK_COLOR[0];
    }
    if (type >= 'a' && type <= 'z') {
        return &HIGHLIGHT_COLORS[(type - 'a') * 3];
    }
    if (type >= 'A' && type <= 'Z') {
        return &HIGHLIGHT_COLORS[(type - 'A') * 3];
    }
    return &HIGHLIGHT_COLORS[0];
}

void lighten_color(float input[3], float output[3]) {
    QColor color = qRgb(
        static_cast<int>(input[0] * 255),
        static_cast<int>(input[1] * 255),
        static_cast<int>(input[2] * 255)
    );
    float prev_lightness = static_cast<float>(color.lightness()) / 255.0f;
    int lightness_increase = static_cast<int>((0.9f / prev_lightness) * 100);

    QColor lighter = color;
    if (lightness_increase > 100) {
        lighter = color.lighter(lightness_increase);
    }

    output[0] = lighter.redF();
    output[1] = lighter.greenF();
    output[2] = lighter.blueF();
}


QString trim_text_after(QString source, QString needle) {
    int needle_index = source.indexOf(needle);
    if (needle_index != -1) {
        return source.left(needle_index);
    }
    return source;
}

std::wstring clean_link_source_text(std::wstring link_source_text) {
    QString text = QString::fromStdWString(link_source_text);

    text = trim_text_after(text, "et. al.");
    text = trim_text_after(text, "et al");
    text = trim_text_after(text, "etal.");

    std::vector<char> garbage_chars = { '[', ']', '.', ',' };
    while ((text.size() > 0) && (std::find(garbage_chars.begin(), garbage_chars.end(), text.at(0)) != garbage_chars.end())) {
        text = text.right(text.size() - 1);
    }

    while ((text.size() > 0) && (std::find(garbage_chars.begin(), garbage_chars.end(), text.at(text.size() - 1)) != garbage_chars.end())) {
        text = text.left(text.size() - 1);
    }
    return text.toStdWString();
}

QString clean_bib_item(QString bib_item) {
    int bracket_index = bib_item.indexOf("]");
    if (bracket_index >= 0 && bracket_index < 10) {
        bib_item = bib_item.right(bib_item.size() - bracket_index - 1);
    }

    int arxiv_index = bib_item.toLower().indexOf("arxiv");
    if (arxiv_index >= 0) {
        bib_item = bib_item.left(arxiv_index);
    }

    QString candid = bib_item;
    //while (candid.size() > 0 && ((candid[candid.size() - 1].unicode() > 128) || !std::isalpha(candid[candid.size() - 1]))) {
    while (candid.size() > 0 && ((candid[candid.size() - 1].unicode() > 128) || !candid.back().isLetter())) {
        candid = candid.right(candid.size() - 1);
    }
    return candid;
}

struct Line2D {
    float nx;
    float ny;
    float c;
};

float point_distance_from_line(AbsoluteDocumentPos point, Line2D line) {
    return std::abs(line.nx * point.x + line.ny * point.y - line.c);
}

Line2D line_from_points(AbsoluteDocumentPos p1, AbsoluteDocumentPos p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    float nx = -dy;
    float ny = dx;
    float size = std::sqrt(nx * nx + ny * ny);

    nx = nx / size;
    ny = ny / size;

    Line2D res;
    res.nx = nx;
    res.ny = ny;
    res.c = nx * p1.x + ny * p1.y;
    return res;
}

std::vector<FreehandDrawingPoint> prune_freehand_drawing_points(const std::vector<FreehandDrawingPoint>& points) {

    if (points.size() < 3) {
        return points;
    }

    std::vector<FreehandDrawingPoint> pruned_points;
    pruned_points.push_back(points[0]);
    int candid_index = 1;

    while (candid_index < points.size() - 1) {
        int next_index = candid_index + 1;
        if ((points[candid_index].pos.x == pruned_points.back().pos.x) && (points[candid_index].pos.y == pruned_points.back().pos.y)) {
            candid_index++;
            continue;
        }

        float dx0 = points[candid_index].pos.x - pruned_points.back().pos.x;
        float dy0 = points[candid_index].pos.y - pruned_points.back().pos.y;

        float dx1 = points[next_index].pos.x - points[candid_index].pos.x;
        float dy1 = points[next_index].pos.y - points[candid_index].pos.y;
        float dot_product = dx0 * dx1 + dy0 * dy1;

        Line2D line = line_from_points(pruned_points.back().pos, points[next_index].pos);
        float thickness_factor = std::min(points[candid_index].thickness, 1.0f);
        if ((dot_product < 0) || (point_distance_from_line(points[candid_index].pos, line) > (0.2f * thickness_factor))) {
            pruned_points.push_back(points[candid_index]);
        }


        candid_index++;
    }

    pruned_points.push_back(points[points.size() - 1]);


    return pruned_points;
}

float rect_area(fz_rect rect) {
    if (rect.x1 < rect.x0 || rect.y1 < rect.y0) return 0;
    return std::abs(rect.x1 - rect.x0) * std::abs(rect.y1 - rect.y0);
}

bool are_rects_same(fz_rect r1, fz_rect r2) {
    float r1_area = rect_area(r1);
    float r2_area = rect_area(r2);
    float max_area = std::max(r1_area, r2_area);
    if (r2_area == 0) {
        return (std::abs(r1.x0 - r2.x0) < 0.01f) && (std::abs(r1.y0 - r2.y0) < 0.01f);
    }
    fz_rect intersection = fz_intersect_rect(r1, r2);
    if (rect_area(intersection) > max_area * 0.9f) {
        return true;
    }
    return false;
}

bool is_new_word(fz_stext_char* old_char, fz_stext_char* new_char) {
    if (old_char == nullptr) return true;
    if (new_char->c == ' ' || new_char->c == '\n') return true;
    return std::abs(new_char->quad.ll.x - old_char->quad.ll.x) > 5 * std::abs(old_char->quad.lr.x - old_char->quad.ll.x);
}

std::optional<DocumentRect> find_shrinking_rect_word(bool before, fz_stext_page* page, DocumentRect rect){
    bool found = false;
    std::optional<DocumentRect> last_before_space_rect = {};
    std::optional<DocumentRect> before_rect = {};
    fz_stext_char* old_char = nullptr;


    bool should_return_next_char = false;
    bool was_last_character_space = true;

    LL_ITER(block, page->first_block) {
        if (block->type == FZ_STEXT_BLOCK_TEXT) {
            LL_ITER(line, block->u.t.first_line) {
                LL_ITER(ch, line->first_char) {
                    DocumentRect cr = DocumentRect(fz_rect_from_quad(ch->quad), rect.page);
                    if (should_return_next_char) {
                        return cr;
                    }
                    if ((!before) && (is_new_word(old_char, ch)) && found) {
                        return last_before_space_rect;
                    }
                    if (is_new_word(old_char, ch)) {
                        last_before_space_rect = before_rect;
                        was_last_character_space = true;
                        if (found && before) {
                            should_return_next_char = true;
                        }
                    }
                    if (are_rects_same(cr.rect, rect.rect)) {
                        found = true;
                    }
                    before_rect = cr;
                    old_char = ch;
                }
            }
        }
    }
    return {};
}

std::vector<DocumentRect> find_expanding_rect_word(bool before, fz_stext_page* page, DocumentRect page_rect) {
    std::vector<DocumentRect> res;
    std::vector<fz_stext_char*> chars;
    get_flat_chars_from_stext_page(page, chars);
    int index = -1;
    for (int i = 0; i < chars.size(); i++) {
        DocumentRect cr = DocumentRect(fz_rect_from_quad(chars[i]->quad), page_rect.page);
        if (are_rects_same(cr.rect, page_rect.rect)) {
            index = i;
            break;
        }
    }
    int original_index = index;
    int prev_index = original_index;

    if (index != -1) {
        if (before) {
            index--;
            while (index >= 0) {
                res.push_back(DocumentRect(fz_rect_from_quad(chars[index]->quad), page_rect.page));
                //if (chars[index]->c == ' ' || chars[index]->c == '\n') {
                if (is_new_word(chars[prev_index], chars[index])) {
                    if (std::abs(original_index - index) > 1) {
                        res.pop_back();
                        break;
                    }
                }
                prev_index = index;
                index--;
            }
        }
        else {
            index++;
            while (index < chars.size()) {
                res.push_back(DocumentRect(fz_rect_from_quad(chars[index]->quad), page_rect.page));
                if (is_new_word(chars[prev_index], chars[index])) {
                    if (std::abs(original_index - index) > 1) {
                        res.pop_back();
                        break;
                    }
                }
                prev_index = index;
                index++;
            }
        }
    }
    return res;

}

//std::vector<fz_rect> find_expanding_rect_word(bool before, fz_stext_page* page, fz_rect page_rect) {
//	bool found = false;
//	std::optional<fz_rect> last_after_space_rect = {};
//	std::optional<fz_rect> before_rect = {};
//	std::vector<fz_rect> res;
//
//	bool was_last_character_space = true;
//
//	LL_ITER(block, page->first_block) {
//		if (block->type == FZ_STEXT_BLOCK_TEXT) {
//			LL_ITER(line, block->u.t.first_line) {
//				LL_ITER(ch, line->first_char) {
//					fz_rect cr = fz_rect_from_quad(ch->quad);
//
//					if (was_last_character_space) {
//						was_last_character_space = false;
//						last_after_space_rect = cr;
//					}
//
//					if (before) {
//						res.push_back(cr);
//					}
//					if (ch->c == ' ' || ch->c == '\n') {
//						was_last_character_space = true;
//						if (before) {
//							res.clear();
//						}
//						if (found) {
//							return res;
//						}
//					}
//					if (found) {
//						res.push_back(cr);
//					}
//
//					if (are_rects_same(cr, page_rect)) {
//						if (before) {
//							return res;
//						}
//						found = true;
//					}
//
//					before_rect = cr;
//
//				}
//			}
//		}
//	}
//
//	return {};
//}

std::optional<DocumentRect> find_expanding_rect(bool before, fz_stext_page* page, DocumentRect page_rect) {
    bool found = false;
    std::optional<DocumentRect> before_rect = {};

    LL_ITER(block, page->first_block) {
        if (block->type == FZ_STEXT_BLOCK_TEXT) {
            LL_ITER(line, block->u.t.first_line) {
                LL_ITER(ch, line->first_char) {
                    DocumentRect cr = DocumentRect(fz_rect_from_quad(ch->quad), page_rect.page);
                    if (found) {
                        return cr;
                    }
                    if (are_rects_same(cr.rect, page_rect.rect)) {
                        if (before) {
                            return before_rect;
                        }
                        found = true;
                    }
                    before_rect = cr;
                }
            }
        }
    }

    return {};
}


QStringList extract_paper_data_from_json_response(QJsonValue json_object, const std::vector<QString>& path) {

    if (path.size() == 0) {
        if (json_object.isArray()) {
            QJsonArray array = json_object.toArray();
            QStringList list;
            for (int i = 0; i < array.size(); i++) {
                list.append(array.at(i).toString());
            }
            return QStringList() << list.join(", ");
        }
        else {
            return { json_object.toString() };
        }
    }


    QString current_path = path[0];
    if (current_path.indexOf("[]") != -1) {
        QJsonArray array = json_object.toObject().value(current_path.left(current_path.size() - 2)).toArray();
        QStringList res;

        for (int i = 0; i < array.size(); i++) {
            QStringList temp_objects = extract_paper_data_from_json_response(array.at(i), std::vector<QString>(path.begin() + 1, path.end()));
            for (int i = 0; i < temp_objects.size(); i++) {
                res.push_back(temp_objects.at(i));
            }
        }
        return res;
    }
    else if (current_path.indexOf("[") != -1) {
        QString index_string = current_path.mid(current_path.indexOf("[") + 1, current_path.indexOf("]") - current_path.indexOf("[") - 1);
        QString key_string = current_path.left(current_path.indexOf("["));
        int index = index_string.toInt();
        return extract_paper_data_from_json_response(json_object.toObject().value(key_string).toArray().at(index),
            std::vector<QString>(path.begin() + 1, path.end()));

    }
    else {
        if (json_object.isArray()) {
            QJsonArray array = json_object.toObject().value(current_path.left(current_path.size() - 2)).toArray();
            QStringList res;

            for (int i = 0; i < array.size(); i++) {
                QStringList temp_objects = extract_paper_data_from_json_response(array.at(i), std::vector<QString>(path.begin() + 1, path.end()));
                res.push_back(temp_objects.join(", "));
            }
            return res;
        }
        else {
            return extract_paper_data_from_json_response(json_object.toObject().value(current_path),
                std::vector<QString>(path.begin() + 1, path.end()));
        }
    }
}

QStringList extract_paper_string_from_json_response(QJsonObject json_object, std::wstring path) {
    std::vector<QString> parts;
    QStringList parts_string_list = QString::fromStdWString(path).split(".");
    for (int i = 0; i < parts_string_list.size(); i++) {
        parts.push_back(parts_string_list.at(i));
    }
    return extract_paper_data_from_json_response(json_object, parts);
}

QString file_size_to_human_readable_string(int file_size) {
    if (file_size < 1000) {
        return QString::number(file_size);
    }
    else if (file_size < 1000 * 1000) {
        return QString::number(file_size / 1000) + "K";
    }
    else if (file_size < 1000 * 1000 * 1000) {
        return QString::number(file_size / (1000 * 1000)) + "M";
    }
    else if (file_size < 1000 * 1000 * 1000 * 1000) {
        return QString::number(file_size / (1000 * 1000 * 1000)) + "G";
    }
    else {
        return QString("inf");
    }
}

std::wstring new_uuid() {
    return QUuid::createUuid().toString().toStdWString();
}

std::string new_uuid_utf8() {
    return QUuid::createUuid().toString().toStdString();
}

bool are_same(float f1, float f2) {
    return std::abs(f1 - f2) < 0.01;
}

bool are_same(const FreehandDrawing& lhs, const FreehandDrawing& rhs) {
    if (lhs.points.size() != rhs.points.size()) {
        return false;
    }
    if (lhs.type != rhs.type) {
        return false;
    }
    for (int i = 0; i < lhs.points.size(); i++) {
        if (!are_same(lhs.points[i].pos, rhs.points[i].pos)) {
            return false;
        }
    }
    return true;

}

PagelessDocumentRect get_range_rect_union(const std::vector<PagelessDocumentRect>& rects, int first_index, int last_index) {
    PagelessDocumentRect res = rects[first_index];
    for (int i = first_index + 1; i <= last_index; i++) {
        res = fz_union_rect(res, rects[i]);
    }
    return res;
}


int get_largest_quote_size(const QString& text, int* begin_index, int* end_index) {
    bool is_in_quote = false;
    int largest_size = -1;
    int largest_begin_index = -1;
    int largest_end_index = -1;

    int current_size = 0;

    int current_begin_index = -1;

    for (int i = 0; i < text.size(); i++) {
        if ((text[i] == '"') || (text[i].unicode() == 8220) || (text[i].unicode() == 8221)) {
            if (is_in_quote) {
                is_in_quote = false;
                if (current_size > largest_size) {
                    largest_size = current_size;
                    largest_begin_index = current_begin_index;
                    largest_end_index = i;
                }
                current_size = 0;
            }
            else {
                is_in_quote = true;
                current_begin_index = i;
                current_size = 0;
            }
        }

        if (is_in_quote) {
            current_size++;
        }

    }
    *begin_index = largest_begin_index;
    *end_index = largest_end_index;

    return largest_size;
}

bool is_quote_reference(const QString& text, int* begin_index, int* end_index) {
    return get_largest_quote_size(text, begin_index, end_index) > 15;
}

QString remove_final_small_comma_from_end(QString reference_text) {
    int last_comma_index = reference_text.lastIndexOf(",");
    if (last_comma_index > reference_text.size() - 7) {
        return reference_text.left(last_comma_index);
    }
    return reference_text;
}

QString strip_garbage_from_paper_name(QString paper_name) {
    std::vector<int> garbage_characters = { '.', ',', ':', '"', '\'', ' ', 8220, 8221 };
    int first_index = 0;
    int last_index = paper_name.size()-1;

    while (std::find(garbage_characters.begin(), garbage_characters.end(), paper_name[first_index].unicode()) != garbage_characters.end()) {
        first_index++;
    }

    while (std::find(garbage_characters.begin(), garbage_characters.end(), paper_name[last_index].unicode()) != garbage_characters.end()) {
        last_index--;
    }

    if (last_index > first_index) {
        return remove_final_small_comma_from_end(paper_name.mid(first_index, last_index - first_index + 1));
    }
    return "";
}

bool could_dot_span_be_refernce_name(const QString& span_text) {
    if (span_text.indexOf("http") != -1) return false;
    if (span_text.startsWith("In Pro") || span_text.startsWith("In Inte") || span_text.startsWith("arxiv") || span_text.startsWith("arXiv") || span_text.startsWith("ArXiv")) return false;
    if (span_text.startsWith("Springer") || span_text.startsWith("semanticscholar") || span_text.startsWith("International") || span_text.startsWith("In ") || span_text.startsWith("In:")) return false;
    QString weird_characters = ";,[]/%_";
    int n_weird = 0;
    for (auto ch : span_text) {
        if (ch.isDigit() || (weird_characters.indexOf(ch) != -1)) {
            n_weird++;
        }
    }
    if (static_cast<float>(n_weird) / span_text.size() > 0.2f) return false;

    return true;
}



QString get_paper_name_from_reference_text(QString reference_text) {

    reference_text = reference_text.trimmed();
    int quote_begin_index, quote_end_index;
    if (is_quote_reference(reference_text, &quote_begin_index, &quote_end_index)) {
        return strip_garbage_from_paper_name(reference_text.mid(quote_begin_index, quote_end_index - quote_begin_index));
    }
    int first_dot_index = reference_text.indexOf('.');
    int last_dot_index = reference_text.lastIndexOf('.');
    if (last_dot_index == (reference_text.size() - 1)) last_dot_index = reference_text.left(reference_text.size() - 1).lastIndexOf('.');
    if (first_dot_index == last_dot_index) {
        return strip_garbage_from_paper_name(reference_text.mid(last_dot_index));
    }

    QString str = reference_text;
    //QRegularExpression reference_ending_dot_regex = QRegularExpression("(\.\w*In )|(\.\w*[aA]r[xX]iv )|(\.\w*[aA]r[X]iv )");
    //QRegularExpression reference_ending_dot_regex = QRegularExpression("(\.\w*In )|(\.\w*[aA]r[xX]iv )");
    QRegularExpression reference_ending_dot_regex = QRegularExpression("(\\.\w*In )|(\\.\w*[aA]r[xX]iv )");

    int ending_index = str.lastIndexOf(reference_ending_dot_regex);
    if (ending_index == -1) {
        int last_dot_index = str.lastIndexOf(".");
        // igonre if the last dot is close to the end
        int distance = str.size() - last_dot_index;
        if ((distance < 8) && (distance > 1)) {
            str = str.left(last_dot_index - 1);
        }
        ending_index = str.lastIndexOf(".") + 1;
    }

    while (ending_index > -1) {
        str = str.left(ending_index);
        int starting_index = str.lastIndexOf(".");
        QString res = str.right(str.size() - starting_index - 1).trimmed();
        //if (res.size() > 0 && res[0] == ':') {
        //    res = res.right(res.size() - 1);
        //}
        if (res.size() > 11 && could_dot_span_be_refernce_name(res)) {
            return strip_garbage_from_paper_name(res);
        }
        else {
            ending_index = starting_index;
        }
    }
    return "";

}

fz_rect get_first_page_size(fz_context* ctx, const std::wstring& document_path) {
    std::string path = utf8_encode(document_path);
    bool failed = false;

    fz_rect bounds;

    fz_try(ctx) {
        fz_document* doc = fz_open_document(ctx, path.c_str());

        fz_page* first_page = fz_load_page(ctx, doc, 0);
        bounds = fz_bound_page(ctx, first_page);

        fz_drop_page(ctx, first_page);
        fz_drop_document(ctx, doc);
    }
    fz_catch(ctx) {
        failed = true;
    }
    if (failed) {
        return fz_rect{ 0,0, 100, 100 };
    }

    return bounds;
}

QString get_direct_pdf_url_from_archive_url(QString url) {
    if (url.indexOf("web.archive.org") == -1) return url;

    int index = url.lastIndexOf("http") - 1;
    if (index > 4) {

        QString prefix = url.left(index);
        QString suffix = url.right(url.size() - index);
        return prefix + "if_" + suffix;
    }
    return url;

}

QString get_original_url_from_archive_url(QString url) {
    return url.right(url.size() - url.lastIndexOf("http"));
}

bool does_paper_name_match_query(std::wstring query, std::wstring paper_name) {
    std::string query_encoded = QString::fromStdWString(query).toLower().toStdString();
    std::string paper_name_encoded = QString::fromStdWString(paper_name).toLower().toStdString();

    int score = lcs(query_encoded.c_str(), paper_name_encoded.c_str(), query_encoded.size(), paper_name_encoded.size());
    int threshold = static_cast<int>(static_cast<float>(std::max(query_encoded.size(), paper_name_encoded.size())) * 0.9f);
    return score >= threshold;
}


bool is_dot_index_end_of_a_reference(const std::vector<DocumentCharacter>& flat_chars, int dot_index) {
    int next_non_whitespace_index = -1;
    int prev_index = dot_index - 1;
    int context_begin = dot_index - 10;
    int context_end = dot_index + 10;

    if (dot_index >= flat_chars.size()-2) {
        return true;
    }

    for (int candid = dot_index; candid < std::min((int)flat_chars.size(), dot_index+4); candid++) {
        if (flat_chars[candid].is_final) {
            next_non_whitespace_index = candid + 1;
            if (next_non_whitespace_index == flat_chars.size()) next_non_whitespace_index = -1;
            break;
        }
        fz_rect candid_rect = flat_chars[candid].rect;
        fz_rect dot_rect = flat_chars[dot_index].rect;
        if (candid_rect.y0 > dot_rect.y1) {
            next_non_whitespace_index = candid;
            break;
        }
    }
    if (next_non_whitespace_index == -1) {
        for (int candid = dot_index; candid < std::min((int)flat_chars.size(), dot_index + 2); candid++) {
            fz_rect candid_rect = flat_chars[candid].rect;
            fz_rect dot_rect = flat_chars[dot_index].rect;
            if (candid_rect.y0 > dot_rect.y1) {
                next_non_whitespace_index = candid;
                break;
            }
        }
    }
    if (next_non_whitespace_index > -1 && prev_index > -1) {
        //if (context_begin >= 0 && context_end < flat_chars.size()) {
        //    std::wstring context;
        //    for (int i = context_begin; i < context_end; i++) {
        //        context.push_back(flat_chars[i].c);
        //    }
        //    int a = 2;
        //    //qDebug() << QString::fromStdWString(context) << "!!" << QString(QChar(flat_chars[next_non_whitespace_index]->c));
        //}
        fz_rect dot_rect = flat_chars[prev_index].rect;
        fz_rect next_rect = flat_chars[next_non_whitespace_index].rect;
        float height = std::abs(next_rect.y1 - next_rect.y0);

        if ((next_rect.y0 > (dot_rect.y0 + height / 2)) && (next_rect.y1 > (dot_rect.y1 + height / 2))) {
            return true;
        }
        if (std::abs(next_rect.y0 - dot_rect.y0) > 5 * height) {
            return true;
        }
    }
    return false;
}

std::wstring remove_et_al(std::wstring ref) {
    int index = ref.find(L"et al.");
    if (index != -1) {
        return ref.substr(0, index) + ref.substr(index + 6);
    }
    else {
        return ref;
    }
}

bool is_year(QString str) {
    if (str.size() == 0) return false;

    for (int i = 0; i < str.size(); i++) {
        if (!str[i].isDigit()) {
            return false;
        }
    }
    int n = str.toInt();
    if (n > 1600 && n < 2100) {
        return true;
    }
    return false;
}

bool is_text_refernce_rather_than_paper_name(QString text) {
    text = strip_garbage_from_paper_name(text);

    if (text.size() > 50) {
        return false;
    }
    if ((text.indexOf("et al") != -1) || (text.indexOf("et. al") != -1)) {
        return true;
    }
    if (text.back() >= 0 && text.back().unicode() <= 128 && text.back().isDigit()) {
        return true;
    }

    QStringList parts = text.split(QRegularExpression("[ \\(\\)]"));
    for (int i = 0; i < parts.size(); i++) {
        if (is_year(parts[i])) {
            return true;
        }
    }
    return false;

}

QJsonObject rect_to_json(fz_rect rect) {
    QJsonObject res;
    res["x0"] = rect.x0;
    res["y0"] = rect.y0;
    res["x1"] = rect.x1;
    res["y1"] = rect.y1;
    return res;
}

bool pred_case_sensitive(const wchar_t& c1, const wchar_t& c2) {
    return c1 == c2;
}

wchar_t case_sensitive_hash(const wchar_t& c){
    return c;
}

bool pred_case_insensitive(const wchar_t& c1, const wchar_t& c2) {
    return std::tolower(c1) == std::tolower(c2);
}

wchar_t case_insensitive_hash(const wchar_t& c){
    return std::tolower(c);
}

// a function to return a pred based on case sensitivity
std::function<bool(const wchar_t&, const wchar_t&)> get_pred(SearchCaseSensitivity cs, const std::wstring& query) {
    if (cs == SearchCaseSensitivity::CaseSensitive) return pred_case_sensitive;
    if (cs == SearchCaseSensitivity::CaseInsensitive) return pred_case_insensitive;
    if (QString::fromStdWString(query).isLower()) return pred_case_insensitive;
    return pred_case_sensitive;
}

std::function<wchar_t(const wchar_t&)> get_hash(SearchCaseSensitivity cs, const std::wstring& query) {
    if (cs == SearchCaseSensitivity::CaseSensitive) return case_sensitive_hash;
    if (cs == SearchCaseSensitivity::CaseInsensitive) return case_insensitive_hash;
    if (QString::fromStdWString(query).isLower()) return case_insensitive_hash;
    return case_sensitive_hash;
}

std::vector<SearchResult> search_text_with_index(const std::wstring& super_fast_search_index,
    const std::vector<int>& page_begin_indices,
    const std::wstring& query,
    SearchCaseSensitivity case_sensitive,
    int begin_page,
    int min_page,
    int max_page) {

    std::vector<SearchResult> output;
    std::vector<SearchResult> before_results;

    if (min_page < 0) {
        min_page = 0;
    }

    if (max_page > page_begin_indices.size() - 1) {
        max_page = page_begin_indices.size() - 1;
    }

    int begin_index = page_begin_indices[min_page];
    int end_index = max_page == page_begin_indices.size()-1? super_fast_search_index.size() : page_begin_indices[max_page+1];
    bool is_before = true;

    auto pred = get_pred(case_sensitive, query);
    auto hash = get_hash(case_sensitive, query);
#ifdef SIOYEK_ANDROID
    // for some reason at the time of this commit std::boyer_moore_searcher doesn't
    // compile on android even though we are using c++17
    auto searcher = std::default_searcher(query.begin(), query.end(), pred);
#else
    auto searcher = std::boyer_moore_searcher(query.begin(), query.end(), hash, pred);
#endif
    auto it = std::search(
        super_fast_search_index.begin() + begin_index,
        super_fast_search_index.begin() + end_index,
        searcher);

    int match_page = min_page;

    for (; it != super_fast_search_index.end(); it = std::search(it + 1, super_fast_search_index.end(), searcher)) {
        int start_index = it - super_fast_search_index.begin();
        //std::deque<fz_rect> match_rects;
        //std::vector<fz_rect> compressed_match_rects;

        while ((match_page < page_begin_indices.size() - 1) && page_begin_indices[match_page + 1] <= start_index) match_page++;

        if (match_page >= begin_page) {
            is_before = false;
        }

        int end_index = start_index + query.size();

        SearchResult res;
        res.page = match_page;
        res.begin_index_in_page = start_index - page_begin_indices[match_page];
        res.end_index_in_page = end_index - page_begin_indices[match_page];

        if (!((match_page < min_page) || (match_page > max_page))) {
            if (is_before) {
                before_results.push_back(res);
            }
            else {
                output.push_back(res);
            }
        }
    }

    output.insert(output.end(), before_results.begin(), before_results.end());
    return output;

}


std::vector<SearchResult> search_regex_with_index(const std::wstring& super_fast_search_index,
    const std::vector<int>& page_begin_indices,
    std::wstring query,
    SearchCaseSensitivity case_sensitive,
    int begin_page,
    int min_page,
    int max_page)
{

    std::vector<SearchResult> output;

    std::wregex regex;
    if (min_page < 0) min_page = 0;
    if (max_page > page_begin_indices.size() - 1) max_page = page_begin_indices.size() - 1;

    try {
        if (case_sensitive != SearchCaseSensitivity::CaseSensitive) {
            regex = std::wregex(query, std::regex_constants::icase);
        }
        else {
            regex = std::wregex(query);
        }
    }
    catch (const std::regex_error&) {
        return output;
    }


    std::vector<SearchResult> before_results;
    bool is_before = true;

    int offset = page_begin_indices[min_page];

    std::wstring::const_iterator search_start(super_fast_search_index.begin() + offset);

    std::wsmatch match;
    int empty_tolerance = 1000;


    int match_page = min_page;

    while (std::regex_search(search_start, super_fast_search_index.cend(), match, regex)) {
        std::deque<fz_rect> match_rects;
        std::vector<fz_rect> compressed_match_rects;

        //int match_page = super_fast_search_index_pages[offset + match.position()];

        if (match_page >= begin_page) {
            is_before = false;
        }

        if (match_page > max_page) {
            break;
        }

        int start_index = offset + match.position();
        int end_index = offset + match.position() + match.length();

        while ((match_page < page_begin_indices.size() - 1) && page_begin_indices[match_page + 1] < start_index) {
            match_page++;
        }

        if (start_index < end_index) {
            SearchResult res;
            res.page = match_page;
            res.begin_index_in_page = start_index - page_begin_indices[match_page];
            res.end_index_in_page = end_index - page_begin_indices[match_page];

            if (!((match_page < min_page) || (match_page > max_page))) {
                if (is_before) {
                    before_results.push_back(res);
                }
                else {
                    output.push_back(res);
                }
            }
        }
        else {
            empty_tolerance--;
            if (empty_tolerance == 0) {
                break;
            }
        }

        offset = end_index;
        search_start = match.suffix().first;
    }
    output.insert(output.end(), before_results.begin(), before_results.end());
    return output;

}

std::vector<std::wstring> get_path_unique_prefix(const std::vector<std::wstring>& paths) {
    std::vector<QStringList> path_parts;
    QChar separator = '/';

    int max_depth = -1;
    for (auto p : paths) {
        path_parts.push_back(QString::fromStdWString(p).split(separator));
        int current_depth = path_parts.back().size();
        if (current_depth > max_depth) max_depth = current_depth;
    }

    std::vector<std::wstring> res;

    for (int depth = 1; depth <= max_depth; depth++) {

        for (auto parts : path_parts) {
            if (depth < parts.size()) {
                res.push_back(parts.mid(parts.size() - depth, depth).join(separator).toStdWString());
            }
            else {
                res.push_back(parts.join(separator).toStdWString());
            }
        }
        std::vector<std::wstring> res_copy = res;
        std::sort(res_copy.begin(), res_copy.end());
        bool found_duplicate = false;

        for (int i = 0; i < res_copy.size() - 1; i++) {
            if (res_copy[i] == res_copy[i + 1]) found_duplicate = true;
        }

        if (!found_duplicate) break;
        res.clear();
    }

    return res;
}

bool is_block_vertical(fz_stext_block* block) {

    int num_vertical = 0;
    int num_horizontal = 0;
    LL_ITER(line, block->u.t.first_line) {

        LL_ITER(ch, line->first_char) {
            if (ch->next != nullptr) {
                float horizontal_diff = std::abs(ch->quad.ll.x - ch->next->quad.ll.x);
                float vertical_diff = std::abs(ch->quad.ll.y - ch->next->quad.ll.y);
                if (vertical_diff > horizontal_diff) {
                    num_vertical++;
                }
                else {
                    num_horizontal++;
                }
            }
        }
    }
    return num_vertical > num_horizontal;
}
QString get_file_name_from_paper_name(QString paper_name) {
    if (paper_name.size() > 0) {
        QStringList parts = paper_name.split(' ');
        QString new_file_name;
        for (int i = 0; i < parts.size(); i++) {
            new_file_name += parts[i].toLower();
            if (i < parts.size() - 1) {
                new_file_name += '_';
            }
        }

        new_file_name.remove(".");
        new_file_name.remove("\\");
        return new_file_name;
    }

    return "";
}


void rgb2hsv(float* rgb_color, float* hsv_color) {
    int rgb_255_color[3];
    convert_color3(rgb_color, rgb_255_color);
    QColor qcolor(rgb_255_color[0], rgb_255_color[1], rgb_255_color[2]);
    QColor hsv_qcolor = qcolor.toHsv();
    hsv_color[0] = hsv_qcolor.hsvHueF();
    hsv_color[1] = hsv_qcolor.hsvSaturationF();
    hsv_color[2] = hsv_qcolor.lightnessF();
    if (hsv_color[0] < 0) hsv_color[0] += 1.0f;
}

void hsv2rgb(float* hsv_color, float* rgb_color) {
    QColor qcolor;
    qcolor.setHsvF(hsv_color[0], hsv_color[1], hsv_color[2]);
    rgb_color[0] = qcolor.redF();
    rgb_color[1] = qcolor.greenF();
    rgb_color[2] = qcolor.blueF();
}

bool operator==(const fz_rect& lhs, const fz_rect& rhs) {
    return lhs.x0 == rhs.x0 &&
        lhs.y0 == rhs.y0 &&
        lhs.x1 == rhs.x1 &&
        lhs.y1 == rhs.y1;
}

bool is_bright(float color[3]){
    return (color[0] + color[1] + color[2]) > 1.5f;
}


bool is_abbreviation(const std::wstring& txt){
    int n_upper = 0;
    int n_lower = 0;

    for (auto c : txt){
        // prevent crash on non-ascii chars
        if (c <= 0 || c > 128) continue;

        if (isupper(c)){
            n_upper++;
        }
        else if (islower(c)){
            n_lower++;
        }
    }

    return n_upper > n_lower;
}

bool is_in(char c, std::vector<char> candidates){
    return std::find(candidates.begin(), candidates.end(), c) != candidates.end();
}


bool is_doc_valid(fz_context* ctx, std::string path) {
    bool is_valid = false;

    fz_try(ctx) {
        fz_document* doc = fz_open_document(ctx, path.c_str());
        if (doc) {
            int n_pages = fz_count_pages(ctx, doc);
            is_valid = n_pages > 0;
            fz_drop_document(ctx, doc);
        }
    }
    fz_catch(ctx) {
        is_valid = false;
    }

    return is_valid;
}

bool should_trigger_delete(QKeyEvent *key_event) {
    if (!key_event) {
        return false;
    }

    // Check for the Delete key
    if (key_event->key() == Qt::Key_Delete) {
        return true;
    }

    // On macOS, treat Shift+Backspace as Delete as well
#ifdef Q_OS_MAC
    auto key = key_event->key();
    bool backspace_p = (key == Qt::Key_Backspace);

    bool is_control_cmd_pressed = key_event->modifiers().testFlag(Qt::ControlModifier) || key_event->modifiers().testFlag(Qt::MetaModifier);
    // I did extensive tests on trying to detect Shift+Backspace on QT 6.6 on macOS 14. But the event for Shift+Backspace was simply not sent to us. I tested both =event->type() == QEvent::KeyRelease= and =event->type() == QEvent::KeyPress=.

    if (backspace_p && is_control_cmd_pressed) {
        return true;
    }
#endif

    return false;

}

QString get_ui_font_face_name() {
    if (UI_FONT_FACE_NAME.empty()) {
        return global_font_family;
    }
    else {
        return QString::fromStdWString(UI_FONT_FACE_NAME);
    }
}

QString get_chat_font_face_name() {
    if (CHAT_FONT_FACE_NAME.size() > 0) {
        return QString::fromStdWString(CHAT_FONT_FACE_NAME);
    }
    return get_ui_font_face_name();
}

int get_chat_font_size() {
    if (CHAT_FONT_SIZE > 0){
        return CHAT_FONT_SIZE;
    }
    return DOCUMENTATION_FONT_SIZE;
}

QString get_status_font_face_name() {
    if (STATUS_FONT_FACE_NAME.empty()) {
        return global_font_family;
    }
    else {
        return QString::fromStdWString(STATUS_FONT_FACE_NAME);
    }
}


int TextToSpeechHandler::get_maximum_tts_text_size(){
    return INT_MAX;
}

QtTextToSpeechHandler::QtTextToSpeechHandler() {
    tts = new QTextToSpeech();
}

std::vector<std::wstring> TextToSpeechHandler::get_available_voices() {
    return {};
}

void TextToSpeechHandler::set_voice(std::wstring voice) {
}

QtTextToSpeechHandler::~QtTextToSpeechHandler() {
    QObject::disconnect(tts, &QTextToSpeech::sayingWord, nullptr, nullptr);
    QObject::disconnect(tts, &QTextToSpeech::stateChanged, nullptr, nullptr);
    delete tts;
}

void QtTextToSpeechHandler::say(QString text, int offset) {
    tts->say(text);
}

void QtTextToSpeechHandler::stop() {
    tts->stop();
}

void QtTextToSpeechHandler::pause() {
    tts->pause(QTextToSpeech::BoundaryHint::Immediate);
}

void QtTextToSpeechHandler::set_rate(float rate) {
    tts->setRate(rate);
}

bool QtTextToSpeechHandler::is_pausable() {
    return tts->engineCapabilities().testFlag(QTextToSpeech::Capability::PauseResume);
}

bool QtTextToSpeechHandler::is_word_by_word() {
    return tts->engineCapabilities().testFlag(QTextToSpeech::Capability::WordByWordProgress);
}

void QtTextToSpeechHandler::set_word_callback(std::function<void(int, int)> callback) {
    QObject::disconnect(tts, &QTextToSpeech::sayingWord, nullptr, nullptr);
    word_callback = callback;

    QObject::connect(tts, &QTextToSpeech::sayingWord, [&](const QString& word, qsizetype id, qsizetype start, qsizetype length) {
        word_callback.value()(start, length);
    });
}

void QtTextToSpeechHandler::set_state_change_callback(std::function<void(QString)> callback) {
    QObject::disconnect(tts, &QTextToSpeech::stateChanged, nullptr, nullptr);
    state_change_callback = callback;

    QObject::connect(tts, &QTextToSpeech::stateChanged, [&](QTextToSpeech::State state) {
        QString new_state_string = QVariant::fromValue(state).toString();

        state_change_callback.value()(new_state_string);
    });
}

void QtTextToSpeechHandler::set_external_state_change_callback(std::function<void(QString)> callback) {
}


void QtTextToSpeechHandler::set_on_app_pause_callback(std::function<QString()>){

}

void QtTextToSpeechHandler::set_on_app_resume_callback(std::function<void(bool, bool, int)>){

}

QString translate_key_mapping_to_macos(QString mapping){

    mapping = mapping.replace("D", "⌘");
    mapping = mapping.replace("C", "^");
    mapping = mapping.replace("S", "⇧");

    mapping = mapping.replace("<left>", "◀");
    mapping = mapping.replace("<up>", "▲");
    mapping = mapping.replace("<right>", "▶");
    mapping = mapping.replace("<down>", "▼");


    mapping = mapping.replace("<backspace>", "⌫");
    mapping = mapping.replace("<pageup>", "↑");
    mapping = mapping.replace("<pagedown>", "↓");

    mapping.replace("--", "<temp>");
    if (mapping.size() > 1){
        mapping.replace("-", "");
    }
    mapping.replace("<temp>", "-");


    if (mapping.startsWith("<") && mapping.endsWith(">")){
        return mapping.mid(1, mapping.size()-2);
    }

    return mapping;
}

#ifdef SIOYEK_ANDROID

AndroidTextToSpeechHandler::AndroidTextToSpeechHandler() {
}

int AndroidTextToSpeechHandler::get_maximum_tts_text_size(){
    return android_tts_get_max_text_size();
}

void AndroidTextToSpeechHandler::say(QString text, int start_offset) {
    android_tts_say(text, start_offset);
}

void AndroidTextToSpeechHandler::stop() {
    android_tts_stop();
}

void AndroidTextToSpeechHandler::pause() {
    android_tts_pause();
}

void AndroidTextToSpeechHandler::set_rate(float rate) {
    android_tts_set_rate(std::pow(4.0f, rate));
}

bool AndroidTextToSpeechHandler::is_pausable() {
    return true;
}

bool AndroidTextToSpeechHandler::is_word_by_word() {
    return true;
}

void AndroidTextToSpeechHandler::set_word_callback(std::function<void(int, int)> callback) {
    android_global_word_callback = callback;
}

void AndroidTextToSpeechHandler::set_state_change_callback(std::function<void(QString)> callback) {
    android_global_state_change_callback = callback;
}

void AndroidTextToSpeechHandler::set_external_state_change_callback(std::function<void(QString)> callback) {
    android_global_external_state_change_callback = callback;
}

void AndroidTextToSpeechHandler::set_on_app_pause_callback(std::function<QString()> callback){
    android_global_on_android_app_pause_callback = callback;
}

void AndroidTextToSpeechHandler::set_on_app_resume_callback(std::function<void(bool, bool, int)> callback){
    android_global_resume_state_callback = callback;
}
#endif

std::vector<PagelessDocumentRect> get_image_blocks_from_stext_page(fz_stext_page* stext_page){

    std::vector<PagelessDocumentRect> image_rects;
    for (fz_stext_block* blk = stext_page->first_block; blk != nullptr; blk = blk->next) {
        if (blk->type == FZ_STEXT_BLOCK_IMAGE) {
                float im_x = blk->u.i.transform.e;
                float im_y = blk->u.i.transform.f;
                float im_w = blk->u.i.transform.a;
                float im_h = blk->u.i.transform.d;
                PagelessDocumentRect image_rect;
                image_rect.x0 = im_x;
                image_rect.x1 = im_x + im_w;
                image_rect.y0 = im_y;
                image_rect.y1 = im_y + im_h;
                image_rects.push_back(image_rect);
        }
    }
    return image_rects;
}

bool is_platform_meta_pressed(QKeyEvent* kevent){
#ifdef Q_OS_MACOS
        return (kevent->modifiers() & Qt::ControlModifier);
#else
        return (kevent->modifiers() & Qt::MetaModifier);
#endif
}

bool is_platform_control_pressed(QKeyEvent* kevent){
#ifdef Q_OS_MACOS
        return (kevent->modifiers() & Qt::MetaModifier);
#else
        return (kevent->modifiers() & Qt::ControlModifier);
#endif
}

int prune_abbreviation_candidate(const std::wstring& super_fast_search_index, int start_index, int end_index, std::wstring abbr){
    abbr = QString::fromStdWString(abbr).toLower().toStdWString();

    while (start_index < end_index - abbr.size()){
        if (QChar(super_fast_search_index[start_index]).toLower() != QChar(abbr[0])){
            start_index++;
        }
        else{
            break;
        }
    }
    return start_index-1;
}

QString create_random_string(int length) {
    const QString chars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    QString s;
    for (int i = 0; i < length; ++i)
    {
        int index = QRandomGenerator::global()->generate() % chars.length();
        QChar nextChar = chars.at(index);
        s.append(nextChar);
    }
    return s;
}


QString get_paper_download_finish_action_string(PaperDownloadFinishedAction action) {
    if (action == PaperDownloadFinishedAction::DoNothing) return "none";
    if (action == PaperDownloadFinishedAction::OpenInSameWindow) return "same_window";
    if (action == PaperDownloadFinishedAction::OpenInNewWindow) return "new_window";
    if (action == PaperDownloadFinishedAction::Portal) return "portal";
    return "";
}

PaperDownloadFinishedAction get_paper_download_action_from_string(QString str) {
    if (str == "none") return PaperDownloadFinishedAction::DoNothing;
    if (str == "same_window") return PaperDownloadFinishedAction::OpenInSameWindow;
    if (str == "new_window") return PaperDownloadFinishedAction::OpenInNewWindow;
    if (str == "portal") return PaperDownloadFinishedAction::Portal;
    return PaperDownloadFinishedAction::DoNothing;
}

std::string get_user_agent_string() {
    return "Sioyek/3.0";
}

void get_color_for_mode(ColorPalette color_mode, const float* input_color, float* output_color) {
    if (!ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE) {
        output_color[0] = input_color[0];
        output_color[1] = input_color[1];
        output_color[2] = input_color[2];
        return;
    }

    if (color_mode == ColorPalette::Dark) {
        float inverted_color[3];
        inverted_color[0] = (0.5f - input_color[0]) * DARK_MODE_CONTRAST + 0.5f;
        inverted_color[1] = (0.5f - input_color[1]) * DARK_MODE_CONTRAST + 0.5f;
        inverted_color[2] = (0.5f - input_color[2]) * DARK_MODE_CONTRAST + 0.5f;
        float hsv_color[3];
        rgb2hsv(inverted_color, hsv_color);
        float new_hue = fmod(hsv_color[0] + 0.5f, 1.0f);
        hsv_color[0] = new_hue;
        hsv2rgb(hsv_color, output_color);
    }
    else if (color_mode == ColorPalette::Custom) {
        float transform_matrix[16];
        float input_vector[4];
        float output_vector[4];
        input_vector[0] = input_color[0];
        input_vector[1] = input_color[1];
        input_vector[2] = input_color[2];
        input_vector[3] = 1.0f;

        get_custom_color_transform_matrix(transform_matrix);
        matmul<4, 4, 1>(transform_matrix, input_vector, output_vector);
        output_color[0] = fz_clamp(output_vector[0], 0, 1);
        output_color[1] = fz_clamp(output_vector[1], 0, 1);
        output_color[2] = fz_clamp(output_vector[2], 0, 1);
        return;
    }
    else {
        output_color[0] = input_color[0];
        output_color[1] = input_color[1];
        output_color[2] = input_color[2];
    }
}

void get_custom_color_transform_matrix(float matrix_data[16]) {
    float inputs_inverse[16] = { 0, 1, 0, 0, -1, 1, -1, 1, 1, -1, 0, 0, 0, -1, 1, 0 };
    float outputs[16] = {
        CUSTOM_BACKGROUND_COLOR[0], CUSTOM_TEXT_COLOR[0], 1, 0,
        CUSTOM_BACKGROUND_COLOR[1], CUSTOM_TEXT_COLOR[1], CUSTOM_COLOR_CONTRAST * (1 - CUSTOM_BACKGROUND_COLOR[1]), CUSTOM_COLOR_CONTRAST * (1 - CUSTOM_BACKGROUND_COLOR[1]),
        CUSTOM_BACKGROUND_COLOR[2], CUSTOM_TEXT_COLOR[2], 0, 1,
        1, 1, 1, 1,
    };

    matmul<4, 4, 4>(outputs, inputs_inverse, matrix_data);
}

// With qpainter backend we need to manually convert pixels to
// the current colorscheme. The following code basically does this
// conversion for every pixel. For performance reasons we cache the previous result
// so we don't have to recompute it when successive pixels are the same
// (which happens a lot)
void convert_pixels_with_converter(unsigned char* pixels, int width, int height, int stride, int n_channels, std::function<void(unsigned char*)> converter) {
    unsigned char* first_pixel = pixels;
    unsigned char last_r = first_pixel[0], last_g = first_pixel[1], last_b = first_pixel[2];
    converter(first_pixel);
    unsigned char res_r = first_pixel[0], res_g = first_pixel[1], res_b = first_pixel[2];

    for (int row = 0; row < height; row++) {
        unsigned char* row_samples = pixels + row * stride;
        for (int col = 0; col < width; col++) {
            unsigned char* pixel = row_samples + col * n_channels;
            if (pixel[0] == last_r && pixel[1] == last_g && pixel[2] == last_b) {
                pixel[0] = res_r;
                pixel[1] = res_g;
                pixel[2] = res_b;
                continue;
            }
            last_r = pixel[0];
            last_g = pixel[1];
            last_b = pixel[2];

            converter(pixel);

            res_r = pixel[0];
            res_g = pixel[1];
            res_b = pixel[2];
        }
    }
};

void convert_pixel_to_dark_mode(unsigned char* pixel){
    QColor inv(
        static_cast<unsigned char>((127 - pixel[0]) * DARK_MODE_CONTRAST + 127),
        static_cast<unsigned char>((127 - pixel[1]) * DARK_MODE_CONTRAST + 127),
        static_cast<unsigned char>((127 - pixel[2]) * DARK_MODE_CONTRAST + 127)
        );

    QColor hsv_color = inv.toHsv();
    int new_hue = (hsv_color.hue() + 180) % 360;
    QColor new_color = QColor::fromHsv(new_hue, hsv_color.saturation(), hsv_color.value());

    pixel[0] = (unsigned char)new_color.red();
    pixel[1] = (unsigned char)new_color.green();
    pixel[2] = (unsigned char)new_color.blue();
}


void convert_pixel_to_custom_color(unsigned char* pixel, float transform_matrix[16]){
    float colorf[4];
    colorf[0] = static_cast<float>(pixel[0]) / 255.0f;
    colorf[1] = static_cast<float>(pixel[1]) / 255.0f;
    colorf[2] = static_cast<float>(pixel[2]) / 255.0f;
    colorf[3] = 1.0f;
    float transformed_color[4];

    matmul<4, 4, 1>(transform_matrix, colorf, transformed_color);
    transformed_color[0] = std::clamp(transformed_color[0], 0.0f, 1.0f);
    transformed_color[1] = std::clamp(transformed_color[1], 0.0f, 1.0f);
    transformed_color[2] = std::clamp(transformed_color[2], 0.0f, 1.0f);

    pixel[0] = static_cast<unsigned char>(transformed_color[0] * 255);
    pixel[1] = static_cast<unsigned char>(transformed_color[1] * 255);
    pixel[2] = static_cast<unsigned char>(transformed_color[2] * 255);
}

bool is_alpha_only(const std::wstring& str) {
    for (auto ch : str) {
        if (!std::iswalpha(ch)) return false;
    }
    return true;
}

QColor qconvert_color3(const float* input_color, ColorPalette palette) {
    std::array<float, 3> result;
    get_color_for_mode(palette, input_color, &result[0]);
    return convert_float3_to_qcolor(&result[0]);
}

//std::pair<int, int> find_smallest_substring_containing_fraction_of_n_grams_unoptimized(const std::wstring& haystack, const std::wstring& needle, int N, float fraction) {
//    std::unordered_map<std::wstring_view, int> n_gram_remaining_counts;
//    std::unordered_map<std::wstring_view, int> n_gram_required_counts;
// 
//    int NGRAMS_TO_MATCH = 0;
//
//    for (int i = 0; i < needle.size() - N + 1; i++) {
//        std::wstring_view n_gram = std::wstring_view(needle.data() + i, N);
//
//        if ((n_gram.find(L" ") != -1) || (n_gram.find(L"\n") != -1)) {
//            continue;
//        }
//        if (n_gram_remaining_counts.find(n_gram) == n_gram_remaining_counts.end()) {
//            n_gram_remaining_counts[n_gram] = 1;
//            n_gram_required_counts[n_gram] = 1;
//        }
//        else {
//            n_gram_remaining_counts[n_gram]++;
//            n_gram_required_counts[n_gram]++;
//        }
//        NGRAMS_TO_MATCH++;
//    }
//
//    int begin_index = 0;
//    int end_index = N - 1;
//    int n_matches_in_span = 0;
//
//    float MAX_MATCH_FRACTION = 2;
//    int best_start_index = -1;
//    int best_end_index = -1;
//    //int best_size = haystack.size() + 1; // inf
//    int best_score = -100000;
//    int haystack_size = haystack.size();
//
//    auto move_end_until_match = [&]() {
//        //bool already_matches = false;
//        if (((end_index - begin_index) > needle.size()) && (n_matches_in_span > (fraction * NGRAMS_TO_MATCH))) {
//            return true;
//        }
//
//        while (end_index < haystack.size() - 1) {
//            end_index++;
//
//            std::wstring_view current_ngram = std::wstring_view(haystack.data() + end_index - N, N);
//
//            auto remaining_it = n_gram_remaining_counts.find(current_ngram);
//            if (remaining_it != n_gram_remaining_counts.end()) {
//                if (remaining_it->second > 0) {
//                    n_matches_in_span++;
//                }
//                remaining_it->second--;
//            }
//            else {
//                if (((end_index - begin_index) > needle.size()) && (n_matches_in_span > (fraction * NGRAMS_TO_MATCH))) {
//                    return true;
//                }
//            }
//        }
//        return false;
//
//        };
//
//    auto move_begin_forward_one = [&]() {
//        std::wstring_view current_substring = std::wstring_view(haystack.data() + begin_index, N);
//
//        auto it = n_gram_remaining_counts.find(current_substring);
//        if ((it != n_gram_remaining_counts.end())) {
//            if (it->second >= 0) {
//                n_matches_in_span--;
//            }
//            it->second++;
//        }
//        begin_index++;
//        };
//
//
//    while (true) {
//        bool finished = !move_end_until_match();
//        if (finished) break;
//
//        int current_match_size = end_index - begin_index + 1;
//        //if (current_match_size < best_size) {
//
//        if (current_match_size < MAX_MATCH_FRACTION * needle.size()) {
//            //int lcs_size = lcs(haystack.substr(begin_index, current_match_size), needle, current_match_size, needle.size());
//            //int current_score = lcs_size * 3 - current_match_size;
//
//            int current_score = n_matches_in_span * 3 - current_match_size;
//
//            //if (lcs_size > (fraction * needle.size())) {
//            if (current_score > best_score) {
//                // possibly we could use a different fraction constant from the other fraction here?
//                best_score = current_score;
//                best_end_index = end_index;
//                best_start_index = begin_index;
//            }
//        }
//        //}
//        move_begin_forward_one();
//        //if (current_match_size < )
//    }
//    //for (int i = 0)
//    return std::make_pair(best_start_index, best_end_index-1);
//
//}

std::pair<int, int> find_smallest_substring_containing_fraction_of_n_grams(const std::wstring& haystack, const std::wstring& needle, int N, float fraction) {
    // optimized version of find_smallest_substring_containing_fraction_of_n_grams_unoptimized

    int n_gram_remaining_counts[256][256] = {0};
    int n_gram_required_counts[256][256] = {0};

    std::unordered_map<std::wstring_view, int> n_gram_remaining_counts_unicode;
    std::unordered_map<std::wstring_view, int> n_gram_required_counts_unicode;
 
    int NGRAMS_TO_MATCH = 0;

    for (int i = 0; i < needle.size() - N + 1; i++) {
        std::wstring_view n_gram = std::wstring_view(needle.data() + i, N);

        if ((n_gram.find(L" ") != -1) || (n_gram.find(L"\n") != -1)) {
            continue;
        }

        if ((N == 2) && n_gram[0] < 256 && n_gram[1] < 256) {
            n_gram_remaining_counts[n_gram[0]][n_gram[1]]++;
            n_gram_required_counts[n_gram[0]][n_gram[1]]++;
        }
        else {
            if (n_gram_remaining_counts_unicode.find(n_gram) == n_gram_remaining_counts_unicode.end()) {
                n_gram_remaining_counts_unicode[n_gram] = 1;
                n_gram_required_counts_unicode[n_gram] = 1;
            }
            else {
                n_gram_remaining_counts_unicode[n_gram]++;
                n_gram_required_counts_unicode[n_gram]++;
            }
        }

        NGRAMS_TO_MATCH++;
    }

    int begin_index = 0;
    int end_index = N - 1;
    int n_matches_in_span = 0;

    float MAX_MATCH_FRACTION = 2;
    int best_start_index = -1;
    int best_end_index = -1;
    //int best_size = haystack.size() + 1; // inf
    int best_score = -100000;
    int haystack_size = haystack.size();

    auto move_end_until_match = [&]() {
        //bool already_matches = false;
        if (((end_index - begin_index) > needle.size()) && (n_matches_in_span > (fraction * NGRAMS_TO_MATCH))) {
            return true;
        }

        while (end_index < haystack.size() - 1) {
            end_index++;

            std::wstring_view current_ngram = std::wstring_view(haystack.data() + end_index - N, N);


            if ((N != 2) || current_ngram[0] > 255 || current_ngram[1] > 255) {
                auto remaining_it = n_gram_remaining_counts_unicode.find(current_ngram);
                if (remaining_it != n_gram_remaining_counts_unicode.end()) {
                    if (remaining_it->second > 0) {
                        n_matches_in_span++;
                    }
                    remaining_it->second--;
                }
                else {
                    if (((end_index - begin_index) > needle.size()) && (n_matches_in_span > (fraction * NGRAMS_TO_MATCH))) {
                        return true;
                    }
                }
            }
            else {
                int* remaining_it = &n_gram_remaining_counts[current_ngram[0]][current_ngram[1]];
                int required = n_gram_required_counts[current_ngram[0]][current_ngram[1]];
                //auto remaining_it = n_gram_remaining_counts.find(current_ngram);
                if (required) {
                    if ((*remaining_it) > 0) {
                        n_matches_in_span++;
                    }
                    *remaining_it = (*remaining_it) - 1;
                }
                else {
                    if (((end_index - begin_index) > needle.size()) && (n_matches_in_span > (fraction * NGRAMS_TO_MATCH))) {
                        return true;
                    }
                }
            }

        }
        return false;

        };

    auto move_begin_forward_one = [&]() {
        std::wstring_view current_substring = std::wstring_view(haystack.data() + begin_index, N);
        if ((N != 2) || current_substring[0] > 255 || current_substring[1] > 255) {
            auto it = n_gram_remaining_counts_unicode.find(current_substring);
            if ((it != n_gram_remaining_counts_unicode.end())) {
                if (it->second >= 0) {
                    n_matches_in_span--;
                }
                it->second++;
            }
            begin_index++;
        }
        else {
            int* remaining_it = &n_gram_remaining_counts[current_substring[0]][current_substring[1]];
            int required = n_gram_required_counts[current_substring[0]][current_substring[1]];

            if (required) {
                if ((*remaining_it) >= 0) {
                    n_matches_in_span--;
                }
                *remaining_it = (*remaining_it) + 1;
            }
            begin_index++;
        }

        };


    while (true) {
        bool finished = !move_end_until_match();
        if (finished) break;

        int current_match_size = end_index - begin_index + 1;
        //if (current_match_size < best_size) {

        if (current_match_size < MAX_MATCH_FRACTION * needle.size()) {
            //int lcs_size = lcs(haystack.substr(begin_index, current_match_size), needle, current_match_size, needle.size());
            //int current_score = lcs_size * 3 - current_match_size;

            int current_score = n_matches_in_span * 3 - current_match_size;

            //if (lcs_size > (fraction * needle.size())) {
            if (current_score > best_score) {
                // possibly we could use a different fraction constant from the other fraction here?
                best_score = current_score;
                best_end_index = end_index;
                best_start_index = begin_index;
            }
        }
        //}
        move_begin_forward_one();
        //if (current_match_size < )
    }
    //for (int i = 0)
    return std::make_pair(best_start_index, best_end_index-1);

}

std::vector<MenuNode*> get_top_level_menu_nodes(){

    MenuNode* file_menu_node = new MenuNode{
        "File",
        "",
        {
            new MenuNode{ "open_document", "", {}},
            new MenuNode { "open_prev_doc", "", {}},
            new MenuNode { "open_document_embedded", "", {}},
            new MenuNode{ "open_document_embedded_from_current_path", "", {}},
            new MenuNode{ "open_last_document", "", {}},
            new MenuNode{ "goto_tab", "", {}},
            new MenuNode{ "-", "", {}},
            new MenuNode{ "download_clipboard_url", "", {}},
            new MenuNode{ "rename", "", {}},
        }
    };

    MenuNode* scratchpad_menu = new MenuNode{
        "Scratchpad",
        "",
        {
            new MenuNode{ "toggle_scratchpad_mode", "", {}},
            new MenuNode{ "save_scratchpad", "", {}},
            new MenuNode{ "load_scratchpad", "", {}},
            new MenuNode{ "copy_screenshot_to_scratchpad", "", {}},
            new MenuNode{ "clear_scratchpad", "", {}},
        }
    };

    MenuNode* ruler_menu = new MenuNode{
        "Ruler",
        "",
        {
            new MenuNode{ "move_visual_mark_down", "", {}},
            new MenuNode{ "move_visual_mark_up", "", {}},
            new MenuNode{ "goto_mark(`)", "Go to the last ruler location", {}},
            new MenuNode{ "toggle_visual_scroll", "Use mouse wheel to move the ruler", {}},
            new MenuNode{ "overview_definition", "", {}},
            new MenuNode{ "goto_definition", "", {}},
            new MenuNode{ "portal_to_definition", "", {}},
        }
    };

    MenuNode* window_menu_node = new MenuNode{
        "Window",
        "",
        {
            new MenuNode{ "toggle_fullscreen", "", {} },
            new MenuNode{ "maximize", "", {} },
            new MenuNode{ "new_window", "", {}},
            new MenuNode { "close_window", "", {} },
            new MenuNode { "toggle_window_configuration", "", {} },
            new MenuNode{ "goto_window", "", {} },
        }
    };

    MenuNode* overview_view_menu = new MenuNode{
        "Overview",
        "",
        {
            new MenuNode{ "zoom_in_overview", "", {} },
            new MenuNode { "zoom_out_overview", "", {} },
            new MenuNode { "move_left_in_overview", "", {} },
            new MenuNode { "move_right_in_overview", "", {} },
            new MenuNode { "close_overview", "", {} },
            new MenuNode { "next_overview", "", {} },
            new MenuNode { "previous_overview", "", {} },
            new MenuNode { "download_overview_paper", "", {} },
        }
    };

    MenuNode* view_menu = new MenuNode{
        "View",
        "",
        {
            new MenuNode { "zoom_in", "", {} },
            new MenuNode { "zoom_out", "", {} },
            new MenuNode{ "fit_to_page_width", "", {} },
            new MenuNode{ "fit_to_page_width_smart", "", {} },
            new MenuNode{ "fit_to_page_height", "", {} },
            new MenuNode{ "fit_to_page_height_smart", "", {} },
            new MenuNode{ "toggle_presentation_mode", "", {} },
            new MenuNode{ "-", "", {} },
            new MenuNode{ "toggle_two_page_mode", "", {} },
            new MenuNode{ "toggle_dark_mode", "", {} },
            new MenuNode{ "toggle_custom_color", "", {} },
            new MenuNode{ "toggle_scrollbar", "", {} },
            new MenuNode{ "toggle_statusbar", "", {} },
            new MenuNode{ "toggle_horizontal_scroll_lock", "", {} },
            new MenuNode{ "toggle_pdf_annotations", "", {} },
            new MenuNode{ "toggle_config('preserve_image_colors_in_dark_mode')", "Toggle preserve image colors in dark mode", {} },
            overview_view_menu
        }
    };

    MenuNode* navigate_menu = new MenuNode{
        "Naviagte",
        "",
        {
            new MenuNode{ "goto_page_with_page_number", "", {} },
            new MenuNode{ "goto_page_with_label", "", {} },
            new MenuNode{ "goto_toc", "", {} },
            new MenuNode{ "next_page", "", {} },
            new MenuNode { "previous_page", "", {} },
            new MenuNode { "goto_beginning", "", {} },
            new MenuNode { "goto_end", "", {} },
            new MenuNode { "screen_down", "", {} },
            new MenuNode { "screen_up", "", {} },
            new MenuNode{ "-", "", {} },
            new MenuNode { "next_state", "", {} },
            new MenuNode { "prev_state", "", {} },
            new MenuNode{ "-", "", {} },
            new MenuNode { "search", "", {} },
            new MenuNode { "regex_search", "", {} },
            new MenuNode { "next_item", "", {} },
            new MenuNode { "previous_item", "", {} },
            new MenuNode { "overview_next_item", "", {} },
            new MenuNode { "overview_prev_item", "", {} },
        }
    };

    MenuNode* bookmark_menu = new MenuNode{
        "Bookmarks",
        "",
        {
            new MenuNode{ "goto_bookmark", "", {} },
            new MenuNode{ "goto_bookmark_g", "", {} },
            new MenuNode{ "add_bookmark", "", {} },
            new MenuNode{ "add_marked_bookmark", "", {} },
            new MenuNode{ "add_freetext_bookmark", "", {} },
            new MenuNode{ "delete_visible_bookmark", "", {} },
            new MenuNode{ "edit_visible_bookmark", "Edit the selected bookmark", {} },
        }
    };

    MenuNode* mark_menu = new MenuNode{
        "Marks",
        "",
        {
            new MenuNode{ "set_mark", "", {} },
            new MenuNode{ "goto_mark", "", {} },
        }
    };
    MenuNode* highlight_menu = new MenuNode{
        "Highlights",
        "",
        {
            new MenuNode{ "goto_highlight", "", {} },
            new MenuNode{ "goto_highlight_g", "", {} },
            new MenuNode{ "add_highlight", "", {} },
            new MenuNode{ "add_annot_to_selected_highlight", "", {} },
            new MenuNode{ "add_highlight_with_current_type", "", {} },
            new MenuNode{ "edit_visible_highlight", "Edit the selected highlight", {} },
            new MenuNode{ "delete_highlight", "", {} },
        }
    };

    MenuNode* portal_menu = new MenuNode{
        "Portals",
        "",
        {
            new MenuNode{ "portal", "Set the source/destination of a portal", {} },
            new MenuNode{ "create_visible_portal", "Set the source of a portal visible on the document", {} },
            new MenuNode{ "delete_portal", "", {} },
            new MenuNode{ "toggle_window_configuration", "", {} },
            new MenuNode{ "goto_portal_list", "", {} },
        }
    };

    MenuNode* drawing_menu = new MenuNode{
        "Drawings",
        "",
        {
            new MenuNode{ "toggle_freehand_drawing_mode", "", {} },
            new MenuNode{ "delete_freehand_drawings", "", {} },
            new MenuNode{ "set_freehand_thickness", "", {} },
            new MenuNode{ "set_freehand_type", "", {} },
            new MenuNode{ "toggle_drawing_mask", "", {} },
        }
    };

    MenuNode* annotation_menu = new MenuNode{
        "Annotations",
        "",
        {
            mark_menu,
            bookmark_menu,
            highlight_menu,
            portal_menu,
            drawing_menu,
            new MenuNode{ "embed_annotations", "", {} },
            new MenuNode{ "import_annotations", "", {} },
        }
    };

    MenuNode* tools_menu = new MenuNode{
        "Tools",
        "",
        {
            new MenuNode{ "command", "Show the list of all commands", {} },
            new MenuNode{ "command_palette", "Show the command palette", {} },
            new MenuNode{ "toggle_reading", "", {} },
            new MenuNode{ "set_config('tts_rate')", "Set reading speed", {} },
            new MenuNode{ "toggle_synctex", "", {} },
            scratchpad_menu,
            ruler_menu,
        }
    };

    MenuNode* prefs_menu = new MenuNode{
        "Preferences",
        "",
        {
            new MenuNode{ "prefs_user", "", {} },
            new MenuNode{ "keys_user", "", {} },
            new MenuNode{ "prefs_user_all", "", {} },
            new MenuNode{ "keys_user_all", "", {} },
            new MenuNode{ "keys", "", {} },
            new MenuNode{ "prefs", "", {} },
        }
    };

    std::vector<MenuNode*> top_level_menus = {
        file_menu_node,
        window_menu_node,
        view_menu,
        navigate_menu,
        annotation_menu,
        tools_menu,
        prefs_menu,
    };

    return top_level_menus;
}

std::wstring replace_verbatim_links(std::wstring input) {
    // convert '@verbatim(string)' in input to '[ref](string)'
    std::wstring result;
    std::wstring::size_type start = 0;
    std::wstring::size_type end = 0;
    int index = 1;

    while (start < input.size()) {
        start = input.find(L"@verbatim({", start);
        if (start == std::wstring::npos) {
            result += input.substr(end);
            break;
        }
        result += input.substr(end, start - end);
        end = input.find(L"})", start);
        if (end == std::wstring::npos) {
            result += input.substr(start);
            break;
        }
        std::wstring link = input.substr(start + 11, end - start - 11);
        link = QString(QUrl::toPercentEncoding(QString::fromStdWString(link))).toStdWString();
        result += L"[[" + QString::number(index).toStdWString() + L"]](sioyek://" + link + L")";
        start = end + 2;
        end = start;
        index++;
    }
    return result;
}

QVariantMap get_color_mapping() {
    QVariantMap color_map;
    for (int i = 'a'; i <= 'z'; i++) {
        QColor color = QColor::fromRgbF(
            HIGHLIGHT_COLORS[3 * (i - 'a') + 0],
            HIGHLIGHT_COLORS[3 * (i - 'a') + 1],
            HIGHLIGHT_COLORS[3 * (i - 'a') + 2]
        );
        color_map[QString::number(i)] = color;
        color_map[QString::number(i + 'A' - 'a')] = color;
    }
    color_map["_"] = QVariant::fromValue(Qt::black);
    return color_map;
}

QListView* get_ui_new_listview(){
    QListView* view = new QListView();
    view->setSpacing(5);
    return view;
}

bool is_process_still_running(qint64 pid) {
#ifndef SIOYEK_MOBILE
#ifdef Q_OS_WIN
    if (pid == -1) {
        return false;
    }
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (process == NULL) {
        return false;
    }
    DWORD exit_code;
    GetExitCodeProcess(process, &exit_code);
    CloseHandle(process);
    return exit_code == STILL_ACTIVE;
#else
    if (pid == -1) {
        return false;
    }
    QFile file(QString("/proc/%1").arg(pid));
    return file.exists();
#endif
#endif
}

void kill_process(qint64 pid) {
#ifndef SIOYEK_MOBILE
#ifdef Q_OS_WIN
    if (pid == -1) {
        return;
    }
    HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (process == NULL) {
        return;
    }
    TerminateProcess(process, 0);
    CloseHandle(process);
#else
    if (pid == -1) {
        return;
    }
    kill(pid, SIGKILL);
#endif
#endif
}

std::vector<std::wstring> get_last_opened_file_name() {
    static bool is_cached = false;
    static std::vector<std::wstring> cached_result = {};

    if (is_cached) return cached_result;

    std::string file_path_;
    std::ifstream last_state_file(last_opened_file_address_path.get_path_utf8());
    std::vector<std::wstring> res;
    while (std::getline(last_state_file, file_path_)) {
        res.push_back(utf8_decode(file_path_));
    }
    last_state_file.close();
    cached_result = res;
    is_cached = true;

    return res;
}

std::wstring clip_string_to_length(const std::wstring& input, int length) {
    if (input.size() > length) {
        return input.substr(0, length) + L"...";
    }
    return input;
}

std::vector<std::wstring> QtTextToSpeechHandler::get_available_voices() {
    auto voices = tts->availableVoices();
    std::vector<std::wstring> res;
    for (auto voice : voices) {
        res.push_back(voice.name().toStdWString());
    }
    return res;
}

void QtTextToSpeechHandler::set_voice(std::wstring voice_name) {
    auto voices = tts->availableVoices();
    for (auto voice : voices) {
        if (voice.name() == QString::fromStdWString(voice_name)) {
            tts->setVoice(voice);
        }
    }
}

std::vector<int> largest_rectangle_helper(const std::vector<int>& heights) {
    std::vector<int> smaller_indices;
    std::vector<int> offsets;

    smaller_indices.push_back(-1);

    for (int i = 0; i < heights.size(); i++) {
        int current_height = heights[i];
        while (smaller_indices.back() != -1 && heights[smaller_indices.back()] >= current_height) {
            smaller_indices.pop_back();
        }
        offsets.push_back((i - 1 - smaller_indices.back()));
        smaller_indices.push_back(i);
    }

    return offsets;
}

struct LargestRectangleOutput {
    std::vector<int> areas;
    std::vector<int> begin_indices;
    std::vector<int> end_indices;
};

struct LargestRectangleOutputMatrix {
    std::vector<std::vector<int>> areas;
    std::vector<std::vector<int>> begin_indices;
    std::vector<std::vector<int>> end_indices;
};

LargestRectangleOutput largest_rectangle(const std::vector<int>& heights){
    LargestRectangleOutput res;

    std::vector<int> reversed;
    reversed.reserve(heights.size());
    for (int i = 0; i < heights.size(); i++) {
        reversed.push_back(heights[heights.size() - 1 - i]);
    }

    std::vector<int> prev_offsets = largest_rectangle_helper(heights);
    std::vector<int> next_offsets = largest_rectangle_helper(reversed);


    for (int i = 0; i < heights.size(); i++) {
        int begin_index = i - prev_offsets[i];
        int end_index = i + next_offsets[heights.size() - 1 - i];
        int area = heights[i] * (end_index - begin_index + 1);
        res.areas.push_back(area);
        res.begin_indices.push_back(begin_index);
        res.end_indices.push_back(end_index);
    }
    return res;
}

std::vector<std::vector<bool>> char_matrix_to_bool_matrix(const std::vector<std::vector<char>>& m) {
    std::vector<std::vector<bool>> res;
    for (int i = 0; i < m.size(); i++) {
        std::vector<bool> row;

        for (int j = 0; j < m[0].size(); j++) {
            row.push_back(m[i][j] == '1');
        }

        res.push_back(row);
    }
    return res;
}


LargestRectangleOutputMatrix areas_from_hist(const std::vector<std::vector<int>>& hist_matrix) {
    LargestRectangleOutputMatrix res;

    for (int i = 0; i < hist_matrix.size(); i++) {
        LargestRectangleOutput row_areas = largest_rectangle(hist_matrix[i]);
        res.areas.push_back(row_areas.areas);
        res.begin_indices.push_back(row_areas.begin_indices);
        res.end_indices.push_back(row_areas.end_indices);
    }
    return res;

}

std::vector<std::vector<int>> histogram_matrix_from_bool_matrix(const std::vector<std::vector<bool>>& bmatrix) {
    std::vector<std::vector<int>> res;
    for (int i = 0; i < bmatrix.size(); i++) {
        std::vector<int> row;
        for (int j = 0; j < bmatrix[0].size(); j++) {
            row.push_back(0);
        }
        res.push_back(row);
    }

    for (int i = 0; i < bmatrix.size(); i++) {
        for (int j = 0; j < bmatrix[0].size(); j++) {
            if (bmatrix[i][j]) {
                if (i > 0) {
                    res[i][j] = res[i - 1][j] + 1;
                }
                else {
                    res[i][j] = 1;
                }

            }
        }
    }
    return res;
}

std::vector<MaximumRectangleResult> maximum_rectangle(std::vector<std::vector<bool>>& rect) {
    auto hist = histogram_matrix_from_bool_matrix(rect);
    LargestRectangleOutputMatrix largest_rectangles = areas_from_hist(hist);
    MaximumRectangleResult res;
    res.area = 0;

    for (int i = 0; i < largest_rectangles.areas.size(); i++) {

        for (int j = 0; j < largest_rectangles.areas[0].size(); j++) {
            if (largest_rectangles.areas[i][j] > res.area) {
                res.area = largest_rectangles.areas[i][j];
                res.begin_col = largest_rectangles.begin_indices[i][j];
                res.end_col = largest_rectangles.end_indices[i][j];
                res.end_row = i;
                res.begin_row = i - res.area / (res.end_col - res.begin_col + 1);
            }
        }
    }
    int threshold = res.area / 4;

    std::vector<std::pair<int, int>> large_non_dominated_indices;

    for (int i = 0; i < largest_rectangles.areas.size(); i++) {
        for (int j = 0; j < largest_rectangles.areas[0].size(); j++) {
            int index_area = largest_rectangles.areas[i][j];
            if (index_area < threshold) continue;
            if (i < largest_rectangles.areas.size() - 1) {
                int next_row_area = largest_rectangles.areas[i + 1][j];
                if (next_row_area >= index_area) continue;
            }
            if (j < largest_rectangles.areas[0].size() - 1) {
                int next_col_area = largest_rectangles.areas[i][j+1];
                if (next_col_area >= index_area) continue;
            }
            large_non_dominated_indices.push_back(std::make_pair(i, j));
        }
    }

    std::vector<MaximumRectangleResult> large_results;
    for (auto [i, j] : large_non_dominated_indices) {
        MaximumRectangleResult index_result;
        index_result.area = largest_rectangles.areas[i][j];
        index_result.begin_col = largest_rectangles.begin_indices[i][j];
        index_result.end_col = largest_rectangles.end_indices[i][j];
        index_result.end_row = i;
        index_result.begin_row = i - index_result.area / (index_result.end_col - index_result.begin_col + 1);
        large_results.push_back(index_result);

    }

    std::sort(large_results.begin(), large_results.end(), [&](const MaximumRectangleResult& lhs, const MaximumRectangleResult& rhs) {
        return lhs.area > rhs.area;
        });

    while (large_results.size() > 20) {
        large_results.pop_back();
    }

    std::vector<MaximumRectangleResult> final_results;
    final_results.push_back(large_results[0]);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < large_results.size(); j++) {
            bool is_acceptable = true;
            for (int k = 0; k < final_results.size(); k++) {
                WindowRect current_rect;
                current_rect.x0 = large_results[j].begin_col;
                current_rect.x1 = large_results[j].end_col;
                current_rect.y0 = large_results[j].begin_row;
                current_rect.y1 = large_results[j].end_row;
                int current_area = current_rect.area();

                WindowRect accepted_rect;
                accepted_rect.x0 = final_results[k].begin_col;
                accepted_rect.x1 = final_results[k].end_col;
                accepted_rect.y0 = final_results[k].begin_row;
                accepted_rect.y1 = final_results[k].end_row;
                int accepted_area = accepted_rect.area();

                auto intersection_rect = current_rect.intersect_rect(accepted_rect);
                int intersection_area = intersection_rect.area();
                int threshold = std::min(current_area, accepted_area) / 2;
                if (intersection_area > threshold) {
                    is_acceptable = false;
                }
            }
            if (is_acceptable) {
                final_results.push_back(large_results[j]);
                break;
            }
        }
    }

    return final_results;

}

void open_text_editor_at_line(QString file_path, int line_number) {
    if (EXTERNAL_TEXT_EDITOR_COMMAND.size() > 0) {
        std::wstring command = QString::fromStdWString(EXTERNAL_TEXT_EDITOR_COMMAND)
            .replace("%{file}", file_path)
            .replace("%{line}", QString::number(line_number))
            .toStdWString();
        QString command_qstring = QString::fromStdWString(command);
        QStringList parts = QProcess::splitCommand(command_qstring);
        QString command_name = parts[0];
        QStringList args = parts.mid(1);

        run_command(command_name.toStdWString(), args);
        //QProcess::startDetached(command_name, args);

    }
    else {
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(file_path))) {
            show_error_message(("Could not open address: " + file_path).toStdWString());
        }
    }
}

bool stext_page_has_lines(fz_stext_page* page) {
    LL_ITER(block, page->first_block) {
        if (block->type == FZ_STEXT_BLOCK_TEXT) {
            LL_ITER(line, block->u.t.first_line) {
                return true;
            }
        }
    }
    return false;
}

#ifdef SIOYEK_ADVANCED_AUDIO

void data_callback_f32(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {

    MyPlayer* player = (MyPlayer*)pDevice->pUserData;

    std::vector<float> temp_output(frameCount * player->decoder.outputChannels);
    ma_decoder_read_pcm_frames(&player->decoder, &temp_output[0], frameCount, NULL);

#ifdef SIOYEK_USE_SOUNDTOUCH
    player->soundTouch.putSamples((const float*)temp_output.data(), frameCount);
    int n_recv = player->soundTouch.receiveSamples((float*)pOutput, frameCount);
#else
    player->vocoder->putSamples((const float*)temp_output.data(), frameCount);
    player->vocoder->receiveSamples((float*)pOutput, frameCount);
    float scale = std::sqrt(player->vocoder->playbackRate);
    for (int i = 0; i < frameCount * player->decoder.outputChannels; i++) {
        *((float*)pOutput + i) *= scale;
    }
#endif

}

void data_callback_s16(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {

    MyPlayer* player = (MyPlayer*)pDevice->pUserData;

    std::vector<int16_t> temp_output(frameCount * player->decoder.outputChannels);
    std::vector<float> temp_output_f32(frameCount * player->decoder.outputChannels);
    std::vector<float> temp_input_f32(frameCount * player->decoder.outputChannels);

    ma_decoder_read_pcm_frames(&player->decoder, &temp_output[0], frameCount, NULL);

    for (int i = 0; i < temp_output.size(); i++) {
        temp_output_f32[i] = temp_output[i] / 32768.0f;
    }
    

#ifdef SIOYEK_USE_SOUNDTOUCH
    player->soundTouch.putSamples((const float*)temp_output_f32.data(), frameCount);
    int n_recv = player->soundTouch.receiveSamples((float*)temp_input_f32.data(), frameCount);
#else
    player->vocoder->putSamples((const float*)temp_output_f32.data(), frameCount);
    player->vocoder->receiveSamples((float*)temp_input_f32.data(), frameCount);

    float scale = std::sqrt(player->vocoder->playbackRate);
    for (int i = 0; i < frameCount * player->decoder.outputChannels; i++) {
        *((float*)pOutput + i) *= scale;
    }
#endif

    for (int i = 0; i < temp_input_f32.size(); i++) {
        *((int16_t*)pOutput + i) = static_cast<int16_t>(temp_input_f32[i] * 32768.0f);
    }

}

void MyPlayer::set_source(std::string path) {
    finishedEmitted = false;

    if (currentRate < 0) {
        currentRate = TTS_RATE + 1;
    }

    if (device != nullptr) {
        ma_device_uninit(device);
        delete device;
        device = nullptr;
    }

    if (ma_decoder_init_file(path.c_str(), NULL, &decoder) != MA_SUCCESS) {
        std::wcout << L"could not load file\n";
    }

#ifdef SIOYEK_USE_SOUNDTOUCH
    soundTouch.setChannels(decoder.outputChannels);
    soundTouch.setSampleRate(decoder.outputSampleRate);
    soundTouch.setPitch(1.0 / currentRate);
    soundTouch.setTempo(1.0);
    soundTouch.setRate(1.0);
    soundTouch.setSetting(SETTING_OVERLAP_MS, 20);
    soundTouch.setSetting(SETTING_NOMINAL_INPUT_SEQUENCE, 6);
    soundTouch.setSetting(SETTING_NOMINAL_OUTPUT_SEQUENCE, 7);
    soundTouch.setSetting(SETTING_INITIAL_LATENCY, 8);
#else
    vocoder = new PhaseVocoder(1.0f / (currentRate * currentRate), 1.0f / currentRate, 512);
#endif


    deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate = decoder.outputSampleRate * currentRate;
    if (decoder.outputFormat == ma_format_f32) {
        deviceConfig.dataCallback = data_callback_f32;
    }
    else {
        deviceConfig.dataCallback = data_callback_s16;
    }
    deviceConfig.pUserData = this;

    device = new ma_device();
    if (ma_device_init(NULL, &deviceConfig, device) != MA_SUCCESS) {
        std::wcout << L"Failed to open playback device.\n";
        ma_decoder_uninit(&decoder);
    }

}

void MyPlayer::play() {
    if (device) {

        if (ma_device_start(device) != MA_SUCCESS) {
            std::wcout << L"Failed to start playback device.\n";
        }
    }
}

void MyPlayer::pause() {
    if (device) {
        ma_device_stop(device);
    }
}

void MyPlayer::stop() {
    if (device) {
        ma_device_stop(device);
        ma_decoder_seek_to_pcm_frame(&decoder, 0);
    }
}

void MyPlayer::seek(unsigned long long miliseconds) {
    finishedEmitted = false;
    if (device) {
        ma_decoder_seek_to_pcm_frame(&decoder, miliseconds * decoder.outputSampleRate / 1000);
    }
}

void MyPlayer::setPosition(unsigned long long miliseconds) {
    seek(miliseconds);
}

int MyPlayer::position() {
    // return the time in miliseconds
    ma_uint64 cursor;
    ma_decoder_get_cursor_in_pcm_frames(&decoder, &cursor);
    return cursor * 1000 / decoder.outputSampleRate;
} 
bool MyPlayer::isFinished() {
    if (device && !finishedEmitted) {
        ma_uint64 cursor;
        ma_decoder_get_cursor_in_pcm_frames(&decoder, &cursor);

        if (cursor >= trackLength) {
            finishedEmitted = true;
            return true;
        }

    }

    return false;
}

bool MyPlayer::isPlaying() {
    if (device) {
        return ma_device_is_started(device);
    }
    return false;
}

void MyPlayer::set_volume(float volume) {
    if (device) {
        ma_device_set_master_volume(device, volume);
    }
}

float MyPlayer::get_volume() {
    if (device) {
        float volume;
        ma_device_get_master_volume(device, &volume);
        return volume;
    }
    return 0;
}

void MyPlayer::setPlaybackRate(float rate) {
    currentRate = rate;

    if (device) {
        int current_position = position();
        bool was_playing = isPlaying();

#ifdef SIOYEK_USE_SOUNDTOUCH
        soundTouch.setPitch(1.0f / rate);
#else
        vocoder->setPlaybackRate(1.0f / (rate * rate));
        vocoder->setPitchScale(1.0f / rate);
#endif
        // recreate the audio device with the new sample rate
        ma_device_uninit(device);
        deviceConfig.sampleRate = decoder.outputSampleRate * rate;

        if (ma_device_init(NULL, &deviceConfig, device) != MA_SUCCESS) {
            std::wcout << L"Failed to open playback device.\n";
        }

        seek(current_position);
        if (was_playing) {
            play();
        }
    }
}

void MyPlayer::setSource(const QUrl& source) {
    set_source(source.toLocalFile().toStdString());
    ma_uint64 length;
    ma_decoder_get_length_in_pcm_frames(&decoder, &length);
    trackLength = length;
}

bool MyPlayer::isSeekable() {
    return true;
}

#endif

template<typename T>
T mymin(T x1, T x2) {
    return x1 < x2 ? x1 : x2;
}

static bool is_power_of_two(size_t n) {
    return n && ((n & (n - 1)) == 0);
}

// Returns the smallest power of two >= n.
static size_t next_power_of_two(size_t n) {
    size_t p = 1;
    while (p < n)
        p *= 2;
    return p;
}

// FFT for power-of-two sizes using the iterative Cooley–Tukey algorithm.
static void fft_pow2(std::vector<std::complex<float>> &a, bool inverse) {
    size_t n = a.size();
    // Bit-reversal permutation.
    for (size_t i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j -= bit;
        j += bit;
        if (i < j)
            std::swap(a[i], a[j]);
    }

    // Cooley–Tukey iterative FFT.
    for (size_t len = 2; len <= n; len <<= 1) {
        float angle = 2 * float(M_PI) / len * (inverse ? 1 : -1);
        std::complex<float> wlen(cos(angle), sin(angle));
        for (size_t i = 0; i < n; i += len) {
            std::complex<float> w(1);
            for (size_t j = 0; j < len / 2; j++) {
                std::complex<float> u = a[i + j];
                std::complex<float> v = a[i + j + len / 2] * w;
                a[i + j] = u + v;
                a[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
    if (inverse) {
        for (size_t i = 0; i < n; i++)
            a[i] /= n;
    }
}

// Convolution helper (for Bluestein's algorithm) -- not used here.
static std::vector<std::complex<float>>
convolve(const std::vector<std::complex<float>> &a,
         const std::vector<std::complex<float>> &b) {
    size_t n = a.size(), m = b.size();
    size_t size = next_power_of_two(n + m - 1);
    std::vector<std::complex<float>> fa(size), fb(size);
    for (size_t i = 0; i < n; i++)
        fa[i] = a[i];
    for (size_t i = n; i < size; i++)
        fa[i] = 0;
    for (size_t i = 0; i < m; i++)
        fb[i] = b[i];
    for (size_t i = m; i < size; i++)
        fb[i] = 0;
    
    fft_pow2(fa, false);
    fft_pow2(fb, false);
    for (size_t i = 0; i < size; i++)
        fa[i] *= fb[i];
    fft_pow2(fa, true);
    fa.resize(n + m - 1);
    return fa;
}

// In-place FFT that supports any size (using Bluestein’s algorithm if needed).
void fft(std::vector<std::complex<float>> &a, bool inverse = false) {
    size_t n = a.size();
    if (n == 0)
        return;
    if (is_power_of_two(n)) {
        fft_pow2(a, inverse);
        return;
    }
    // Bluestein's algorithm (omitted here for brevity).
    // For this example we assume that FFT sizes used in the vocoder
    // are powers of two.
    assert(false && "Bluestein FFT not implemented in this example");
}

// ==================================================================
// Utility: wrap a phase to the interval [-pi, pi)
static float princarg(float phase) {
    while(phase < -M_PI) phase += 2*M_PI;
    while(phase >= M_PI) phase -= 2*M_PI;
    return phase;
}

PhaseVocoder::PhaseVocoder(float playbackRate, float pitchScale, int fftSize)
    : playbackRate(playbackRate), pitchScale(pitchScale), fftSize(fftSize)
{
    // Choose analysis hop size (e.g. 1/4 frame)
    analysisHop = fftSize / 4;
    // The vocoder time-scale factor is (playbackRate / pitchScale)
    timeScale = playbackRate / pitchScale;
    // Synthesis hop is analysisHop scaled by timeScale.
    synthesisHop = int(analysisHop * timeScale + 0.5f);

    // Prepare a Hann window.
    window.resize(fftSize);
    for (int n = 0; n < fftSize; n++) {
        window[n] = 0.5f * (1 - cos(2 * M_PI * n / (fftSize - 1)));
    }

    // Phase–tracking buffers (one value per FFT bin)
    prevPhase.assign(fftSize / 2 + 1, 0.0f);
    sumPhase.assign(fftSize / 2 + 1, 0.0f);
    firstFrame = true;

    // Buffers:
    inputBuffer.clear();
    vocoderBuffer.clear();
    nextSynthesisWritePos = 0;
    resamplePos = 0.0f;

    // Set caps on maximum sizes.
    maxBufferSize = fftSize * 10;      // Maximum vocoderBuffer size.
    maxInputBufferSize = fftSize * 10;   // Maximum inputBuffer size.
}

void PhaseVocoder::putSamples(const float* data, int count) {
    // If adding these samples would exceed the maxInputBufferSize,
    // remove the oldest samples from the front.
    int totalAfterInsert = static_cast<int>(inputBuffer.size()) + count;
    if (totalAfterInsert > maxInputBufferSize) {
        int toDrop = totalAfterInsert - maxInputBufferSize;
        // Ensure we do not drop more than available.
        toDrop = mymin(toDrop, static_cast<int>(inputBuffer.size()));
        inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + toDrop);
    }
    // Now append the new samples.
    inputBuffer.insert(inputBuffer.end(), data, data + count);
    processFrames();
}

int PhaseVocoder::receiveSamples(float* out, int count) {
    int samplesWritten = 0;
    // While we have enough samples in vocoderBuffer for linear interpolation…
    while (samplesWritten < count && (static_cast<int>(vocoderBuffer.size()) - static_cast<int>(resamplePos)) > 1) {
        int idx = static_cast<int>(resamplePos);
        float frac = resamplePos - idx;
        float sample = vocoderBuffer[idx] * (1 - frac) + vocoderBuffer[idx + 1] * frac;
        out[samplesWritten++] = sample;
        // Advance the read pointer by the resampling step (pitchScale).
        resamplePos += pitchScale;
    }
    // Remove consumed samples from vocoderBuffer.
    int removeCount = static_cast<int>(resamplePos);
    if (removeCount > 0) {
        vocoderBuffer.erase(vocoderBuffer.begin(), vocoderBuffer.begin() + removeCount);
        resamplePos -= removeCount;
        nextSynthesisWritePos -= removeCount;
        if (nextSynthesisWritePos < 0) nextSynthesisWritePos = 0;
    }
    return samplesWritten;
}

void PhaseVocoder::setPlaybackRate(float rate) {
    playbackRate = rate;
    updateParameters();
}

void PhaseVocoder::setPitchScale(float scale) {
    pitchScale = scale;
    updateParameters();
}

void PhaseVocoder::updateParameters() {
    timeScale = playbackRate / pitchScale;
    synthesisHop = int(analysisHop * timeScale + 0.5f);
}

void PhaseVocoder::processFrames() {
    // Process frames only if we have at least fftSize samples in inputBuffer
    // and if adding another frame will not exceed the vocoderBuffer cap.
    while (static_cast<int>(inputBuffer.size()) >= fftSize &&
        (nextSynthesisWritePos + fftSize <= maxBufferSize)) {
        // Copy one frame and apply the window.
        std::vector<float> frame(fftSize);
        for (int n = 0; n < fftSize; n++) {
            frame[n] = inputBuffer[n] * window[n];
        }
        // Remove analysisHop samples from inputBuffer.
        inputBuffer.erase(inputBuffer.begin(), inputBuffer.begin() + analysisHop);

        // Prepare FFT input.
        std::vector<std::complex<float>> X(fftSize);
        for (int n = 0; n < fftSize; n++) {
            X[n] = std::complex<float>(frame[n], 0.0f);
        }
        // Compute forward FFT.
        fft(X, false);

        // Process nonredundant bins (0 .. fftSize/2).
        for (int k = 0; k <= fftSize / 2; k++) {
            float mag = std::abs(X[k]);
            float phase = std::arg(X[k]);
            float deltaPhase = phase - prevPhase[k];
            prevPhase[k] = phase;
            float expected = 2 * M_PI * k * analysisHop / fftSize;
            float delta = princarg(deltaPhase - expected);
            float trueFreq = (2 * M_PI * k / fftSize) + (delta / analysisHop);
            if (firstFrame)
                sumPhase[k] = phase;
            else
                sumPhase[k] += synthesisHop * trueFreq;
            float newPhase = sumPhase[k];
            X[k] = std::polar(mag, newPhase);
            if (k > 0 && k < fftSize / 2)
                X[fftSize - k] = std::conj(X[k]);
        }
        firstFrame = false;

        // Compute inverse FFT.
        fft(X, true);

        // Reconstruct time-domain output frame.
        std::vector<float> outputFrame(fftSize);
        for (int n = 0; n < fftSize; n++) {
            outputFrame[n] = X[n].real() * window[n];
        }

        // Ensure vocoderBuffer is long enough.
        int requiredSize = nextSynthesisWritePos + fftSize;
        if (requiredSize > static_cast<int>(vocoderBuffer.size()))
            vocoderBuffer.resize(requiredSize, 0.0f);

        // Overlap–add the output frame into vocoderBuffer.
        for (int n = 0; n < fftSize; n++) {
            vocoderBuffer[nextSynthesisWritePos + n] += outputFrame[n];
        }
        // Advance the synthesis write pointer.
        nextSynthesisWritePos += synthesisHop;
    }
}

void focus_on_widget(QWidget* widget, bool no_unminimize) {
    widget->activateWindow();
    if (no_unminimize) {
        widget->setWindowState(widget->windowState() | Qt::WindowActive);
    }
    else {
        widget->setWindowState(widget->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    }
}

#ifdef Q_OS_WIN
HWND get_window_hwnd_with_pid(qint64 pid) {
    // Windows implementation: enumerate top-level windows for matching PID.
    struct FindWindowData {
        DWORD pid;
        HWND hwnd;
    } data = { static_cast<DWORD>(pid), nullptr };

    auto enumWindowsProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
        FindWindowData* pData = reinterpret_cast<FindWindowData*>(lParam);
        DWORD winPid = 0;
        GetWindowThreadProcessId(hwnd, &winPid);
        // Check visible window from matching process.
        if (winPid == pData->pid && IsWindowVisible(hwnd)) {
            pData->hwnd = hwnd;
            return FALSE; // found; stop enumeration
        }
        return TRUE;
    };

    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

void clip_child_to_parent(HWND hChild, HWND hParent, RECT child_rect) {

    // Get the parent window's client area
    RECT parentRect;
    GetClientRect(hParent, &parentRect);
    
    // Convert to screen coordinates
    POINT pt = { parentRect.left, parentRect.top };
    ClientToScreen(hParent, &pt);
    parentRect.left = pt.x;
    parentRect.top = pt.y;

    pt = { parentRect.right, parentRect.bottom };
    ClientToScreen(hParent, &pt);
    parentRect.right = pt.x;
    parentRect.bottom = pt.y;

    RECT intersectRect;
    if (IntersectRect(&intersectRect, &parentRect, &child_rect)) {
        HRGN hRgn = CreateRectRgn(
            intersectRect.left - child_rect.left,
            intersectRect.top - child_rect.top,
            intersectRect.right - child_rect.left,
            intersectRect.bottom - child_rect.top
        );
        SetWindowRgn(hChild, hRgn, TRUE);

    } else {
        HRGN hRgn = CreateRectRgn(
            0,
            0,
            0,
            0);
        SetWindowRgn(hChild, hRgn, TRUE);
    }
}
#endif

void move_resize_window(WId parent_hwnd, qint64 pid, int x, int y, int width, int height, bool is_focused) {
#if defined(_WIN32)
    HWND hwnd = get_window_hwnd_with_pid(pid);
    if (hwnd) {
        // this makes sure that the window doesn't have a titlebar and border
        SetWindowLong(hwnd, GWL_STYLE, 0);

        if (is_focused) {
            SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        }
        else {
            SetWindowPos(hwnd, (HWND)parent_hwnd, x, y, width, height, SWP_SHOWWINDOW | SWP_NOACTIVATE);
            SetWindowPos((HWND)parent_hwnd, hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }

        RECT child_rect;
        child_rect.left = x;
        child_rect.right = x + width;
        child_rect.top = y;
        child_rect.bottom = y + height;
        // only show the part of the window that is inside the parent window
        clip_child_to_parent(hwnd, (HWND)parent_hwnd, child_rect);
    }
#elif defined(Q_OS_LINUX)
    // Moving windows on linux? I don't think so.
#elif defined(Q_OS_MACOS)
    // // macOS implementation: use Accessibility API to get the app's main window.
    // // Note: the calling process must have the accessibility permission.
    // AXUIElementRef appElem = AXUIElementCreateApplication(static_cast<pid_t>(pid));
    // if (!appElem) return;

    // // First try to get the main window.
    // AXUIElementRef window = nullptr;
    // AXError err = AXUIElementCopyAttributeValue(appElem, kAXMainWindowAttribute, (CFTypeRef*)&window);
    // if (err != kAXErrorSuccess || !window) {
    //     // Fallback: get the first window in the kAXWindowsAttribute.
    //     CFArrayRef windowList = nullptr;
    //     err = AXUIElementCopyAttributeValue(appElem, kAXWindowsAttribute, (CFTypeRef*)&windowList);
    //     if (err == kAXErrorSuccess && windowList) {
    //         if (CFArrayGetCount(windowList) > 0) {
    //             window = (AXUIElementRef)CFArrayGetValueAtIndex(windowList, 0);
    //             // Retain the window since we are going to use it.
    //             if(window) CFRelease(window);
    //         }
    //         CFRelease(windowList);
    //     }
    // }
    // if (window) {
    //     // Set position.
    //     CGPoint pt = { static_cast<CGFloat>(x), static_cast<CGFloat>(y) };
    //     AXValueRef posValue = AXValueCreate(kAXValueCGPointType, &pt);
    //     if (posValue) {
    //         AXUIElementSetAttributeValue(window, kAXPositionAttribute, posValue);
    //         CFRelease(posValue);
    //     }
    //     // Set size.
    //     CGSize size = { static_cast<CGFloat>(width), static_cast<CGFloat>(height) };
    //     AXValueRef sizeValue = AXValueCreate(kAXValueCGSizeType, &size);
    //     if (sizeValue) {
    //         AXUIElementSetAttributeValue(window, kAXSizeAttribute, sizeValue);
    //         CFRelease(sizeValue);
    //     }
    //     CFRelease(window);
    // }
    // CFRelease(appElem);
#endif
}