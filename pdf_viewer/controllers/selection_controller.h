#pragma once

#include "controllers/base_controller.h"
#include "book.h"

class RulerController : public BaseController{
private:
    int ruler_moving_distance_traveled = 0;
    // indicates if mouse was in next/prev ruler rect in touch mode
    // if this is the case, we use mouse movement to perform next/prev ruler command
    // after a certain threshold, so the user doesn't have to click on the ruler rect
    bool was_last_mouse_down_in_ruler_next_rect = false;
    bool was_last_mouse_down_in_ruler_prev_rect = false;
    // this is used so we can keep track of mouse movement after press and holding on ruler rect
    WindowPos ruler_moving_last_window_pos;
public:
    RulerController(MainWidget* parent);
    void handle_ruler_touch_move(float distance);
    bool was_last_mouse_down_in_ruler_movement_rect();
    void handle_ruler_auto_move_with_new_position(WindowPos current_window_pos, float sensitivity=1.0f);
    void start_ruler_auto_move(WindowPos pos, bool is_next);
    void stop_ruler_auto_move();
    void move_ruler_next();
    void move_ruler_prev();
    AbsoluteRect move_visual_mark(int offset);
    bool move_ruler_to_next_page();
};
