#pragma once
#include <vector>
#include <string>
#include <optional>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <unordered_map>
#include <deque>
#include <regex>
#include <shared_mutex>
#include <qtypes.h>

//#include <Windows.h>
#include <qstandarditemmodel.h>
#include <qdatetime.h>

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
#include <mupdf/pdf/annot.h>
#include <qobject.h>
#include <qnetworkreply.h>
#include <qjsondocument.h>
#include <qurlquery.h>

#include "book.h"
#include "coordinates.h"
// #include "database.h"
// #include "utils.h"

class CachedChecksummer;
class DatabaseManager;


struct RegexMatchInfo {
    std::wstring match_text;
    std::pair<int, int> match_range;
};

struct PdfSpecificsInfo {
    fz_buffer* buffer;
    fz_output* output;
    pdf_document* pdf_doc;
};

struct SelectedDrawings {
    int page;
    AbsoluteRect selection_absrect_;
    std::vector<SelectedObjectIndex> selected_indices;
};

class CharacterIterator {
    fz_stext_block* block = nullptr;
    fz_stext_line* line = nullptr;
    fz_stext_char* chr = nullptr;
    bool is_line_only = false;

public:
    CharacterIterator(fz_stext_page* page, bool line_only=false);
    CharacterIterator(fz_stext_block* b, fz_stext_line* l, fz_stext_char* c, bool line_only=false);
    CharacterIterator& operator++();
    CharacterIterator operator++(int);
    bool operator==(const CharacterIterator& other) const;
    bool operator!=(const CharacterIterator& other) const;
    std::tuple<fz_stext_block*, fz_stext_line*, fz_stext_char*> operator*() const;

    using difference_type = long;
    using value_type = std::tuple<fz_stext_block*, fz_stext_line*, fz_stext_char*>;
    using pointer = const std::tuple<fz_stext_block*, fz_stext_line*, fz_stext_char*>*;
    using reference = const std::tuple<fz_stext_block*, fz_stext_line*, fz_stext_char*>&;
    using iterator_category = std::forward_iterator_tag;
};

class PageIterator {
    fz_stext_page* page;
    bool is_line_only = false;
public:
    PageIterator(fz_stext_page* page, bool line_only=false);
    CharacterIterator begin() const;
    CharacterIterator end() const;
};

struct CachedPageIndex {
    std::wstring text;
    std::vector<PagelessDocumentRect> rects;
};


struct PageFreehandDrawing{
    Q_GADGET
    Q_PROPERTY(QList<FreehandDrawing> drawings READ get_drawings)
public:
    QList<FreehandDrawing> drawings;
    QDateTime last_addition_time;
    QDateTime last_deletion_time;

    QList<FreehandDrawing> get_drawings();
};

class Document : public QObject{
    Q_OBJECT
    Q_PROPERTY(QList<QVariant> bookmarks READ get_bookmarks_qlist)
    Q_PROPERTY(QList<QVariant> highlights READ get_highlights_qlist)

private:

    bool is_opened = false;
    bool are_drawings_loaded = false;
    std::mutex drawings_mutex;
    std::mutex highlights_mutex;

    // it means we have modified freehand drawings since the document was loaded
    // which means that when we exit, we must write the modified drawings to the drawings file
    bool is_drawings_dirty = false;
    bool is_annotations_dirty = false;
    bool is_loading_annotations = false;

    std::vector<Mark> marks;
    std::vector<BookMark> bookmarks;
    std::vector<Highlight> highlights;
    std::vector<Portal> portals;
    std::unordered_map<int, std::vector<int>> page_highlight_indices;
    std::unordered_map<int, std::vector<int>> page_bookmark_indices;
    std::unordered_map<int, std::vector<int>> page_portal_indices;

    DatabaseManager* db_manager = nullptr;
    std::vector<TocNode*> top_level_toc_nodes;
    //bool only_for_portal = true;

    // automatically generated table of contents entries
    std::vector<TocNode*> created_top_level_toc_nodes;
    // flattened table of contents entries when we don't want to (or can't)
    // show a tree view (e.g. due to performance reasons on PC and lack of availablity on mobile)
    std::vector<std::wstring> flat_toc_names;
    //std::vector<int> flat_toc_pages;
    std::vector<DocumentPos> flat_toc_position;
    std::optional<QDateTime> drawings_last_server_mofication_time = {};

    std::map<int, PageMergedLinesInfoAbsolute> cached_page_line_info;

    std::deque<std::pair<int, CachedPageIndex>> cached_page_index;

    bool super_fast_search_index_ready = false;
    bool super_fast_search_was_ready = false;

