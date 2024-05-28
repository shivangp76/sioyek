#include <cmath>

#include "document_view.h"
#include "checksum.h"
#include "database.h"
#include "document.h"
#include "qlogging.h"
#include "utils.h"
#include "config.h"
#include "ui.h"
#include "pdf_renderer.h"

extern float MOVE_SCREEN_PERCENTAGE;
extern float FIT_TO_PAGE_WIDTH_RATIO;
extern float RULER_PADDING;
extern float RULER_X_PADDING;
extern bool EXACT_HIGHLIGHT_SELECT;
extern bool VERBOSE;
extern float PAGE_SPACE_X;
extern float PAGE_SPACE_Y;
extern bool REAL_PAGE_SEPARATION;
extern float OVERVIEW_SIZE[2];
extern float OVERVIEW_OFFSET[2];
extern bool SHOULD_HIGHLIGHT_LINKS;
extern float HIDE_SYNCTEX_HIGHLIGHT_TIMEOUT;
extern int PAGE_PADDINGS;

DocumentView::DocumentView(DatabaseManager* db_manager,
    DocumentManager* document_manager,
    CachedChecksummer* checksummer) :
    db_manager(db_manager),
    document_manager(document_manager),
    checksummer(checksummer)
{
    page_space_x = PAGE_SPACE_X;
    page_space_y = PAGE_SPACE_Y;

    overview_half_width = OVERVIEW_SIZE[0];
    overview_half_height = OVERVIEW_SIZE[1];

    overview_offset_x = OVERVIEW_OFFSET[0];
    overview_offset_y = OVERVIEW_OFFSET[1];
}
DocumentView::~DocumentView() {
}

float DocumentView::get_zoom_level() {
    return zoom_level;
}

DocumentViewState DocumentView::get_state() {
    DocumentViewState res;

    if (current_document) {
        res.document_path = current_document->get_path();
        res.book_state.offset_x = get_offset_x();
        res.book_state.offset_y = get_offset_y();
        res.book_state.zoom_level = get_zoom_level();
        res.book_state.ruler_mode = is_ruler_mode_;
        res.book_state.ruler_pos = ruler_pos;
        res.book_state.ruler_rect = ruler_rect;
        res.book_state.ruler_info = ruler_line_index;
        res.book_state.presentation_page = presentation_page_number;
    }
    return res;
}

PortalViewState DocumentView::get_checksum_state() {
    PortalViewState res;

    if (current_document) {
        res.document_checksum = current_document->get_checksum();
        res.book_state.offset_x = get_offset_x();
        res.book_state.offset_y = get_offset_y();
        res.book_state.zoom_level = get_zoom_level();
    }
    return res;
}

//void DocumentView::set_opened_book_state(const OpenedBookState& state) {
//    set_offsets(state.offset_x, state.offset_y);
//    set_zoom_level(state.zoom_level, true);
//    is_ruler_mode_ = state.ruler_mode;
//    ruler_pos = state.ruler_pos;
//    ruler_rect = state.ruler_rect;
//
//}


void DocumentView::handle_escape() {
    if (!SHOULD_HIGHLIGHT_LINKS) {
        should_highlight_links = false;
    }
    should_highlight_words = false;
    should_show_numbers = false;
    highlighted_tags = {};
    character_highlight_rect = {};
    wrong_character_rect = {};
    synctex_highlights.clear();
    if (line_select_mode) {
        toggle_line_select_mode();
    }
}

void DocumentView::exit_ruler_mode() {
    is_ruler_mode_ = false;
}

void DocumentView::set_book_state(OpenedBookState state) {
    set_offsets(state.offset_x, state.offset_y);
    set_zoom_level(state.zoom_level, true);
    presentation_page_number = state.presentation_page;
    is_ruler_mode_ = state.ruler_mode;
    ruler_pos = state.ruler_pos;
    ruler_rect = state.ruler_rect;
    ruler_line_index = state.ruler_info;
}

bool DocumentView::set_pos(AbsoluteDocumentPos pos) {
    return set_offsets(pos.x, pos.y);
}

void DocumentView::set_virtual_pos(VirtualPos pos) {
    if (!fast_coordinates()) {
        offset = pos;
    }
    else {
        set_offsets(pos.x, pos.y);
    }
}

bool DocumentView::set_offsets(float new_offset_x, float new_offset_y, bool force) {

    // if move was truncated
    bool truncated = false;

    if (current_document == nullptr) return false;

    int num_pages = current_document->num_pages();
    if (num_pages == 0) return false;

    float max_y_offset = current_document->get_accum_page_height(num_pages - 1) + current_document->get_page_height(num_pages - 1);
    float min_y_offset = 0;
    float min_x_offset_normal = get_min_valid_x(false);
    float max_x_offset_normal = get_max_valid_x(false);
    float min_x_offset_relenting = get_min_valid_x(true);
    float max_x_offset_relenting = get_max_valid_x(true);
    float max_x_offset = is_relenting ? max_x_offset_relenting : max_x_offset_normal;
    float min_x_offset = is_relenting ? min_x_offset_relenting : min_x_offset_normal;
    float relent_threshold = view_width / 4 / zoom_level;

    if (TOUCH_MODE) {
        if ((new_offset_x - max_x_offset_normal > relent_threshold) || (min_x_offset_normal - new_offset_x > relent_threshold)) {
            is_relenting = true;
        }
        else {
            is_relenting = false;
        }
    }

    if (!force) {
        if (new_offset_y > max_y_offset) { new_offset_y = max_y_offset; truncated = true; }
        if (new_offset_y < min_y_offset) { new_offset_y = min_y_offset; truncated = true; }
        if (new_offset_x > max_x_offset) { new_offset_x = max_x_offset; truncated = true; }
        if (new_offset_x < min_x_offset) { new_offset_x = min_x_offset; truncated = true; }
    }

    //offset_x = new_offset_x;
    //offset_y = new_offset_y;
    offset = absolute_to_virtual_pos(AbsoluteDocumentPos{new_offset_x, new_offset_y});
    return truncated;
}

Document* DocumentView::get_document() {
    return current_document;
}

//int DocumentView::get_num_search_results() {
//	search_results_mutex.lock();
//	int num = search_results.size();
//	search_results_mutex.unlock();
//	return num;
//}
//
//int DocumentView::get_current_search_result_index() {
//	return current_search_result_index;
//}

std::optional<Portal> DocumentView::find_closest_portal(bool limit) {
    if (current_document) {
        float offset_y = get_offsets().y;
        auto res = current_document->find_closest_portal(offset_y);
        if (res) {
            if (!limit) {
                return res;
            }
            else {
                if (std::abs(res.value().src_offset_y - offset_y) < 500.0f) {
                    return res;
                }
            }
        }
    }
    return {};
}

std::optional<BookMark> DocumentView::find_closest_bookmark() {

    if (current_document) {
        float offset_y = get_offsets().y;
        int bookmark_index = current_document->find_closest_bookmark_index(current_document->get_bookmarks(), offset_y);
        const std::vector<BookMark>& bookmarks = current_document->get_bookmarks();
        if ((bookmark_index >= 0) && (bookmark_index < bookmarks.size())) {
            if (std::abs(bookmarks[bookmark_index].get_y_offset() - offset_y) < 1000.0f) {
                return bookmarks[bookmark_index];
            }
        }
    }
    return {};
}

void DocumentView::goto_portal(Portal* link) {
    if (link) {
        if (get_document() &&
            get_document()->get_checksum() == link->dst.document_checksum) {
            set_book_state(link->dst.book_state);
        }
        else {
            auto destination_path = checksummer->get_path(link->dst.document_checksum);
            if (destination_path) {
                open_document(destination_path.value(), nullptr);
                set_book_state(link->dst.book_state);
            }
        }
    }
}

std::string DocumentView::delete_closest_portal() {
    if (current_document) {
        return current_document->delete_closest_portal(get_offsets().y);
    }
}

std::string DocumentView::delete_closest_bookmark() {
    if (current_document) {
        return delete_closest_bookmark_to_offset(get_offsets().y);
    }
    return "";
}

// todo: these should be in Document not here
Highlight DocumentView::get_highlight_with_index(int index) {
    return current_document->get_highlights()[index];
}

std::string DocumentView::delete_highlight_with_index(int index) {
    return current_document->delete_highlight_with_index(index);
}

std::string DocumentView::delete_bookmark_with_index(int index) {
    return current_document->delete_bookmark_with_index(index);
}

std::string DocumentView::delete_portal_with_index(int index) {
    return current_document->delete_portal_with_index(index);
}

void DocumentView::delete_highlight(Highlight hl) {
    current_document->delete_highlight(hl);
}

std::string DocumentView::delete_closest_bookmark_to_offset(float offset) {
    return current_document->delete_closest_bookmark(offset);
}

float DocumentView::get_offset_x() {
    return virtual_to_absolute_pos(offset).x;
}

float DocumentView::get_offset_y() {
    return virtual_to_absolute_pos(offset).y;
}

AbsoluteDocumentPos DocumentView::get_offsets() {
    return virtual_to_absolute_pos(offset);
}

VirtualPos DocumentView::get_virtual_offset() {
    return offset;
}

int DocumentView::get_view_height() {
    return view_height;
}

int DocumentView::get_view_width() {
    return view_width;
}

void DocumentView::set_null_document() {
    current_document = nullptr;
}

void DocumentView::set_offset_x(float new_offset_x) {
    set_offsets(new_offset_x, get_offset_y());
}

void DocumentView::set_offset_y(float new_offset_y) {
    if (is_two_page_mode()) {
        AbsoluteDocumentPos current = get_offsets();
        current.y = new_offset_y;
        VirtualPos new_pos = absolute_to_virtual_pos(current);
        offset.y = new_pos.y;
    }
    else {
        set_offsets(get_offset_x(), new_offset_y);
    }
}

std::optional<PdfLink> DocumentView::get_link_in_pos(WindowPos pos) {
    if (!current_document) return {};

    DocumentPos doc_pos = window_to_document_pos(pos);
    return current_document->get_link_in_pos(doc_pos);
}

int DocumentView::get_highlight_index_in_pos(WindowPos window_pos) {
    //auto [view_x, view_y] = window_to_absolute_document_pos(window_pos);

    //fz_point pos = { view_x, view_y };
    AbsoluteDocumentPos pos = window_pos.to_absolute(this);

    // if multiple highlights contain the position, we return the smallest highlight
    // see: https://github.com/ahrm/sioyek/issues/773
    int smallest_containing_highlight_index = -1;
    int min_length = INT_MAX;

    if (current_document->can_use_highlights()) {
        const std::vector<Highlight>& highlights = current_document->get_highlights();

        for (size_t i = 0; i < highlights.size(); i++) {
            for (size_t j = 0; j < highlights[i].highlight_rects.size(); j++) {
                //if (fz_is_point_inside_rect(pos, highlights[i].highlight_rects[j])) {
                if (highlights[i].highlight_rects[j].contains(pos)) {
                    int length = highlights[i].description.size();
                    if (length < min_length) {
                        min_length = length;
                        smallest_containing_highlight_index = i;
                    }
                }
            }
        }
    }
    return smallest_containing_highlight_index;
}

std::string DocumentView::add_mark(char symbol) {
    //assert(current_document);
    if (current_document) {
        AbsoluteDocumentPos current_offset = get_offsets();
        return current_document->add_mark(symbol, current_offset.y, current_offset.x, zoom_level);
    }
    return "";
}

std::string DocumentView::add_bookmark(std::wstring desc) {
    //assert(current_document);
    if (current_document) {
        return current_document->add_bookmark(desc, get_offset_y());
    }
    return "";
}

std::string DocumentView::add_highlight_(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type) {

    if (current_document) {
        std::deque<AbsoluteRect> selected_characters;
        std::vector<AbsoluteRect> merged_characters;
        std::wstring selected_text;

        get_text_selection(selection_begin, selection_end, !EXACT_HIGHLIGHT_SELECT, selected_characters, selected_text);
        merge_selected_character_rects(selected_characters, merged_characters);
        if (selected_text.size() > 0) {
            return current_document->add_highlight(selected_text, (std::vector<AbsoluteRect>&)merged_characters, selection_begin, selection_end, type);
        }
    }

    return "";
}

void DocumentView::on_view_size_change(int new_width, int new_height) {
    view_width = new_width;
    view_height = new_height;
}

//void DocumentView::absolute_to_window_pos_pixels(float absolute_x, float absolute_y, float* window_x, float* window_y) {
//
//}

NormalizedWindowPos DocumentView::absolute_to_window_pos(AbsoluteDocumentPos abs) {
    VirtualPos vpos = absolute_to_virtual_pos(abs);
    WindowPos window_pos = virtual_to_window_pos(vpos);
    return window_pos.to_window_normalized(this);
    //NormalizedWindowPos res;
    //float half_width = static_cast<float>(view_width) / zoom_level / 2;
    //float half_height = static_cast<float>(view_height) / zoom_level / 2;

    //res.x = (abs.x + offset.x) / half_width;
    //res.y = (-abs.y + offset.y) / half_height;
    //return res;

}

NormalizedWindowRect DocumentView::absolute_to_window_rect(AbsoluteRect doc_rect) {
    NormalizedWindowPos top_left = doc_rect.top_left().to_window_normalized(this);
    NormalizedWindowPos bottom_right = doc_rect.bottom_right().to_window_normalized(this);

    return NormalizedWindowRect(top_left, bottom_right);
}

NormalizedWindowPos DocumentView::document_to_window_pos(DocumentPos doc_pos) {

    if (current_document) {
        WindowPos window_pos = document_to_window_pos_in_pixels_uncentered(doc_pos);
        double halfwidth = static_cast<double>(view_width) / 2;
        double halfheight = static_cast<double>(view_height) / 2;

        float window_x = static_cast<float>((window_pos.x - halfwidth) / halfwidth);
        float window_y = static_cast<float>((window_pos.y - halfheight) / halfheight);
        return { window_x, -window_y };
    }
}

WindowPos DocumentView::absolute_to_window_pos_in_pixels(AbsoluteDocumentPos abspos) {
    VirtualPos vpos = absolute_to_virtual_pos(abspos);

    WindowPos window_pos;
    window_pos.y = (vpos.y - offset.y) * zoom_level + view_height / 2;
    window_pos.x = (vpos.x + offset.x) * zoom_level + view_width / 2;
    return window_pos;
}

WindowPos DocumentView::document_to_window_pos_in_pixels_uncentered(DocumentPos doc_pos) {
    VirtualPos vpos = document_to_virtual_pos(doc_pos);
    return virtual_to_window_pos(vpos);
    //AbsoluteDocumentPos abspos = current_document->document_to_absolute_pos(doc_pos);
    //return absolute_to_window_pos_in_pixels(abspos);
}

WindowPos DocumentView::document_to_window_pos_in_pixels_banded(DocumentPos doc_pos) {
    if (is_two_page_mode() || (REAL_PAGE_SEPARATION)) {
        VirtualPos vpos = document_to_virtual_pos(doc_pos);
        WindowPos window_pos = virtual_to_window_pos(vpos);
        return window_pos;
    }
    else {
        AbsoluteDocumentPos abspos = current_document->document_to_absolute_pos(doc_pos);
        WindowPos window_pos;
        window_pos.y = static_cast<int>(std::roundf((abspos.y - offset.y) * zoom_level + static_cast<float>(view_height) / 2.0f));
        window_pos.x = static_cast<int>(std::roundf((abspos.x + offset.x) * zoom_level + static_cast<float>(view_width) / 2.0f));
        return window_pos;
    }
}

