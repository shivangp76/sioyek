#pragma once

#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <set>
#include <regex>
#include <optional>
#include <memory>
#include <qcommandlineparser.h>
#include <qvariantmap.h>
#include <qmediaplayer.h>

#include <QKeyEvent>
#include <qpoint.h>
#include <qjsonarray.h>
#include <qtexttospeech.h>

#include <mupdf/fitz.h>

#include "book.h"
#include "types/common_types.h"
#include "utf8.h"
#include "coordinates.h"

#ifndef Q_OS_MACOS
#ifdef SIOYEK_ADVANCED_AUDIO
#ifdef SIOYEK_USE_SOUNDTOUCH
#include <soundtouch/SoundTouch.h>
#endif
#include "miniaudio.h"

#endif
#endif

#define LL_ITER(name, start) for(auto name=start;(name);name=name->next)
#define LOG(expr) if (VERBOSE) {(expr);};


struct JsCommandInfo {
    std::wstring pref_file_path;
    int line_number = 0;
    std::wstring js_file_path;
    std::optional<std::wstring> entry_point;
};

struct CustomCommandInfo {
    std::wstring text;
    std::wstring definition_file_path;
    int definition_line_number = 0;
};


class QListView;
std::wstring to_lower(const std::wstring& inp);
bool is_separator(fz_stext_char* last_char, fz_stext_char* current_char);
void get_flat_toc(const std::vector<TocNode*>& roots, std::vector<std::wstring>& output, std::vector<DocumentPos>& pages);
int mod(int a, int b);
ParsedUri parse_uri(fz_context* mupdf_context, fz_document* document, std::string uri);
char get_symbol(int key, bool is_shift_pressed, const std::vector<char>& special_symbols);

template<typename T>
int argminf(const std::vector<T>& collection, std::function<float(T)> f) {

    float min = std::numeric_limits<float>::infinity();
    int min_index = -1;
    for (size_t i = 0; i < collection.size(); i++) {
        float element_value = f(collection[i]);
        if (element_value < min) {
            min = element_value;
            min_index = i;
        }
    }
    return min_index;
}
void rect_to_quad(fz_rect rect, float quad[8]);
void copy_to_clipboard(const std::wstring& text, bool selection = false);
void install_app(const char* argv0);
int get_f_key(std::wstring name);
void show_error_message(const std::wstring& error_message);
int show_option_buttons(const std::wstring& error_message, const std::vector<std::wstring>& buttons);
std::wstring utf8_decode(const std::string& encoded_str);
std::string utf8_encode(const std::wstring& decoded_str);
// is the character a right to left character
bool is_rtl(int c);
std::wstring reverse_wstring(const std::wstring& inp);
bool parse_search_command(const std::wstring& search_command, int* out_begin, int* out_end, std::wstring* search_text);

// given a tree of toc nodes and an array of indices, returns the node whose ith parent is indexed by the ith element
// of the indices array. That is:
// root[indices[0]][indices[1]] ... [indices[indices.size()-1]]
TocNode* get_toc_node_from_indices(const std::vector<TocNode*>& roots, const std::vector<int>& indices);

fz_stext_char* find_closest_char_to_document_point(const std::vector<fz_stext_char*> flat_chars, fz_point document_point, int* location_index);
fz_stext_line* find_closest_line_to_document_point(fz_stext_page* page, fz_point document_point, int* index);
//void merge_selected_character_rects(const std::deque<fz_rect>& selected_character_rects, std::vector<fz_rect>& resulting_rects, bool touch_vertically = true);
void split_key_string(std::wstring haystack, const std::wstring& needle, std::vector<std::wstring>& res);
void run_command(std::wstring command, QStringList parameters, bool wait = true);

std::wstring get_string_from_stext_block(fz_stext_block* block, bool handle_rtl=false, bool dehyphenate=true);
std::wstring get_string_from_stext_line(fz_stext_line* line, bool handle_rtl=false);
std::vector<PagelessDocumentRect> get_char_rects_from_stext_line(fz_stext_line* line);
void sleep_ms(unsigned int ms);
//void open_url(const std::string& url_string);
//void open_url(const std::wstring& url_string);