    // super fast index is the concatenated text of all pages along with two lists which map the
    // characters to pages and rects of those characters this index is built only if the 
    // super_fast_search config option is enabled
    std::wstring super_fast_search_index;
    std::vector<int> super_fast_page_begin_indices;

    QJsonObject extras;

    // DEPRECATED a page offset which could manually be set to make the page numbers correct
    // on PDF files with page numbers that start at a number other than 1. This is now
    // unnecessary because mupdf 1.22 allows us to get actual page labels
    int page_offset = 0;

    // number of pages in the document
    std::optional<int> cached_num_pages = {};

    std::vector<std::pair<int, fz_stext_page*>> cached_stext_pages;
    std::vector<std::pair<int, fz_pixmap*>> cached_small_pixmaps;
    std::map<int, std::optional<std::string>> cached_fastread_highlights;
    PdfLink merge_links(const std::vector<PdfLink>& links_to_merge);

    fz_context* context = nullptr;
    std::wstring file_name;
    std::unordered_map<int, fz_link*> cached_page_links;
    std::unordered_map<int, std::vector<PdfLink>> cached_merged_pdf_links;
    std::unordered_map<int, std::vector<PagelessDocumentRect>> cached_flat_words;
    std::unordered_map<int, std::vector<std::vector<PagelessDocumentRect>>> cached_flat_word_chars;
    QStandardItemModel* cached_toc_model = nullptr;

    // accumulated page heights (i.e. the height of the page plus the height of all the pages before it)
    std::vector<float> accum_page_heights;
    std::vector<float> page_heights;
    std::vector<float> page_widths;

    // label of the pages, e.g. "i", "ii", "iii", "1", "2", "3", etc.
    std::vector<std::wstring> page_labels;
    mutable std::mutex page_dims_mutex;
    std::string correct_password = "";
    bool password_was_correct = false;
    bool document_needs_password = false;

    // These are a heuristic index of all figures and references in the document
    // The reason that we use a hashmap for reference_indices and a vector for figures is that
    // the reference we are looking for is usually the last reference with that name, but this is not
    // necessarily true for the figures.
    std::vector<IndexedData> generic_indices;
    std::map<std::wstring, IndexedData> reference_indices;
    std::map<std::wstring, std::vector<IndexedData>> equation_indices;

    std::mutex document_indexing_mutex;
    std::optional<std::thread> document_indexing_thread = {};
    std::optional<std::thread> background_page_dimensions_thread = {};
    bool is_document_indexing_required = true;
    bool is_indexing = false;
    float indexing_progress = 0.0f;
    bool are_highlights_loaded = false;
    bool should_render_annotations = true;
    bool should_reload_annotations = false;
    std::string dl_checksum = "";

    QDateTime last_update_time;
    CachedChecksummer* checksummer;

    // we do some of the document processing in a background thread (for example indexing all the
    // figures/indices and computing page heights. we use this pointer to notify the main thread when
    // processing is complete.
    //bool* invalid_flag_pointer = nullptr;

    bool is_document_validation_data_changed = false;

    int get_mark_index(char symbol);
    //std::optional<std::vector<RegexMatchInfo>> get_cached_regex_info(int page, std::wstring regex_string);
    //void add_cached_regex_info(int page, std::wstring regex_string, std::vector<RegexMatchInfo> infos);
    fz_outline* get_toc_outline();


    // load marks, bookmarks, links, etc.

    // convetr the fz_outline structure to our own TocNode structure
    void create_toc_tree(std::vector<TocNode*>& toc);

    Document(fz_context* context, std::wstring file_name, DatabaseManager* db_manager, CachedChecksummer* checksummer, std::string downloaded_checksum="");
    void clear_toc_nodes();
    void clear_toc_node(TocNode* node);
    int find_highlight_index_with_uuid(const std::string& uuid);

    template<typename T>
    void rebuild_page_annot_indices() {

        const std::vector<T>& annots = get_annots<T>();
        std::unordered_map<int, std::vector<int>>& annot_page_indices = get_annot_page_indices<T>();

        if ((annots.size() > 0) && (annot_page_indices.size() == 0)) {
            for (int i = 0; i < annots.size(); i++) {

                std::optional<AbsoluteRect> rect = annots[i].get_rectangle();
                if (!rect.has_value()) continue;

                int begin_page = rect->top_left().to_document(this).page;
                int end_page = rect->bottom_right().to_document(this).page;
                if (begin_page > end_page) {
                    std::swap(begin_page, end_page);
                }
                for (int p = begin_page; p <= end_page; p++) {
                    annot_page_indices[p].push_back(i);
                }
            }
        }
    }

public:
    QMap<int, PageFreehandDrawing> page_freehand_drawings;

