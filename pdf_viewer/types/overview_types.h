#pragma once
#include "coordinates.h"
#include "types/common_types.h"
#include "types/annotation_types.h"


struct OverviewResizeData {
    fz_rect original_rect;
    NormalizedWindowPos original_normal_mouse_pos;
    OverviewSide side_index;
};

struct OverviewMoveData {
    fvec2 original_offsets;
    NormalizedWindowPos original_normal_mouse_pos;
};

struct OverviewTouchMoveData {
    AbsoluteDocumentPos overview_original_pos_absolute;
    NormalizedWindowPos original_mouse_normalized_pos;
};

struct OverviewState {
    float absolute_offset_y;
    float absolute_offset_x = 0;
    float zoom_level = -1;
    Document* doc = nullptr;
    std::optional<std::string> overview_type = {};
    std::vector<DocumentRect> highlight_rects;
    std::optional<Portal> source_portal = {};
    std::optional<AbsoluteRect> source_rect = {};
    std::optional<float> original_zoom_level = {};

    float get_zoom_level(DocumentView* dv);
    AbsoluteDocumentPos get_absolute_pos();
};
