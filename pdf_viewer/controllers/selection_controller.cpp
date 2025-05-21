#include <algorithm>

#include "controllers/selection_controller.h"
#include "main_widget.h"
#include "document_view.h"

extern float RULER_AUTO_MOVE_SENSITIVITY;
extern bool FORCE_CUSTOM_LINE_ALGORITHM;
extern bool AUTOCENTER_VISUAL_SCROLL;

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