    fz_document* doc = nullptr;
    std::wstring detected_paper_name = L"";
    bool annotations_are_freshly_loaded = false;
    bool checksum_is_new = false;
    std::optional<bool> cached_is_synced;


    PageIterator page_iterator(int page_number, bool line_only=false);
    int get_page_text_and_line_rects_after_rect(int page_number, int maximum_size,
        AbsoluteRect after,
        std::wstring& text,
        std::vector<PagelessDocumentRect>& line_rects,
        std::vector<PagelessDocumentRect>& char_rects);

    void load_document_metadata_from_db();
    std::string add_bookmark(const std::wstring& desc, float y_offset);
    std::string add_marked_bookmark(const std::wstring& desc, AbsoluteDocumentPos pos);
    std::string add_incomplete_bookmark(BookMark incomplete_bookmark);
    std::string add_pending_bookmark(const std::string& uuid, const std::wstring& desc);
    void undo_pending_bookmark(const std::string& uuid);
    void add_freetext_bookmark(const std::wstring& desc, AbsoluteRect absrect);
    void add_freetext_bookmark_with_color(const std::wstring& desc, AbsoluteRect absrect, float* color, float font_size = -1);
    std::string add_highlight_with_existing_uuid(const Highlight& highlight);
    std::string add_bookmark_with_existing_uuid(const BookMark& bookmark);
    std::string add_portal_with_existing_uuid(const Portal& portal);
    std::string add_highlight(const std::wstring& desc, const std::vector<AbsoluteRect>& highlight_rects, AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);
    std::string add_highlight(const std::wstring& annot, AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);
    std::optional<Highlight> delete_highlight_with_index(int index);
    std::optional<Highlight> delete_highlight_with_uuid(const std::string& uuid, bool delete_only_if_synced=false);
    std::optional<BookMark> delete_bookmark_with_index(int index);
    std::optional<Portal> delete_portal_with_index(int index);
    std::optional<BookMark> delete_bookmark_with_uuid(const std::string& uuid, bool delete_only_if_synced=false);
    std::optional<Highlight> delete_highlight(Highlight hl);
    std::string get_bookmark_uuid_at_pos(AbsoluteDocumentPos abspos);
    std::optional<BookMark> get_bookmark_at_pos(AbsoluteDocumentPos abspos);
    std::string get_pinned_portal_uuid_at_pos(AbsoluteDocumentPos abspos);
    std::string get_icon_portal_uuid_at_pos(AbsoluteDocumentPos abspos);
    bool should_render_pdf_annotations();
    void set_should_render_pdf_annotations(bool val);
    bool get_should_render_pdf_annotations();
    std::vector<Portal> get_intersecting_visible_portals(float absrange_begin, float absrange_end);
    CachedPageIndex& get_page_index(int page);
    void fill_search_result(SearchResult* result);
    QString get_all_document_text();
    QString get_page_text(int page);
    QString get_rest_of_document_pages_text(int from);
    int get_page_offset_into_super_fast_index(int from);
    int get_page_from_character_offset(int offset);
    const std::wstring& get_super_fast_index();
    std::wstring get_super_fast_index_lower();
    const std::vector<int>& get_super_fast_page_begin_indices();
    int get_first_page_end_index();

    PdfSpecificsInfo open_pdf_document_for_current_doc();
    void finalize_pdf_document_for_current_file(PdfSpecificsInfo info);

    void update_last_local_edit_time();
    std::optional<QDateTime> last_server_update_time();

    bool get_is_synced();
    void set_is_synced(bool synced);
    bool get_drawings_are_dirty();

    std::vector<PdfLink> find_references(std::string uri);
    std::vector<PdfLink> find_references_to_range(float begin_y, float end_y);

    std::vector<fz_stext_char*> get_flat_chars_around_pos(DocumentPos docpos, int count=-1);
    void fill_highlight_rects(fz_context* ctx, fz_document* doc);
    void fill_index_highlight_rects(int highlight_index, fz_context* thread_context = nullptr, fz_document* thread_document = nullptr);
    void count_chapter_pages(std::vector<int>& page_counts);
    void convert_toc_tree(fz_outline* root, std::vector<TocNode*>& output);
    void count_chapter_pages_accum(std::vector<int>& page_counts);
    bool get_is_indexing();
    fz_stext_page* get_stext_with_page_number(fz_context* ctx, int page_number, fz_document* doc = nullptr);
    fz_stext_page* get_stext_with_page_number(int page_number);
    std::string add_portal(Portal link, bool insert_into_database = true);
    std::wstring get_path();
    std::wstring get_path_platform();
    std::string get_checksum();
    std::optional<std::string> get_checksum_fast();
    //int find_closest_bookmark_index(float to_offset_y);

