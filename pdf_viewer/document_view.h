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

extern float ZOOM_INC_FACTOR;

class CachedChecksummer;
class Document;
class DatabaseManager;
class DocumentManager;
class ConfigManager;
class PdfRenderer;


struct MarkedDataRect {
    DocumentRect rect;
    int type;
};

class DocumentView {
protected:

    DatabaseManager* db_manager = nullptr;
    DocumentManager* document_manager = nullptr;
    CachedChecksummer* checksummer;
    Document* current_document = nullptr;

    float zoom_level = 0.0f;
    //float offset_x = 0.0f;
    //float offset_y = 0.0f;
    VirtualPos offset = {0, 0};
    std::vector<VirtualRect> cached_virtual_rects;
    bool two_page_mode = false;

    // absolute rect of the current ruler if this is {} then ruler_pos is used instead
    std::optional<AbsoluteRect> ruler_rect;
    float ruler_pos = 0;

    // index of the current highlighted line in ruler mode
    int line_index = -1;

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
    bool should_show_numbers = false;
    bool should_show_rect_hints = false;
    ColorPalette color_mode = ColorPalette::Normal;

    std::string tag_prefix = "";
    std::vector<std::string> highlighted_tags;

    std::vector<DocumentRect> word_rects;

    std::optional<AbsoluteRect> pending_portal_rect = {};

    std::optional<AbsoluteRect> character_highlight_rect = {};
    std::optional<AbsoluteRect> wrong_character_rect = {};
    bool show_control_rect;

    std::optional<AbsoluteDocumentPos> underline = {};
    std::vector<DocumentRect> overview_highlights;

    std::vector<AbsoluteRect> pending_download_portals;
    std::optional<AbsoluteRect> selected_rectangle = {};

    // selected text (using mouse cursor or other methods) which is used e.g. for copying or highlighting
    std::wstring selected_text;

    // A list of candiadates to be shown in the overview window. We use simple heuristics to determine the
    // target of references, while this works most of the time, it is not perfect. So we keep a list of candidates
    // which the user can naviagte through using `next_preview` and `previous_preview` commands which move
    // `index_into_candidates` pointer to the next/previous candidate
    std::vector<SmartViewCandidate> smart_view_candidates;
    int index_into_candidates = 0;

    int rotation_index = 0;
    bool fastread_mode = false;
    int selected_highlight_index = -1;
    int selected_bookmark_index = -1;

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

    DocumentView(DatabaseManager* db_manager, DocumentManager* document_manager, CachedChecksummer* checksummer);
    ~DocumentView();
    float get_zoom_level();
    DocumentViewState get_state();
    PortalViewState get_checksum_state();
    //void set_opened_book_state(const OpenedBookState& state);
    void handle_escape();
    void set_book_state(OpenedBookState state);
    virtual bool set_offsets(float new_offset_x, float new_offset_y, bool force = false);
    bool set_pos(AbsoluteDocumentPos pos);
    void set_virtual_pos(VirtualPos pos);
    Document* get_document();
    bool is_ruler_mode();
    void exit_ruler_mode();

    void toggle_highlight_links();
    void set_highlight_links(bool should_highlight, bool should_show_numbers);
    void set_dark_mode(bool mode);
    void toggle_dark_mode();
    void set_custom_color_mode(bool mode);
    void toggle_custom_color_mode();
    void toggle_highlight_words();
    void set_highlight_words(std::vector<DocumentRect>& rects);
    void set_should_highlight_words(bool should_highlight);
    std::vector<DocumentRect> get_highlight_word_rects();
    void show_rect_hints();
    void hide_rect_hints();
    bool is_showing_rect_hints();
    bool on_vertical_scroll();
    bool is_pos_inside_selected_text(AbsoluteDocumentPos pos);
    bool is_pos_inside_selected_text(DocumentPos docpos);
    bool is_pos_inside_selected_text(WindowPos pos);
    std::optional<QString> get_paper_name_under_pos(DocumentPos docpos, bool clean=false);
    std::optional<QString> get_direct_paper_name_under_pos(DocumentPos docpos);
    TextUnderPointerInfo find_location_of_text_under_pointer(DocumentPos docpos, bool update_candidates=false);
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
    void set_selected_highlight_index(int index);
    void set_selected_bookmark_index(int index);
    void set_overview_highlights(const std::vector<DocumentRect>& rects);

