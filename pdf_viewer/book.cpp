#include "book.h"
#include "document.h"
#include "document_view.h"


bool operator==(const DocumentViewState& lhs, const DocumentViewState& rhs)
{
    return (lhs.book_state.offset_x == rhs.book_state.offset_x) &&
        (lhs.book_state.offset_y == rhs.book_state.offset_y) &&
        (lhs.book_state.zoom_level == rhs.book_state.zoom_level) &&
        (lhs.document_path == rhs.document_path);
}

bool operator==(const CachedPageData& lhs, const CachedPageData& rhs) {
    if (lhs.doc != rhs.doc) return false;
    if (lhs.page != rhs.page) return false;
    if (lhs.zoom_level != rhs.zoom_level) return false;
    return true;
}

Portal Portal::with_src_offset(float src_offset)
{
    Portal res = Portal();
    res.src_offset_y = src_offset;
    return res;
}

QJsonObject Mark::to_json(std::string doc_checksum) const
{
    QJsonObject res;
    res[Mark::Y_OFFSET_COLUMN_NAME] = y_offset;
    if (x_offset) {
        res[Mark::X_OFFSET_COLUMN_NAME] = x_offset.value();
    }
    res[Mark::SYMBOL_COLUMN_NAME] = symbol;

    add_metadata_to_json(res, "mark");
    return res;
}


bool operator==(const fz_point& lhs, const fz_point& rhs) {
    return (lhs.y == rhs.y) && (lhs.x == rhs.x);
}


AbsoluteRect FreehandDrawing::bbox() const {
    AbsoluteRect res;
    if (points.size() > 0) {
        res.x0 = points[0].pos.x;
        res.x1 = points[0].pos.x;
        res.y0 = points[0].pos.y;
        res.y1 = points[0].pos.y;
        for (int i = 1; i < points.size(); i++) {
            res.x0 = std::min(points[i].pos.x, res.x0);
            res.x1 = std::max(points[i].pos.x, res.x1);
            res.y0 = std::min(points[i].pos.y, res.y0);
            res.y1 = std::max(points[i].pos.y, res.y1);
        }
    }
    return res;
}

void SearchResult::fill(Document* doc) {
    if (rects.size() == 0) {
        doc->fill_search_result(this);
    }
}

std::string reference_type_string(ReferenceType rt) {
    if (rt == ReferenceType::NoReference) return "none";
    if (rt == ReferenceType::Equation) return "equation";
    if (rt == ReferenceType::Reference) return "reference";
    if (rt == ReferenceType::Abbreviation) return "abbreviation";
    if (rt == ReferenceType::Generic) return "generic";
    if (rt == ReferenceType::Link) return "link";
    if (rt == ReferenceType::RefLink) return "reflink";
    return "";
}

void Portal::update_merged_rect(Document* doc) const{

    if (!is_visible()) return;
    if (is_pinned()) return;

    if (merged_rect) {
        if (!merged_rect->intersects(get_actual_rectangle())) {
            merged_rect = {};
        }
    }
    if (!merged_rect.has_value()) {
        int source_page = doc->absolute_to_page_pos(AbsoluteDocumentPos{ 0, src_offset_y }).page;
        float max_intersection_area = 0;
        for (const auto& link : doc->get_page_merged_pdf_links(source_page)) {
            if (link.rects.size() > 0) {
                AbsoluteRect link_rect = DocumentRect{ link.rects[0], source_page }.to_absolute(doc);
                float interseciton_area = link_rect.intersect_rect(get_actual_rectangle()).area();
                if (interseciton_area > max_intersection_area) {
                    max_intersection_area = interseciton_area;
                    merged_rect = link_rect;
                }
            }
        }
    }
}

void SmartViewCandidate::set_highlight_rects(std::vector<DocumentRect> rects){
    highlight_rects_ = rects;
    are_highlights_computed = true;
}

const std::vector<DocumentRect> SmartViewCandidate::get_highlight_rects() {
    if (are_highlights_computed) {
        return highlight_rects_;
    }
    else {
        if (highlight_rects_func.has_value()) {
            highlight_rects_ = highlight_rects_func.value()();
            are_highlights_computed = true;
            return highlight_rects_;
        }
    }
    return {};
}




AbsoluteDocumentPos FreehandDrawingPoint::get_pos(){
    return pos;
}

QList<FreehandDrawingPoint> FreehandDrawing::get_points(){
    return points;
}