WindowRect DocumentView::document_to_window_irect(DocumentRect doc_rect) {
    WindowPos top_left = doc_rect.top_left().to_window(this);
    WindowPos bottom_right = doc_rect.bottom_right().to_window(this);
    return WindowRect(top_left, bottom_right);
}

NormalizedWindowRect DocumentView::document_to_window_rect(DocumentRect doc_rect) {
    NormalizedWindowPos top_left = doc_rect.top_left().to_window_normalized(this);
    NormalizedWindowPos bottom_right = doc_rect.bottom_right().to_window_normalized(this);

    return NormalizedWindowRect(top_left, bottom_right);
}

NormalizedWindowRect DocumentView::document_to_window_rect_pixel_perfect(DocumentRect doc_rect, int pixel_width, int pixel_height, bool banded) {

    if ((pixel_width <= 0) || (pixel_height <= 0)) {
        return doc_rect.to_window_normalized(this);
    }


    WindowPos top_left, bottom_right;
    if (banded) {
        top_left = document_to_window_pos_in_pixels_banded(doc_rect.top_left());
        bottom_right = document_to_window_pos_in_pixels_banded(doc_rect.bottom_right());
    }
    else {
        top_left = document_to_window_pos_in_pixels_uncentered(doc_rect.top_left());
        bottom_right = document_to_window_pos_in_pixels_uncentered(doc_rect.bottom_right());
    }

    bottom_right.x -= ((bottom_right.x - top_left.x) - pixel_width);
    if (!banded) {
        //w1.y -= ((w1.y - w0.y) - pixel_height);
        bottom_right.y -= ((bottom_right.y - top_left.y) - pixel_height);
    }

    NormalizedWindowPos top_left_normalized = top_left.to_window_normalized(this);
    NormalizedWindowPos bottom_right_normalized = bottom_right.to_window_normalized(this);

    return NormalizedWindowRect(top_left_normalized, bottom_right_normalized);
}

//DocumentPos DocumentView::window_to_document_pos_uncentered(WindowPos window_pos) {
//    if (current_document) {
//        return current_document->absolute_to_page_pos_uncentered(
//            { (window_pos.x - view_width / 2) / zoom_level - offset.x,
//            (window_pos.y - view_height / 2) / zoom_level + offset.y });
//    }
//    else {
//        return { -1, 0, 0 };
//    }
//}

DocumentPos DocumentView::window_to_document_pos(WindowPos window_pos) {
    if (current_document) {
        return window_to_absolute_document_pos(window_pos).to_document(current_document);
        //return current_document->absolute_to_page_pos_uncentered(
        //    { (window_pos.x - view_width / 2) / zoom_level - offset.x,
        //    (window_pos.y - view_height / 2) / zoom_level + offset.y });
    }
    else {
        return { -1, 0, 0 };
    }
}

AbsoluteDocumentPos DocumentView::window_to_absolute_document_pos(WindowPos window_pos) {
    VirtualPos virtual_pos = window_to_virtual_pos(window_pos);
    return virtual_to_absolute_pos(virtual_pos);
    //float doc_x = (window_pos.x - view_width / 2) / zoom_level - offset.x;
    //float doc_y = (window_pos.y - view_height / 2) / zoom_level + offset.y;
    //return { doc_x, doc_y };
}

NormalizedWindowPos DocumentView::window_to_normalized_window_pos(WindowPos window_pos) {
    float normal_x = 2 * (static_cast<float>(window_pos.x) - view_width / 2.0f) / view_width;
    float normal_y = -2 * (static_cast<float>(window_pos.y) - view_height / 2.0f) / view_height;
    return { normal_x, normal_y };
}


void DocumentView::goto_mark(char symbol) {
    if (current_document) {
        float new_y_offset = 0.0f;
        std::optional<Mark> mark = current_document->get_mark_if_exists(symbol);
        if (mark) {
            set_offset_y(mark->y_offset);
            if (mark->x_offset) {
                set_offset_x(mark->x_offset.value());
                set_zoom_level(mark->zoom_level.value(), true);
            }
        }
    }
}
void DocumentView::goto_end() {
    if (current_document) {
        int last_page_index = current_document->num_pages() - 1;
        set_offset_y(current_document->get_accum_page_height(last_page_index) + current_document->get_page_height(last_page_index));
    }
}

void DocumentView::goto_left_smart() {

    float left_ratio, right_ratio;
    int normal_page_width;
    float page_width = current_document->get_page_size_smart(true, get_center_page_number(), &left_ratio, &right_ratio, &normal_page_width);
    float view_left_offset = (page_width / 2 - view_width / zoom_level / 2);

    set_offset_x(view_left_offset);
}

void DocumentView::goto_left() {
    float page_width = current_document->get_page_width(get_center_page_number());
    float view_left_offset = (page_width / 2 - view_width / zoom_level / 2);
    set_offset_x(view_left_offset);
}

void DocumentView::goto_right_smart() {

    float left_ratio, right_ratio;
    int normal_page_width;
    float page_width = current_document->get_page_size_smart(true, get_center_page_number(), &left_ratio, &right_ratio, &normal_page_width);
    float view_left_offset = -(page_width / 2 - view_width / zoom_level / 2);

    set_offset_x(view_left_offset);
}

void DocumentView::goto_right() {
    float page_width = current_document->get_page_width(get_center_page_number());
    float view_left_offset = -(page_width / 2 - view_width / zoom_level / 2);
    set_offset_x(view_left_offset);
}

float DocumentView::set_zoom_level(float zl, bool should_exit_auto_resize_mode) {
#ifdef SIOYEK_ANDROID
    const float max_zoom_level = 6.0f;
#else
    const float max_zoom_level = 10.0f;
#endif

    if (TOUCH_MODE){
        int page_number = get_center_page_number();
        if (page_number >= 0){
            float min_zoom_level = view_width / current_document->get_page_width(page_number);
            if (zl < min_zoom_level){
                zl = min_zoom_level;
            }
        }
    }

    if (zl > max_zoom_level) {
        zl = max_zoom_level;
    }
    if (should_exit_auto_resize_mode) {
        this->is_auto_resize_mode = false;
    }
    zoom_level = zl;
    this->readjust_to_screen();
    return zoom_level;
}

float DocumentView::zoom_in(float zoom_factor) {
#ifdef SIOYEK_ANDROID
    const float max_zoom_level = 6.0f;
#else
    const float max_zoom_level = 10.0f;
#endif
    float new_zoom_level = zoom_level * zoom_factor;

    if (new_zoom_level > max_zoom_level) {
        new_zoom_level = max_zoom_level;
    }

    return set_zoom_level(new_zoom_level, true);
}
float DocumentView::zoom_out(float zoom_factor) {
    return set_zoom_level(zoom_level / zoom_factor, true);
}

float DocumentView::zoom_in_cursor(WindowPos mouse_pos, float zoom_factor) {

    AbsoluteDocumentPos prev_doc_pos = window_to_absolute_document_pos(mouse_pos);

    float res = zoom_in(zoom_factor);

    AbsoluteDocumentPos new_doc_pos = window_to_absolute_document_pos(mouse_pos);

    move_absolute(-prev_doc_pos.x + new_doc_pos.x, prev_doc_pos.y - new_doc_pos.y);

    return res;
}

float DocumentView::zoom_out_cursor(WindowPos mouse_pos, float zoom_factor) {
    auto [prev_doc_x, prev_doc_y] = window_to_absolute_document_pos(mouse_pos);

    float res = zoom_out(zoom_factor);

    auto [new_doc_x, new_doc_y] = window_to_absolute_document_pos(mouse_pos);

    move_absolute(-prev_doc_x + new_doc_x, prev_doc_y - new_doc_y);
    return res;
}

bool DocumentView::move_absolute(float dx, float dy, bool force) {
    AbsoluteDocumentPos prev_offsets = get_offsets();

    return set_offsets(prev_offsets.x + dx, prev_offsets.y + dy, force);
    //offset.x += dx;
    //offset.y += dy;
    //return false;
}

bool DocumentView::move_virtual(float dx, float dy, bool force) {
    offset.x += dx;
    offset.y += dy;
    return false;
}

bool DocumentView::move(float dx, float dy, bool force) {
    float abs_dx = (dx / zoom_level);
    float abs_dy = (dy / zoom_level);
    if (!fast_coordinates()) {
        return move_virtual(abs_dx, abs_dy, force);
    }
    else {
        return move_absolute(abs_dx, abs_dy, force);
    }
}
void DocumentView::get_absolute_delta_from_doc_delta(float dx, float dy, float* abs_dx, float* abs_dy) {
    *abs_dx = (dx / zoom_level);
    *abs_dy = (dy / zoom_level);
}

int DocumentView::get_center_page_number() {
    if (current_document) {
        return current_document->get_offset_page_number(get_offset_y());
    }
    else {
        return -1;
    }
}

void DocumentView::get_visible_pages(int window_height, std::vector<int>& visible_pages) {
    if (!current_document) return;

    AbsoluteDocumentPos abs_offset = virtual_to_absolute_pos(offset);
    float window_y_range_begin = abs_offset.y - window_height / (1.5 * zoom_level);
    float window_y_range_end = abs_offset.y + window_height / (1.5 * zoom_level);
    window_y_range_begin -= 1;
    window_y_range_end += 1;

    if (!fast_coordinates()) {
        fill_cached_virtual_rects();

        for (int i = 0; i < cached_virtual_rects.size(); i++){
            if (virtual_to_normalized_window_rect(cached_virtual_rects[i]).is_visible(0.5f)) {
                visible_pages.push_back(i);
            }
        }
    }
    else {
        current_document->get_visible_pages(window_y_range_begin, window_y_range_end, visible_pages);
    }
}

void DocumentView::move_pages(int num_pages) {
    if (!current_document) return;
    int current_page = get_center_page_number();
    if (current_page == -1) {
        current_page = 0;
    }
    move_absolute(0, num_pages * (current_document->get_page_height(current_page) + PAGE_PADDINGS));
}

void DocumentView::move_screens(int num_screens) {
    float screen_height_in_doc_space = view_height / zoom_level;
    set_offset_y(get_offset_y() + num_screens * screen_height_in_doc_space * MOVE_SCREEN_PERCENTAGE);
    //return move_amount;
}

void DocumentView::reset_doc_state() {
    zoom_level = 1.0f;
    set_offsets(0.0f, 0.0f);
    is_ruler_mode_ = false;
    presentation_page_number = {};
    cached_virtual_rects.clear();

    search_results_mutex.lock();
    cancel_search();
    search_results_mutex.unlock();

    overview_page = {};
    synctex_highlights.clear();
    handle_escape();
}

void DocumentView::open_document(const std::wstring& doc_path,
    bool* invalid_flag,
    bool load_prev_state,
    std::optional<OpenedBookState> prev_state,
    bool force_load_dimensions,
    std::string downloaded_checksum) {

    std::wstring canonical_path = get_canonical_path(doc_path);

    if (canonical_path == L"") {
        current_document = nullptr;
        return;
    }

    //if (error_code) {
    //	current_document = nullptr;
    //	return;
    //}

    //document_path = cannonical_path;


    //current_document = new Document(mupdf_context, doc_path, database);
    //current_document = document_manager->get_document(doc_path);
    current_document = document_manager->get_document(canonical_path, downloaded_checksum);
    //current_document->open();
    if (!current_document->open(invalid_flag, force_load_dimensions)) {
        current_document = nullptr;
    }

    reset_doc_state();

    if (prev_state) {
        zoom_level = prev_state.value().zoom_level;
        AbsoluteDocumentPos prev_offset;
        prev_offset.x = prev_state.value().offset_x;
        prev_offset.y = prev_state.value().offset_y;
        set_offsets(prev_offset.x, prev_offset.y);
        is_auto_resize_mode = false;
    }
    else if (load_prev_state) {

        std::optional<std::string> checksum = checksummer->get_checksum_fast(canonical_path);
        std::vector<OpenedBookState> prev_state;
        if (checksum && db_manager->select_opened_book(checksum.value(), prev_state)) {
            if (prev_state.size() > 1) {
                LOG(std::cerr << "more than one file with one path, this should not happen!" << std::endl);
            }
        }
        if (prev_state.size() > 0) {
            OpenedBookState previous_state = prev_state[0];
            zoom_level = previous_state.zoom_level;
            //offset_x = previous_state.offset_x;
            //offset_y = previous_state.offset_y;
            set_offsets(previous_state.offset_x, previous_state.offset_y);
            is_auto_resize_mode = false;
        }
        else {
            if (current_document) {
                // automatically adjust width
                fit_to_page_width();
                is_auto_resize_mode = true;
                set_offset_y(view_height / 2 / zoom_level);
            }
        }
    }
}

float DocumentView::get_page_offset(int page) {

    if (!current_document) return 0.0f;

    int max_page = current_document->num_pages() - 1;
    if (page > max_page) {
        page = max_page;
    }
    return current_document->get_accum_page_height(page);
}

void DocumentView::goto_offset_within_page(int page, float offset_y) {
    AbsoluteDocumentPos prev_offset = get_offsets();
    set_offsets(prev_offset.x, get_page_offset(page) + offset_y);
}

void DocumentView::goto_page(int page) {
    set_offset_y(get_page_offset(page) + view_height_in_document_space() / 2);
}

//void DocumentView::goto_toc_link(std::variant<PageTocLink, ChapterTocLink> toc_link) {
//	int page = -1;
//
//	if (std::holds_alternative<PageTocLink>(toc_link)) {
//		PageTocLink l = std::get<PageTocLink>(toc_link);
//		page = l.page;
//	}
//	else{
//		ChapterTocLink l = std::get<ChapterTocLink>(toc_link);
//		std::vector<int> accum_chapter_page_counts;
//		current_document->count_chapter_pages_accum(accum_chapter_page_counts);
//		page = accum_chapter_page_counts[l.chapter] + l.page;
//	}
//	set_offset_y(get_page_offset(page) + view_height_in_document_space()/2);
//}

void DocumentView::fit_to_page_height_and_width_smart() {

    int cp = get_center_page_number();
    if (cp == -1) return;

    float top_ratio, bottom_ratio;
    int normal_page_height;

    float left_ratio, right_ratio;
    int normal_page_width;
    int page_height = current_document->get_page_size_smart(false, cp, &top_ratio, &bottom_ratio, &normal_page_height);
    int page_width = current_document->get_page_size_smart(true, cp, &left_ratio, &right_ratio, &normal_page_width);

    float bottom_leftover = 1.0f - bottom_ratio;
    float right_leftover = 1.0f - right_ratio;
    float height_imbalance = top_ratio - bottom_leftover;
    float width_imbalance = left_ratio - right_leftover;

    float height_zoom_level = static_cast<float>(view_height - 20) / page_height;
    float width_zoom_level = static_cast<float>(view_width - 20) / page_width;

    float best_zoom_level = 1.0f;

    set_zoom_level(qMin(width_zoom_level, height_zoom_level), true);
    goto_offset_within_page(cp, (height_imbalance / 2.0f + 0.5f) * normal_page_height);
    set_offset_x(-width_imbalance * normal_page_width / 2.0f);
}

