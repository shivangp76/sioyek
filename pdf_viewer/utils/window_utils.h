#pragma once

void focus_on_widget(QWidget* widget, bool no_unminimize=false);
void move_resize_window(WId parent_hwnd, qint64 pid, int x, int y, int width, int height, bool is_focused);