    int find_closest_bookmark_index(const std::vector<BookMark>& sorted_bookmarks, float to_offset_y) const;
    int find_closest_portal_index(const std::vector<Portal>& sorted_bookmarks, float to_offset_y) const;
    int find_closest_highlight_index(const std::vector<Highlight>& sorted_highlights, float to_offset_y) const;

    std::wstring_view get_page_range_text(int begin_page, int end_page);

    int get_page_intersecting_rect_index(DocumentRect r);
    std::optional<AbsoluteRect> get_page_intersecting_rect(DocumentRect rect);
    bool get_is_opened();

    void update_annotation_with_server_annotation(const Annotation* server_annotation);

    std::optional<Portal> find_closest_portal(float to_offset_y, int* index = nullptr);
    bool update_portal(Portal new_link);
    std::optional<BookMark> delete_closest_bookmark(float to_y_offset);
    std::optional<BookMark> delete_bookmark(int index);
    std::optional<Portal> delete_closest_portal(float to_offset_y);
    int get_portal_index_with_uuid(const std::string& uuid);
    std::optional<Portal> delete_portal_with_uuid(const std::string& uuid, bool delete_only_if_synced=false);
    const std::vector<BookMark>& get_bookmarks() const;
    const std::vector<Portal>& get_portals() const;
    std::vector<BookMark> get_sorted_bookmarks() const;
    std::vector<Portal> get_sorted_portals() const;
    const std::vector<Highlight>& get_highlights() const;
    int get_highlight_index_with_uuid(std::string uuid);
    //int get_annot_index_with_uuid(const std::string& annot_type, const std::string& uuid);
    int get_bookmark_index_with_uuid(std::string uuid);
    const std::vector<Highlight> get_highlights_of_type(char type) const;
    const std::vector<Highlight> get_highlights_sorted(char type = 0) const;

    Annotation* get_annot_with_uuid(const std::string& annot_type, const std::string& uuid);

    std::optional<Highlight> get_next_highlight(float abs_y, char type = 0, int offset = 0) const;
    std::optional<Highlight> get_prev_highlight(float abs_y, char type = 0, int offset = 0) const;

    fz_link* get_page_links(int page_number);
    const std::vector<PdfLink>& get_page_merged_pdf_links(int page_number);
    PdfLink pdf_link_from_fz_link(int page, fz_link* link);
    std::string add_mark(char symbol, float y_offset, std::optional<float> x_offset, std::optional<float> zoom_level);
    bool remove_mark(char symbol);
    std::optional<Mark> get_mark_if_exists(char symbol);
    ~Document();
    const std::vector<TocNode*>& get_toc();
    bool has_toc();
    const std::vector<std::wstring>& get_flat_toc_names();
    const std::vector<DocumentPos>& get_flat_toc_pages();
    bool open(bool force_load_dimensions = false, std::string password = "", bool temp = false);
    void reload(std::string password = "");
    QDateTime get_last_edit_time();
    unsigned int get_milies_since_last_document_update_time();
    unsigned int get_milies_since_last_edit_time();
    float get_page_height(int page_index) const;
    fz_pixmap* get_small_pixmap(int page);
    float get_page_width(int page_index);
    float get_page_width_median();
    std::wstring get_page_label(int page_index);
    int get_page_number_with_label(std::wstring page_label);
    bool is_reflowable();
    // float get_page_width_smart(int page_index, float* left_ratio, float* right_ratio, int* normal_page_width);
    float get_page_size_smart(bool width, int page_index, float* left_ratio, float* right_ratio, int* normal_page_width);
    float get_accum_page_height(int page_index);
    void rotate();
    void get_visible_pages(float doc_y_range_begin, float doc_y_range_end, std::vector<int>& visible_pages);
    void load_page_dimensions(bool force_load_now);
    int num_pages();
    AbsoluteRect get_page_absolute_rect(int page) const ;
    DocumentPos absolute_to_page_pos(AbsoluteDocumentPos absolute_pos) const ;
    DocumentPos absolute_to_page_pos_uncentered(AbsoluteDocumentPos absolute_pos) const;
    DocumentRect absolute_to_page_rect(AbsoluteRect abs_rect) const;
    QStandardItemModel* get_toc_model();
    int get_offset_page_number(float y_offset);
    void index_document();
    void stop_indexing();
    void delete_page_intersecting_drawings(int page, AbsoluteRect absolute_rect, bool mask[26]);
    void delete_drawings_with_indices(int page, std::vector<int>& indices);
    void delete_drawings_with_indices(int page, std::vector<SelectedObjectIndex>& indices);
    void delete_drawings_with_network_request_id(int page, int request_id);
    void delete_all_page_drawings(int page);
    void delete_all_drawings();
    std::vector<SelectedObjectIndex> get_page_intersecting_drawing_indices(int page, AbsoluteRect absolute_rect, bool mask[26]);
    std::optional<QDateTime> get_local_drawings_modification_time();
    void set_server_drawings_modification_time(QDateTime datetime);
    std::optional<QDateTime> get_server_drawings_modification_time();

