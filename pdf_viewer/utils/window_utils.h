#pragma once
#include <QListView>

void focus_on_widget(QWidget* widget, bool no_unminimize=false);
void move_resize_window(WId parent_hwnd, qint64 pid, int x, int y, int width, int height, bool is_focused);
QListView* get_ui_new_listview();
QString get_ui_font_face_name();
QString get_chat_font_face_name();
QString get_status_font_face_name();

QString get_status_stylesheet(bool nofont = false, int font_size=-1);
QString get_status_button_stylesheet(bool nofont = false, int font_size=-1);
QString get_ui_stylesheet(bool nofont, int font_size=-1);
QString get_selected_stylesheet(bool nofont = false);
QString get_list_item_stylesheet();
QString get_scrollbar_stylesheet();

int get_status_bar_height();
