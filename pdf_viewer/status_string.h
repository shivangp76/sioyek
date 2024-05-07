#pragma once

#include <qlineedit.h>
#include "main_widget.h"

enum class StatusStringPart {
    CURRENT_PAGE,
    CURRENT_PAGE_LABEL,
    NUM_PAGES,
    CHAPTER_NAME,
    DOCUMENT_NAME,
    SEARCH_RESULTS,
    LINK_STATUS,
    WAITING_FOR_SYMBOL,
    INDEXING,
    PREVIEW_INDEX,
    SYNCTEX,
    DRAG,
    PRESENTATION,
    AUTO_NAME,
    VISUAL_SCROLL,
    LOCKED_SCROLL,
    HIGHLIGHT,
    FREEHAND_DRAWING,
    MODE_STRING,
    CLOSEST_BOOKMARK,
    CLOSEST_PORTAL,
    RECT_SELECT,
    POINT_SELECT,
    CUSTOM_MESSAGE,
    CURRENT_REQUIREMENT_DESC,
    DOWNLOAD
};

class StatusLabelLineEdit : public QLineEdit {
public:
    std::optional<std::function<void()>> on_click = {};

    StatusLabelLineEdit(QWidget* parent = nullptr);

    void mousePressEvent(QMouseEvent* mevent);
};

std::function<std::pair<QString, std::vector<int>>()> compile_status_string(QString status_string, MainWidget* widget);