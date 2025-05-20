#include <QWidget>

#if defined(_WIN32)
    #include <windows.h>
#endif

#include "utils/window_utils.h"

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