void DocumentView::fit_to_page_height(bool smart) {
    int cp = get_center_page_number();
    if (cp == -1) return;

    float top_ratio, bottom_ratio;
    int normal_page_height;
    int page_height = current_document->get_page_size_smart(false, cp, &top_ratio, &bottom_ratio, &normal_page_height);
    float bottom_leftover = 1.0f - bottom_ratio;
    float imbalance = top_ratio - bottom_leftover;

    if (!smart) {
        page_height = current_document->get_page_height(cp);
        imbalance = 0;
    }

    set_zoom_level(static_cast<float>(view_height - 20) / page_height, true);
    //set_offset_y(-imbalance * normal_page_height / 2.0f);
    goto_offset_within_page(cp, (imbalance / 2.0f + 0.5f) * normal_page_height);
}

void DocumentView::fit_to_page_width(bool smart, bool ratio) {
    if (!current_document) return;

    int cp = get_center_page_number();
    if (cp == -1) return;

    //int page_width = current_document->get_page_width(cp);
    if (smart) {

        if (two_page_mode) {
            int num_pages = current_document->num_pages();
            int other_page = cp;
            if (cp % 2 == 0) {
                if (cp + 1 < num_pages) {
                    other_page = cp + 1;
                }
            }
            else {
                other_page = cp - 1;
            }
            float left_left_ratio, left_right_ratio;
            int left_normal_page_width;
            float left_page_width = current_document->get_page_size_smart(true, cp, &left_left_ratio, &left_right_ratio, &left_normal_page_width);

            float right_left_ratio, right_right_ratio;
            int right_normal_page_width;
            float right_page_width = current_document->get_page_size_smart(true, other_page, &right_left_ratio, &right_right_ratio, &right_normal_page_width);

            float right_leftover = 1.0f - right_right_ratio;
            float imbalance = left_left_ratio - right_leftover;

            left_page_width += static_cast<int>((1-left_right_ratio) * left_normal_page_width);
            right_page_width += static_cast<int>(right_left_ratio * right_normal_page_width);
            page_space_x = -(left_normal_page_width * (1 - left_right_ratio) + right_normal_page_width * right_left_ratio) / 2;
            cached_virtual_rects.clear();

            set_zoom_level(static_cast<float>(view_width) / (left_page_width + right_page_width + page_space_x * 2), false);
            offset.x = -imbalance * (left_normal_page_width + right_normal_page_width + page_space_x * 2) / 4.0f;
        }

        else {
            float left_ratio, right_ratio;
            int normal_page_width;
            int page_width = current_document->get_page_size_smart(true, cp, &left_ratio, &right_ratio, &normal_page_width);
            float right_leftover = 1.0f - right_ratio;
            float imbalance = left_ratio - right_leftover;

            set_zoom_level(static_cast<float>(view_width) / page_width, false);
            set_offset_x(-imbalance * normal_page_width / 2.0f);
        }
    }
    else {
        int page_width = current_document->get_page_width(cp);
        int virtual_view_width = view_width;

        if (ratio) {
            virtual_view_width = static_cast<int>(static_cast<float>(view_width) * FIT_TO_PAGE_WIDTH_RATIO);
        }

        if (two_page_mode) {
            page_space_x = PAGE_SPACE_X;
            cached_virtual_rects.clear();
            offset.x = 0;
            page_width += page_width + page_space_x;
        }
        else {
            set_offset_x(0);
        }

        set_zoom_level(static_cast<float>(virtual_view_width) / page_width, true);
    }

}

void DocumentView::fit_to_page_height_width_minimum(int statusbar_height) {
    int cp = get_center_page_number();
    if (cp == -1) return;

    int page_width = current_document->get_page_width(cp);
    int page_height = current_document->get_page_height(cp);

    float x_zoom_level = static_cast<float>(view_width) / page_width;
    float y_zoom_level;
    y_zoom_level = (static_cast<float>(view_height) - statusbar_height) / page_height;

    set_offset_x(0);
    set_zoom_level(std::min(x_zoom_level, y_zoom_level), true);

}

void DocumentView::fit_overview_width() {

    if (overview_page) {
        AbsoluteDocumentPos overview_abspos = { overview_page->absolute_offset_x, overview_page->absolute_offset_y };
        DocumentPos overview_docpos = overview_abspos.to_document(current_document);
        float left_ratio = 0, right_ratio = 0;
        int normal_page_width;
        float overview_page_width = current_document->get_page_size_smart(true, overview_docpos.page, &left_ratio, &right_ratio, &normal_page_width);
        float overview_width_in_pixels = overview_half_width * view_width;
        float overview_zoom_level = overview_width_in_pixels / overview_page_width;
        if (overview_zoom_level > 10) {
            overview_zoom_level = 10;
        }
        overview_page->absolute_offset_x = normal_page_width / 2 -
            (left_ratio * normal_page_width + overview_width_in_pixels / overview_zoom_level / 2);
        overview_page->zoom_level = overview_zoom_level;

    }
}

void DocumentView::persist(bool persist_drawings) {
    if (!current_document) return;
    AbsoluteDocumentPos abs_offset = get_offsets();

    if (two_page_mode) {
        abs_offset.x = 0;
    }

    db_manager->update_book(current_document->get_checksum(), current_document->get_is_synced(), zoom_level, abs_offset.x, abs_offset.y, current_document->detect_paper_name());
    if (persist_drawings) {
        current_document->persist_drawings();
        current_document->persist_annotations();
        current_document->persist_extras();
    }
}

int DocumentView::get_current_chapter_index() {
    const std::vector<int>& chapter_pages = current_document->get_flat_toc_pages();

    if (chapter_pages.size() == 0) {
        return -1;
    }

    int cp = get_center_page_number();

    int current_chapter_index = 0;

    int index = 0;
    for (int p : chapter_pages) {
        if (p <= cp) {
            current_chapter_index = index;
        }
        index++;
    }

    return current_chapter_index;
}

std::wstring DocumentView::get_current_chapter_name() {
    const std::vector<std::wstring>& chapter_names = current_document->get_flat_toc_names();
    int current_chapter_index = get_current_chapter_index();
    if (current_chapter_index > 0) {
        return chapter_names[current_chapter_index];
    }
    return L"";
}

std::optional<std::pair<int, int>> DocumentView::get_current_page_range() {
    int ci = get_current_chapter_index();
    if (ci < 0) {
        return {};
    }
    const std::vector<int>& chapter_pages = current_document->get_flat_toc_pages();
    int range_begin = chapter_pages[ci];
    int range_end = current_document->num_pages() - 1;

    if ((size_t)ci < chapter_pages.size() - 1) {
        range_end = chapter_pages[ci + 1];
    }

    return std::make_pair(range_begin, range_end);
}

void DocumentView::get_page_chapter_index(int page, std::vector<TocNode*> nodes, std::vector<int>& res) {


    for (size_t i = 0; i < nodes.size(); i++) {
        if ((i == nodes.size() - 1) && (nodes[i]->page <= page)) {
            res.push_back(i);
            get_page_chapter_index(page, nodes[i]->children, res);
            return;
        }
        else {
            if ((nodes[i]->page <= page) && (nodes[i + 1]->page > page)) {
                res.push_back(i);
                get_page_chapter_index(page, nodes[i]->children, res);
                return;
            }
        }
    }

}
std::vector<int> DocumentView::get_current_chapter_recursive_index() {
    int curr_page = get_center_page_number();
    std::vector<TocNode*> nodes = current_document->get_toc();
    std::vector<int> res;
    get_page_chapter_index(curr_page, nodes, res);
    return res;
}

void DocumentView::goto_chapter(int diff) {
    const std::vector<int>& chapter_pages = current_document->get_flat_toc_pages();
    int curr_page = get_center_page_number();

    int index = 0;

    while ((size_t)index < chapter_pages.size() && chapter_pages[index] < curr_page) {
        index++;
    }

    if (index < chapter_pages.size() && chapter_pages[index] > curr_page) {
        index--;
    }

    int new_index = index + diff;
    if (new_index < 0) {
        goto_page(0);
    }
    else if ((size_t)new_index >= chapter_pages.size()) {
        goto_end();
    }
    else {
        goto_page(chapter_pages[new_index]);
    }
}

float DocumentView::view_height_in_document_space() {
    return static_cast<float>(view_height) / zoom_level;
}

void DocumentView::set_vertical_line_pos(float pos) {
    ruler_pos = pos;
    ruler_rect = {};
    is_ruler_mode_ = true;
}

float DocumentView::get_ruler_pos() {
    if (ruler_rect.has_value()) {
        return ruler_rect->y1;
    }
    else {
        return ruler_pos;
    }
}

std::optional<AbsoluteRect> DocumentView::get_ruler_rect() {
    return ruler_rect;
}

bool DocumentView::has_ruler_rect() {
    return ruler_rect.has_value();
}

float DocumentView::get_ruler_window_y() {

    float absol_end_y = get_ruler_pos();

    absol_end_y += RULER_PADDING;

    return absolute_to_window_pos({ 0.0f, absol_end_y }).y;
}

std::optional<NormalizedWindowRect> DocumentView::get_ruler_window_rect() {
    if (has_ruler_rect()) {
        AbsoluteRect absol_ruler_rect = get_ruler_rect().value();

        absol_ruler_rect.y0 -= RULER_PADDING;
        absol_ruler_rect.y1 += RULER_PADDING;

        absol_ruler_rect.x0 -= RULER_X_PADDING;
        absol_ruler_rect.x1 += RULER_X_PADDING;
        return NormalizedWindowRect(absolute_to_window_rect(absol_ruler_rect));
    }
    return {};
}

//float DocumentView::get_vertical_line_window_y() {
//
//	float absol_end_y = get_vertical_line_pos();
//
//	absol_end_y += RULER_PADDING;
//
//	float window_begin_x, window_begin_y;
//	float window_end_x, window_end_y;
//	absolute_to_window_pos(0.0, absol_end_y, &window_end_x, &window_end_y);
//
//	return window_end_y;
//}

void DocumentView::goto_vertical_line_pos() {
    if (current_document) {
        //float new_y_offset = vertical_line_pos;
        float new_y_offset = get_ruler_pos();
        set_offset_y(new_y_offset);
        is_ruler_mode_ = true;
    }
}

void DocumentView::get_text_selection(AbsoluteDocumentPos selection_begin,
    AbsoluteDocumentPos selection_end,
    bool is_word_selection, // when in word select mode, we select entire words even if the range only partially includes the word
    std::deque<AbsoluteRect>& selected_characters,
    std::wstring& selected_text) {

    if (current_document) {
        current_document->get_text_selection(selection_begin, selection_end, is_word_selection, selected_characters, selected_text);
    }

}

int DocumentView::get_page_offset() {
    return current_document->get_page_offset();
}

void DocumentView::set_page_offset(int new_offset) {
    current_document->set_page_offset(new_offset);
}

float DocumentView::get_max_valid_x(bool relenting) {
    float page_width = current_document->get_page_width(get_center_page_number());
    if (!relenting){
        return std::abs(-view_width / zoom_level / 2 + page_width / 2);
    }
    else{
        return std::abs(-view_width / zoom_level / 2 + 3 * page_width / 2);
    }
}

float DocumentView::get_min_valid_x(bool relenting) {
    float page_width = current_document->get_page_width(get_center_page_number());
    if (!relenting){
        return -std::abs(-view_width / zoom_level / 2 + page_width / 2);
    }
    else{
        return -std::abs(-view_width / zoom_level / 2 + 3 * page_width / 2);
    }
}

void DocumentView::rotate() {
    int current_page = get_center_page_number();

    current_document->rotate();
    float new_offset = current_document->get_accum_page_height(current_page) + current_document->get_page_height(current_page) / 2;
    set_offset_y(new_offset);
}

void DocumentView::goto_top_of_page() {
    int current_page = get_center_page_number();
    float offset_y = get_document()->get_accum_page_height(current_page) + static_cast<float>(view_height) / 2.0f / zoom_level;
    set_offset_y(offset_y);
}

void DocumentView::goto_bottom_of_page() {
    int current_page = get_center_page_number();
    float offset_y = get_document()->get_accum_page_height(current_page + 1) - static_cast<float>(view_height) / 2.0f / zoom_level;

    if (current_page + 1 == current_document->num_pages()) {
        offset_y = get_document()->get_accum_page_height(current_page) + get_document()->get_page_height(current_page) - static_cast<float>(view_height) / 2.0f / zoom_level;
    }
    set_offset_y(offset_y);
}

int DocumentView::get_line_index() {
    if (!ruler_line_index.has_value()) {
        return get_line_index_of_vertical_pos();
    }
    else {
        return ruler_line_index->merged_index;
    }
}

//void DocumentView::set_ruler_rect(AbsoluteRect rect, bool after){
//    int last_index = line_index;
//    ruler_rect = rect;
//    int page = rect.to_document(current_document).page;
//    auto lines = get_document()->get_page_lines(page).merged_line_rects;
//    float min_distance = 100000;
//    int min_index = -1;
//
//    for (int i = 0; i < lines.size(); i++) {
//        if (after && i <= last_index) continue;
//        if (!after && i >= last_index) continue;
//
//        //float distance = lines[i].distance(rect);
//        float distance = rect.distance(lines[i]);
//        if (distance < min_distance) {
//            min_distance = distance;
//            min_index = i;
//        }
//    }
//    if (min_index >= 0) {
//        line_index = min_index;
//        set_line_index(line_index, page);
//    }
//
//}

void DocumentView::set_line_index(int index, int page) {
    RulerLineIndexInfo index_info;
    index_info.merged_index = index;
    is_ruler_mode_ = true;
    if (page >= 0) {
        PageMergedLinesInfoAbsolute lines = get_document()->get_page_lines(page);
        if (index >= 0 && index < lines.merged_line_rects.size()) {
            ruler_rect = lines.merged_line_rects[index];
            index_info.unmerged_indices = lines.merged_line_indices[index];
            ruler_line_index = index_info;
        }
    }

    if (line_select_mode) {
        select_ruler_text();
    }

}

int DocumentView::get_line_index_of_vertical_pos() {
    DocumentPos line_doc_pos = current_document->absolute_to_page_pos_uncentered({ 0, get_ruler_pos() });
    auto rects = current_document->get_page_lines(line_doc_pos.page).merged_line_rects;
    int index = 0;
    while ((size_t)index < rects.size() && rects[index].y0 < get_ruler_pos()) {
        index++;
    }
    return index - 1;
}

int DocumentView::get_line_index_of_pos(DocumentPos line_doc_pos) {
    AbsoluteDocumentPos line_abs_pos = line_doc_pos.to_absolute(current_document);
    auto rects = current_document->get_page_lines(line_doc_pos.page).merged_line_rects;
    int page_width = current_document->get_page_width(line_doc_pos.page);

    for (int i = 0; i < rects.size(); i++) {
        if (rects[i].contains(line_abs_pos)) return i;
    }
    return -1;
}

int DocumentView::get_vertical_line_page() {
    return current_document->absolute_to_page_pos({ 0, get_ruler_pos() }).page;
}

std::optional<std::wstring> DocumentView::get_selected_line_text() {
    if (ruler_line_index.has_value()) {
        const int line_index = ruler_line_index->merged_index;
        //std::vector<std::wstring> lines;
        const PageMergedLinesInfoAbsolute& line_info = current_document->get_page_lines(get_vertical_line_page());
        auto line_rects = line_info.merged_line_rects;
        auto lines = line_info.merged_line_texts;
        //std::vector<AbsoluteRect> line_rects = current_document->get_page_lines(get_vertical_line_page(), &lines);
        if ((size_t)line_index < lines.size()) {
            std::wstring content = lines[line_index];
            return content;
        }
        else {
            return {};
        }
    }
    return {};
}

