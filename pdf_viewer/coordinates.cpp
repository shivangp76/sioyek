#include "coordinates.h"
#include "utils.h"
#include "document.h"
#include "document_view.h"

bool rects_intersect(fz_rect rect1, fz_rect rect2) {
    return range_intersects(rect1.x0, rect1.x1, rect2.x0, rect2.x1) && range_intersects(rect1.y0, rect1.y1, rect2.y0, rect2.y1);
}

bool rects_intersect(fz_irect rect1, fz_irect rect2) {
    return range_intersects(rect1.x0, rect1.x1, rect2.x0, rect2.x1) && range_intersects(rect1.y0, rect1.y1, rect2.y0, rect2.y1);
}

bool range_intersects(float range1_start, float range1_end, float range2_start, float range2_end) {
    if (range1_start > range1_end) {
        std::swap(range1_start, range1_end);
    }
    if (range2_start > range2_end) {
        std::swap(range2_start, range2_end);
    }
    if (range2_start > range1_end || range1_start > range2_end) {
        return false;
    }
    return true;
}

WindowPos::WindowPos(float x_, float y_) {
    x = static_cast<int>(x_);
    y = static_cast<int>(y_);
}

WindowPos::WindowPos(int x_, int y_) {
    x = x_;
    y = y_;
}

WindowPos::WindowPos() {
    x = 0;
    y = 0;
}

WindowPos::WindowPos(QPoint pos) {
    x = pos.x();
    y = pos.y();
}

bool are_same(const AbsoluteDocumentPos& lhs, const AbsoluteDocumentPos& rhs) {
    return are_same(lhs.x, rhs.x) && are_same(lhs.y, rhs.y);
}

//bool operator==(const AbsoluteDocumentPos& lhs, const AbsoluteDocumentPos& rhs)
//{
//	return are_same(lhs.x, rhs.x) && are_same(lhs.y, rhs.y);
//}

AbsoluteDocumentPos DocumentPos::to_absolute(Document* doc) const{
    return doc->document_to_absolute_pos(*this);
}

NormalizedWindowPos DocumentPos::to_window_normalized(DocumentView* document_view) const{
    return document_view->document_to_window_pos(*this);
}

WindowPos DocumentPos::to_window(DocumentView* document_view) const{
    return document_view->document_to_window_pos_in_pixels_uncentered(*this);
}

DocumentPos AbsoluteDocumentPos::to_document(Document* doc) const{
    return doc->absolute_to_page_pos(*this);
}

NormalizedWindowPos AbsoluteDocumentPos::to_window_normalized(DocumentView* document_view) const{
    return document_view->absolute_to_window_pos(*this);
}

WindowPos AbsoluteDocumentPos::to_window(DocumentView* document_view) const{
    return document_view->absolute_to_window_pos_in_pixels(*this);
}

DocumentPos NormalizedWindowPos::to_document(DocumentView* document_view) {
    WindowPos window_pos = document_view->normalized_window_to_window_pos(*this);
    return document_view->window_to_document_pos(window_pos);
}

AbsoluteDocumentPos NormalizedWindowPos::to_absolute(DocumentView* document_view) {
    WindowPos window_pos = document_view->normalized_window_to_window_pos(*this);
    return document_view->window_to_absolute_document_pos(window_pos);
}

WindowPos NormalizedWindowPos::to_window(DocumentView* document_view) {
    return document_view->normalized_window_to_window_pos(*this);
}

DocumentPos WindowPos::to_document(DocumentView* document_view) {
    return document_view->window_to_document_pos(*this);
}

AbsoluteDocumentPos WindowPos::to_absolute(DocumentView* document_view) {
    return document_view->window_to_absolute_document_pos(*this);
}

NormalizedWindowPos WindowPos::to_window_normalized(DocumentView* document_view) {
    return document_view->window_to_normalized_window_pos(*this);
}
AbsoluteRect::AbsoluteRect(fz_rect r) : EnhancedRect<fz_rect, AbsoluteDocumentPos>(r) {
}

AbsoluteRect DocumentRect::to_absolute(Document* doc) {
    return doc->document_to_absolute_rect(*this);
}

