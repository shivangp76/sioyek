#pragma once

#include <vector>
#include <string>
#include <assert.h>
#include <utility>
#include <algorithm>
#include <thread>
#include <optional>
#include <deque>
#include <mutex>

#include <mupdf/fitz.h>
#include "sqlite3.h"

#include "coordinates.h"
#include "book.h"
#include "document.h"

extern float ZOOM_INC_FACTOR;
extern float SCROLL_ZOOM_INC_FACTOR;

class CachedChecksummer;
class Document;
class DatabaseManager;
class DocumentManager;
class ConfigManager;
class PdfRenderer;
class ScratchPad;


struct MarkedDataRect {
    DocumentRect rect;
    int type;
};

enum class SelectionMode {
    Character,
    Word,
    Line
};


struct LineSelectBeginData {
    AbsoluteDocumentPos pos;
    AbsoluteDocumentPos end_pos;
    RulerLineIndexInfo index_info;
};

struct ScheduledPortalUpdate {
    Portal portal;
    OpenedBookState state;
};

struct SelectedDrawings {
    int page;
    AbsoluteRect selection_absrect_;
    std::vector<SelectedObjectIndex> selected_indices;
};

struct FreehandDrawingMoveData {
    std::vector<FreehandDrawing> initial_drawings;
    std::vector<PixmapDrawing> initial_pixmaps;
    AbsoluteDocumentPos initial_mouse_position;
};

struct VisibleObjectMoveData {
    VisibleObjectIndex index;
    AbsoluteDocumentPos initial_position;
    AbsoluteDocumentPos initial_mouse_position;
    bool is_moving = true;

    void handle_move(MainWidget* widget);
    void handle_move_end(MainWidget* widget);
};

struct PendingDownloadPortal {
    Portal pending_portal;
    std::wstring paper_name;
    std::wstring source_document_path;
    std::string handle;
    float downloaded_fraction = -1.0f;
    // the pending portal is marked for deletion
    bool marked = false;
};

class DocumentView {
protected:

    DatabaseManager* db_manager = nullptr;
    DocumentManager* document_manager = nullptr;
    CachedChecksummer* checksummer;
    Document* current_document = nullptr;

    float zoom_level = 0.0f;
    VirtualPos offset = {0, 0};
    std::vector<VirtualRect> cached_virtual_rects;
    float max_cached_y_offset = 0;
    bool two_page_mode = false;


    // absolute rect of the current ruler if this is {} then ruler_pos is used instead
    std::optional<AbsoluteRect> ruler_rect;
    float ruler_pos = 0;

    // index of the current highlighted line in ruler mode
    std::optional<RulerLineIndexInfo> ruler_line_index = {};

    int view_width = 0;
    int view_height = 0;


    // In touch mode normally we don't allow the user to scroll the document horizontally past the 
    // screen edges, however after the user tries to move the document past a certain distance, we
    // "relent" and allow the user to scroll horizontally past the screen edges.
    bool is_relenting = false;

    // in auto resize mode, we automatically set the zoom level to fit the page when resizing the document
    bool is_auto_resize_mode = true;
    bool is_ruler_mode_ = false;
    std::optional<int> presentation_page_number;


    float page_space_x = 0;
    float page_space_y = 0;

public:
    std::vector<std::vector<AbsoluteRect>>  debug_highlight_rects;

    float velocity_x = 0;
    float velocity_y = 0;
    bool is_velocity_fixed = false;

    // when we want to focus on an offset within the document but the super fast search index is not ready
    std::optional<int> on_super_fast_compute_focus_offset = {};

    std::vector<SearchResult> search_results;
    int current_search_result_index = -1;
    std::mutex search_results_mutex;
    bool is_search_cancelled = true;
    bool is_searching = false;
    float percent_done = 0.0f;

    std::optional<OverviewState> overview_page = {};
    float overview_half_width = 0.8f;
    float overview_half_height = 0.4f;
    float overview_offset_x = 0.0f;
    float overview_offset_y = 0.0f;

    bool should_highlight_links = false;
    bool should_highlight_words = false;
    bool should_highlight_rect_mode = false;
    //bool should_show_numbers = false;
    bool should_show_rect_hints = false;

    //ColorPalette color_mode = ColorPalette::Normal;

    bool line_select_mode = false;
    std::optional<LineSelectBeginData> line_select_begin_data = {};

    // begin/end position of the current text selection
    AbsoluteDocumentPos selection_begin;
    AbsoluteDocumentPos selection_end;

    std::string tag_prefix = "";
    std::vector<std::string> highlighted_tags;

    std::vector<DocumentRect> word_rects;