void open_file_url(const std::wstring& file_url, bool show_fail_message);
void open_web_url(const std::wstring& web_url);

void open_file(const std::wstring& path, bool show_fail_message);
void search_custom_engine(const std::wstring& search_string, const std::wstring& custom_engine_url);
void index_references(fz_stext_page* page, int page_number, std::map<std::wstring, IndexedData>& indices);
void get_flat_chars_from_stext_page(fz_stext_page* stext_page, std::vector<fz_stext_char*>& flat_chars, bool dehyphenate=false);
//void get_flat_chars_from_stext_page_for_bib_detection(fz_stext_page* stext_page, std::vector<fz_stext_char*>& flat_chars);
int find_best_vertical_line_location(fz_pixmap* pixmap, int relative_click_x, int relative_click_y);
//void get_flat_chars_from_stext_page_with_space(fz_stext_page* stext_page, std::vector<fz_stext_char*>& flat_chars, fz_stext_char* space);
void index_equations(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::map<std::wstring, std::vector<IndexedData>>& indices);
void find_regex_matches_in_stext_page(const std::vector<fz_stext_char*>& flat_chars,
    const std::wregex& regex,
    std::vector<std::pair<int, int>>& match_ranges, std::vector<std::wstring>& match_texts);
bool is_string_numeric(const std::wstring& str);
bool is_string_numeric_float(const std::wstring& str);
void create_file_if_not_exists(const std::wstring& path);
QByteArray serialize_string_array(const QStringList& string_list);
QStringList deserialize_string_array(const QByteArray& byte_array);
//Path add_redundant_dot_to_path(const Path& sane_path);
bool should_reuse_instance(int argc, char** argv);
bool should_new_instance(int argc, char** argv);
QCommandLineParser* get_command_line_parser();
std::wstring concatenate_path(const std::wstring& prefix, const std::wstring& suffix);
std::wstring get_canonical_path(const std::wstring& path);
void split_path(std::wstring path, std::vector<std::wstring>& res);
//std::wstring canonicalize_path(const std::wstring& path);
std::wstring add_redundant_dot_to_path(const std::wstring& path);
float manhattan_distance(float x1, float y1, float x2, float y2);
float manhattan_distance(fvec2 v1, fvec2 v2);
QWidget* get_top_level_widget(QWidget* widget);
std::wstring strip_string(std::wstring& input_string);
//void index_generic(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::vector<IndexedData>& indices);
void index_generic(const std::vector<fz_stext_char*>& flat_chars, int page_number, std::vector<IndexedData>& indices);
std::vector<std::wstring> split_whitespace(std::wstring const& input);
float type_name_similarity_score(std::wstring name1, std::wstring name2);
bool is_stext_page_rtl(fz_stext_page* stext_page);
void check_for_updates(QWidget* parent, std::string current_version);
char* get_argv_value(int argc, char** argv, std::string key);
void split_root_file(QString path, QString& out_root, QString& out_partial);
QString expand_home_dir(QString path);
std::vector<unsigned int> get_max_width_histogram_from_pixmap(fz_pixmap* pixmap);
//std::vector<unsigned int> get_line_ends_from_histogram(std::vector<unsigned int> histogram);
void get_line_begins_and_ends_from_histogram(std::vector<unsigned int> histogram, std::vector<unsigned int>& begins, std::vector<unsigned int>& ends);

template<typename T>
int find_nth_larger_element_in_sorted_list(std::vector<T> sorted_list, T value, int n) {
    int i = 0;
    while (i < sorted_list.size() && (value >= sorted_list[i])) i++;
    if ((i < sorted_list.size()) && (sorted_list[i] == value)) i--;
    if ((i + n - 1) < sorted_list.size()) {
        return i + n - 1;
    }
    else {
        return -1;
    }

}

