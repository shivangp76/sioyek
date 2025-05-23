#pragma once
#include "coordinates.h"
#include "types/overview_types.h"

class DocumentView;

enum class VisibleObjectType {
    Portal,
    PinnedPortal,
    PendingPortal,
    Bookmark,
    Highlight
};

struct VisibleObjectIndex {
    VisibleObjectType object_type;
    std::string uuid;
    //int index;

    void handle_move_begin(DocumentView* view, AbsoluteDocumentPos mouse_pos);
};

struct VisibleObjectResizeData {
    VisibleObjectType type;
    std::string object_uuid;
    AbsoluteRect original_rect;
    AbsoluteDocumentPos original_mouse_pos;
    OverviewSide side_index;
};

struct VisibleObjectScrollData {
    VisibleObjectType type;
    std::string object_uuid;
    float original_scroll_amount;
    std::optional<float> original_scroll_amount_x = {};
    AbsoluteDocumentPos original_mouse_pos;
    bool has_mouse_lifted = false;
    float speed = 0;
    float bookmark_height = 0;
};