void DocumentView::get_rects_from_ranges(int page_number, const std::vector<PagelessDocumentRect>& line_char_rects, const std::vector<std::pair<int, int>>& ranges, std::vector<PagelessDocumentRect>& out_rects) {
    for (int i = 0; i < ranges.size(); i++) {
        auto [first, last] = ranges[i];
        PagelessDocumentRect current_source_rect = get_range_rect_union(line_char_rects, first, last);
        current_source_rect = current_document->document_to_absolute_rect(DocumentRect(current_source_rect, page_number));
        out_rects.push_back(current_source_rect);
    }
}

std::vector<SmartViewCandidate> DocumentView::find_line_definitions() {
    //todo: remove duplicate code from this function, this just needs to find the location of the
    // reference, the rest can be handled by find_definition_of_location

    std::vector<SmartViewCandidate> result;

    if (ruler_line_index.has_value()) {
        const int line_index = ruler_line_index->merged_index;
        //std::vector<std::wstring> lines;
        //std::vector<std::vector<PagelessDocumentRect>> line_char_rects;

        int line_page_number = get_vertical_line_page();

        const PageMergedLinesInfoAbsolute& line_info = current_document->get_page_lines(line_page_number);
        auto lines = line_info.merged_line_texts;
        auto line_char_rects = line_info.merged_line_chars;
        auto line_rects = line_info.merged_line_rects;
        //std::vector<AbsoluteRect> line_rects = current_document->get_page_lines(line_page_number, &lines, &line_char_rects);
        for (int i = 0; i < lines.size(); i++) {
            assert(lines[i].size() == line_char_rects[i].size());
        }
        if ((size_t)line_index < lines.size()) {
            std::wstring content = lines[line_index];

            //todo: deduplicate this code

            AbsoluteRect line_rect = line_rects[line_index];
            float mid_y = (line_rect.y1 + line_rect.y0) / 2.0f;
            line_rect.y0 = line_rect.y0 = mid_y;

            std::vector<PdfLink> pdf_links = current_document->get_links_in_page_rect(get_vertical_line_page(), line_rect);
            if (pdf_links.size() > 0) {

                for (auto link : pdf_links) {
                    auto parsed_uri = parse_uri(get_document()->get_mupdf_context(), get_document()->doc, link.uri);
                    PdfLinkTextInfo link_info = get_document()->get_pdf_link_text(link);

                    SmartViewCandidate candid;
                    candid.doc = get_document();
                    candid.source_rect = current_document->document_to_absolute_rect(DocumentRect(link.rects[0], line_page_number));
                    candid.source_text = link_info.link_text;
                    candid.target_pos = DocumentPos{ parsed_uri.page - 1, parsed_uri.x, parsed_uri.y };
                    candid.reference_type = ReferenceType::Link;

                    //is_reference
                    bool is_reference = is_link_a_reference(link, link_info);
                    if (is_reference) {
                        candid.highlight_rects = get_reference_link_highlights(parsed_uri.page - 1, link, link_info);
                        candid.reference_type = ReferenceType::RefLink;
                    }

                    result.push_back(candid);
                }

                return result;
            }

            std::wstring item_regex(L"[a-zA-Z]{2,}\\.?[ \t]+[0-9]+(\.[0-9]+)*");
            std::wstring reference_regex(L"\\[[a-zA-Z0-9, ]+\\]");
            std::wstring equation_regex(L"\\([0-9]+(\\.[0-9]+)*\\)");

            std::vector<std::pair<int, int>> generic_item_ranges;
            std::vector<std::pair<int, int>> reference_ranges;
            std::vector<std::pair<int, int>> equation_ranges;

            std::vector<std::wstring> generic_item_texts = find_all_regex_matches(content, item_regex, &generic_item_ranges);
            std::vector<std::wstring> reference_texts = find_all_regex_matches(content, reference_regex, &reference_ranges);
            std::vector<std::wstring> equation_texts = find_all_regex_matches(content, equation_regex, &equation_ranges);

            std::vector<PagelessDocumentRect> generic_item_rects;
            std::vector<PagelessDocumentRect> reference_rects;
            std::vector<PagelessDocumentRect> equation_rects;

            int ruler_page = get_vertical_line_page();
            get_rects_from_ranges(ruler_page, line_char_rects[line_index], generic_item_ranges, generic_item_rects);
            get_rects_from_ranges(ruler_page, line_char_rects[line_index], reference_ranges, reference_rects);
            get_rects_from_ranges(ruler_page, line_char_rects[line_index], equation_ranges, equation_rects);

            std::vector<SmartViewCandidate> generic_positions;
            std::vector<SmartViewCandidate> reference_positions;
            std::vector<SmartViewCandidate> equation_positions;

            for (int i = 0; i < generic_item_texts.size(); i++) {
                std::vector<IndexedData> possible_targets = current_document->find_generic_with_string(generic_item_texts[i], ruler_page);
                for (int j = 0; j < possible_targets.size(); j++) {
                    SmartViewCandidate candid;
                    candid.doc = get_document();
                    candid.source_rect = generic_item_rects[i];
                    candid.source_text = generic_item_texts[i];
                    candid.target_pos = DocumentPos{ possible_targets[j].page, 0, possible_targets[j].y_offset };
                    candid.reference_type = ReferenceType::Generic;
                    generic_positions.push_back(candid);
                    //generic_positions.push_back(
                    //    std::make_pair(DocumentPos{ possible_targets[j].page, 0, possible_targets[j].y_offset },
                    //        generic_item_rects[i])
                    //);
                }
            }
            for (int i = 0; i < reference_texts.size(); i++) {
                if (reference_texts[i].find(L",") != -1) {
                    // remove [ and ]
                    QString references_string = QString::fromStdWString(reference_texts[i].substr(1, reference_texts[i].size()-2));
                    QStringList parts = references_string.split(',');
                    int n_chars_seen = 1;
                    for (int j = 0; j < parts.size(); j++) {
                        auto index = current_document->find_reference_with_string(parts[j].trimmed().toStdWString(), ruler_page);

                        // range of the substring
                        int rect_range_begin = reference_ranges[i].first + n_chars_seen;
                        int rect_range_end = rect_range_begin + parts[j].size();


                        if (index.size() > 0) {
                            std::vector<PagelessDocumentRect> subrects;
                            get_rects_from_ranges(ruler_page, line_char_rects[line_index], {std::make_pair(rect_range_begin, rect_range_end)}, subrects);

                            SmartViewCandidate candid;
                            candid.doc = get_document();
                            candid.source_rect = subrects[0];
                            candid.source_text = parts[j].trimmed().toStdWString();
                            candid.target_pos = DocumentPos{ index[0].page, 0, index[0].y_offset };
                            candid.reference_type = ReferenceType::Reference;
                            fill_smart_view_candidate_reference_highlight_rects(candid);
                            reference_positions.push_back(candid);
                        }
                        n_chars_seen += parts[j].size() + 1;
                    }
                }
                auto index = current_document->find_reference_with_string(reference_texts[i], ruler_page);

                if (index.size() > 0) {

                    SmartViewCandidate candid;
                    candid.doc = get_document();
                    candid.source_rect = reference_rects[i];
                    candid.source_text = reference_texts[i];
                    candid.target_pos = DocumentPos{ index[0].page, 0, index[0].y_offset };
                    candid.reference_type = ReferenceType::Reference;
                    fill_smart_view_candidate_reference_highlight_rects(candid);
                    reference_positions.push_back(candid);
                    //reference_positions.push_back(
                    //    std::make_pair(
                    //        DocumentPos{ index[0].page, 0, index[0].y_offset },
                    //        reference_rects[i])
                    //);
                }
            }
            //for (auto equation_text : equation_texts) {
            for (int i = 0; i < equation_texts.size(); i++) {
                auto index = current_document->find_equation_with_string(equation_texts[i], get_vertical_line_page());

                if (index.size() > 0) {
                    SmartViewCandidate candid;
                    candid.doc = get_document();
                    candid.source_rect = equation_rects[i];
                    candid.source_text = equation_texts[i];
                    candid.target_pos = DocumentPos{ index[0].page, 0, index[0].y_offset };
                    candid.reference_type = ReferenceType::Equation;
                    equation_positions.push_back(candid);
                    //equation_positions.push_back(
                    //    std::make_pair(
                    //        DocumentPos { index[0].page, 0, index[0].y_offset },
                    //        equation_rects[i]
                    //    )
                    //);
                }
            }

            std::vector<std::vector<SmartViewCandidate>*> res_vectors = { &equation_positions, &reference_positions, &generic_positions };
            int index = 0;
            int max_size = 0;

            for (auto vec : res_vectors) {
                if (vec->size() > max_size) {
                    max_size = vec->size();
                }
            }
            // interleave the results
            for (int i = 0; i < max_size; i++) {
                for (auto vec : res_vectors) {
                    if (i < vec->size()) {
                        result.push_back(vec->at(i));
                    }
                }
            }

            return result;
        }
    }
    return result;
}

bool DocumentView::goto_definition() {
    std::vector<SmartViewCandidate> defloc = find_line_definitions();
    if (defloc.size() > 0) {
        //goto_offset_within_page(defloc[0].first.page, defloc[0].first.y);
        DocumentPos docpos = defloc[0].get_docpos(this);
        goto_offset_within_page(docpos.page, docpos.y);
        return true;
    }
    return false;
}

bool DocumentView::get_is_auto_resize_mode() {
    return is_auto_resize_mode;
}

void DocumentView::disable_auto_resize_mode() {
    this->is_auto_resize_mode = false;
}

void DocumentView::readjust_to_screen() {
    this->set_offsets(this->get_offset_x(), this->get_offset_y());
}

float DocumentView::get_half_screen_offset() {
    return (static_cast<float>(view_height) / 2.0f);
}

void DocumentView::scroll_mid_to_top() {
    float offset = get_half_screen_offset();
    move(0, offset);
}

void DocumentView::get_visible_links(std::vector<PdfLink>& visible_page_links) {

    std::vector<int> visible_pages;
    get_visible_pages(get_view_height(), visible_pages);
    for (auto page : visible_pages) {
        std::vector<PdfLink> links = get_document()->get_page_merged_pdf_links(page);
        for (auto link : links) {
            ParsedUri parsed_uri = parse_uri(get_document()->get_mupdf_context(), get_document()->doc, link.uri);
            bool is_visible = false;
            for (auto& pageless_rect : link.rects) {
                NormalizedWindowRect window_rect = DocumentRect(pageless_rect, page).to_window_normalized(this);
                if (window_rect.is_visible()) {
                    is_visible = true;
                    break;
                }
            }

            if (is_visible) {
                visible_page_links.push_back(link);
            }
        }
    }
}

std::deque<AbsoluteRect>* DocumentView::get_selected_character_rects() {
    return &this->selected_character_rects;
}

std::optional<AbsoluteRect> DocumentView::get_control_rect() {
    if (selected_character_rects.size() > 0) {
        if (mark_end) {
            return selected_character_rects[selected_character_rects.size() - 1];
        }
        else {
            return selected_character_rects[0];
        }
    }
    return {};
}

std::optional<AbsoluteRect> DocumentView::shrink_selection(bool is_begin, bool word) {
    if (selected_character_rects.size() > 1) {
        if (word) {
            int page;
            int index = is_begin ? 0 : selected_character_rects.size() - 1;
            DocumentRect page_rect = selected_character_rects[index].to_document(current_document);
            if (page >= 0) {
                fz_stext_page* stext_page = current_document->get_stext_with_page_number(page);
                std::optional<DocumentRect> new_rect_ = find_shrinking_rect_word(is_begin, stext_page, page_rect);
                if (new_rect_) {
                    AbsoluteRect new_rect = new_rect_->to_absolute(current_document);

                    if (is_begin) {
                        while (!are_rects_same(new_rect, selected_character_rects[0])) {
                            selected_character_rects.pop_front();
                            if (selected_character_rects.size() == 1) break;
                        }
                        return selected_character_rects[0];
                    }
                    else {
                        while (!are_rects_same(new_rect, selected_character_rects[selected_character_rects.size() - 1])) {
                            selected_character_rects.pop_back();
                            if (selected_character_rects.size() == 1) break;
                        }
                        return selected_character_rects[selected_character_rects.size() - 1];
                    }

                }
                return {};
            }

        }
        else {
            if (is_begin) {
                selected_character_rects.pop_front();
                return selected_character_rects[0];
            }
            else {
                selected_character_rects.pop_back();
                return selected_character_rects[selected_character_rects.size() - 1];
            }
        }
    }

    return {};
}

std::optional<AbsoluteRect> DocumentView::expand_selection(bool is_begin, bool word) {
    //current_document->get_stext_with_page_number()
    if (selected_character_rects.size() > 0) {
        int index = is_begin ? 0 : selected_character_rects.size() - 1;

        DocumentRect page_rect = selected_character_rects[index].to_document(current_document);

        if (page_rect.page >= 0) {
            fz_stext_page* stext_page = current_document->get_stext_with_page_number(page_rect.page);
            std::optional<DocumentRect> next_rect = {};
            if (word) {
                std::vector<DocumentRect> next_rects_document = find_expanding_rect_word(is_begin, stext_page, page_rect);
                std::vector<AbsoluteRect> next_rects;
                for (auto dr : next_rects_document) {
                    next_rects.push_back(dr.to_absolute(current_document));
                }
                if (is_begin) {
                    for (int i = 0; i < next_rects.size(); i++) {
                        selected_character_rects.push_front(next_rects[i]);
                    }
                    return selected_character_rects[0];
                }
                else {
                    for (int i = 0; i < next_rects.size(); i++) {
                        selected_character_rects.push_back(next_rects[i]);
                    }
                    return selected_character_rects[selected_character_rects.size() - 1];
                }
            }
            else {
                next_rect = find_expanding_rect(is_begin, stext_page, page_rect);
            }
            if (next_rect) {
                AbsoluteRect next_rect_abs = next_rect->to_absolute(current_document);
                if (is_begin) {
                    selected_character_rects.push_front(next_rect_abs);
                }
                else {
                    selected_character_rects.push_back(next_rect_abs);
                }
                return next_rect_abs;
            }
        }
    }
    return {};
}
void DocumentView::set_text_mark(bool is_begin) {
    if (is_begin) {
        mark_end = false;
    }
    else {
        mark_end = true;
    }
}

void DocumentView::toggle_text_mark() {
    set_text_mark(mark_end);
}


WindowPos DocumentView::normalized_window_to_window_pos(NormalizedWindowPos nwp) {
    int window_x0 = static_cast<int>(nwp.x * view_width / 2 + view_width / 2);
    int window_y0 = static_cast<int>(-nwp.y * view_height / 2 + view_height / 2);
    return { window_x0, window_y0 };
}

WindowRect DocumentView::normalized_to_window_rect(NormalizedWindowRect normalized_rect) {
    return WindowRect(normalized_rect.top_left().to_window(this), normalized_rect.bottom_right().to_window(this));
}

QRect DocumentView::normalized_to_window_qrect(NormalizedWindowRect normalized_rect) {
    auto window_rect = normalized_to_window_rect(normalized_rect);
    return QRect(window_rect.x0, window_rect.y0, window_rect.width(), window_rect.height());
}