DocumentRect AbsoluteRect::to_document(Document* doc) const {
    return doc->absolute_to_page_rect(*this);
}


int WindowPos::manhattan(const WindowPos& other) {
    return std::abs(x - other.x) + std::abs(y - other.y);
}

AbsoluteRect::AbsoluteRect(AbsoluteDocumentPos top_left, AbsoluteDocumentPos bottom_right) {
    x0 = top_left.x;
    y0 = top_left.y;
    x1 = bottom_right.x;
    y1 = bottom_right.y;
}

NormalizedWindowRect AbsoluteRect::to_window_normalized(DocumentView* document_view) {
    return NormalizedWindowRect{ document_view->absolute_to_window_rect(*this) };
}

NormalizedWindowRect DocumentRect::to_window_normalized(DocumentView* document_view) {
    return NormalizedWindowRect{ document_view->document_to_window_rect(*this) };
}

DocumentPos DocumentRect::top_left() {
    return DocumentPos{ page, rect.x0, rect.y0 };
}

DocumentPos DocumentRect::bottom_right() {
    return DocumentPos{ page, rect.x1, rect.y1 };
}

NormalizedWindowRect::NormalizedWindowRect(NormalizedWindowPos top_left, NormalizedWindowPos bottom_right) {
    x0 = top_left.x;
    y0 = top_left.y;
    x1 = bottom_right.x;
    y1 = bottom_right.y;
}

NormalizedWindowRect::NormalizedWindowRect(fz_rect r) : EnhancedRect<fz_rect, NormalizedWindowPos>(r) {

}

AbsoluteRect::AbsoluteRect() : EnhancedRect<fz_rect, AbsoluteDocumentPos>(fz_empty_rect) {

}

DocumentRect::DocumentRect() : rect(fz_empty_rect), page(-1){

}

DocumentRect::DocumentRect(fz_rect r, int p) : rect(r), page(p) {

}
DocumentRect::DocumentRect(DocumentPos top_left, DocumentPos bottom_right, int p) {
    rect.x0 = top_left.x;
    rect.y0 = top_left.y;
    rect.x1 = bottom_right.x;
    rect.y1 = bottom_right.y;
    page = p;
}

NormalizedWindowRect::NormalizedWindowRect() {
    x0 = 0;
    y0 = 0;
    x1 = 0;
    y1 = 0;
}

fvec2 operator-(const AbsoluteDocumentPos& lhs, const AbsoluteDocumentPos& rhs) {
    return fvec2( lhs.x - rhs.x, lhs.y - rhs.y );
}

fvec2 operator-(const VirtualPos& lhs, const VirtualPos& rhs) {
    return fvec2( lhs.x - rhs.x, lhs.y - rhs.y );
}

fvec2 operator-(const DocumentPos& lhs, const DocumentPos& rhs) {
    return fvec2( lhs.x - rhs.x, lhs.y - rhs.y );
}

fvec2 operator-(const NormalizedWindowPos& lhs, const NormalizedWindowPos& rhs) {
    return fvec2( lhs.x - rhs.x, lhs.y - rhs.y );
}

ivec2 operator-(const WindowPos& lhs, const WindowPos& rhs) {
    return ivec2( lhs.x - rhs.x, lhs.y - rhs.y );
}

AbsoluteDocumentPos operator+(const AbsoluteDocumentPos& lhs, const fvec2& rhs) {
    return AbsoluteDocumentPos{ lhs.x + rhs[0], lhs.y + rhs[1]};
}

AbsoluteDocumentPos operator-(const AbsoluteDocumentPos& lhs, const fvec2& rhs) {
    return AbsoluteDocumentPos{ lhs.x - rhs[0], lhs.y - rhs[1]};
}

DocumentPos operator+(const DocumentPos& lhs, const fvec2& rhs) {
    return DocumentPos{ lhs.page, lhs.x + rhs[0], lhs.y + rhs[1]};
}

DocumentPos operator-(const DocumentPos& lhs, const fvec2& rhs) {
    return DocumentPos{ lhs.page, lhs.x - rhs[0], lhs.y - rhs[1]};
}