void copy_file(std::wstring src_path, std::wstring dst_path);
fz_quad quad_from_rect(fz_rect r);
std::vector<fz_quad> quads_from_rects(const std::vector<fz_rect>& rects);
std::wifstream open_wifstream(const std::wstring& file_name);
std::wofstream open_wofstream(const std::wstring& file_name);
void get_flat_words_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars, std::vector<PagelessDocumentRect>& flat_word_rects, std::vector<std::vector<PagelessDocumentRect>>* out_char_rects = nullptr);
void get_word_rect_list_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars,
    std::vector<std::wstring>& words,
    std::vector<std::vector<PagelessDocumentRect>>& flat_word_rects);

std::vector<std::string> get_tags(int n);
int get_num_tag_digits(int n);
int get_index_from_tag(std::string tag, bool reversed=false);
std::wstring truncate_string(const std::wstring& inp, int size);
std::wstring get_page_formatted_string(int page);
PagelessDocumentRect create_word_rect(const std::vector<PagelessDocumentRect>& chars);
std::vector<PagelessDocumentRect> create_word_rects_multiline(const std::vector<PagelessDocumentRect>& chars);
void get_flat_chars_from_block(fz_stext_block* block, std::vector<fz_stext_char*>& flat_chars, bool dehyphenate=false);
void get_text_from_flat_chars(const std::vector<fz_stext_char*>& flat_chars, std::wstring& string_res, std::vector<int>& indices);
bool is_string_titlish(const std::wstring& str);
bool is_title_parent_of(const std::wstring& parent_title, const std::wstring& child_title, bool* are_same);
std::wstring find_first_regex_match(const std::wstring& haystack, const std::wstring& regex_string);
//void merge_lines(
//    std::vector<fz_stext_line*> lines,
//    std::vector<PagelessDocumentRect>& out_rects,
//    std::vector<std::wstring>& out_texts,
//    std::vector<std::vector<PagelessDocumentRect>>* out_line_characters,
//    std::vector<PagelessDocumentRect>* out_next_rects);

//int lcs(const char* X, const char* Y, int m, int n);

bool has_arg(int argc, char** argv, std::string key);
std::vector<std::wstring> find_all_regex_matches(std::wstring haystack, const std::wstring& regex_string, std::vector<std::pair<int, int>>* match_ranges = nullptr);
bool command_requires_text(const std::wstring& command);
bool command_requires_rect(const std::wstring& command);
void parse_command_string(std::wstring command_string, std::string& command_name, std::wstring& command_data);
void flat_char_prism2(const std::vector<fz_stext_char*>& chars, int page, std::wstring& output_text, std::vector<int>& page_begin_indices);
void flat_char_prism(const std::vector<fz_stext_char*>& chars, int page, std::wstring& output_text, std::vector<int>& pages, std::vector<PagelessDocumentRect>& rects);



std::string get_aplph_tag(int n, int max_n);
fz_document* open_document_with_file_name(fz_context* context, std::wstring file_name);


#ifdef SIOYEK_ANDROID
QString android_file_name_from_uri(QString uri);
void android_tts_say(QString text);
void check_pending_intents(const QString workingDirPath);
void android_tts_pause();
void android_tts_stop();
void android_tts_set_rate(float rate);
// void android_tts_set_rest_of_document(QString rest);
void on_android_pause_global();
void on_android_resume_global();
void android_brightness_set(float brightness);
float android_brightness_get();

#endif

float dampen_velocity(float v, float dt);

template<typename T>
T compute_average(std::vector<T> items) {

    T acc = items[0];
    for (int i = 1; i < items.size(); i++) {
        acc += items[i];
    }
    return acc / items.size();
}

fz_irect get_index_irect(fz_rect original, int index, fz_matrix transform, int num_h_slices, int num_v_slices);
fz_rect get_index_rect(fz_rect original, int index, int num_h_slices, int num_v_slices);

#ifdef SIOYEK_ANDROID
QString android_file_uri_from_content_uri(QString uri);
#endif