    // std::optional<AbsoluteRect> pending_portal_rect = {};

    std::optional<AbsoluteRect> character_highlight_rect = {};
    std::optional<AbsoluteRect> wrong_character_rect = {};
    bool show_control_rect;

    // the index of highlight in doc()->get_highlights() that is selected. This is used to
    // delete/edit highlights e.g. by selecting a highlight by clicking on it and then executing `delete_highlight`
    std::optional<VisibleObjectIndex> selected_object_index = {};

    std::optional<VisibleObjectResizeData> visible_object_resize_data = {};
    std::optional<VisibleObjectScrollData> visible_object_scroll_data = {};

    std::optional<SelectedDrawings> selected_freehand_drawings = {};
    std::optional<FreehandDrawingMoveData> freehand_drawing_move_data = {};

    std::optional<VisibleObjectMoveData> visible_object_move_data = {};

    std::optional<ScheduledPortalUpdate> scheduled_portal_update = {};

    // last page to be fit when we are in smart fit mode
    // this value not being `{}` indicates that we are in smart fit mode
    // which means that every time page is changed, we execute `fit_to_page_width_smart`
    std::optional<int> last_smart_fit_page = {};

    std::unordered_map<std::string, float> bookmark_scroll_amounts;

    std::optional<AbsoluteDocumentPos> underline = {};
    // std::vector<DocumentRect> overview_highlights;

    // An incomplete portal that is being created. The source of the portal is filled
    // but the destination still needs to be set.
    std::optional<std::pair<std::optional<std::wstring>, Portal>> current_pending_portal;

    // std::vector<std::pair<AbsoluteRect, float>> pending_download_portals;
    std::vector<PendingDownloadPortal> pending_download_portals;
    std::optional<AbsoluteRect> selected_rectangle = {};

    // selected text (using mouse cursor or other methods) which is used e.g. for copying or highlighting
    std::wstring selected_text;

    // are we currently freehand drawing on the document
    bool is_drawing = false;

    // color type to use when freehand drawing
    char current_freehand_type = 'r';

    // alpha of freehand drawings
    float freehand_alpha = 1.0f;

    // line thickness of freehand drawings
    float freehand_thickness = 1.0f;

    // when moving the text selection using keyboard, `selection_begin` and `selection_end`
    // might be out of sync with `selected_text_`. `selected_text_is_dirty` is true when this
    // is the case, which means that we need to update `selected_text_` before using it.
    bool selected_text_is_dirty = false;

    // is the user currently selecing text? (happens when we left click and move the cursor)
    bool is_selecting = false;

    // when the user double clicks, we select words, when the user triple clicks we select lines
    SelectionMode selection_mode = SelectionMode::Character;


    // A list of candiadates to be shown in the overview window. We use simple heuristics to determine the
    // target of references, while this works most of the time, it is not perfect. So we keep a list of candidates
    // which the user can naviagte through using `next_preview` and `previous_preview` commands which move
    // `index_into_candidates` pointer to the next/previous candidate
    std::vector<SmartViewCandidate> smart_view_candidates;
    int index_into_candidates = 0;

    int rotation_index = 0;
    bool fastread_mode = false;

    ScratchPad* scratchpad = nullptr;

    //int selected_highlight_index = -1;
    //int selected_bookmark_index = -1;
    // std::optional<VisibleObjectIndex> selected_object_index = {};

    bool visible_drawing_mask[26];
    FreehandDrawing current_drawing;
    std::vector<FreehandDrawing> moving_drawings;
    std::vector<PixmapDrawing> moving_pixmaps;

    std::vector<DocumentRect> synctex_highlights;
    QTime synctex_highlight_time;
    std::vector<MarkedDataRect> marked_data_rects;

    // list of selected characters (e.g. using mouse select) to be highlighted
    std::deque<AbsoluteRect> selected_character_rects;
    // whether we should show a keyboard text selection marker at the end/begin of current
    // text selection (depending on `mark_end` value)
    bool should_show_text_selection_marker = false;
    bool mark_end = true;
    std::wstring last_opened_file_path = L"";
    bool was_set_to_null = false;

    DocumentView(DatabaseManager* db_manager, DocumentManager* document_manager, CachedChecksummer* checksummer);
    ~DocumentView();
    float get_zoom_level();
    DocumentViewState get_state();
    PortalViewState get_checksum_state();
    //void set_opened_book_state(const OpenedBookState& state);
    void handle_escape();
    void handle_validate_render(float status_label_height);
    void set_book_state(OpenedBookState state);
    virtual bool set_offsets(float new_offset_x, float new_offset_y, bool force = false, bool is_dragging=false);
    bool set_pos(AbsoluteDocumentPos pos);
    void set_virtual_pos(VirtualPos pos, bool is_dragging=false, bool force=false);
    Document* get_document();
    bool is_ruler_mode();
    void exit_ruler_mode();