    void set_selected_rectangle(AbsoluteRect selected);
    void clear_selected_rectangle();
    std::optional<AbsoluteRect> get_selected_rectangle();
    void set_pending_download_portals(std::vector<AbsoluteRect>&& portal_rects);

    // find the closest portal to the current position
    // if limit is true, we only search for portals near the current location and not all portals
    std::optional<Portal> find_closest_portal(bool limit = false);
    std::optional<BookMark> find_closest_bookmark();
    void goto_portal(Portal* link);
    std::string delete_closest_portal();
    std::string delete_closest_bookmark();
    Highlight get_highlight_with_index(int index);
    std::string delete_highlight_with_index(int index);
    std::string delete_bookmark_with_index(int index);
    void delete_highlight(Highlight hl);
    std::string delete_closest_bookmark_to_offset(float offset);
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
    int get_highlight_index_in_pos(WindowPos pos);
    void get_text_selection(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, bool is_word_selection, std::deque<AbsoluteRect>& selected_characters, std::wstring& text_selection);
    std::string add_mark(char symbol);
    std::string add_bookmark(std::wstring desc);
    std::string add_highlight_(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);
    void on_view_size_change(int new_width, int new_height);
    //void absolute_to_window_pos(float absolute_x, float absolute_y, float* window_x, float* window_y);
    NormalizedWindowPos absolute_to_window_pos(AbsoluteDocumentPos absolute_pos);

    void set_pending_portal_position(std::optional<AbsoluteRect> rect);
    void set_synctex_highlights(std::vector<DocumentRect> highlights);

    bool should_show_synxtex_highlights();
    bool has_synctex_timed_out();

    ColorPalette get_current_color_mode();

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

    virtual float set_zoom_level(float zl, bool should_exit_auto_resize_mode);
    virtual float zoom_in(float zoom_factor = ZOOM_INC_FACTOR);
    virtual float zoom_out(float zoom_factor = ZOOM_INC_FACTOR);
    float zoom_in_cursor(WindowPos mouse_pos, float zoom_factor = ZOOM_INC_FACTOR);
    float zoom_out_cursor(WindowPos mouse_pos, float zoom_factor = ZOOM_INC_FACTOR);
    bool move_absolute(float dx, float dy, bool force = false);
    bool move_virtual(float dx, float dy, bool force = false);
    bool move(float dx, float dy, bool force = false);
    void get_absolute_delta_from_doc_delta(float doc_dx, float doc_dy, float* abs_dx, float* abs_dy);
    int get_center_page_number();
    void get_visible_pages(int window_height, std::vector<int>& visible_pages);
    void move_pages(int num_pages);

    std::vector<int> get_visible_search_results(std::vector<int>& visible_pages);
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
    void search_text(PdfRenderer* background_searcher, const std::wstring& text, SearchCaseSensitivity case_sensitive, bool regex, std::optional<std::pair<int, int>> range);