bool DocumentView::is_ruler_mode() {
    return is_ruler_mode_;
}

Document* SmartViewCandidate::get_document(DocumentView* view) {
    if (doc) return doc;
    return view->get_document();

}

DocumentPos SmartViewCandidate::get_docpos(DocumentView* view) {
    if (std::holds_alternative<DocumentPos>(target_pos)) {
        return std::get<DocumentPos>(target_pos);
    }
    else {
        return get_document(view)->absolute_to_page_pos_uncentered(std::get<AbsoluteDocumentPos>(target_pos));
    }
}

AbsoluteDocumentPos SmartViewCandidate::get_abspos(DocumentView* view) {
    if (std::holds_alternative<AbsoluteDocumentPos>(target_pos)) {
        return std::get<AbsoluteDocumentPos>(target_pos);
    }
    else {
        return get_document(view)->document_to_absolute_pos(std::get<DocumentPos>(target_pos));
    }
}

ScratchPad::ScratchPad() : DocumentView(nullptr, nullptr, nullptr) {
    zoom_level = 1;
}

bool ScratchPad::set_offsets(float new_offset_x, float new_offset_y, bool force) {
    offset.x = new_offset_x;
    offset.y = new_offset_y;
    return false;
}

float ScratchPad::set_zoom_level(float zl, bool should_exit_auto_resize_mode) {
    float min_zoom_level = 0.1f;
    float max_zoom_level = 1000.0f;

    if (zl < min_zoom_level) {
        zl = min_zoom_level;
    }
    if (zl > max_zoom_level) {
        zl = max_zoom_level;
    }

    zoom_level = zl;
    return zoom_level;
}

float ScratchPad::zoom_in(float zoom_factor) {
    return set_zoom_level(zoom_level * zoom_factor, true);
}

float ScratchPad::zoom_out(float zoom_factor) {
    return set_zoom_level(zoom_level / zoom_factor, true);
}

std::vector<int> ScratchPad::get_intersecting_drawing_indices(AbsoluteRect selection) {
    invalidate_compile();

    std::vector<int> res;

    for (int i = 0; i < all_drawings.size(); i++) {
        for (auto p : all_drawings[i].points) {
            if (selection.contains(p.pos)) {
                res.push_back(i);
                break;
            }
        }
    }
    return res;
}

void ScratchPad::delete_intersecting_drawings(AbsoluteRect selection) {
    invalidate_compile(true);

    std::vector<int> indices = get_intersecting_drawing_indices(selection);
    for (int i = 0; i < indices.size(); i++) {
        all_drawings.erase(all_drawings.begin() + indices[indices.size() - 1 - i]);
    }
}

std::vector<int> ScratchPad::get_intersecting_pixmap_indices(AbsoluteRect selection) {
    std::vector<int> res;
    for (int i = 0; i < pixmaps.size(); i++) {
        if (selection.intersects(pixmaps[i].rect)) {
            res.push_back(i);
        }
    }
    return res;
}

void ScratchPad::delete_intersecting_pixmaps(AbsoluteRect selection) {
    std::vector<int> indices = get_intersecting_pixmap_indices(selection);
    for (int i = 0; i < indices.size(); i++) {
        pixmaps.erase(pixmaps.begin() + indices[indices.size() - 1 - i]);
    }
}

void ScratchPad::delete_intersecting_objects(AbsoluteRect selection) {
    delete_intersecting_drawings(selection);
    delete_intersecting_pixmaps(selection);
}

void ScratchPad::get_selected_objects_with_indices(const std::vector<SelectedObjectIndex>&indices, std::vector<FreehandDrawing>&freehand_drawings, std::vector<PixmapDrawing>&pixmap_drawings){

    for (auto [index, type] : indices) {
        if (type == SelectedObjectType::Drawing) {
            freehand_drawings.push_back(all_drawings[index]);
        }
        else if (type == SelectedObjectType::Pixmap) {
            pixmap_drawings.push_back(pixmaps[index]);
        }
    }
}

void ScratchPad::add_pixmap(QPixmap pixmap) {
    AbsoluteDocumentPos top_abs_pos = get_bounding_box().bottom_right();
    int top_window_pos = top_abs_pos.to_window(this).y;

    float dpr = pixmap.devicePixelRatio();
    int pixmap_width = pixmap.width() / dpr;
    int pixmap_height = pixmap.height() / dpr;

    int pixmap_window_left = view_width / 2 - pixmap_width / 2;
    int pixmap_window_right = view_width / 2 + pixmap_width / 2;
    int pixmap_window_top = top_window_pos;
    int pixmap_window_bottom = top_window_pos + pixmap_height;

    WindowPos top_left = { pixmap_window_left, pixmap_window_top };
    WindowPos bottom_right = { pixmap_window_right, pixmap_window_bottom };

    AbsoluteDocumentPos top_left_abs = top_left.to_absolute(this);
    AbsoluteDocumentPos bottom_right_abs = bottom_right.to_absolute(this);
    AbsoluteRect pixmap_rect = AbsoluteRect(top_left_abs, bottom_right_abs);
    AbsoluteDocumentPos center_pos = pixmap_rect.center();
    offset.x = center_pos.x;
    offset.y = center_pos.y;

    pixmaps.push_back(PixmapDrawing{ pixmap, pixmap_rect });

}

AbsoluteRect ScratchPad::get_bounding_box() {
    AbsoluteRect res;
    res.x0 = res.x1 = res.y0 = res.y1 = 0;

    if (all_drawings.size() > 0) {
        res = all_drawings[0].bbox();
    }
    else if (pixmaps.size() > 0) {
        res = pixmaps[0].rect;
    }
    else {
        return res;
    }

    for (auto drawing : all_drawings) {
        res = res.union_rect(drawing.bbox());
    }
    for (auto [_, pixmap_rect] : pixmaps) {
        res = res.union_rect(pixmap_rect);
    }

    return res;
}

std::vector<SelectedObjectIndex> ScratchPad::get_intersecting_objects(AbsoluteRect selection) {
    std::vector<SelectedObjectIndex> selected_objects;

    std::vector<int> drawing_indices = get_intersecting_drawing_indices(selection);
    std::vector<int> pixmap_indices = get_intersecting_pixmap_indices(selection);

    for (auto index : drawing_indices) {
        selected_objects.push_back(SelectedObjectIndex{ index, SelectedObjectType::Drawing });
    }
    for (auto index : pixmap_indices) {
        selected_objects.push_back(SelectedObjectIndex{ index, SelectedObjectType::Pixmap });
    }

    return selected_objects;
}

const std::vector<FreehandDrawing>& ScratchPad::get_all_drawings() {
    return all_drawings;
}

const std::vector<FreehandDrawing>& ScratchPad::get_non_compiled_drawings() {
    return non_compiled_drawings;
}

void ScratchPad::on_compile() {
    is_compile_valid = true;
    non_compiled_drawings.clear();
}

void ScratchPad::invalidate_compile(bool force) {

    if (non_compiled_drawings.size() > 0) {
        is_compile_valid = false;
    }
    if (force) {
        is_compile_valid = false;
    }
}

void ScratchPad::add_drawing(FreehandDrawing drawing) {
    all_drawings.push_back(drawing);
    non_compiled_drawings.push_back(drawing);
}

void ScratchPad::clear() {
    all_drawings.clear();
    pixmaps.clear();
    non_compiled_drawings.clear();
    is_compile_valid = false;
}

bool ScratchPad::is_compile_invalid() {
#ifdef SIOYEK_OPENGL_BACKEND
    return !is_compile_valid;
#else
    if (!is_compile_valid) return true;
    if (cached_pixmap) {
        if (cached_pixmap->zoom_level != zoom_level) return true;
        if (cached_pixmap->offset_x != offset.x) return true;
        if (cached_pixmap->offset_y != offset.y) return true;
    }
    return false;
#endif
}

std::vector<int> DocumentView::get_visible_bookmark_indices() {
    const std::vector<BookMark>& bookmarks = get_document()->get_bookmarks();
    std::vector<int> res;
    for (int i = 0; i < bookmarks.size(); i++) {
        if (bookmarks[i].is_marked() || bookmarks[i].is_freetext()) {
            if (bookmarks[i].get_rectangle().to_window_normalized(this).is_visible()) {
                res.push_back(i);
            }
        }
    }
    return res;
}

std::vector<int> DocumentView::get_visible_portal_indices() {
    const std::vector<Portal>& portals = get_document()->get_portals();
    std::vector<int> res;
    for (int i = 0; i < portals.size(); i++) {
        if (portals[i].is_visible() && portals[i].get_rectangle().to_window_normalized(this).is_visible()) {
            res.push_back(i);
        }
    }
    return res;
}

std::vector<VisibleObjectIndex> DocumentView::get_generic_visible_item_indices() {
    std::vector<int> visible_highlight_indices = get_visible_highlight_indices();
    std::vector<int> visible_bookmark_indices = get_visible_bookmark_indices();
    std::vector<int> visible_portal_indices = get_visible_portal_indices();

    std::vector<VisibleObjectIndex> res;
    for (auto index : visible_highlight_indices) {
        res.push_back(VisibleObjectIndex{ VisibleObjectType::Highlight, index });
    }

    for (auto index : visible_bookmark_indices) {
        res.push_back(VisibleObjectIndex{ VisibleObjectType::Bookmark, index });
    }

    for (auto index : visible_portal_indices) {
        res.push_back(VisibleObjectIndex{ VisibleObjectType::Portal, index });
    }

    return res;

}

std::vector<int> DocumentView::get_visible_highlight_indices() {

    const std::vector<Highlight>& highlights = get_document()->get_highlights();

    std::vector<int> res;

    for (size_t i = 0; i < highlights.size(); i++) {

        NormalizedWindowPos selection_begin_window_pos = absolute_to_window_pos(
            { highlights[i].selection_begin.x, highlights[i].selection_begin.y }
        );

        NormalizedWindowPos selection_end_window_pos = absolute_to_window_pos(
            { highlights[i].selection_end.x, highlights[i].selection_end.y }
        );

        if (selection_begin_window_pos.y > selection_end_window_pos.y) {
            std::swap(selection_begin_window_pos.y, selection_end_window_pos.y);
        }
        if (range_intersects(selection_begin_window_pos.y, selection_end_window_pos.y, -1.0f, 1.0f)) {
            res.push_back(i);
        }
    }

    return res;
}

void DocumentView::set_presentation_page_number(std::optional<int> page) {
    presentation_page_number = page;
}

std::optional<int> DocumentView::get_presentation_page_number() {
    return presentation_page_number;
}

bool DocumentView::is_presentation_mode() {
    return presentation_page_number.has_value();
}

VirtualPos DocumentView::absolute_to_virtual_pos(const AbsoluteDocumentPos& abspos) {
    if (fast_coordinates()) {
        return VirtualPos{ abspos.x, abspos.y };
    }

    fill_cached_virtual_rects();
    if (cached_virtual_rects.size() == 0) {
        return VirtualPos{ abspos.x, abspos.y };
    }

    DocumentPos docpos = abspos.to_document(current_document);
    return document_to_virtual_pos(docpos);
}

VirtualPos DocumentView::document_to_virtual_pos(DocumentPos docpos) {
    if (fast_coordinates()) {
        AbsoluteDocumentPos abspos = docpos.to_absolute(current_document);
        return { abspos.x, abspos.y };
    }
    else {
        VirtualRect page_virtual_rect = cached_virtual_rects[docpos.page];

        VirtualPos pos = page_virtual_rect.top_left();
        pos.x += docpos.x;
        pos.y += docpos.y;
        return pos;
    }
}

AbsoluteDocumentPos DocumentView::virtual_to_absolute_pos(const VirtualPos& vpos) {
    if (fast_coordinates()) {
        return AbsoluteDocumentPos{ vpos.x, vpos.y };
    }

    fill_cached_virtual_rects();
    if (cached_virtual_rects.size() == 0) {
        return AbsoluteDocumentPos{ vpos.x, vpos.y };
    }

    int page = -1;

    for (int i = 0; i < cached_virtual_rects.size(); i++) {
        if (cached_virtual_rects[i].y1 > vpos.y) {
            page = i;
            break;
        }
    }

    for (int i = 0; i < cached_virtual_rects.size(); i++) {
        if (cached_virtual_rects[i].contains(vpos)) {
            page = i;
            break;
        }
    }


    if (page == -1) {
        page = cached_virtual_rects.size() - 1;
    }


    DocumentPos docpos;
    docpos.x = vpos.x - cached_virtual_rects[page].x0;
    docpos.y = vpos.y - cached_virtual_rects[page].y0;
    docpos.page = page;


    return docpos.to_absolute(current_document);
}

void DocumentView::fill_cached_virtual_rects(bool force) {
    if (!current_document) return;


    float cum_offset = 0;

    if ((cached_virtual_rects.size() == 0) || force) {
        cached_virtual_rects.clear();
        int num_pages = current_document->num_pages();

        if (two_page_mode) {

            for (int i = 0; i < num_pages; i++) {
                float page_width = current_document->get_page_width(i);
                float page_height = current_document->get_page_height(i);
                VirtualRect page_rect;
                page_rect.x0 = -page_width / 2;
                page_rect.x1 = page_width / 2;
                page_rect.y0 = cum_offset;
                page_rect.y1 = cum_offset + page_height;

                float mult = 1.0f;
                if (i % 2 == 1) {
                    cum_offset += page_height + page_space_y;
                }
                else {
                    mult = -1.0f;
                }

                if (page_space_x >= 0) {
                    page_rect.x0 += mult * (page_width + page_space_x) / 2;
                    page_rect.x1 += mult * (page_width + page_space_x) / 2;
                }
                else {
                    page_rect.x0 += mult * (page_width / 2) + mult * page_space_x;
                    page_rect.x1 += mult * (page_width / 2) + mult * page_space_x;

                }


                cached_virtual_rects.push_back(page_rect);

            }
        }
        else {
            cached_virtual_rects.clear();
            if (REAL_PAGE_SEPARATION) {
                for (int i = 0; i < num_pages; i++) {
                    float page_width = current_document->get_page_width(i);
                    float page_height = current_document->get_page_height(i);
                    VirtualRect page_rect;
                    page_rect.x0 = -page_width / 2;
                    page_rect.x1 = page_width / 2;
                    page_rect.y0 = cum_offset;
                    page_rect.y1 = cum_offset + page_height;

                    cached_virtual_rects.push_back(page_rect);

                    cum_offset += page_height + page_space_y;
                }
            }
        }
    }
}

VirtualPos DocumentView::window_to_virtual_pos(const WindowPos& window_pos) {

    float vx = (window_pos.x - view_width / 2) / zoom_level - offset.x;
    float vy = (window_pos.y - view_height / 2) / zoom_level + offset.y;
    return { vx, vy };
}

WindowPos DocumentView::virtual_to_window_pos(const VirtualPos& vpos) {


    WindowPos res;
    res.x = (vpos.x + offset.x)* zoom_level + view_width / 2;
    res.y = (vpos.y - offset.y)* zoom_level + view_height / 2;
    return res;
}

void DocumentView::toggle_two_page(){
    AbsoluteDocumentPos current_abs_offset = get_offsets();

    two_page_mode = !two_page_mode;
    cached_virtual_rects.clear();
    fill_cached_virtual_rects();

    set_offsets(current_abs_offset.x, current_abs_offset.y);
    fit_to_page_width();
}