    void toggle_highlight_links();
    void set_highlight_links(bool should_highlight);

    void toggle_highlight_words();
    void set_highlight_words(std::vector<DocumentRect> rects);
    void set_should_highlight_words(bool should_highlight, bool rect_mode=false);
    std::vector<DocumentRect> get_highlight_word_rects();
    void show_rect_hints();
    void hide_rect_hints();
    bool is_showing_rect_hints();
    bool on_vertical_scroll();
    bool is_pos_inside_selected_text(AbsoluteDocumentPos pos);
    bool is_pos_inside_selected_text(DocumentPos docpos);
    bool is_pos_inside_selected_text(WindowPos pos);
    //std::optional<QString> get_paper_name_from_reference_info(const TextUnderPointerInfo& info);
    //void fill_text_under_pointer_info_reference_highlight_rects(TextUnderPointerInfo& info);
    void fill_smart_view_candidate_reference_highlight_rects(SmartViewCandidate& candidate);
    std::vector<DocumentRect> get_paper_name_rects_from_page_and_source_text(int page, const std::wstring& source_text);
    std::optional<QString> get_paper_name_under_pos(DocumentPos docpos, bool clean=false);
    std::optional<PaperNameWithRects> get_direct_paper_name_under_pos(DocumentPos docpos);
    TextUnderPointerInfo find_location_of_text_under_pointer(DocumentPos docpos, int max_size=-1);
    int get_current_page_number();
    ReferenceType find_location_of_selected_text(int* out_page, float* out_offset, AbsoluteRect* out_rect, std::wstring* out_source_text, std::vector<DocumentRect>* out_highlight_rects = nullptr);


    void set_tag_prefix(std::wstring prefix);
    void clear_tag_prefix();
    void set_highlighted_tags(std::vector<std::string> tags);
    bool is_tag_highlighted(const std::string& tag);

    void rotate_clockwise();
    void rotate_counterclockwise();
    bool is_rotated();
    void toggle_fastread_mode();
    void set_typing_rect(DocumentRect highlight_rect, std::optional<DocumentRect> wrong_rect);
    void set_underline(AbsoluteDocumentPos abspos);
    void clear_underline();

    std::string get_selected_highlight_uuid();
    std::string get_selected_pinned_portal_uuid();
    std::string get_selected_bookmark_uuid();
    std::string get_selected_portal_uuid();

    // void set_selected_object_index(VisibleObjectIndex index);
    //void set_selected_bookmark_index(int index);

    // void set_overview_highlights(const std::vector<DocumentRect>& rects);

    void set_selected_rectangle(AbsoluteRect selected);
    void clear_selected_rectangle();
    std::optional<AbsoluteRect> get_selected_rectangle();
    // void set_pending_download_portals(std::vector<std::pair<AbsoluteRect, float>>&& portal_rects);

    // find the closest portal to the current position
    // if limit is true, we only search for portals near the current location and not all portals
    std::optional<Portal> find_closest_portal(bool limit = false);
    std::optional<BookMark> find_closest_bookmark();
    void goto_portal(Portal* link);
    std::optional<Portal> delete_closest_portal();
    std::optional<BookMark> delete_closest_bookmark();

    std::optional<BookMark> delete_closest_bookmark_to_offset(float offset);
    float get_offset_x();
    float get_offset_y();
    AbsoluteDocumentPos get_offsets();
    VirtualPos get_virtual_offset();
    int get_view_height();
    int get_view_width();
    void set_null_document();
    void set_offset_x(float new_offset_x);
    void set_offset_y(float new_offset_y);
    std::optional<PdfLink> get_link_in_pos(WindowPos pos);
    std::string get_highlight_uuid_in_pos(WindowPos pos);
    void get_text_selection(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, bool is_word_selection, std::deque<AbsoluteRect>& selected_characters, std::wstring& text_selection);
    std::string add_mark(char symbol);
    void get_line_selection(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, std::deque<AbsoluteRect>& selected_characters, std::wstring& text_selection);
    std::string add_bookmark(std::wstring desc);
    std::string add_highlight_(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);
    void on_view_size_change(int new_width, int new_height);
    //void absolute_to_window_pos(float absolute_x, float absolute_y, float* window_x, float* window_y);
    NormalizedWindowPos absolute_to_window_pos(AbsoluteDocumentPos absolute_pos);

