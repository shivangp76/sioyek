#include <QWidget>

#if defined(_WIN32)
    #include <windows.h>
#endif

#include "utils/window_utils.h"
#include "utils/color_utils.h"

extern std::wstring UI_FONT_FACE_NAME;
extern QString global_font_family;
extern std::wstring CHAT_FONT_FACE_NAME;
extern std::wstring STATUS_FONT_FACE_NAME;
extern int STATUS_BAR_FONT_SIZE;
extern float STATUS_BAR_COLOR[3];
extern float STATUS_BAR_TEXT_COLOR[3];
extern float UI_SELECTED_TEXT_COLOR[3];
extern float UI_SELECTED_BACKGROUND_COLOR[3];
extern float UI_BACKGROUND_COLOR[3];
extern float UI_TEXT_COLOR[3];

#ifdef Q_OS_WIN
HWND get_window_hwnd_with_pid(qint64 pid) {
    // Windows implementation: enumerate top-level windows for matching PID.
    struct FindWindowData {
        DWORD pid;
        HWND hwnd;
    } data = { static_cast<DWORD>(pid), nullptr };

    auto enumWindowsProc = [](HWND hwnd, LPARAM lParam) -> BOOL {
        FindWindowData* pData = reinterpret_cast<FindWindowData*>(lParam);
        DWORD winPid = 0;
        GetWindowThreadProcessId(hwnd, &winPid);
        // Check visible window from matching process.
        if (winPid == pData->pid && IsWindowVisible(hwnd)) {
            pData->hwnd = hwnd;
            return FALSE; // found; stop enumeration
        }
        return TRUE;
    };

    EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

void clip_child_to_parent(HWND hChild, HWND hParent, RECT child_rect) {

    // Get the parent window's client area
    RECT parentRect;
    GetClientRect(hParent, &parentRect);

    // Convert to screen coordinates
    POINT pt = { parentRect.left, parentRect.top };
    ClientToScreen(hParent, &pt);
    parentRect.left = pt.x;
    parentRect.top = pt.y;

    pt = { parentRect.right, parentRect.bottom };
    ClientToScreen(hParent, &pt);
    parentRect.right = pt.x;
    parentRect.bottom = pt.y;

    RECT intersectRect;
    if (IntersectRect(&intersectRect, &parentRect, &child_rect)) {
        HRGN hRgn = CreateRectRgn(
            intersectRect.left - child_rect.left,
            intersectRect.top - child_rect.top,
            intersectRect.right - child_rect.left,
            intersectRect.bottom - child_rect.top
        );
        SetWindowRgn(hChild, hRgn, TRUE);

    } else {
        HRGN hRgn = CreateRectRgn(
            0,
            0,
            0,
            0);
        SetWindowRgn(hChild, hRgn, TRUE);
    }
}
#endif

void focus_on_widget(QWidget* widget, bool no_unminimize) {
#ifdef Q_OS_WIN
    widget->activateWindow();
#else
    widget->raise();
#endif
    if (no_unminimize) {
        widget->setWindowState(widget->windowState() | Qt::WindowActive);
    }
    else {
        widget->setWindowState(widget->windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    }
}

void move_resize_window(WId parent_hwnd, qint64 pid, int x, int y, int width, int height, bool is_focused) {
#if defined(_WIN32)
    HWND hwnd = get_window_hwnd_with_pid(pid);
    if (hwnd) {
        // this makes sure that the window doesn't have a titlebar and border
        SetWindowLong(hwnd, GWL_STYLE, 0);

        if (is_focused) {
            SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        }
        else {
            SetWindowPos(hwnd, (HWND)parent_hwnd, x, y, width, height, SWP_SHOWWINDOW | SWP_NOACTIVATE);
            SetWindowPos((HWND)parent_hwnd, hwnd, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        }

        RECT child_rect;
        child_rect.left = x;
        child_rect.right = x + width;
        child_rect.top = y;
        child_rect.bottom = y + height;
        // only show the part of the window that is inside the parent window
        clip_child_to_parent(hwnd, (HWND)parent_hwnd, child_rect);
    }
#elif defined(Q_OS_LINUX)
    // Moving windows on linux? I don't think so.
#elif defined(Q_OS_MACOS)
#endif
}

QListView* get_ui_new_listview(){
    QListView* view = new QListView();
    view->setSpacing(5);
    return view;
}

QString get_ui_font_face_name() {
    if (UI_FONT_FACE_NAME.empty()) {
        return global_font_family;
    }
    else {
        return QString::fromStdWString(UI_FONT_FACE_NAME);
    }
}

QString get_chat_font_face_name() {
    if (CHAT_FONT_FACE_NAME.size() > 0) {
        return QString::fromStdWString(CHAT_FONT_FACE_NAME);
    }
    return get_ui_font_face_name();
}

QString get_status_font_face_name() {
    if (STATUS_FONT_FACE_NAME.empty()) {
        return global_font_family;
    }
    else {
        return QString::fromStdWString(STATUS_FONT_FACE_NAME);
    }
}

QString get_color_stylesheet(float* bg_color, float* text_color, bool nofont, int font_size) {
    if ((!nofont) && (STATUS_BAR_FONT_SIZE > -1 || font_size > -1)) {
        int size = font_size > 0 ? font_size : STATUS_BAR_FONT_SIZE;
        QString	font_size_stylesheet = QString("font-size: %1px").arg(size);
        return QString("background-color: %1; color: %2; border: 0; %3;").arg(
            get_color_qml_string(bg_color[0], bg_color[1], bg_color[2]),
            get_color_qml_string(text_color[0], text_color[1], text_color[2]),
            font_size_stylesheet
        );
    }
    else {
        return QString("background-color: %1; color: %2; border: 0;").arg(
            get_color_qml_string(bg_color[0], bg_color[1], bg_color[2]),
            get_color_qml_string(text_color[0], text_color[1], text_color[2])
        );
    }

}

QString get_ui_stylesheet(bool nofont, int font_size) {
    return get_color_stylesheet(UI_BACKGROUND_COLOR, UI_TEXT_COLOR, nofont, font_size) + "background-color: transparent;";
}

QString get_status_stylesheet(bool nofont, int font_size) {
    return get_color_stylesheet(STATUS_BAR_COLOR, STATUS_BAR_TEXT_COLOR, nofont, font_size);
}

QString get_status_button_stylesheet(bool nofont, int font_size) {
    return get_color_stylesheet(STATUS_BAR_TEXT_COLOR, STATUS_BAR_COLOR, nofont, font_size) + "padding-left: 5px; padding-right: 5px;";
}

QString get_list_item_stylesheet() {
    return QString("background-color: red; padding-bottom: 20px; padding-top: 20px;");
}

QString get_scrollbar_stylesheet(){

    QColor ui_background_color = QColor::fromRgbF(UI_BACKGROUND_COLOR[0], UI_BACKGROUND_COLOR[1], UI_BACKGROUND_COLOR[2]);
    QColor handle_color;
    QColor handle_color_hover;
    // if color is black
    if (ui_background_color.red() == 0 && ui_background_color.green() == 0 && ui_background_color.blue() == 0) {
        handle_color = QColor::fromRgbF(0.15, 0.15, 0.15);
        handle_color_hover = QColor::fromRgbF(0.25, 0.25, 0.25);
    }
    else {
        handle_color = ui_background_color.lighter();
        handle_color_hover = handle_color.lighter();
    }

    QString handle_color_string = get_color_qml_string(handle_color.redF(), handle_color.greenF(), handle_color.blueF());
    QString handle_color_hover_string = get_color_qml_string(handle_color_hover.redF(), handle_color_hover.greenF(), handle_color_hover.blueF());

    return QString(R"(

        QScrollBar:horizontal {
            border: none;
            background: transparent;
            min-height: 8px;
            margin: 0px 20px 0 20px;
        }

        QScrollBar::handle:horizontal {
            background: %1;
            min-width: 8px;
        }

        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            background: none;
            width: 0px;
            subcontrol-position: left;
            subcontrol-origin: margin;
        }

        QScrollBar:vertical {
            border: none;
            background: transparent;
            margin: 0 0 0 0;
        }

        QScrollBar::handle:vertical {
            background: %1;
            min-height: 20px;
        }

        QScrollBar::handle:vertical:hover {
            background: %2;
        }

        QScrollBar::handle:horizontal:hover {
            background: %2;
        }

        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            background: none;
            height: 0px;
            subcontrol-position: top;
            subcontrol-origin: margin;
        }
    )").arg(handle_color_string, handle_color_hover_string);
}
QString get_selected_stylesheet(bool nofont) {
    if ((!nofont) && STATUS_BAR_FONT_SIZE > -1) {
        QString	font_size_stylesheet = QString("font-size: %1px").arg(STATUS_BAR_FONT_SIZE);
        return QString("background-color: %1; color: %2; border: 0; %3;").arg(
            get_color_qml_string(UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2]),
            get_color_qml_string(UI_SELECTED_TEXT_COLOR[0], UI_SELECTED_TEXT_COLOR[1], UI_SELECTED_TEXT_COLOR[2]),
            font_size_stylesheet
        );
    }
    else {
        return QString("background-color: %1; color: %2; border: 0;").arg(
            get_color_qml_string(UI_SELECTED_BACKGROUND_COLOR[0], UI_SELECTED_BACKGROUND_COLOR[1], UI_SELECTED_BACKGROUND_COLOR[2]),
            get_color_qml_string(UI_SELECTED_TEXT_COLOR[0], UI_SELECTED_TEXT_COLOR[1], UI_SELECTED_TEXT_COLOR[2])
        );
    }
}

int get_status_bar_height() {
    if (STATUS_BAR_FONT_SIZE > 0) {
        return STATUS_BAR_FONT_SIZE + 5;
    }
    else {
#ifdef SIOYEK_IOS
        return 20 * 3;
#else
        return 20;
#endif
    }
}