QString clean_bib_item(QString bib_item);
std::wstring clean_link_source_text(std::wstring link_source_text);

QList<FreehandDrawingPoint> smooth_filter_drawing_points(const QList<FreehandDrawingPoint>& points, int amount);
QList<FreehandDrawingPoint> prune_freehand_drawing_points(const QList<FreehandDrawingPoint>& points);
std::optional<DocumentRect> find_expanding_rect(bool before, fz_stext_page* page, DocumentRect page_rect);
std::vector<DocumentRect> find_expanding_rect_word(bool before, fz_stext_page* page, DocumentRect page_rect);
std::optional<DocumentRect> find_shrinking_rect_word(bool before, fz_stext_page* page, DocumentRect page_rect);
bool are_rects_same(fz_rect r1, fz_rect r2);

QStringList extract_paper_data_from_json_response(QJsonValue json_object, const std::vector<QString>& path);
QStringList extract_paper_string_from_json_response(QJsonObject json_object, std::wstring path);
QString file_size_to_human_readable_string(int file_size);

std::wstring new_uuid();
std::string new_uuid_utf8();
bool is_text_rtl(const std::wstring& text);
bool are_same(float f1, float f2);
bool are_same(const FreehandDrawing& lhs, const FreehandDrawing& rhs);

template<typename T>
QJsonArray export_array(std::vector<T> objects, std::string checksum) {
    QJsonArray res;

    for (const T& obj : objects) {
        res.append(obj.to_json(checksum));
    }
    return res;
}

template <typename T>
std::vector<T> load_from_json_array(const QJsonArray& item_list) {

    std::vector<T> res;

    for (int i = 0; i < item_list.size(); i++) {
        QJsonObject current_json_object = item_list.at(i).toObject();
        auto current_object = T::from_json(current_json_object);
        res.push_back(current_object);
    }
    return res;
}

template<typename T>
std::map<std::string, int> annotation_prism(std::vector<T>& file_annotations,
    std::vector<T>& existing_annotations,
    std::vector<Annotation*>& new_annotations,
    std::vector<Annotation*>& updated_annotations,
    std::vector<Annotation*>& deleted_annotations)
{

    std::map<std::string, int> existing_annotation_ids;
    std::map<std::string, int> file_annotation_ids;

    for (int i = 0; i < existing_annotations.size(); i++) {
        existing_annotation_ids[existing_annotations[i].uuid] = i;
    }

    for (int i = 0; i < file_annotations.size(); i++) {
        file_annotation_ids[file_annotations[i].uuid] = i;
    }

    //for (auto annot : file_annotations) {
    for (int i = 0; i < file_annotations.size(); i++) {
        if (existing_annotation_ids.find(file_annotations[i].uuid) == existing_annotation_ids.end()) {
            new_annotations.push_back(&file_annotations[i]);
        }

        else {
            int index = existing_annotation_ids[file_annotations[i].uuid];
            if (existing_annotations[index].get_modification_datetime().msecsTo(file_annotations[i].get_modification_datetime()) > 1000) {
                updated_annotations.push_back(&file_annotations[i]);
            }
        }
    }
    //for (auto annot : existing_annotations) {
    for (int i = 0; i < existing_annotations.size(); i++) {
        if (file_annotation_ids.find(existing_annotations[i].uuid) == file_annotation_ids.end()) {
            deleted_annotations.push_back(&existing_annotations[i]);
        }
    }

    return existing_annotation_ids;
}

PagelessDocumentRect get_range_rect_union(const std::vector<PagelessDocumentRect>& rects, int first_index, int last_index);
QString get_paper_name_from_reference_text(QString reference_text);
fz_rect get_first_page_size(fz_context* ctx, const std::wstring& document_path);
QString get_direct_pdf_url_from_archive_url(QString url);
QString get_original_url_from_archive_url(QString url);
bool does_paper_name_match_query(std::wstring query, std::wstring paper_name);
bool is_dot_index_end_of_a_reference(const std::vector<DocumentCharacter>& flat_chars, int dot_index);
std::wstring remove_et_al(std::wstring ref);
void get_flat_chars_from_stext_page_for_bib_detection(fz_stext_page* stext_page, std::vector<DocumentCharacter>& flat_chars);
QJsonObject rect_to_json(fz_rect rect);