    // void set_pending_portal_position(std::optional<AbsoluteRect> rect);
    void set_synctex_highlights(std::vector<DocumentRect> highlights);

    bool should_show_synxtex_highlights();
    bool has_synctex_timed_out();

    ColorPalette get_current_color_mode();

    void swap_line_select_cursor();
    void fill_cached_virtual_rects(bool force=false);
    NormalizedWindowRect absolute_to_window_rect(AbsoluteRect doc_rect);
    NormalizedWindowPos document_to_window_pos(DocumentPos pos);
    WindowPos absolute_to_window_pos_in_pixels(AbsoluteDocumentPos abs_pos);
    WindowPos document_to_window_pos_in_pixels_uncentered(DocumentPos doc_pos);
    WindowPos document_to_window_pos_in_pixels_banded(DocumentPos doc_pos);
    NormalizedWindowRect document_to_window_rect(DocumentRect doc_rect);
    WindowRect document_to_window_irect(DocumentRect);
    NormalizedWindowRect document_to_window_rect_pixel_perfect(DocumentRect doc_rect, int pixel_width, int pixel_height, bool banded = false);
    DocumentPos window_to_document_pos(WindowPos window_pos);
    //DocumentPos window_to_document_pos_uncentered(WindowPos window_pos);
    AbsoluteDocumentPos window_to_absolute_document_pos(WindowPos window_pos);
    NormalizedWindowPos window_to_normalized_window_pos(WindowPos window_pos);
    WindowPos normalized_window_to_window_pos(NormalizedWindowPos normalized_window_pos);
    WindowRect normalized_to_window_rect(NormalizedWindowRect normalized_rect);
    QRect normalized_to_window_qrect(NormalizedWindowRect normalized_rect);
    void goto_mark(char symbol);
    void goto_end();

    void goto_left();
    void goto_left_smart();

    void goto_right();
    void goto_right_smart();

    float get_max_valid_x(bool relenting);
    float get_min_valid_x(bool relenting);

    virtual float set_zoom_level(float zl, bool should_exit_auto_resize_mode, bool readjust=true);
    virtual float zoom_in(float zoom_factor = ZOOM_INC_FACTOR, bool readjust=true);
    virtual float zoom_out(float zoom_factor = ZOOM_INC_FACTOR, bool readjust=true);
    float zoom_in_cursor(WindowPos mouse_pos, float zoom_factor = ZOOM_INC_FACTOR);
    float zoom_out_cursor(WindowPos mouse_pos, float zoom_factor = ZOOM_INC_FACTOR);
    bool move_absolute(float dx, float dy, bool force = false);
    bool move_virtual(float dx, float dy, bool force = false);
    bool move(float dx, float dy, bool force = false);
    void get_absolute_delta_from_doc_delta(float doc_dx, float doc_dy, float* abs_dx, float* abs_dy);
    int get_center_page_number();
    void get_visible_pages(int window_height, std::vector<int>& visible_pages);
    void move_pages(int num_pages);

    std::vector<int> get_visible_search_results(const std::vector<int>& visible_pages);
    int find_search_index_for_visible_page(int page, int breakpoint);
    int find_search_results_breakpoint();
    int find_search_result_for_page_range(int page, int range_begin, int range_end);
    int find_search_results_breakpoint_helper(int begin_index, int end_index);
    void cancel_search();
    int get_num_search_results();
    int get_current_search_result_index();
    std::optional<SearchResult> get_current_search_result();
    std::optional<SearchResult> set_search_result_offset(int offset);
    void goto_search_result(int offset, bool overview);
    bool get_is_searching(float* prog);
    void set_search_results(std::vector<SearchResult>&& results);
    void search_text(PdfRenderer* background_searcher, const std::wstring& text, SearchCaseSensitivity case_sensitive, bool regex, std::optional<std::pair<int, int>> range);