NormalizedWindowRect DocumentView::virtual_to_normalized_window_rect(const VirtualRect& virtual_rect){
    NormalizedWindowPos top_left = virtual_to_window_pos(VirtualPos{ virtual_rect.x0, virtual_rect.y0 }).to_window_normalized(this);
    NormalizedWindowPos bottom_right = virtual_to_window_pos(VirtualPos{ virtual_rect.x1, virtual_rect.y1 }).to_window_normalized(this);
    return NormalizedWindowRect(top_left, bottom_right);
}

bool DocumentView::is_two_page_mode() {
    return two_page_mode;
}

void DocumentView::set_page_space_x(float space_x) {
    page_space_x = space_x;
}

void DocumentView::set_page_space_y(float space_y) {
    page_space_y = space_y;
}

float DocumentView::get_page_space_x() {
    return page_space_x;
}

float DocumentView::get_page_space_y() {
    return page_space_x;
}

bool DocumentView::fast_coordinates() {
    return (!two_page_mode) && (!REAL_PAGE_SEPARATION);
}


std::vector<int> DocumentView::get_visible_search_results(std::vector<int>& visible_pages) {
    std::vector<int> res;

    int index = find_search_index_for_visible_pages(visible_pages);
    if (index == -1) return res;

    auto next_index = [&](int ind) {
        return (ind + 1) % search_results.size();
    };

    auto prev_index = [&](int ind) {
        int res = ind - 1;
        if (res == -1) {
            return static_cast<int>(search_results.size() - 1);
        }
        return res;
    };

    auto is_page_visible = [&](int page) {
        return std::find(visible_pages.begin(), visible_pages.end(), search_results[page].page) != visible_pages.end();
    };

    res.push_back(index);
    if (search_results.size() > 0) {
        int next = next_index(index);
        while ((next != index) && is_page_visible(next)) {
            res.push_back(next);
            next = next_index(next);
        }
        if (next != index) {
            int prev = prev_index(index);
            while (is_page_visible(prev)) {
                res.push_back(prev);
                prev = prev_index(prev);
            }

        }
    }

    return res;
}

int DocumentView::find_search_index_for_visible_pages(std::vector<int>& visible_pages) {
    // finds some search index located in the visible pages

    int breakpoint = find_search_results_breakpoint();
    for (int page : visible_pages) {
        int index = find_search_index_for_visible_page(page, breakpoint);
        if (index != -1) {
            return index;
        }
    }
    return -1;
}

int DocumentView::find_search_index_for_visible_page(int page, int breakpoint) {
    // array is sorted, only one binary search
    if ((breakpoint == search_results.size() - 1) || (search_results.size() == 1)) {
        return find_search_result_for_page_range(page, 0, breakpoint);
    }
    else {
        int index = find_search_result_for_page_range(page, 0, breakpoint);
        if (index != -1) return index;
        return find_search_result_for_page_range(page, breakpoint + 1, search_results.size() - 1);
    }
}

int DocumentView::find_search_results_breakpoint() {
    if (search_results.size() > 0) {
        int begin_index = 0;
        int end_index = search_results.size() - 1;
        return find_search_results_breakpoint_helper(begin_index, end_index);
    }
    else {
        return -1;
    }
}

int DocumentView::find_search_result_for_page_range(int page, int range_begin, int range_end) {
    if (range_begin > range_end) {
        return find_search_result_for_page_range(page, range_end, range_begin);
    }

    int midpoint = (range_begin + range_end) / 2;
    if (midpoint == range_begin) {
        if (search_results[range_begin].page == page) {
            return range_begin;
        }
        if (search_results[range_end].page == page) {
            return range_end;
        }
        return -1;
    }
    else {
        if (search_results[midpoint].page >= page) {
            return find_search_result_for_page_range(page, range_begin, midpoint);
        }
        else {
            return find_search_result_for_page_range(page, midpoint, range_end);
        }
    }

}

int DocumentView::find_search_results_breakpoint_helper(int begin_index, int end_index) {
    int midpoint = (begin_index + end_index) / 2;
    if (midpoint == begin_index) {
        if (search_results[end_index].page > search_results[begin_index].page) {
            return end_index;
        }
        else {
            return begin_index;
        }
    }
    else {
        if (search_results[midpoint].page >= search_results[begin_index].page) {
            return find_search_results_breakpoint_helper(midpoint, end_index);
        }
        else {
            return find_search_results_breakpoint_helper(begin_index, midpoint);
        }
    }
}

void DocumentView::cancel_search() {
    search_results.clear();
    current_search_result_index = -1;
    is_searching = false;
    is_search_cancelled = true;
}

int DocumentView::get_num_search_results() {
    search_results_mutex.lock();
    int num = search_results.size();
    search_results_mutex.unlock();
    return num;
}

int DocumentView::get_current_search_result_index() {
    return current_search_result_index;
}

std::optional<SearchResult> DocumentView::get_current_search_result() {
    if (!current_document) return {};
    if (current_search_result_index == -1) return {};
    search_results_mutex.lock();
    if (search_results.size() == 0) {
        search_results_mutex.unlock();
        return {};
    }
    SearchResult res = search_results[current_search_result_index];
    search_results_mutex.unlock();
    return res;
}

std::optional<SearchResult> DocumentView::set_search_result_offset(int offset) {
    if (!current_document) return {};
    search_results_mutex.lock();
    if (search_results.size() == 0) {
        search_results_mutex.unlock();
        return {};
    }
    int target_index = mod(current_search_result_index + offset, search_results.size());
    current_search_result_index = target_index;

    SearchResult res = search_results[target_index];
    search_results_mutex.unlock();
    return res;

}

void DocumentView::goto_search_result(int offset, bool overview) {
    if (!current_document) return;

    std::optional<SearchResult> result_ = set_search_result_offset(offset);
    if (result_) {
        SearchResult result = result_.value();

        if (result.rects.size() == 0) {
            result.fill(current_document);
        }
        if (result.rects.size() == 0){
            return;
        }
        float result_center_y = (result.rects.front().y0 + result.rects.front().y1) / 2;
        float new_offset_y = result_center_y + get_document()->get_accum_page_height(result.page);
        DocumentRect result_rect(result.rects.front(), result.page);
        NormalizedWindowRect result_normalized_rect = result_rect.to_window_normalized(this);

        if (overview) {
            OverviewState state = { new_offset_y, 0, -1, nullptr };
            set_overview_page(state);
        }
        else {
            set_offset_y(new_offset_y);
            float normalized_center_x = (result_normalized_rect.x0 + result_normalized_rect.x1) / 2;
            if (normalized_center_x < -1 || normalized_center_x > 1) {
                move(-normalized_center_x / 2 * get_view_width(), 0);
            }
        }
    }
}

bool DocumentView::get_is_searching(float* prog) {
    if (is_search_cancelled) {
        return false;
    }

    search_results_mutex.lock();
    if (is_searching) {
        if (prog) {
            *prog = percent_done;
        }
    }
    search_results_mutex.unlock();
    return true;
}

void DocumentView::set_search_results(std::vector<SearchResult>&& results) {
    is_search_cancelled = false;
    search_results = std::move(results);
}

void DocumentView::search_text(PdfRenderer* background_searcher, const std::wstring& text, SearchCaseSensitivity case_sensitive, bool regex, std::optional<std::pair<int, int>> range) {
    search_results_mutex.lock();
    search_results.clear();
    current_search_result_index = -1;
    search_results_mutex.unlock();

    int min_page = -1;
    int max_page = 2147483647;
    if (range.has_value()) {
        min_page = range.value().first;
        max_page = range.value().second;
    }

    if (get_document()->is_super_fast_index_ready()) {
        if (!text.empty()) {
            int current_page = get_center_page_number();
            std::vector<SearchResult> results;
            if (regex) {
                results = get_document()->search_regex(text, case_sensitive, current_page, min_page, max_page);
            }
            else {
                results = get_document()->search_text(text, case_sensitive, current_page, min_page, max_page);
            }
            search_results = std::move(results);
            is_searching = false;
            is_search_cancelled = false;
            percent_done = 1.0f;
        }
    }
    else {

        is_searching = true;
        is_search_cancelled = false;

        int current_page = get_center_page_number();
        if (current_page >= 0) {
            background_searcher->add_request(
                get_document()->get_path(),
                current_page,
                text,
                regex,
                &search_results,
                &percent_done,
                &is_searching,
                &search_results_mutex,
                range);

        }
    }
}

void DocumentView::set_overview_page(std::optional<OverviewState> overview) {
    if (overview.has_value()) {
        Document* target = get_document();
        if (overview.value().doc != nullptr) {
            target = overview.value().doc;
        }

        float offset = overview.value().absolute_offset_y;
        if (offset < 0) {
            overview.value().absolute_offset_y = 0;
        }
        if (offset > target->max_y_offset()) {
            overview.value().absolute_offset_y = target->max_y_offset();
        }
    }

    this->overview_page = overview;
}

std::optional<OverviewState> DocumentView::get_overview_page() {
    return overview_page;
}

Document* DocumentView::get_current_overview_document() {
    if (overview_page) {
        if (overview_page.value().doc) {
            return overview_page->doc;
        }
        else {
            return get_document();
        }
    }
    else {
        return get_document();
    }

}

float DocumentView::get_overview_zoom_level(){

    if (overview_page->zoom_level > 0){
        return overview_page->zoom_level;
    }
    auto overview_doc = get_current_overview_document();
    DocumentPos docpos = overview_doc->absolute_to_page_pos_uncentered({ 0, overview_page->absolute_offset_y });
    overview_page->zoom_level = (get_view_width() * overview_half_width) / overview_doc->get_page_width(docpos.page);
    return overview_page->zoom_level;

}

NormalizedWindowPos DocumentView::document_to_overview_pos(DocumentPos pos) {
    NormalizedWindowPos res;

    if (overview_page) {
        OverviewState overview = overview_page.value();
        Document* target_doc = get_current_overview_document();
        /* DocumentPos docpos = target_doc->absolute_to_page_pos_uncentered({ 0, overview.absolute_offset_y }); */

        AbsoluteDocumentPos abspos = target_doc->document_to_absolute_pos(pos);

        float overview_zoom_level = get_overview_zoom_level() / get_view_width() * 2;

        float relative_x = abspos.x * overview_zoom_level + overview.absolute_offset_x * overview_zoom_level;
        float aspect = static_cast<float>(view_width) / static_cast<float>(view_height);
        float relative_y = (abspos.y - overview.absolute_offset_y) * overview_zoom_level * aspect;
        //float left = overview_offset_x - overview_half_width;
        float top = overview_offset_y;
        return { overview_offset_x + relative_x, top - relative_y };

        return res;
    }
    else {
        return res;
    }
}

NormalizedWindowRect DocumentView::document_to_overview_rect(DocumentRect doc_rect) {
    DocumentPos top_left = doc_rect.top_left();
    DocumentPos bottom_right = doc_rect.bottom_right();
    NormalizedWindowPos top_left_pos = document_to_overview_pos(top_left);
    NormalizedWindowPos bottom_right_pos = document_to_overview_pos(bottom_right);
    return NormalizedWindowRect(top_left_pos, bottom_right_pos);
}

void DocumentView::zoom_overview(float scale){
    if (overview_page){
        float new_zoom_level = overview_page->zoom_level * scale;
        float min_zoom_level = 1.0f;
        if (new_zoom_level < min_zoom_level) new_zoom_level = min_zoom_level;
        if (new_zoom_level > 6.0) new_zoom_level = 6.0;
        overview_page->zoom_level = new_zoom_level;
    }
}

void DocumentView::zoom_in_overview(){
    zoom_overview(ZOOM_INC_FACTOR);
}

void DocumentView::zoom_out_overview(){
    zoom_overview(1.0f / ZOOM_INC_FACTOR);
}

NormalizedWindowRect DocumentView::get_overview_rect() {
    NormalizedWindowRect res;
    res.x0 = overview_offset_x - overview_half_width;
    res.y0 = overview_offset_y - overview_half_height;
    res.x1 = overview_offset_x + overview_half_width;
    res.y1 = overview_offset_y + overview_half_height;
    return res;
}

NormalizedWindowRect DocumentView::get_overview_rect_pixel_perfect(int widget_width, int widget_height, int view_width, int view_height) {
    NormalizedWindowRect res;
    int x0_pixel = static_cast<int>((((overview_offset_x - overview_half_width) + 1.0f) / 2.0f) * widget_width);
    int x1_pixel = x0_pixel + view_width;
    int y0_pixel = static_cast<int>((((-overview_offset_y - overview_half_height) + 1.0f) / 2.0f) * widget_height);
    int y1_pixel = y0_pixel + view_height;

    res.x0 = (static_cast<float>(x0_pixel) / static_cast<float>(widget_width)) * 2.0f - 1.0f;
    res.x1 = (static_cast<float>(x1_pixel) / static_cast<float>(widget_width)) * 2.0f - 1.0f;
    res.y0 = (static_cast<float>(y0_pixel) / static_cast<float>(widget_height)) * 2.0f - 1.0f;
    res.y1 = (static_cast<float>(y1_pixel) / static_cast<float>(widget_height)) * 2.0f - 1.0f;

    return res;
}

WindowRect DocumentView::get_overview_download_rect() {
    WindowPos top_left = NormalizedWindowPos{ overview_offset_x - overview_half_width, overview_offset_y + overview_half_height}.to_window(this);
    //top_left.x += 1;
    top_left.y += 2;

    WindowPos bottom_right = WindowPos{ top_left.x + 20, top_left.y + 20 };
    return WindowRect(top_left, bottom_right);
}

std::vector<NormalizedWindowRect> DocumentView::get_overview_border_rects() {
    std::vector<NormalizedWindowRect> res;

    NormalizedWindowRect bottom_rect;
    NormalizedWindowRect top_rect;
    NormalizedWindowRect left_rect;
    NormalizedWindowRect right_rect;

    bottom_rect.x0 = overview_offset_x - overview_half_width;
    bottom_rect.y0 = overview_offset_y - overview_half_height - 0.05f;
    bottom_rect.x1 = overview_offset_x + overview_half_width;
    bottom_rect.y1 = overview_offset_y - overview_half_height + 0.05f;

    top_rect.x0 = overview_offset_x - overview_half_width;
    top_rect.y0 = overview_offset_y + overview_half_height - 0.05f;
    top_rect.x1 = overview_offset_x + overview_half_width;
    top_rect.y1 = overview_offset_y + overview_half_height + 0.05f;

    left_rect.x0 = overview_offset_x - overview_half_width - 0.05f;
    left_rect.y0 = overview_offset_y - overview_half_height;
    left_rect.x1 = overview_offset_x - overview_half_width + 0.05f;
    left_rect.y1 = overview_offset_y + overview_half_height;

    right_rect.x0 = overview_offset_x + overview_half_width - 0.05f;
    right_rect.y0 = overview_offset_y - overview_half_height;
    right_rect.x1 = overview_offset_x + overview_half_width + 0.05f;
    right_rect.y1 = overview_offset_y + overview_half_height;

    res.push_back(bottom_rect);
    res.push_back(top_rect);
    res.push_back(left_rect);
    res.push_back(right_rect);

    return res;
}

bool DocumentView::is_window_point_in_overview(NormalizedWindowPos window_point) {
    if (get_overview_page()) {
        NormalizedWindowRect rect = get_overview_rect();
        return rect.contains(window_point);
    }
    return false;
}

