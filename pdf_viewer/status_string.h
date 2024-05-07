#pragma once

#include <qlineedit.h>

class MainWidget;

const std::vector<QString> STATUS_STRING_PARTS = {
    "current_page",
    "current_page_label",
    "num_pages",
    "chapter_name",
    "document_name",
    "search_results",
    "link_status",
    "waiting_for_symbol",
    "indexing",
    "preview_index",
    "synctex",
    "drag",
    "presentation",
    "auto_name",
    "visual_scroll",
    "locked_scroll",
    "highlight",
    "freehand_drawing",
    "mode_string",
    "closest_bookmark",
    "closest_portal",
    "rect_select",
    "point_select",
    "custom_message",
    "current_requirement_desc",
    "download"
};

class StatusLabelLineEdit : public QLineEdit {
public:
    std::optional<std::function<void()>> on_click = {};

    StatusLabelLineEdit(QWidget* parent = nullptr);

    void mousePressEvent(QMouseEvent* mevent);
};

std::function<std::pair<QString, std::vector<int>>()> compile_status_string(QString status_string, MainWidget* widget);