    void set_overview_page(std::optional<OverviewState> overview);
    std::optional<OverviewState> get_overview_page();
    Document* get_current_overview_document(std::optional<OverviewState> maybe_overview = {});
    float get_overview_zoom_level(std::optional<OverviewState> maybe_overview = {});
    DocumentPos window_pos_to_overview_pos(NormalizedWindowPos window_pos);
    NormalizedWindowPos document_to_overview_pos(DocumentPos pos, std::optional<OverviewState> maybe_overview = {});
    NormalizedWindowRect document_to_overview_rect(DocumentRect doc_rect, std::optional<OverviewState> maybe_overview = {});
    void zoom_overview(float scale);
    void zoom_in_overview();
    void zoom_out_overview();
    NormalizedWindowRect get_overview_rect(std::optional<OverviewState> maybe_overview = {});
    NormalizedWindowRect get_overview_rect_pixel_perfect(int widget_width, int widget_height, int view_width, int view_height);
    std::vector<NormalizedWindowRect> get_overview_border_rects();
    WindowRect get_overview_download_rect();
    bool is_window_point_in_overview(NormalizedWindowPos window_point);
    bool is_window_point_in_overview_border(NormalizedWindowPos window_point, OverviewSide* which_border);
    void get_overview_offsets(float* offset_x, float* offset_y);
    void set_overview_offsets(float offset_x, float offset_y);
    void set_overview_offsets(fvec2 offsets);
    float get_overview_side_pos(int index);
    void set_overview_side_pos(OverviewSide index, NormalizedWindowRect original_rect, fvec2 diff);
    void set_overview_rect(NormalizedWindowRect rect);
    void get_overview_size(float* width, float* height);

    int find_search_index_for_visible_pages(const std::vector<int>& visible_pages);
    void move_screens(int num_screens);
    void reset_doc_state();
    void open_document(const std::wstring& doc_path, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions = false, std::string downloaded_checksum="");
    float get_page_offset(int page);
    void goto_offset_within_page(int page, float offset_y);
    void goto_page(int page);
    void fit_to_page_width(bool smart = false, bool ratio = false);
    void fit_to_page_height(bool smart = false);
    void fit_to_page_height_and_width_smart();
    void fit_to_page_height_width_minimum(int statusbar_height);
    void fit_overview_width();
    void persist(bool persist_drawings = false);
    std::wstring get_current_chapter_name();
    std::optional<AbsoluteRect> get_control_rect();
    std::optional<std::pair<int, int>> get_current_page_range();
    int get_current_chapter_index();
    void goto_chapter(int diff);
    void get_page_chapter_index(int page, std::vector<TocNode*> toc_nodes, std::vector<int>& res);
    std::vector<int> get_current_chapter_recursive_index();
    float view_height_in_document_space();
    void set_vertical_line_pos(float pos);
    bool has_ruler_rect();
    std::optional<AbsoluteRect> get_ruler_rect();
    std::vector<int> get_ruler_unmerged_line_indices();
    //std::pair<fz_stext_line*, fz_stext_block*> get_ruler_line_and_block();
    //float get_vertical_line_pos();
    float get_ruler_pos();

    bool is_link_a_reference(const PdfLink& link, const PdfLinkTextInfo& link_info);
    std::vector<DocumentRect> get_reference_link_highlights(int dest_page, const PdfLink& link, const PdfLinkTextInfo& link_info);
    bool is_text_source_referncish_at_position(const std::wstring& text, int position, const QString link_text = "");
    void set_overview_link(PdfLink link);
    void set_overview_position(
        int page,
        float offset,
        std::optional<std::string> overview_type,
        std::optional<std::vector<DocumentRect>> overview_highlights = {}
    );
    // bool overview_under_pos(WindowPos pos);

    //float get_vertical_line_window_y();
    float get_ruler_window_y();
    std::optional<NormalizedWindowRect> get_ruler_window_rect();

    void goto_vertical_line_pos();
    int get_page_offset();
    void set_page_offset(int new_offset);
    void rotate();
    void goto_top_of_page();
    void goto_bottom_of_page();
    int get_line_index_of_vertical_pos();
    int get_line_index_of_pos(DocumentPos docpos);
    int get_line_index();
    void set_line_index(int index, int page);
    //void set_ruler_rect(AbsoluteRect rect, bool after);
    int get_vertical_line_page();
    bool goto_definition();
    std::vector<SmartViewCandidate> find_line_definitions();
    std::optional<std::wstring> get_selected_line_text();
    bool get_is_auto_resize_mode();
    void disable_auto_resize_mode();
    void readjust_to_screen();
    float get_half_screen_offset();
    void scroll_mid_to_top();
    std::vector<AbsoluteRect> get_visible_line_rects(std::vector<int>& index_in_page);
    void get_visible_links(std::vector<PdfLink>& visible_page_links);
    void set_text_mark(bool is_begin);
    void toggle_text_mark();
    void get_rects_from_ranges(int page_number, const std::vector<PagelessDocumentRect>& line_char_rects, const std::vector<std::pair<int, int>>& ranges, std::vector<PagelessDocumentRect>& out_rects);
    std::optional<AbsoluteRect> expand_selection(bool is_begin, bool word);
    std::optional<AbsoluteRect> shrink_selection(bool is_begin, bool word);
    std::deque<AbsoluteRect>* get_selected_character_rects();
    void scroll_bookmark_with_uuid(const std::string& bookmark_uuid, int amount, float height);