std::vector<SearchResult> search_text_with_index(const std::wstring& super_fast_search_index,
    const std::vector<int>& page_begin_indices,
    const std::wstring& query,
    SearchCaseSensitivity case_sensitive,
    int begin_page,
    int min_page,
    int max_page);

bool pred_case_sensitive(const wchar_t& c1, const wchar_t& c2);
bool pred_case_insensitive(const wchar_t& c1, const wchar_t& c2);


std::vector<SearchResult> search_regex_with_index(const std::wstring& super_fast_search_index,
    const std::vector<int>& page_begin_indcies,
    std::wstring query,
    SearchCaseSensitivity case_sensitive,
    int begin_page,
    int min_page,
    int max_page);

float rect_area(fz_rect rect);
std::vector<std::wstring> get_path_unique_prefix(const std::vector<std::wstring>& paths);
bool is_block_vertical(fz_stext_block* block);
QString get_file_name_from_paper_name(QString paper_name);

bool operator==(const fz_rect& lhs, const fz_rect& rhs);

fz_rect bound_rects(const std::vector<fz_rect>& rects);
bool is_consequtive(fz_rect rect1, fz_rect rect2);

template<typename R>
void merge_selected_character_rects(const std::deque<R>& selected_character_rects, std::vector<R>& resulting_rects, bool touch_vertically=true) {
    /*
        This function merges the bounding boxes of all selected characters into large line chunks.
    */

    if (selected_character_rects.size() == 0) {
        return;
    }

    std::vector<fz_rect> line_rects;

    fz_rect last_rect = selected_character_rects[0];
    line_rects.push_back(selected_character_rects[0]);

    for (size_t i = 1; i < selected_character_rects.size(); i++) {
        if (is_consequtive(last_rect, selected_character_rects[i])) {
            last_rect = selected_character_rects[i];
            line_rects.push_back(selected_character_rects[i]);
        }
        else {
            fz_rect bounding_rect = bound_rects(line_rects);
            resulting_rects.push_back(bounding_rect);
            line_rects.clear();
            last_rect = selected_character_rects[i];
            line_rects.push_back(selected_character_rects[i]);
        }
    }

    if (line_rects.size() > 0) {
        fz_rect bounding_rect = bound_rects(line_rects);
        resulting_rects.push_back(bounding_rect);
    }

    // avoid overlapping rects
    for (size_t i = 0; i < resulting_rects.size() - 1; i++) {
        // we don't need to do this across columns of document
        float height = std::abs(resulting_rects[i].y1 - resulting_rects[i].y0);
        if (std::abs(resulting_rects[i + 1].y0 - resulting_rects[i].y0) < (0.5 * height)) {
            continue;
        }
        if (touch_vertically) {
            if ((resulting_rects[i + 1].x0 < resulting_rects[i].x1)) {
                const float MERGING_LINE_DISTANCE_THRESHOLD = 25.0f;
                if (std::abs(resulting_rects[i + 1].y0 - resulting_rects[i].y1) < MERGING_LINE_DISTANCE_THRESHOLD) {
                    resulting_rects[i + 1].y0 = resulting_rects[i].y1;
                }
            }
        }
    }

}

template<typename R>
std::vector<fz_quad> quads_from_rects(const std::vector<R>& rects) {
    std::vector<fz_quad> res;
    for (auto rect : rects) {
        res.push_back(quad_from_rect(rect));
    }
    return res;
}
bool is_bright(float color[3]);
bool is_abbreviation(const std::wstring& txt);
bool is_in(char c, std::vector<char> candidates);
bool is_doc_valid(fz_context* ctx, std::string path);
int get_chat_font_size();
std::vector<fz_stext_char*> reorder_stext_line(fz_stext_line* line);
std::vector<fz_stext_char*> reorder_mixed_stext_line(fz_stext_line* line);
bool should_trigger_delete(QKeyEvent *key_event);