bool DocumentView::is_window_point_in_overview_border(NormalizedWindowPos window_point, OverviewSide* which_border) {

    if (!get_overview_page().has_value()) {
        return false;
    }

    std::vector<NormalizedWindowRect> rects = get_overview_border_rects();
    for (size_t i = 0; i < rects.size(); i++) {
        if (rects[i].contains(window_point)) {
            *which_border = static_cast<OverviewSide>(i);
            return true;
        }
    }
    return false;
}

void DocumentView::get_overview_offsets(float* offset_x, float* offset_y) {
    *offset_x = overview_offset_x;
    *offset_y = overview_offset_y;
}

void DocumentView::set_overview_offsets(float offset_x, float offset_y) {
    overview_offset_x = offset_x;
    overview_offset_y = offset_y;
}

void DocumentView::set_overview_offsets(fvec2 offsets) {
    set_overview_offsets(offsets.x(), offsets.y());
}

float DocumentView::get_overview_side_pos(int index) {
    if (index == OverviewSide::bottom) {
        return overview_offset_y - overview_half_height;
    }
    if (index == OverviewSide::top) {
        return overview_offset_y + overview_half_height;
    }
    if (index == OverviewSide::left) {
        return overview_offset_x - overview_half_width;
    }
    if (index == OverviewSide::right) {
        return overview_offset_x + overview_half_width;
    }
}

void DocumentView::set_overview_side_pos(OverviewSide index, NormalizedWindowRect original_rect, fvec2 diff) {

    NormalizedWindowRect new_rect = original_rect;

    if (index == OverviewSide::bottom) {
        float new_bottom_pos = original_rect.y0 + diff.y();
        if (new_bottom_pos < original_rect.y1) {
            new_rect.y0 = new_bottom_pos;
        }
    }
    if (index == OverviewSide::top) {
        float new_top_pos = original_rect.y1 + diff.y();
        if (new_top_pos > original_rect.y0) {
            new_rect.y1 = new_top_pos;
        }
    }
    if (index == OverviewSide::left) {
        float new_left_pos = original_rect.x0 + diff.x();
        if (new_left_pos < original_rect.x1) {
            new_rect.x0 = new_left_pos;
        }
    }
    if (index == OverviewSide::right) {
        float new_right_pos = original_rect.x1 + diff.x();
        if (new_right_pos > original_rect.x0) {
            new_rect.x1 = new_right_pos;
        }
    }
    set_overview_rect(new_rect);

}

void DocumentView::set_overview_rect(NormalizedWindowRect rect) {
    float halfwidth = rect.width() / 2;
    float halfheight = rect.height() / 2;
    float offset_x = rect.x0 + halfwidth;
    float offset_y = rect.y0 + halfheight;

    overview_offset_x = offset_x;
    overview_offset_y = offset_y;
    overview_half_width = halfwidth;
    overview_half_height = halfheight;
}

void DocumentView::get_overview_size(float* width, float* height) {
    *width = overview_half_width;
    *height = overview_half_height;
}

DocumentPos DocumentView::window_pos_to_overview_pos(NormalizedWindowPos window_pos) {
    Document* target = get_current_overview_document();

    float window_width = static_cast<float>(view_width);
    float window_height = static_cast<float>(view_height);
    int window_x = static_cast<int>((1.0f + window_pos.x) / 2 * window_width);
    int window_y = static_cast<int>((1.0f - window_pos.y) / 2 * window_height);
    DocumentPos docpos = target->absolute_to_page_pos_uncentered(
        { get_overview_page()->absolute_offset_x, get_overview_page()->absolute_offset_y});

    float zoom_level = get_overview_zoom_level();

    int overview_h_mid = overview_offset_x * window_width / 2 + window_width / 2;
    int overview_mid = (-overview_offset_y) * window_height / 2 + window_height / 2;

    float relative_window_x = static_cast<float>(window_x - overview_h_mid) / zoom_level;
    float relative_window_y = static_cast<float>(window_y - overview_mid) / zoom_level;

    int page_half_width = target->get_page_width(docpos.page);
    float doc_offset_x = -docpos.x + relative_window_x + page_half_width;
    float doc_offset_y = docpos.y + relative_window_y;
    int doc_page = docpos.page;
    return { doc_page, doc_offset_x, doc_offset_y };
}

ColorPalette DocumentView::get_current_color_mode() {
    return color_mode;
}

void DocumentView::toggle_highlight_links() {
    this->should_highlight_links = !this->should_highlight_links;
}

void DocumentView::set_highlight_links(bool should_highlight, bool should_show_numbers) {
    this->should_highlight_links = should_highlight;
    this->should_show_numbers = should_show_numbers;
}

void DocumentView::set_dark_mode(bool mode) {
    if (mode == true) {
        this->color_mode = ColorPalette::Dark;
    }
    else {
        this->color_mode = ColorPalette::Normal;
    }
}

void DocumentView::toggle_dark_mode() {
    set_dark_mode(!(this->color_mode == ColorPalette::Dark));
}

void DocumentView::set_custom_color_mode(bool mode) {
    if (mode) {
        this->color_mode = ColorPalette::Custom;
    }
    else {
        this->color_mode = ColorPalette::Normal;
    }
}

void DocumentView::toggle_custom_color_mode() {
    set_custom_color_mode(!(this->color_mode == ColorPalette::Custom));
}

void DocumentView::toggle_highlight_words() {
    this->should_highlight_words = !this->should_highlight_words;
}

void DocumentView::set_highlight_words(std::vector<DocumentRect>& rects) {
    word_rects = std::move(rects);
}

std::vector<int> DocumentView::get_ruler_unmerged_line_indices() {

    if (ruler_line_index.has_value()){
        return ruler_line_index->unmerged_indices;
    }
    return {};
}

//std::pair<fz_stext_line*, fz_stext_block*> DocumentView::get_ruler_line_and_block() {
//    if (ruler_rect.has_value()) {
//        AbsoluteRect rect = ruler_rect.value();
//        float ruler_area = rect.area();
//        int ruler_page = get_vertical_line_page();
//        fz_stext_line* min_line = nullptr;
//        fz_stext_block* min_block = nullptr;
//        float min_distance = 100000;
//
//        for (auto [block, line, _] : current_document->page_iterator(ruler_page, true)) {
//            DocumentRect doc_rect(line->bbox, ruler_page);
//            float distance = doc_rect.to_absolute(current_document).distance(rect);
//
//            if (distance < min_distance) {
//                min_distance = distance;
//                min_block = block;
//                min_line = line;
//            }
//        }
//        return std::make_pair(min_line, min_block);
//    }
//    return std::make_pair(nullptr, nullptr);
//}

void DocumentView::set_should_highlight_words(bool should_highlight) {
    this->should_highlight_words = should_highlight;
}

std::vector<DocumentRect> DocumentView::get_highlight_word_rects() {
    return word_rects;
}

void DocumentView::show_rect_hints() {
    should_show_rect_hints = true;
}

void DocumentView::hide_rect_hints() {
    should_show_rect_hints = false;
}

bool DocumentView::is_showing_rect_hints() {
    return should_show_rect_hints;
}

bool DocumentView::on_vertical_scroll(){

    // returns true if the scroll even caused some change
    // in this widget
    bool res = false;

    if (should_highlight_words){
        should_highlight_words = false;
        res = true;
    }
    if (should_highlight_links && !SHOULD_HIGHLIGHT_LINKS){
        should_highlight_links = false;
        res = true;
    }
    return res;
}

void DocumentView::set_tag_prefix(std::wstring prefix) {
    tag_prefix = utf8_encode(prefix);
}

void DocumentView::clear_tag_prefix() {
    tag_prefix = "";
}

void DocumentView::set_highlighted_tags(std::vector<std::string> tags) {
    highlighted_tags = tags;
}
bool DocumentView::is_tag_highlighted(const std::string& tag) {
    for (auto& htag : highlighted_tags) {
        if (tag == htag) return true;
    }
    return false;
}

void DocumentView::set_pending_portal_position(std::optional<AbsoluteRect> rect) {
    pending_portal_rect = rect;
}

void DocumentView::rotate_clockwise() {
    rotation_index = (rotation_index + 1) % 4;
}

void DocumentView::rotate_counterclockwise() {
    rotation_index = (rotation_index - 1) % 4;
    if (rotation_index < 0) {
        rotation_index += 4;
    }
}

bool DocumentView::is_rotated() {
    return rotation_index != 0;
}

void DocumentView::toggle_fastread_mode() {
    fastread_mode = !fastread_mode;
}

void DocumentView::set_typing_rect(DocumentRect highlight_rect, std::optional<DocumentRect> wrong_rect) {
    AbsoluteRect absrect = current_document->document_to_absolute_rect(highlight_rect);
    character_highlight_rect = absrect;

    if (wrong_rect) {
        AbsoluteRect abswrong = current_document->document_to_absolute_rect(wrong_rect.value());
        wrong_character_rect = abswrong;
    }
    else {
        wrong_character_rect = {};
    }

}

void DocumentView::set_underline(AbsoluteDocumentPos abspos) {
    underline = abspos;
}

void DocumentView::clear_underline() {
    underline = {};
}

void DocumentView::set_selected_object_index(VisibleObjectIndex index) {
    selected_object_index = index;
}

//void DocumentView::set_selected_highlight_index(int index) {
//    selected_highlight_index = index;
//}
//
//void DocumentView::set_selected_bookmark_index(int index) {
//    selected_bookmark_index = index;
//}

void DocumentView::set_overview_highlights(const std::vector<DocumentRect>& rects){
    if (overview_page) {
        overview_page->highlight_rects = rects;
    }
    overview_highlights = rects;
}

void DocumentView::set_selected_rectangle(AbsoluteRect selected) {
    selected_rectangle = selected;
}

void DocumentView::clear_selected_rectangle() {
    selected_rectangle = {};
}
std::optional<AbsoluteRect> DocumentView::get_selected_rectangle() {
    return selected_rectangle;
}

void DocumentView::set_pending_download_portals(std::vector<AbsoluteRect>&& portal_rects){
    pending_download_portals = std::move(portal_rects);
}

void DocumentView::set_synctex_highlights(std::vector<DocumentRect> highlights) {
    synctex_highlight_time = QTime::currentTime();
    synctex_highlights = std::move(highlights);
}

bool DocumentView::should_show_synxtex_highlights() {
    if (synctex_highlights.size() > 0) {
        if ((HIDE_SYNCTEX_HIGHLIGHT_TIMEOUT < 0) || (synctex_highlight_time.msecsTo(QTime::currentTime()) < (HIDE_SYNCTEX_HIGHLIGHT_TIMEOUT * 1000.0f))) {
            return true;
        }
    }
    return false;

}

bool DocumentView::has_synctex_timed_out() {
    if (synctex_highlights.size() > 0 && (!should_show_synxtex_highlights())) {
        synctex_highlights.clear();
        return true;
    }
    return false;
}

bool DocumentView::is_pos_inside_selected_text(AbsoluteDocumentPos pos) {
    for (auto rect : selected_character_rects) {
        if (rect.contains(pos)) {
            return true;
        }
    }
    return false;
}

bool DocumentView::is_pos_inside_selected_text(DocumentPos docpos) {
    return is_pos_inside_selected_text(docpos.to_absolute(current_document));
}

bool DocumentView::is_pos_inside_selected_text(WindowPos pos) {
    return is_pos_inside_selected_text(pos.to_absolute(this));
}

std::vector<DocumentRect> DocumentView::get_paper_name_rects_from_page_and_source_text(int page, const std::wstring& source_text) {

    auto ref_ = current_document->get_page_bib_with_reference(page, source_text);

    if (ref_) {
        auto [bib_str, rects] = ref_.value();
        QString paper_name = get_paper_name_from_reference_text(bib_str);
        int paper_name_index = bib_str.indexOf(paper_name);
        std::deque<PagelessDocumentRect> paper_name_rects;
        std::vector<PagelessDocumentRect> paper_name_merged_rects;

        for (int i = 0; i < paper_name.size(); i++) {
            paper_name_rects.push_back(rects[paper_name_index + i]);
        }

        merge_selected_character_rects(paper_name_rects, paper_name_merged_rects);
        std::vector<DocumentRect> paper_name_document_rects;
        for (auto rect : paper_name_merged_rects) {
            paper_name_document_rects.push_back(DocumentRect{ rect, page });
        }
        return paper_name_document_rects;

    }
    return {};
}

void DocumentView::fill_text_under_pointer_info_reference_highlight_rects(TextUnderPointerInfo& info) {
    info.overview_highlight_rects = get_paper_name_rects_from_page_and_source_text(info.targets[0].page, info.source_text);
}

void DocumentView::fill_smart_view_candidate_reference_highlight_rects(SmartViewCandidate& candidate) {
    candidate.highlight_rects = get_paper_name_rects_from_page_and_source_text(
        candidate.get_docpos(this).page, candidate.source_text
    );

}

std::optional<QString> DocumentView::get_paper_name_under_pos(DocumentPos docpos, bool clean) {

    std::optional<PdfLink> pdf_link_ = current_document->get_link_in_pos(docpos);

    if (is_pos_inside_selected_text(docpos)) {
        // if user is clicking on a selected text, we assume they want to download the text
        return QString::fromStdWString(selected_text);
    }
    else if (pdf_link_) {
        // first, we  try to detect if we are on a PDF link or a non-link reference
        // (something like [14] or [Doe et. al.]) and then find the paper name in the
        // referenced location. If we can't match the current text as a refernce source,
        // we assume the text under cursor is the paper name.

        PdfLink pdf_link = pdf_link_.value();
        std::wstring link_text = current_document->get_pdf_link_text(pdf_link).link_text;
        auto [link_page, offset_x, offset_y] = parse_uri(current_document->get_mupdf_context(), current_document->doc, pdf_link.uri);

        auto res = current_document->get_page_bib_with_reference(link_page - 1, link_text);
        if (res) {
            if (clean) {
                return get_paper_name_from_reference_text(res.value().first);
            }
            else {
                return res.value().first;
            }
        }
        else {
            return {};
        }
    }
    else {
        auto ref_ = current_document->get_reference_text_at_position(docpos, nullptr);
        /* int target_page = -1; */
        /* float target_offset; */
        /* std::wstring source_text; */

        if (ref_){
            TextUnderPointerInfo reference_info = find_location_of_text_under_pointer(docpos);
            if ((reference_info.reference_type == ReferenceType::Reference) && (reference_info.targets.size() > 0)){
                std::wstring ref = ref_.value();
                auto res = current_document->get_page_bib_with_reference(reference_info.targets[0].page, ref);
                if (res) {
                    if (clean) {
                        return get_paper_name_from_reference_text(res.value().first);
                    }
                    else {
                        return res.value().first;
                    }
                }
                else {
                    return {};
                }
            }

        }
        else {
            //std::optional<std::wstring> paper_name = get_paper_name_under_cursor(alksdh);
            std::optional<QString> paper_name = get_direct_paper_name_under_pos(docpos);
            if (paper_name) {
                return paper_name;
            }
        }
    }

    return {};
}

std::optional<QString> DocumentView::get_direct_paper_name_under_pos(DocumentPos docpos) {
    return current_document->get_paper_name_at_position(docpos);
}