    Document* doc();

    std::vector<VisibleObjectIndex> get_generic_visible_item_indices();

    template<typename T>
    std::vector<int> get_visible_annot_indices(const std::vector<int>& visible_pages) {
        std::vector<int> page_range_annots;
        for (auto page : visible_pages) {
            std::vector<int> page_highlights = doc()->get_page_visible_annot_indices<T>(page);
            for (auto ind : page_highlights) {
                page_range_annots.push_back(ind);
            }
        }

        std::sort(page_range_annots.begin(), page_range_annots.end());
        auto last = std::unique(page_range_annots.begin(), page_range_annots.end());
        int last_index = std::distance(page_range_annots.begin(), last);
        page_range_annots.erase(last, page_range_annots.end());
        return page_range_annots;
    }

    //std::vector<int> get_visible_highlight_indices(const std::vector<int>& visible_pages);
    //std::vector<int> get_visible_bookmark_indices(const std::vector<int>& visible_pages);

    template<typename T>
    std::vector<std::string> get_visible_annot_uuids(std::vector<int> visible_pages = {}) {

        const std::vector<T>& annots = doc()->get_annots<T>();
        if (visible_pages.size() == 0) {
            get_visible_pages(get_view_height(), visible_pages);
        }

        std::vector<int> indices = get_visible_annot_indices<T>(visible_pages);

        std::vector<std::string> res;
        res.reserve(indices.size());

        for (auto index : indices) {
            std::optional<AbsoluteRect> annot_rect  = annots[index].get_rectangle();
            if (annot_rect.has_value()) {
                bool is_visible = annot_rect->to_window_normalized(this).is_visible();
                if (is_visible) {
                    res.push_back(annots[index].uuid);
                }
            }
        }
        return res;
    }

    std::vector<std::string> get_visible_highlight_uuids(std::vector<int> visible_pages = {});
    std::vector<std::string> get_visible_bookmark_uuids(std::vector<int> visible_pages = {});
    std::vector<std::string> get_visible_portal_uuids(std::vector<int> visible_pages = {});

    void set_presentation_page_number(std::optional<int> page);
    std::optional<int> get_presentation_page_number();
    bool is_presentation_mode();
    VirtualPos absolute_to_virtual_pos(const AbsoluteDocumentPos& abspos);
    VirtualPos document_to_virtual_pos(DocumentPos docpos);
    AbsoluteDocumentPos virtual_to_absolute_pos(const VirtualPos& vpos);
    VirtualPos window_to_virtual_pos(const WindowPos& window_pos);
    WindowPos virtual_to_window_pos(const VirtualPos& virtual_pos);
    NormalizedWindowRect virtual_to_normalized_window_rect(const VirtualRect& virtual_rect);
    void toggle_two_page();
    bool is_two_page_mode();
    void set_page_space_x(float space_x);
    void set_page_space_y(float space_y);
    void focus_page_text(int page, const std::wstring& text);

    float get_page_space_x();
    float get_page_space_y();
    bool fast_coordinates();

    float get_bookmark_scroll_amount(const std::string& uuid);
    void set_bookmark_scroll_amount(const std::string& uuid, float amount);

    void select_ruler_text();
    void toggle_line_select_mode();
    bool is_line_select_mode();
    void debug();
    void focus_rect(DocumentRect rect);
    void focus_on_line_with_index(int page, int index);
    bool focus_on_visual_mark_pos(bool moving_down);
    AbsoluteRect move_visual_mark(int offset);

    void focus_text(int page, const std::wstring& text);
    void goto_next_block();
    void goto_prev_block();
    void focus_on_character_offset_into_document(int character_offset_into_document);
    bool handle_right_click_bookmark(WindowPos click_pos, BookMark* bookmark);
    QString get_markdown_bookmark_anchor_text_under_pos(QPoint cursor_pos);
    std::optional<VisibleObjectIndex> get_visible_object_at_pos(AbsoluteDocumentPos pos);
    std::string get_pending_portal_uuid_at_pos(AbsoluteDocumentPos abspos);
    std::vector<SearchResult> get_fuzzy_search_results(std::wstring query);
    std::string create_pending_download_portal(AbsoluteDocumentPos source_position, std::wstring paper_name);
    int get_pending_portal_index_with_handle(const std::string& handle);
    PendingDownloadPortal* get_pending_portal_with_uuid(const std::string& uuid);
    bool handle_visible_object_click(WindowPos click_pos, AbsoluteDocumentPos abs_doc_pos, std::optional<VisibleObjectIndex> visible_object);
    void begin_portal_scroll(WindowPos window_mpos);
    void begin_bookmark_move(const std::string& uuid, AbsoluteDocumentPos begin_cursor_pos);
    void begin_portal_move(const std::string& uuid, AbsoluteDocumentPos begin_cursor_pos, bool is_pending);
    std::string finish_pending_download_portal(std::wstring download_paper_name, std::wstring downloaded_file_path);