NormalizedWindowPos operator+(const NormalizedWindowPos& lhs, const fvec2& rhs) {
    return NormalizedWindowPos{ lhs.x + rhs[0], lhs.y + rhs[1]};
}

NormalizedWindowPos operator-(const NormalizedWindowPos& lhs, const fvec2& rhs) {
    return NormalizedWindowPos{ lhs.x - rhs[0], lhs.y - rhs[1]};
}

WindowPos operator+(const WindowPos& lhs, const ivec2& rhs) {
    return WindowPos{ lhs.x + rhs[0], lhs.y + rhs[1]};
}

WindowRect DocumentRect::to_window(DocumentView* document_view) {
    return document_view->document_to_window_irect(*this);
}

QRect DocumentRect::to_window_qrect(DocumentView* document_view) {
    WindowRect wind_rect = document_view->document_to_window_irect(*this);
    return QRect(wind_rect.x0, wind_rect.y0, wind_rect.width(), wind_rect.height());
}

PagelessDocumentPos DocumentPos::pageless() const {
    return PagelessDocumentPos { x, y };
}

PagelessDocumentRect rect_from_quad(fz_quad quad) {
    return PagelessDocumentRect(fz_rect_from_quad(quad));
}

bool NormalizedWindowRect::is_visible(float t) {
    return (x1 >= -1) && (x0 <= 1) && (y0 >= (-1 - t)) && (y1 <= (1 + t));
}

WindowRect AbsoluteRect::to_window(DocumentView* document_view) {
    return document_view->normalized_to_window_rect(to_window_normalized(document_view));
}

VirtualPos operator+(const VirtualPos& lhs, const fvec2& rhs) {
    return VirtualPos{ lhs.x + rhs[0], lhs.y + rhs[1]};
}

VirtualPos operator-(const VirtualPos& lhs, const fvec2& rhs) {
    return VirtualPos{ lhs.x - rhs[0], lhs.y - rhs[1]};
}

DocumentRect to_document(const WindowRect& window_rect, DocumentView* dv) {
    DocumentPos top_left = window_rect.top_left().to_document(dv);
    DocumentPos bottom_right = window_rect.bottom_right().to_document(dv);
    return DocumentRect(top_left, bottom_right, top_left.page);
}
AbsoluteDocumentPos AbsoluteDocumentPos::x_shift(float amount){
    auto res = *this;
    res.x += amount;
    return res;
}

AbsoluteDocumentPos AbsoluteDocumentPos::y_shift(float amount) {
    auto res = *this;
    res.y += amount;
    return res;
}

WindowRect NormalizedWindowRect::to_window(DocumentView* document_view) const {
    return document_view->normalized_to_window_rect(*this);
}

WindowRect::WindowRect(WindowPos top_left, WindowPos bottom_right) : EnhancedRect<fz_irect, WindowPos>(top_left, bottom_right) {

}

WindowRect::WindowRect() {

}
WindowRect WindowRect::from_qrect(const QRect& qrect){
    WindowRect res;
    res.x0 = qrect.x();
    res.y0 = qrect.y();
    res.x1 = qrect.x() + qrect.width();
    res.y1 = qrect.y() + qrect.height();
    return res;
}

std::array<float, 12> NormalizedWindowRect::to_vertices() const{
    std::array<float, 12> vertices = {
        x0, y0,
        x0, y1,
        x1, y0,

        x1, y1,
        x0, y1,
        x1, y0,
    };
    return vertices;
}

AbsoluteRect WindowRect::to_absolute(DocumentView* dv) {
    AbsoluteDocumentPos abs_top_left = dv->window_to_absolute_document_pos(top_left());
    AbsoluteDocumentPos abs_bottom_right = dv->window_to_absolute_document_pos(bottom_right());
    return AbsoluteRect(abs_top_left, abs_bottom_right);
}

bool DocumentRect::is_visible(DocumentView *document_view) {
    return document_view->is_rect_visible(*this);
}

DocumentPos DocumentPos::to_page(int target_page, Document* doc){
    DocumentPos res = *this;

    while (target_page > res.page){
        res.y -= doc->get_page_height(res.page);
        res.page++;
    }
    while (target_page < res.page){
        res.y += doc->get_page_height(res.page);
        res.page--;
    }
    return res;

}
