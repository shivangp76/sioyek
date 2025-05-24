#include <algorithm>

#include "controllers/selection_controller.h"
#include "main_widget.h"
#include "document_view.h"
#include "document.h"
#include "utils/image_utils.h"

extern float RULER_AUTO_MOVE_SENSITIVITY;
extern bool FORCE_CUSTOM_LINE_ALGORITHM;
extern bool AUTOCENTER_VISUAL_SCROLL;
extern float SMALL_PIXMAP_SCALE;

RulerController::RulerController(MainWidget* parent) : BaseController(parent){
}

void RulerController::handle_ruler_touch_move(float distance){
    ruler_moving_distance_traveled += distance;
    float auto_move_thresh = 1.0f / (1 - RULER_AUTO_MOVE_SENSITIVITY) * 100;
    int num_next = num_next = ruler_moving_distance_traveled /
            static_cast<int>(std::max(auto_move_thresh, 1.0f));

    if (num_next != 0) {
        ruler_moving_distance_traveled = 0;
    }

    mdv()->move_visual_mark(num_next);

}

bool RulerController::was_last_mouse_down_in_ruler_movement_rect(){
    return was_last_mouse_down_in_ruler_next_rect || was_last_mouse_down_in_ruler_prev_rect;
}

void RulerController::handle_ruler_auto_move_with_new_position(WindowPos current_window_pos, float sensitvity){
    int distance = current_window_pos.manhattan(ruler_moving_last_window_pos);
    ruler_moving_last_window_pos = current_window_pos;
    handle_ruler_touch_move(distance * sensitvity);
}

void RulerController::start_ruler_auto_move(WindowPos pos, bool is_next){
    if (is_next){
        was_last_mouse_down_in_ruler_next_rect = true;
        was_last_mouse_down_in_ruler_prev_rect = false;
    }
    else{
        was_last_mouse_down_in_ruler_prev_rect = true;
        was_last_mouse_down_in_ruler_next_rect = false;
    }
    ruler_moving_last_window_pos = pos;
}

void RulerController::stop_ruler_auto_move(){
    was_last_mouse_down_in_ruler_next_rect = false;
    was_last_mouse_down_in_ruler_prev_rect = false;
}

void RulerController::move_ruler_next(){
    if (FORCE_CUSTOM_LINE_ALGORITHM) {
        move_visual_mark(1);
        return;
    }
    mdv()->move_visual_mark_next();
    if (mw->is_ttsing()) {
        mw->read_current_line();
    }
}

void RulerController::move_ruler_prev(){
    if (FORCE_CUSTOM_LINE_ALGORITHM) {
        move_visual_mark(-1);
        return;
    }
    mdv()->move_visual_mark_prev();
    if (mw->is_ttsing()) {
        mw->read_current_line();
    }
}

AbsoluteRect RulerController::move_visual_mark(int offset) {
    if ((!mdv()->is_ruler_mode()) || (mdv()->get_overview_page().has_value())){
        dv()->handle_vertical_move(offset);
        return fz_empty_rect;
    }
    else{
        AbsoluteRect ruler_rect = mdv()->move_visual_mark(offset);

        if (mw->is_ttsing()) {
            mw->read_current_line();
        }
        if (AUTOCENTER_VISUAL_SCROLL) {
            mw->return_to_last_visual_mark();
        }
        mdv()->clear_underline();
        return ruler_rect;
    }
}

bool RulerController::move_ruler_to_next_page(){
    if (dv()->is_ruler_mode()){
        int current_ruler_page = dv()->get_vertical_line_page();
        int num_pages = doc()->num_pages();
        if (current_ruler_page < num_pages - 1){
            while (dv()->get_vertical_line_page() == current_ruler_page){
                move_visual_mark(1);
            }
            return true;
        }
    }
    return false;
}

void RulerController::ruler_under_pos(WindowPos pos) {
    //float doc_x, doc_y;
    //int page;
    auto main_document_view = mdv();
    DocumentPos document_pos = main_document_view->window_to_document_pos(pos);
    if (document_pos.page != -1) {
        //opengl_widget->set_should_draw_vertical_line(true);
        int container_line_index = main_document_view->get_line_index_of_pos(document_pos);

        if (container_line_index == -1) {
            main_document_view->set_line_index(main_document_view->get_line_index_of_vertical_pos(), -1);
        }
        else {
            fz_pixmap* pixmap = main_document_view->get_document()->get_small_pixmap(document_pos.page);
            std::vector<unsigned int> hist = get_max_width_histogram_from_pixmap(pixmap);
            std::vector<unsigned int> line_locations;
            std::vector<unsigned int> _;
            get_line_begins_and_ends_from_histogram(hist, _, line_locations);
            int small_doc_x = static_cast<int>(document_pos.x * SMALL_PIXMAP_SCALE);
            int small_doc_y = static_cast<int>(document_pos.y * SMALL_PIXMAP_SCALE);
            int best_vertical_loc = find_best_vertical_line_location(pixmap, small_doc_x, small_doc_y);
            //int best_vertical_loc = line_locations[find_nth_larger_element_in_sorted_list(line_locations, static_cast<unsigned int>(small_doc_y), 2)];
            float best_vertical_loc_doc_pos = best_vertical_loc / SMALL_PIXMAP_SCALE;
            WindowPos window_pos = main_document_view->document_to_window_pos_in_pixels_uncentered(DocumentPos{ document_pos.page, 0, best_vertical_loc_doc_pos });
            auto [abs_doc_x, abs_doc_y] = main_document_view->window_to_absolute_document_pos(window_pos);
            main_document_view->set_vertical_line_pos(abs_doc_y);
            main_document_view->set_line_index(container_line_index, document_pos.page);
        }
        mw->validate_render();

        if (mw->is_lq_ttsing()) {
            mw->read_current_line();
        }
    }
}