class TextToSpeechHandler {
public:
    virtual void set_rate(float rate) = 0;

    virtual void say(QString text, int start_offset=-1) = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual bool is_playing() = 0;
    virtual bool is_pausable() = 0;
    virtual bool is_word_by_word() = 0;
    virtual void set_word_callback(std::function<void(int, int)>) = 0;
    virtual void set_state_change_callback(std::function<void(QString)>) = 0;
    virtual void set_external_state_change_callback(std::function<void(QString)>) = 0;
    virtual void set_on_app_pause_callback(std::function<QString()>) = 0;
    virtual void set_on_app_resume_callback(std::function<void(bool, bool, int)>) = 0;
    virtual int get_maximum_tts_text_size();
    virtual std::vector<std::wstring> get_available_voices();
    virtual std::wstring current_voice();
    virtual void set_voice(std::wstring voice);
};

class QtTextToSpeechHandler : public TextToSpeechHandler {
public:
    QTextToSpeech* tts;
    std::optional<std::function<void(int, int)>> word_callback = {};
    std::optional<std::function<void(QString)>> state_change_callback = {};

    QtTextToSpeechHandler();

    ~QtTextToSpeechHandler();

    void say(QString text, int start_offset=-1) override;

    void stop() override;

    void pause() override;

    void set_rate(float rate);

    bool is_pausable();

    bool is_playing() override;

    bool is_word_by_word();

    virtual void set_word_callback(std::function<void(int, int)> callback);

    virtual void set_state_change_callback(std::function<void(QString)> callback);
    virtual void set_external_state_change_callback(std::function<void(QString)> callback);
    virtual void set_on_app_pause_callback(std::function<QString()>);
    virtual void set_on_app_resume_callback(std::function<void(bool, bool, int)>);

    std::vector<std::wstring> get_available_voices() override;
    std::wstring current_voice() override;
    void set_voice(std::wstring voice) override;
};


#ifdef SIOYEK_ANDROID
class AndroidTextToSpeechHandler : public TextToSpeechHandler {
public:
    // std::optional<std::function<void(int, int)>> word_callback = {};
    // std::optional<std::function<void(QString)>> state_change_callback = {};

    AndroidTextToSpeechHandler();

    void say(QString text, int start_offset=-1) override;

    void stop() override;

    void pause() override;

    void set_rate(float rate);

    bool is_pausable();

    bool is_word_by_word();

    void set_word_callback(std::function<void(int, int)> callback);

    void set_state_change_callback(std::function<void(QString)> callback);
    void set_external_state_change_callback(std::function<void(QString)> callback);
    virtual void set_on_app_pause_callback(std::function<QString()>);
    virtual void set_on_app_resume_callback(std::function<void(bool, bool, int)>);
    int get_maximum_tts_text_size();
};
#endif

std::wstring get_path_extras_file_name(const std::wstring& path);
QString translate_key_mapping_to_macos(QString mapping);
std::vector<PagelessDocumentRect> get_image_blocks_from_stext_page(fz_stext_page* stext_page);

bool is_platform_meta_pressed(Qt::KeyboardModifiers modifiers);
bool is_platform_control_pressed(Qt::KeyboardModifiers modifiers);

int prune_abbreviation_candidate(const std::wstring& super_fast_search_index, int start_index, int end_index, std::wstring abbr);
QString create_random_string(int length=31);
QString get_paper_download_finish_action_string(PaperDownloadFinishedAction action);
PaperDownloadFinishedAction get_paper_download_action_from_string(QString str);
std::string get_user_agent_string();

template<typename T>
struct DecomposeSetResult {
    std::vector<T> A_minus_B;
    std::vector<T> B_minus_A;
    std::vector<std::pair<T, T>> intersection;
};

