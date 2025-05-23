#include "types/overview_types.h"
#include "document_view.h"

AbsoluteDocumentPos OverviewState::get_absolute_pos() {
    return AbsoluteDocumentPos{ absolute_offset_x, absolute_offset_y };
}

float OverviewState::get_zoom_level(DocumentView* dv) {
    if (original_zoom_level.has_value()) {
        return zoom_level * dv->get_zoom_level() / original_zoom_level.value();
    }
    else {
        return zoom_level;
    }

}