    void set_selected_portal_uuid(std::string uuid, bool is_pinned=false);
    void clear_selected_object();
    void set_selected_highlight_uuid(std::string uuid);
    void set_selected_bookmark_uuid(std::string uuid);
    bool handle_visible_object_scroll_mouse_move(AbsoluteDocumentPos abs_mpos, float height);
    void schedule_update_link_with_opened_book_state(Portal lnk, const OpenedBookState& new_state);
    // bool overview_under_pos(WindowPos pos);
    Portal* get_portal_under_window_pos(WindowPos pos);
    bool handle_visible_object_resize_mouse_move(AbsoluteDocumentPos abs_mpos);
    bool is_pinned_portal_selected();
    Portal* get_pinned_portal();
    void move_pinned_portal(float horizontal_amount, float vertical_amount);
    void zoom_pinned_portal(bool zoom_in);
    std::optional<Portal> pin_current_overview_as_portal();
    void perform_fuzzy_searches(std::vector<std::wstring> queries);
    void perform_fuzzy_search(std::wstring query);
    virtual bool is_scratchpad();

    void set_pending_portal(std::optional<std::pair<std::optional<std::wstring>, Portal>> pending_portal);
    void set_pending_portal(std::wstring path, Portal portal);
    bool is_pending_link_source_filled();
    void velocity_tick(float dt_secs, bool horizontal_scroll_locked);
    bool is_moving();
    void move_selected_bookmark_to_pos(AbsoluteDocumentPos pos);
    DocumentPos get_index_document_pos(int index);
    void highlight_window_points();
    std::vector<Portal> get_ruler_portals();
    void highlight_ruler_portals();
    std::optional<OverviewState> overview_to_ruler_portal(bool* is_render_invalid);
    void handle_visible_bookmark_tags_pre_perform(const std::vector<std::string>& visible_bookmark_uuids);
    void handle_generic_tags_pre_perform(const std::vector<VisibleObjectIndex>& visible_objects);
    void handle_highlight_tags_pre_perform(const std::vector<std::string>& visible_highlight_uuids);
    void clear_keyboard_select_highlights();
    void handle_fit_to_page_width(bool smart);
    void toggle_rect_hints();
    bool select_current_search_match();
    std::optional<Portal> get_target_portal(bool limit);
    std::optional<QString> get_overview_paper_name();
    std::optional<AbsoluteRect> get_overview_source_rect();
    std::optional<OverviewState> get_ith_next_overview(int i);
    void move_selected_drawings(AbsoluteDocumentPos new_pos, std::vector<FreehandDrawing>& moved_drawings, std::vector<PixmapDrawing>& moved_pixmaps);
    bool is_moving_annotations();
    void handle_freehand_drawing_selection_click(AbsoluteDocumentPos click_pos);
    void select_freehand_drawings(AbsoluteRect rect);
    void freehand_drawing_move_finish(AbsoluteDocumentPos mpos_absolute);
    void handle_portal_move(AbsoluteDocumentPos current_mouse_abspos);
    void handle_bookmark_move(AbsoluteDocumentPos current_mouse_abspos);
    void handle_portal_move_finish();
    void handle_bookmark_move_finish();
    const std::wstring& get_selected_text(bool insert_newlines=false);
    void expand_selection_vertical(bool begin, bool below);
    void handle_move_text_mark_down();
    void handle_move_text_mark_up();
    void handle_move_text_mark_backward(bool word);
    void move_selection_end(bool expand, bool word);
    void move_selection_begin(bool expand, bool word);
    void handle_move_text_mark_forward(bool word);
    void clear_selected_text();
    void finish_drawing(QPoint pos);
    void handle_drawing_move(QPoint pos, float pressure);
    AbsoluteDocumentPos get_window_abspos(WindowPos window_pos);
    char get_current_freehand_type();
    float get_current_freehand_alpha();
    void set_current_freehand_alpha(float alpha);
    bool handle_freehand_drawing_click_event(AbsoluteDocumentPos mpos_absolute);
    void start_drawing();
    void handle_freehand_drawing_move_finish(AbsoluteDocumentPos mpos_absolute);
    QSizeF get_bookmark_text_size(const BookMark& bookmark);
    void focus_on_current_page_text(const std::wstring& text);
    void handle_horizontal_move(int amount);
    void handle_portal_overview_update();
    void scroll_overview(int vertical_amount, int horizontal_amount=0);
    void scroll_overview_vertical(float amount);
    virtual bool move_document(float dx, float dy, bool force=false);
    void move_document_screens(int num_screens);
    void handle_vertical_move(int amount);
    void move_ruler(int amount);
    void return_to_last_visual_mark();
    void move_visual_mark_next();
    void move_visual_mark_prev();
    std::optional<float> move_visual_mark_next_get_offset();
    void handle_move_screen(int amount);
    std::optional<DocumentPos> get_overview_position();
    std::optional<Portal> create_portal_to_overview();
    bool is_rect_visible(DocumentRect rect);
    std::optional<AbsoluteRect> get_selected_rect_absolute();
    std::optional<DocumentRect> get_selected_rect_document();
    void handle_keyboard_select(const std::wstring& text, const std::vector<DocumentRect>& tag_rects);
    std::vector<PagelessDocumentRect> get_current_page_flat_words(std::vector<std::vector<PagelessDocumentRect>>* flat_word_chars=nullptr);
    //std::optional<DocumentRect> get_tag_rect(const std::vector<DocumentRect>& tag_rects, std::string tag, std::vector<PagelessDocumentRect>* word_chars = nullptr);
    std::optional<DocumentRect> get_tag_rect(const std::vector<DocumentRect>& tag_rects, std::string tag);
    //std::optional<WindowRect> get_tag_window_rect(const std::vector<DocumentRect>& tag_rects, std::string tag, std::vector<WindowRect>* char_rects=nullptr);
    std::optional<WindowRect> get_tag_window_rect(const std::vector<DocumentRect>& tag_rects, std::string tag);
    std::vector<DocumentRect> highlight_words();
    void toggle_presentation_mode();
    void set_presentation_mode(bool mode);
    fz_stext_char* get_closest_character_to_cusrsor(AbsoluteDocumentPos pos);
    void update_overview_highlighted_paper_with_position(DocumentPos docpos);
    void zoom_selected_freehand_drawings(float zoom_factor);
};