template<typename T>
DecomposeSetResult<T> decompose_sets(const std::vector<T>& A, const std::vector<T>& B){
    std::unordered_map<std::string, int> A_id_to_index;
    std::unordered_map<std::string, int> B_id_to_index;

    std::vector<std::string> A_ids;
    std::vector<std::string> B_ids;

    std::vector<std::string> A_minus_B_ids;
    std::vector<std::string> B_minus_A_ids;
    std::vector<std::string> intersection_ids;

    DecomposeSetResult<T> res;


    for (int i = 0; i < A.size(); i++) {
        A_id_to_index[A[i].uuid] = i;
        A_ids.push_back(A[i].uuid);
    }

    for (int i = 0; i < B.size(); i++) {
        B_id_to_index[B[i].uuid] = i;
        B_ids.push_back(B[i].uuid);
    }

    std::sort(A_ids.begin(), A_ids.end());
    std::sort(B_ids.begin(), B_ids.end());

    std::set_difference(
        A_ids.begin(), A_ids.end(),
        B_ids.begin(), B_ids.end(),
        std::back_inserter(A_minus_B_ids)
    );

    std::set_difference(
        B_ids.begin(), B_ids.end(),
        A_ids.begin(), A_ids.end(),
        std::back_inserter(B_minus_A_ids)
    );

    std::set_intersection(
        A_ids.begin(), A_ids.end(),
        B_ids.begin(), B_ids.end(),
        std::back_inserter(intersection_ids)
    );

    for (auto& id : A_minus_B_ids) {
        res.A_minus_B.push_back(A[A_id_to_index[id]]);
    }

    for (auto& id : B_minus_A_ids) {
        res.B_minus_A.push_back(B[B_id_to_index[id]]);
    }

    for (auto& id : intersection_ids) {
        res.intersection.push_back(std::make_pair(A[A_id_to_index[id]], B[B_id_to_index[id]]));
    }
    return res;

}

template<typename T>
std::vector<std::string> get_uuids(const std::vector<T>& annots) {
    std::vector<std::string> res;
    for (auto& annot : annots) {
        res.push_back(annot.uuid);
    }
    return res;
}

struct PageMergedLinesInfo {
    std::vector<PagelessDocumentRect> merged_line_rects;
    std::vector<std::wstring> merged_line_texts;
    std::vector<std::vector<PagelessDocumentRect>> merged_line_chars;
    std::vector<std::vector<int>> merged_line_indices;
};

struct PageMergedLinesInfoAbsolute {
    std::vector<AbsoluteRect> merged_line_rects;
    std::vector<std::wstring> merged_line_texts;
    std::vector<std::vector<PagelessDocumentRect>> merged_line_chars;
    std::vector<std::vector<int>> merged_line_indices;
};

PageMergedLinesInfo merge_lines2(const std::vector<fz_stext_line*>& lines);

#define TIME_BEGIN QDateTime time_begin=QDateTime::currentDateTime();
#define TIME_END QDateTime time_end=QDateTime::currentDateTime();qDebug() << time_begin.msecsTo(time_end);
void transpose_matrix(float mat[16]);
//std::pair<int, int> find_smallest_containing_substring(const std::wstring& haystack, const std::wstring needle, wchar_t delimeter = ' ');
//int similarity_score(const std::wstring& haystack, const std::wstring& needle);




bool is_alpha_only(const std::wstring& str);
std::pair<int, int> find_smallest_substring_containing_fraction_of_n_grams(const std::wstring& haystack, const std::wstring& needle, int N, float fraction);
std::vector<MenuNode*> get_top_level_menu_nodes();
std::wstring replace_verbatim_links(std::wstring input);

void log_d(QString text);
bool is_process_still_running(qint64 pid);
void kill_process(qint64 pid);
std::vector<std::wstring> get_last_opened_file_name();
std::wstring clip_string_to_length(const std::wstring& input, int length);