    void set_overview_page(std::optional<OverviewState> overview);
    std::optional<OverviewState> get_overview_page();
    Document* get_current_overview_document();
    float get_overview_zoom_level();
    DocumentPos window_pos_to_overview_pos(NormalizedWindowPos window_pos);
    NormalizedWindowPos document_to_overview_pos(DocumentPos pos);
    NormalizedWindowRect document_to_overview_rect(DocumentRect doc_rect);
    void zoom_overview(float scale);
    void zoom_in_overview();
    void zoom_out_overview();
    NormalizedWindowRect get_overview_rect();
    NormalizedWindowRect get_overview_rect_pixel_perfect(int widget_width, int widget_height, int view_width, int view_height);
    std::vector<NormalizedWindowRect> get_overview_border_rects();
    bool is_window_point_in_overview(NormalizedWindowPos window_point);
    bool is_window_point_in_overview_border(NormalizedWindowPos window_point, OverviewSide* which_border);
    void get_overview_offsets(float* offset_x, float* offset_y);
    void set_overview_offsets(float offset_x, float offset_y);
    void set_overview_offsets(fvec2 offsets);
    float get_overview_side_pos(int index);
    void set_overview_side_pos(OverviewSide index, NormalizedWindowRect original_rect, fvec2 diff);
    void set_overview_rect(NormalizedWindowRect rect);
    void get_overview_size(float* width, float* height);

    int find_search_index_for_visible_pages(std::vector<int>& visible_pages);
    void move_screens(int num_screens);
    void reset_doc_state();
    void open_document(const std::wstring& doc_path, bool* invalid_flag, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions = false);
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
    //float get_vertical_line_pos();
    float get_ruler_pos();

    bool is_link_a_reference(const PdfLink& link, const PdfLinkTextInfo& link_info);
    std::vector<DocumentRect> get_reference_link_highlights(int dest_page, const PdfLink& link, const PdfLinkTextInfo& link_info);
    bool is_text_source_referncish_at_position(const std::wstring& text, int position);
    void set_overview_link(PdfLink link);
    void set_overview_position(
        int page,
        float offset,
        std::optional<std::string> overview_type,
        std::optional<std::vector<DocumentRect>> overview_highlights = {}
    );
    bool overview_under_pos(WindowPos pos);

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
    int get_vertical_line_page();
    bool goto_definition();
    std::vector<SmartViewCandidate> find_line_definitions();
    std::optional<std::wstring> get_selected_line_text();
    bool get_is_auto_resize_mode();
    void disable_auto_resize_mode();
    void readjust_to_screen();
    float get_half_screen_offset();
    void scroll_mid_to_top();
    void get_visible_links(std::vector<PdfLink>& visible_page_links);
    void set_text_mark(bool is_begin);
    void toggle_text_mark();
    void get_rects_from_ranges(int page_number, const std::vector<PagelessDocumentRect>& line_char_rects, const std::vector<std::pair<int, int>>& ranges, std::vector<PagelessDocumentRect>& out_rects);
    std::optional<AbsoluteRect> expand_selection(bool is_begin, bool word);
    std::optional<AbsoluteRect> shrink_selection(bool is_begin, bool word);
    std::deque<AbsoluteRect>* get_selected_character_rects();

    std::vector<int> get_visible_highlight_indices();
    std::vector<int> get_visible_bookmark_indices();
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

    float get_page_space_x();
    float get_page_space_y();
    bool fast_coordinates();

};


class ScratchPad : public DocumentView {
private:
    std::vector<FreehandDrawing> all_drawings;
    std::vector<FreehandDrawing> non_compiled_drawings;
    bool is_compile_valid = false;
public:

    std::vector<PixmapDrawing> pixmaps;
    std::optional<CompiledDrawingData> cached_compiled_drawing_data = {};

    ScratchPad();
    bool set_offsets(float new_offset_x, float new_offset_y, bool force = false);
    float set_zoom_level(float zl, bool should_exit_auto_resize_mode);
    float zoom_in(float zoom_factor = ZOOM_INC_FACTOR);
    float zoom_out(float zoom_factor = ZOOM_INC_FACTOR);

    std::vector<int> get_intersecting_drawing_indices(AbsoluteRect selection);
    std::vector<int> get_intersecting_pixmap_indices(AbsoluteRect selection);
    std::vector<SelectedObjectIndex> get_intersecting_objects(AbsoluteRect selection);
    void delete_intersecting_drawings(AbsoluteRect selection);
    void delete_intersecting_pixmaps(AbsoluteRect selection);
    void delete_intersecting_objects(AbsoluteRect selection);
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

};