TextUnderPointerInfo DocumentView::find_location_of_text_under_pointer(DocumentPos docpos,  bool update_candidates) {

    //auto [page, offset_x, offset_y] = main_document_view->window_to_document_pos(pointer_pos);
    //auto [page, offset_x, offset_y] = docpos;
    TextUnderPointerInfo res;

    int current_page_number = get_current_page_number();

    fz_stext_page* stext_page = current_document->get_stext_with_page_number(docpos.page);
    std::vector<fz_stext_char*> flat_chars;
    get_flat_chars_from_stext_page(stext_page, flat_chars);

    std::pair<int, int> reference_range = std::make_pair(-1, -1);

    std::optional<std::pair<std::wstring, std::wstring>> generic_pair = \
        current_document->get_generic_link_name_at_position(flat_chars, docpos.pageless(), &reference_range);

    std::optional<std::wstring> reference_text_on_pointer = current_document->get_reference_text_at_position(flat_chars, docpos.pageless(), &reference_range);
    std::optional<std::wstring> equation_text_on_pointer = current_document->get_equation_text_at_position(flat_chars, docpos.pageless(), &reference_range);

    DocumentRect source_rect_document = DocumentRect{ fz_empty_rect, docpos.page };
    AbsoluteRect source_rect_absolute = { fz_empty_rect };

    if ((reference_range.first > -1) && (reference_range.second > 0)) {
        source_rect_document.rect = rect_from_quad(flat_chars[reference_range.first]->quad);
        for (int i = reference_range.first + 1; i <= reference_range.second; i++) {
            source_rect_document.rect = fz_union_rect(source_rect_document.rect, rect_from_quad(flat_chars[i]->quad));
        }
        source_rect_absolute = source_rect_document.to_absolute(current_document);
        res.source_rect = source_rect_absolute;
        /* *out_rect = source_rect_absolute; */
    }

    if (generic_pair) {
        std::vector<DocumentPos> candidates = current_document->find_generic_locations(generic_pair.value().first,
            generic_pair.value().second);
        if (candidates.size() > 0) {
            if (update_candidates) {
                smart_view_candidates.clear();
                for (auto candid : candidates) {
                    SmartViewCandidate smart_view_candid;
                    smart_view_candid.source_rect = source_rect_absolute;
                    smart_view_candid.target_pos = candid;
                    smart_view_candid.source_text = generic_pair.value().first + L" " + generic_pair.value().second;
                    smart_view_candidates.push_back(smart_view_candid);
                }
                //smart_view_candidates = candidates;
                index_into_candidates = 0;
                //on_overview_source_updated();
                res.source_text = smart_view_candidates[index_into_candidates].source_text;
            }
            else {
                res.source_text = generic_pair.value().first + L" " + generic_pair.value().second;
            }

            res.targets.push_back(candidates[index_into_candidates]);
            //res.page = candidates[index_into_candidates].page;
            //res.offset = candidates[index_into_candidates].y;
            res.reference_type = ReferenceType::Generic;
            return res;
        }
    }
    if (equation_text_on_pointer) {
        std::vector<IndexedData> eqdata_ = current_document->find_equation_with_string(equation_text_on_pointer.value(), current_page_number);
        if (eqdata_.size() > 0) {
            IndexedData refdata = eqdata_[0];
            res.source_text = refdata.text;

            res.targets.push_back(DocumentPos{refdata.page, 0, refdata.y_offset});
            //res.page = refdata.page;
            //res.offset = refdata.y_offset;
            res.reference_type = ReferenceType::Equation;
            return res;
        }
    }

    if (reference_text_on_pointer) {
        std::vector<IndexedData> refdata_ = current_document->find_reference_with_string(reference_text_on_pointer.value(), current_page_number);
        if (refdata_.size() > 0) {
            res.reference_type = ReferenceType::Reference;

            for (auto refdata : refdata_) {
                res.source_text = refdata.text;
                res.targets.push_back(DocumentPos{refdata.page, 0, refdata.y_offset});
            }

            fill_text_under_pointer_info_reference_highlight_rects(res);
            return res;
        }

    }
    if (is_pos_inside_selected_text(docpos)){
        if (selected_text.size() > 0 && current_document->is_super_fast_index_ready()) {
            int target_page;
            float target_y_offset;
            res.reference_type = find_location_of_selected_text(&target_page, &target_y_offset, &res.source_rect, &res.source_text, &res.overview_highlight_rects);
            res.targets.push_back(DocumentPos{ target_page, 0, target_y_offset });
            if (res.reference_type == ReferenceType::Reference) {
                fill_text_under_pointer_info_reference_highlight_rects(res);
            }
            return res;
            /* ReferenceType MainWidget::find_location_of_selected_text(int* out_page, float* out_offset, AbsoluteRect* out_rect, std::wstring* out_source_text, std::vector<DocumentRect>* out_highlight_rects) { */
        }
    }
    else{
        std::wregex abbr_regex(L"[A-Z]+s?");
        std::optional<std::wstring> abbr_under_pointer = current_document->get_regex_match_at_position(abbr_regex, flat_chars, docpos.pageless(), &reference_range);
        if (abbr_under_pointer){
            std::optional<DocumentPos> abbr_definition_location = current_document->find_abbreviation(abbr_under_pointer.value(), res.overview_highlight_rects);
            if (abbr_definition_location){
                res.source_text = abbr_under_pointer.value();
                res.targets.push_back(DocumentPos{ abbr_definition_location->page, 0, abbr_definition_location->y});
                //res.page = abbr_definition_location->page;
                //res.offset = abbr_definition_location->y;
                res.reference_type = ReferenceType::Abbreviation;
                return res;
            }
        }
    }


    res.reference_type = ReferenceType::None;
    return res;
}

int DocumentView::get_current_page_number() {
    if (is_ruler_mode()) {
        return get_vertical_line_page();
    }
    else {
        return get_center_page_number();
    }
}

ReferenceType DocumentView::find_location_of_selected_text(int* out_page, float* out_offset, AbsoluteRect* out_rect, std::wstring* out_source_text, std::vector<DocumentRect>* out_highlight_rects) {
    if (selected_text.size() > 0) {

        std::wstring query = selected_text;
        *out_source_text = query;
        if (selected_character_rects.size() > 0) {
            if (out_rect) {
                *out_rect = selected_character_rects[0];
            }
        }

        if (is_abbreviation(query) && (out_highlight_rects != nullptr)){
            /* std::vector<DocumentRect> overview_highlights; */
            std::optional<DocumentPos> abbr_definition_location = current_document->find_abbreviation(query, *out_highlight_rects);
            /* opengl_widget->set_overview_highlights(overview_highlights); */
            if (abbr_definition_location){
                *out_page = abbr_definition_location->page;
                *out_offset = abbr_definition_location->y;
                return ReferenceType::Abbreviation;
            }
            else{
                return ReferenceType::None;
            }
        }
        else{
            int page = current_document->find_reference_page_with_reference_text(query);
            if (page < 0) return ReferenceType::None;
            auto res = current_document->get_page_bib_with_reference(page, query);
            if (res) {
                *out_page = page;
                *out_offset = res.value().second.back().y0;
                if (out_highlight_rects) {
                    QString paper_name = get_paper_name_from_reference_text(res->first);
                    int paper_name_index = res->first.indexOf(paper_name);
                    //for (auto& r : res.value().second) {
                    //    out_highlight_rects->push_back(DocumentRect{ r, page });
                    //}
                    for (int i = 0; i < paper_name.size(); i++) {
                        out_highlight_rects->push_back(DocumentRect{ res->second[paper_name_index + i], page});
                    }
                }
                return ReferenceType::Reference;
            }
        }

    }
    return ReferenceType::None;
}

bool DocumentView::is_text_source_referncish_at_position(const std::wstring& text, int position) {
    // todo: should be moved to Document

    QString qtext = QString::fromStdWString(text);
    QStringList parts = qtext.split(" ");
    int num_chars_skipped = 0;
    int word_index = 0;
    for (int i = 0; i < parts.size(); i++) {
        QString w = parts[i];
        if (num_chars_skipped + 1 + w.size() > position) {
            word_index = i;
            break;
        }

        num_chars_skipped += 1 + w.size();
    }
    int prev_word_index = word_index - 1;
    QString prev_word = "";
    if (prev_word_index >= 0) {
        prev_word = parts[prev_word_index];
    }
    prev_word = prev_word.toLower();
    QStringList non_ref_words = {
        "table",
        "equation",
        "figure",
        "fig.",
        "fig",
        "theorem",
        "lemma",
        "section",
        "sect.",
        "appendix"
    };
    if (non_ref_words.indexOf(prev_word) != -1) return false;
    //TextUnderPointerInfo reference_info = find_location_of_text_under_pointer(docpos, true);
    return true;
}

bool DocumentView::is_link_a_reference(const PdfLink& link, const PdfLinkTextInfo& link_info) {

    PagelessDocumentPos pageless_pos = PagelessDocumentRect{ fz_rect_from_quad(link_info.chr->quad) }.center();
    DocumentPos link_pos = { link.source_page, pageless_pos.x, pageless_pos.y };
    TextUnderPointerInfo reference_info = find_location_of_text_under_pointer(link_pos, true);
    bool is_reference = reference_info.reference_type == ReferenceType::None || reference_info.reference_type == ReferenceType::Reference;
    if (is_reference) {
        std::wstring block_string = get_string_from_stext_block(link_info.block, false, false);
        is_reference = is_reference && is_text_source_referncish_at_position(block_string, link_info.position_in_block);
    }
    return is_reference;
}

std::vector<DocumentRect> DocumentView::get_reference_link_highlights(int dest_page, const PdfLink& link, const PdfLinkTextInfo& link_info) {
    auto bib_res = current_document->get_page_bib_with_reference(dest_page, link_info.link_text);
    std::vector<DocumentRect> overview_highlight_rects;
    if (bib_res.has_value()) {
        QString paper_name = get_paper_name_from_reference_text(bib_res->first);
        int paper_name_start_index = bib_res->first.indexOf(paper_name);
        const std::vector<PagelessDocumentRect>& bib_rects = bib_res->second;

        std::deque<PagelessDocumentRect> paper_name_character_rects;
        std::vector<PagelessDocumentRect> paper_name_merged_rects;

        for (int i = 0; i < paper_name.size(); i++) {
            paper_name_character_rects.push_back(bib_rects[paper_name_start_index + i]);
            //overview_highlight_rects.push_back(DocumentRect{ bib_rects[paper_name_start_index + i] , dest_page });
        }
        merge_selected_character_rects<PagelessDocumentRect>(paper_name_character_rects, paper_name_merged_rects);

        for (auto& r : paper_name_merged_rects) {
            overview_highlight_rects.push_back(DocumentRect{ r , dest_page });
        }
        //for (int i = 0; i < paper_name.size(); i++) {
        //    overview_highlight_rects.push_back(DocumentRect{ bib_rects[paper_name_start_index + i] , dest_page });
        //}
    }
    return overview_highlight_rects;
}

void DocumentView::set_overview_link(PdfLink link) {

    auto [page, offset_x, offset_y] = parse_uri(current_document->get_mupdf_context(), current_document->doc, link.uri);
    if (page >= 1) {
        AbsoluteRect source_absolute_rect = DocumentRect(link.rects[0], link.source_page).to_absolute(current_document);
        PdfLinkTextInfo link_info = current_document->get_pdf_link_text(link);
        std::wstring source_text = link_info.link_text;
        bool is_reference = is_link_a_reference(link, link_info);

        SmartViewCandidate current_candidate;
        current_candidate.source_rect = source_absolute_rect;
        current_candidate.target_pos = DocumentPos{ page - 1, 0, offset_y };
        current_candidate.source_text = source_text;
        smart_view_candidates = { current_candidate };
        index_into_candidates = 0;
        std::vector<DocumentRect> overview_highlight_rects = get_reference_link_highlights(page - 1, link, link_info);

        set_overview_position(page - 1, offset_y, is_reference > 0 ? "reflink" : "link", overview_highlight_rects);
        //main_document_view->set_overview_highlights(overview_highlight_rects);

    }
}

void DocumentView::set_overview_position(
    int page,
    float offset,
    std::optional<std::string> overview_type,
    std::optional<std::vector<DocumentRect>> overview_highlights
) {
    if (page >= 0) {

        auto overview_state = OverviewState{ DocumentPos{ page, 0, offset }.to_absolute(current_document).y, 0, -1, nullptr };

        if (overview_highlights.has_value()) {
            overview_state.highlight_rects = overview_highlights.value();
        }
        overview_state.overview_type = overview_type;
        set_overview_page(overview_state);
        set_overview_highlights(overview_state.highlight_rects);
    }
}

void DocumentView::select_ruler_text() {
    std::optional<AbsoluteRect> ruler_rect_ = get_ruler_rect();
    if (ruler_rect_) {
        auto ruler_rect = ruler_rect_.value();

        AbsoluteDocumentPos abspos_begin = ruler_rect.center_left();
        AbsoluteDocumentPos abspos_end = ruler_rect.center_right();
        if (line_select_mode && line_select_begin_data.has_value()) {
            abspos_begin = line_select_begin_data->pos;
            selection_end = abspos_end;
        }
        else {
            LineSelectBeginData data;
            data.pos = abspos_begin;
            data.end_pos = abspos_end;
            data.index_info = ruler_line_index.value();
            line_select_begin_data = data;
            selection_begin = abspos_begin;
            selection_end = abspos_end;
        }

        selected_character_rects.clear();
        if (line_select_begin_data) {
            if (line_select_begin_data->index_info.merged_index > ruler_line_index->merged_index) {
                abspos_end = line_select_begin_data->end_pos;
                abspos_begin = ruler_rect.center_left();
                //abspos_end = line_sele
                //std::swap(abspos_begin, abspos_end);
            }

        }
        current_document->get_text_selection(abspos_begin, abspos_end, false, selected_character_rects, selected_text);
    }
}

void DocumentView::swap_line_select_cursor() {
    std::optional<AbsoluteRect> ruler_rect_ = get_ruler_rect();
    if (ruler_rect_) {
        int ruler_page = get_vertical_line_page();
        LineSelectBeginData new_begin;
        auto ruler_rect = ruler_rect_.value();
        new_begin.pos = ruler_rect.center_left();
        new_begin.end_pos = ruler_rect.center_right();
        new_begin.index_info = ruler_line_index.value();
        int old_ruler_index = line_select_begin_data->index_info.merged_index;

        line_select_begin_data = new_begin;
        set_line_index(old_ruler_index, ruler_page);
    }
}

void DocumentView::toggle_line_select_mode() {

    if (line_select_mode == false) {
        select_ruler_text();
        line_select_mode = true;
    }
    else {
        line_select_mode = false;
        line_select_begin_data = {};
    }
}

bool DocumentView::is_line_select_mode() {
    return line_select_mode;
}

void DocumentView::debug() {
    qDebug() << "_______";
    qDebug() << ruler_line_index->merged_index;
    qDebug() << ruler_line_index->unmerged_indices;
}

int DocumentView::get_selected_highlight_index() {
    if (selected_object_index.has_value() && selected_object_index->object_type == VisibleObjectType::Highlight) {
        return selected_object_index->index;
    }
    return -1;
}

int DocumentView::get_selected_bookmark_index() {
    if (selected_object_index.has_value() && selected_object_index->object_type == VisibleObjectType::Bookmark) {
        return selected_object_index->index;
    }
    return -1;
}

int DocumentView::get_selected_portal_index() {
    if (selected_object_index.has_value() && selected_object_index->object_type == VisibleObjectType::Portal) {
        return selected_object_index->index;
    }
    return -1;
}