    std::vector<IndexedData> find_reference_with_string(std::wstring reference_name, int page_number);
    std::vector<IndexedData> find_equation_with_string(std::wstring equation_name, int page_number);
    std::vector<IndexedData> find_generic_with_string(std::wstring equation_name, int page_number);

    std::optional<std::wstring> get_text_at_position(const std::vector<fz_stext_char*>& flat_chars, DocumentPos position);
    std::optional<std::wstring> get_reference_text_at_position(const std::vector<fz_stext_char*>& flat_chars, DocumentPos position, std::pair<int, int>* out_range);
    std::optional<PaperNameWithRects> get_paper_name_at_position(const std::vector<fz_stext_char*>& flat_chars, DocumentPos position);
    std::optional<PaperNameWithRects> get_paper_name_at_position(DocumentPos position);
    std::optional<std::wstring> get_equation_text_at_position(const std::vector<fz_stext_char*>& flat_chars, DocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::pair<std::wstring, std::wstring>> get_generic_link_name_at_position(const std::vector<fz_stext_char*>& flat_chars, DocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::wstring> get_text_at_position(DocumentPos position);
    std::optional<std::wstring> get_reference_text_at_position(DocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::wstring> get_equation_text_at_position(DocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::pair<std::wstring, std::wstring>> get_generic_link_name_at_position(DocumentPos position, std::pair<int, int>* out_range);
    std::optional<std::wstring> get_regex_match_at_position(const std::wstring& regex, const std::vector<fz_stext_char*>& flat_chars, DocumentPos position, int max_match_size, std::pair<int, int>* out_range);
    std::optional<std::wstring> get_regex_match_at_position(const std::wstring& regex, DocumentPos position, int max_match_size, std::pair<int, int>* out_range);
    std::vector<DocumentPos> find_generic_locations(const std::wstring& type, const std::wstring& name);
    bool can_use_highlights();
    QList<FreehandDrawing> zoom_selected_freehand_drawings(float zoom_factor, std::optional<SelectedDrawings> selected_drawings);

    bool load_extras();
    bool persist_extras();
    std::optional<QVariant> get_extra(QString name);

    template<typename T>
    void set_extra(QString name, T value) {
        extras[name] = value;
    }

    std::vector<QString> get_page_bib_candidates_old(int page_number, std::vector<std::vector<PagelessDocumentRect>>* out_rects = nullptr);
    std::vector<QString> get_page_bib_candidates(int page_number, std::vector<std::vector<PagelessDocumentRect>>* out_end_rects = nullptr);
    std::optional<std::pair<QString, std::vector<PagelessDocumentRect>>> get_page_bib_with_reference(int page_number, std::wstring reference_text);

    void get_line_selection(AbsoluteDocumentPos selection_begin,
        AbsoluteDocumentPos selection_end,
        std::deque<AbsoluteRect>& selected_characters,
        std::wstring& selected_text);

    void get_text_selection(AbsoluteDocumentPos selection_begin,
        AbsoluteDocumentPos selection_end,
        bool is_word_selection, // when in word select mode, we select entire words even if the range only partially includes the word
        std::deque<AbsoluteRect>& selected_characters,
        std::wstring& selected_text);
    void get_text_selection(fz_context* ctx, AbsoluteDocumentPos selection_begin,
        AbsoluteDocumentPos selection_end,
        bool is_word_selection,
        std::deque<AbsoluteRect>& selected_characters,
        std::wstring& selected_text,
        fz_document* doc = nullptr);

    std::wstring get_raw_text_selection(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end);

    bool is_bookmark_new(const BookMark& bookmark);
    bool is_highlight_new(const Highlight& highlight);
    bool is_drawing_new(const FreehandDrawing& drawing);
    int num_freehand_drawings();
    std::vector<BookMark> get_new_sioyek_bookmarks(const std::vector<BookMark>& pdf_bookmarks);
    std::vector<Highlight> get_new_sioyek_highlights(const std::vector<Highlight>& pdf_highlights);
    fz_context* get_mupdf_context();

    int get_page_offset();
    void set_page_offset(int new_offset);
    std::pair<pdf_page*, pdf_annot*> embed_highlight(pdf_document* pdf_doc, fz_page* page, const Highlight& hl);
    void embed_single_annot(const std::string& uuid);
    void delete_pdf_annotations();
    void delete_intersecting_annotations(AbsoluteRect rect);
    std::pair<pdf_page*, pdf_annot*> embed_bookmark(pdf_document* pdf_doc, fz_page* page, const BookMark& bm);
    void embed_annotations(std::wstring new_file_path);
    void get_pdf_annotations(std::vector<BookMark>& pdf_bookmarks, std::vector<Highlight>& pdf_highlights, std::vector<FreehandDrawing>& pdf_drawings);
    void import_annotations();
    std::vector<PagelessDocumentRect> get_page_flat_words(int page);
    std::vector<std::vector<PagelessDocumentRect>> get_page_flat_word_chars(int page);
    void clear_document_caches();
    void load_document_caches(bool force_now);
    int reflow(int page);
    void update_highlight_add_text_annotation(const std::string& uuid, const std::wstring& text_annot);
    void update_highlight_type(const std::string& uuid, char new_type);
    //void update_highlight_type(const std::string& uuid, char new_type);
    BookMark* update_bookmark_text(const std::string& uuid, const std::wstring& new_text, float new_font_size);
    void update_bookmark_position(const std::string& uuid, AbsoluteDocumentPos new_begin_position, AbsoluteDocumentPos new_end_position);
    void update_portal_src_position(const std::string& uuid, AbsoluteDocumentPos new_position, std::optional<AbsoluteDocumentPos> new_end_position);

    bool needs_password();
    bool needs_authentication();
    bool apply_password(const char* password);
    //std::optional<std::string> get_page_fastread_highlights(int page);
    std::vector<PagelessDocumentRect> get_highlighted_character_masks(int page);
    PagelessDocumentRect get_page_rect_no_cache(int page);
    std::optional<PdfLink> get_link_in_pos(int page, float x, float y);
    std::optional<PdfLink> get_link_in_pos(const DocumentPos& pos);
    std::vector<PdfLink> get_links_in_page_rect(int page, AbsoluteRect rect);
    PdfLinkTextInfo get_pdf_link_text(PdfLink link);
    std::string get_highlight_index_uuid(int index);
    std::string get_bookmark_index_uuid(int index);

    std::optional<BookMark> get_bookmark_with_index(int index);
    std::optional<Portal> get_portal_with_index(int index);

    Portal* get_portal_with_uuid(const std::string& uuid);
    BookMark* get_bookmark_with_uuid(const std::string& uuid);
    BookMark* get_bookmark_pointer_with_index(int index);
    Portal* get_portal_pointer_with_index(int index);
    Highlight* get_highlight_with_uuid(const std::string& uuid);

    //void create_table_of_contents(std::vector<TocNode*>& top_nodes);
    int add_stext_page_to_created_toc(fz_stext_page* stext_page,
        int page_number,
        std::vector<TocNode*>& toc_node_stack,
        std::vector<TocNode*>& top_level_node);

    float document_to_absolute_y(int page, float doc_y) const;
    //AbsoluteDocumentPos document_to_absolute_pos(DocumentPos, bool center_mid = false);
    AbsoluteDocumentPos document_to_absolute_pos(DocumentPos docpos) const;

    AbsoluteRect document_to_absolute_rect(DocumentRect doc_rect) const;

    //void get_ith_next_line_from_absolute_y(float absolute_y, int i, bool cont, float* out_begin, float* out_end);
    AbsoluteRect get_ith_next_line_from_absolute_y(int page, int line_index, int i, bool continue_to_next_page, int* out_index, int* out_page);
    const PageMergedLinesInfoAbsolute& get_page_lines(int page);

    std::wstring get_addtional_sioyek_file_path(QString type);
    std::wstring get_drawings_file_path();
    std::wstring get_old_drawings_file_path();
    std::wstring get_scratchpad_file_path();
    std::wstring get_extras_file_path();
    std::wstring get_annotations_file_path();
    bool annotations_file_exists();
    bool annotations_file_is_newer_than_database();
    std::optional<AbsoluteRect> get_rect_vertically(bool below, AbsoluteRect rect);

    Portal* get_portal_under_absolute_pos(AbsoluteDocumentPos abspos);
    void persist_drawings(bool force = false);
    void persist_drawings_binary(bool force = false);

    void persist_annotations(bool force = false);
    void load_drawings();
    void load_drawings_binary();
    QMap<int, PageFreehandDrawing> load_drawings_from_data(QByteArray& data, bool &is_dirty);
    QMap<int, PageFreehandDrawing> load_drawings_from_io_device(QIODevice& data, bool &is_dirty) const;
    void load_annotations(bool sync = false);
    void load_drawings_async();
    void lock_highlights_mutex();
    void unlock_highlights_mutex();
    void merge_with_server_drawings(const QMap<int, PageFreehandDrawing>& server_drawings, bool& has_local_drawings_not_on_server);
    //void persist_drawings_async();

    std::wstring detect_paper_name(fz_context* context, fz_document* doc);
    std::wstring detect_paper_name();
    //void set_only_for_portal(bool val);
    //bool get_only_for_portal();

    bool is_super_fast_index_ready();
    std::vector<SearchResult> search_text(std::wstring query, SearchCaseSensitivity case_sensitive, int begin_page, int min_page, int max_page);
    std::vector<SearchResult> search_regex(std::wstring query, SearchCaseSensitivity case_sensitive, int begin_page, int min_page, int max_page);
    std::optional<std::pair<AbsoluteDocumentPos, AbsoluteDocumentPos>> fuzzy_page_select_text(int page, std::wstring selected_text);
    float max_y_offset();
    void add_freehand_drawing(FreehandDrawing new_drawing);
    void get_page_freehand_drawings_with_indices(int page, const std::vector<SelectedObjectIndex>& indices, QList<FreehandDrawing>& freehand_drawings, std::vector<PixmapDrawing>& pixmap_drawings);
    void undo_freehand_drawing();
    const PageFreehandDrawing& get_page_drawings(int page);
    PageFreehandDrawing& get_page_drawings_mut(int page);
    Q_INVOKABLE QVariantMap get_drawings();
    AbsoluteRect to_absolute(int page, fz_quad quad);
    AbsoluteRect to_absolute(int page, PagelessDocumentRect rect);

    int get_first_line_index_after_block(int page, int after_index);
    int get_last_line_of_block(int page, int after_index);
    int get_first_line_before_block(int page, int before_index);

    void validate();
    bool get_valid();

    bool get_should_reload_annotations();
    void reload_annotations_on_new_checksum();
    int find_reference_page_with_reference_text(std::wstring query);
    std::optional<DocumentPos> find_abbreviation(std::wstring abbr, std::vector<DocumentRect>& overview_highlight_rects);
    std::vector<std::wstring> get_fulltext_tags();
    void delete_fulltext_tag(std::wstring tag);

    std::pair<float, float> get_min_max_annot_x_for_page(int page);

    int get_page_merged_line_index_from_unmerged_index(int page, int unmerged_index);
    bool super_fast_search_index_is_new();
    float get_indexing_progress();

    std::vector<DocumentRect> get_rects_for_highlight_indices(const std::vector<std::string>& indices);
    std::vector <DocumentRect> get_rects_for_bookmark_indices(const std::vector<std::string>& indices);
    std::vector <DocumentRect> get_rects_for_portal_indices(const std::vector<std::string>& indices);

    QJsonArray get_bookmarks_json();
    QJsonArray get_highlights_json();
    QJsonArray get_portals_json();
    QJsonArray get_marks_json();

    std::wstring get_detected_paper_name_if_exists();
    int absolute_to_page_index(int absolute_index, int& page);

    template <typename T>
    const std::vector<T>& get_annots() const = delete;

    template <typename T>
    const QList<QVariant> get_annotation_qlist() const {
        QList<QVariant> result;
        const std::vector<T>& annots = get_annots<T>();
        for (auto& annot : annots){
            result.push_back(QVariant::fromValue(annot));
        }
        return result;
    };

    QList<QVariant> get_bookmarks_qlist();
    QList<QVariant> get_highlights_qlist();

    template <typename T>
    std::unordered_map<int, std::vector<int>>& get_annot_page_indices() = delete;


    template <typename T>
    std::vector<T>& get_annots_mut() = delete;

    template <typename T>
    std::vector<T> get_unsynced_annots() {
        auto all_annots = get_annots<T>();
        std::vector<T> res;
        for (auto& annot : all_annots) {
            if (!annot.is_synced) {
                res.push_back(annot);
            }
        }
        return res;
    }

    void set_annots_to_synced(const std::string& table_name,  const std::vector<std::string>& uuids);

    template<typename T>
    void set_annots_to_synced(std::vector<std::string> uuids) {
        std::sort(uuids.begin(), uuids.end());

        auto& annots = get_annots_mut<T>();
        std::vector<int> unsynced_indices;
        for (int i = 0; i < annots.size(); i++) {
            auto& annot = annots[i];
            if (!annot.is_synced) {
                unsynced_indices.push_back(i);
            }
        }
        for (auto index : unsynced_indices) {
            if (std::binary_search(uuids.begin(), uuids.end(), annots[index].uuid)) {
                annots[index].is_synced = true;
            }
        }
        set_annots_to_synced(T::TABLE_NAME, uuids);
        // this->db_manager->set_annot_uuids_to_synced_(T::TABLE_NAME, uuids);

    }


    void set_annots_to_synced_with_type(std::string annot_type, std::vector<std::string> uuids);
    ParsedUri parse_link(const PdfLink& link);

    Q_INVOKABLE QVariantMap get_annotations() const;
    Q_INVOKABLE void apply_annotation_changes(QVariant annot);
    Q_INVOKABLE void update_annotation_js(QJsonObject annot);
    Q_INVOKABLE QJsonObject get_annotation_js(QString uuid);

    template<typename T>
    std::vector<int> get_page_visible_annot_indices(int page) {

        rebuild_page_annot_indices<T>();
        std::unordered_map<int, std::vector<int>>& annot_page_indices = get_annot_page_indices<T>();

        if (annot_page_indices.find(page) == annot_page_indices.end()) {
            return {};
        }

        return annot_page_indices[page];
    }

    void debug();

    template<typename T>
    void add_annot_index_to_page(int page, int index) {
        std::unordered_map<int, std::vector<int>>& annot_page_indices = get_annot_page_indices<T>();

        if (annot_page_indices.find(page) == annot_page_indices.end()) {
            annot_page_indices[page] = { index };
        }
        else {
            auto it = std::find(annot_page_indices[page].begin(), annot_page_indices[page].end(), index);
            bool already_exists = it != annot_page_indices[page].end();
            if (!already_exists) {
                annot_page_indices[page].push_back(index);
            }
        }
    }

    void invalidate_page_visible_bookmarks();
    void invalidate_page_visible_highlights();
    void invalidate_page_visible_portals();

    void on_bookmark_added();
    void on_bookmark_deleted();

    void on_highlight_added();
    void on_highlight_deleted();

    void on_portal_added();
    void on_portal_deleted();

    void delete_page_drawings_with_network_request_id(int page, int request_id);
    void set_drawings_dirty(bool val);

    friend class DocumentManager;
};

class DocumentManager {
private:
    fz_context* mupdf_context = nullptr;
    DatabaseManager* db_manager = nullptr;
    CachedChecksummer* checksummer;
    std::shared_mutex cached_hash_mutex;
    std::unordered_map<std::wstring, Document*> cached_documents;
    std::unordered_map<std::string, std::wstring> hash_to_path;
public:
    std::vector<std::wstring> tabs;

    DocumentManager(fz_context* mupdf_context, DatabaseManager* db_manager, CachedChecksummer* checksummer);

    int get_tab_index(const std::wstring& path);
    int add_tab(const std::wstring& path);
    void remove_tab(const std::wstring& path);
    std::vector<std::wstring> get_tabs();

    Document* get_document(std::wstring path, std::string downloaded_checksum="");
    std::optional<std::wstring> get_path_from_hash(const std::string& checksum);

    Document* get_document_with_checksum(const std::string& checksum);
    std::optional<Document*> get_cached_document(const std::wstring& path);
    void free_document(Document* document);
    std::vector<std::wstring> get_loaded_document_paths();
    void delete_global_mark(char symbol);
    std::optional<std::pair<std::string, Highlight>> delete_highlight_with_uuid(const std::string& uuid);
    void update_checksum(const std::string& old_checksum, const std::string& new_checksum);
    std::vector<std::wstring> get_new_files_from_scan_directory();
    void scan_new_files_from_scan_directory();

    ~DocumentManager();
};


template <>
const std::vector<Highlight>& Document::get_annots<Highlight>() const;
template <>
const std::vector<BookMark>& Document::get_annots<BookMark>() const;
template <>
const std::vector<Portal>& Document::get_annots<Portal>() const;
template <>
std::vector<Highlight>& Document::get_annots_mut<Highlight>();
template <>
std::vector<BookMark>& Document::get_annots_mut<BookMark>();
template <>
std::vector<Portal>& Document::get_annots_mut<Portal>();


template <>
std::unordered_map<int, std::vector<int>>& Document::get_annot_page_indices<Highlight>();

template <>
std::unordered_map<int, std::vector<int>>& Document::get_annot_page_indices<BookMark>();

template <>
std::unordered_map<int, std::vector<int>>& Document::get_annot_page_indices<Portal>();

void save_drawings_to_file(QIODevice& binary_file, const QMap<int, PageFreehandDrawing>& drawings);