struct CachedScratchpadPixmapData {
    QPixmap pixmap;
    float zoom_level;
    float offset_x;
    float offset_y;
};

class ScratchPad : public DocumentView {
private:
    std::vector<FreehandDrawing> all_drawings;
    std::vector<FreehandDrawing> non_compiled_drawings;
    bool is_compile_valid = false;
public:

    std::vector<PixmapDrawing> pixmaps;
    std::optional<CompiledDrawingData> cached_compiled_drawing_data = {};
    std::unique_ptr<CachedScratchpadPixmapData> cached_pixmap = {};

    ScratchPad();
    bool set_offsets(float new_offset_x, float new_offset_y, bool force = false, bool is_dragging=false) override;
    float set_zoom_level(float zl, bool should_exit_auto_resize_mode, bool readjust=true);
    float zoom_in(float zoom_factor = ZOOM_INC_FACTOR, bool readjust=true);
    float zoom_out(float zoom_factor = ZOOM_INC_FACTOR, bool readjust=true);

    std::vector<int> get_intersecting_drawing_indices(AbsoluteRect selection);
    std::vector<int> get_intersecting_pixmap_indices(AbsoluteRect selection);
    std::vector<SelectedObjectIndex> get_intersecting_objects(AbsoluteRect selection);
    void delete_intersecting_drawings(AbsoluteRect selection);
    void delete_intersecting_pixmaps(AbsoluteRect selection);
    void delete_intersecting_objects(AbsoluteRect selection);
    void delete_objects_with_indices(const std::vector<SelectedObjectIndex> object_indices);
    void get_selected_objects_with_indices(const std::vector<SelectedObjectIndex>& indices, std::vector<FreehandDrawing>& freehand_drawings, std::vector<PixmapDrawing>& pixmap_drawings);
    void add_pixmap(QPixmap pixmap);
    AbsoluteRect get_bounding_box();

    const std::vector<FreehandDrawing>& get_all_drawings();
    const std::vector<FreehandDrawing>& get_non_compiled_drawings();
    void on_compile();
    void invalidate_compile(bool force=false);
    void add_drawing(FreehandDrawing drawing);
    void clear();
    bool is_compile_invalid();
    void load(std::wstring path);
    void save(std::wstring path);
    bool move_document(float dx, float dy, bool force=false) override;
    bool is_scratchpad() override;


};