void open_text_editor_at_line(QString file_path, int line_number);
bool stext_page_has_lines(fz_stext_page* page);

class PhaseVocoder {
public:
    PhaseVocoder(float playbackRate, float pitchScale, int fftSize = 1024);
    
    void putSamples(const float* data, int count);
    
    int receiveSamples(float* out, int count);
    
    void setPlaybackRate(float rate);

    void setPitchScale(float scale);
    
    float playbackRate;  // desired overall playback rate factor.
private:
    // Parameters.
    float pitchScale;    // pitch shift factor.
    float timeScale;     // internal time-scale factor = playbackRate / pitchScale.
    int fftSize;         // FFT frame size (power of two)
    int analysisHop;     // analysis hop size.
    int synthesisHop;    // synthesis hop size.
    std::vector<float> window; // analysis/synthesis window.
    
    // Phase tracking for frequency bins (0 .. fftSize/2).
    std::vector<float> prevPhase;
    std::vector<float> sumPhase;
    bool firstFrame;
    
    // Input sample buffer.
    std::vector<float> inputBuffer;
    
    // Intermediate output (after time-stretch).
    std::vector<float> vocoderBuffer;
    int nextSynthesisWritePos; // Global index for overlap-add.
    float resamplePos;         // Fractional index for output reading.
    
    // Maximum allowed buffer sizes.
    int maxBufferSize;     // Maximum size of vocoderBuffer.
    int maxInputBufferSize; // Maximum size of inputBuffer.
    
    void updateParameters();
    void processFrames();
};

#ifdef Q_OS_APPLE
class MacosMediaPlayer{

public:
    bool playback_finished = false;
    void set_source(std::string path, QString file_name, int page_number);
    void play();
    void pause();
    void stop();
    void seek(unsigned long long miliseconds);
    void setPosition(unsigned long long miliseconds);
    int position();
    bool isPlaying();
    void set_volume(float volume);
    float get_volume();
    void setPlaybackRate(float rate);
    bool isSeekable();
    bool isFinished();
    bool isNewlyFinished();
    void setSource(const QUrl& source, QString name, int page);
    float duration();
};

#else

#ifdef SIOYEK_ADVANCED_AUDIO
class MyPlayer {
public:

#ifdef SIOYEK_USE_SOUNDTOUCH
    soundtouch::SoundTouch soundTouch;
#else
    PhaseVocoder* vocoder = nullptr;
#endif

    ma_decoder decoder;
    ma_device_config deviceConfig;
    ma_device* device = nullptr;
    float currentRate = -1.0f;
    unsigned long long trackLength = 0;
    bool finishedEmitted = false;



    void set_source(std::string path);

    void play();

    void pause();
    void stop();

    void seek(unsigned long long miliseconds);
    void setPosition(unsigned long long miliseconds);

    int position();

    bool isPlaying();

    void set_volume(float volume);

    float get_volume();

    void setPlaybackRate(float rate);
    bool isSeekable();
    bool isFinished();

    void setSource(const QUrl& source);


};
#endif // SIOYEK_ADVACNED_AUDIO
#endif // Q_OS_MACOS


#ifdef SIOYEK_IOS
std::wstring ios_add_appdir(std::wstring path);
std::wstring ios_remove_appdir(std::wstring path);
#endif

struct DetectedRectResult{
    AbsoluteRect rect;
    float x0_strength;
    float x1_strength;
    float y0_strength;
    float y1_strength;
};
std::optional<DetectedRectResult> detect_rect_drawing(const std::vector<FreehandDrawing>& drawings);


template<typename T>
std::optional<T> optional_from_qvariant(QVariant variant){
    if (variant.isNull()) return{};
    return variant.value<T>();
}
void open_directory(QString path);

void clean_old_high_quality_tts_cached_files(bool force_all=false);

std::optional<AbsoluteRect> get_absolute_rect_from_json_coordinates(QJsonArray gemini_coordinates, AbsoluteRect window_rect);
