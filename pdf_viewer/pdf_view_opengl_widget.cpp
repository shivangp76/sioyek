
#include <cmath>

#include <qcolor.h>
#include <qmouseevent.h>
#include <qapplication.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qpainterpath.h>>
#include <qregion.h>>
#include <qtextdocument.h>
#include <qabstracttextdocumentlayout.h>
#include <qtextcursor.h>


#include "pdf_view_opengl_widget.h"
#include "path.h"
#include "book.h"
#include "document.h"
#include "document_view.h"
#include "pdf_renderer.h"
#include "config.h"
#include "utils.h"
#include "background_tasks.h"

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#ifndef GL_PRIMITIVE_RESTART_FIXED_INDEX
#define GL_PRIMITIVE_RESTART_FIXED_INDEX  0x8D69
#endif

extern int NUM_PAGE_COLUMNS;
extern bool DEBUG_DISPLAY_FREEHAND_POINTS;
extern bool DEBUG_SMOOTH_FREEHAND_DRAWINGS;
extern Path shader_path;
extern float GAMMA;
extern float BACKGROUND_COLOR[3];
extern float DARK_MODE_CONTRAST;
extern float ZOOM_INC_FACTOR;
extern float SCROLL_ZOOM_INC_FACTOR;
extern float VERTICAL_MOVE_AMOUNT;
extern float HIGHLIGHT_COLORS[26 * 3];
extern bool SHOULD_DRAW_UNRENDERED_PAGES;
extern float CUSTOM_BACKGROUND_COLOR[3];
extern float CUSTOM_TEXT_COLOR[3];
// extern bool RULER_MODE;
extern float PAGE_SEPARATOR_WIDTH;
extern float PAGE_SEPARATOR_COLOR[3];
extern float RULER_PADDING;
extern float FASTREAD_OPACITY;
extern bool PRERENDER_NEXT_PAGE;
extern int PRERENDERED_PAGE_COUNT;
extern bool SHOULD_HIGHLIGHT_LINKS;
extern bool SHOULD_HIGHLIGHT_UNSELECTED_SEARCH;
extern float UNSELECTED_SEARCH_HIGHLIGHT_COLOR[3];
extern int KEYBOARD_SELECT_FONT_SIZE;
extern float CUSTOM_COLOR_CONTRAST;
extern float DISPLAY_RESOLUTION_SCALE;
extern float KEYBOARD_SELECT_BACKGROUND_COLOR[4];
extern float KEYBOARD_SELECT_TEXT_COLOR[4];
// extern bool ALPHABETIC_LINK_TAGS;
extern bool NUMERIC_TAGS;
extern int NUM_H_SLICES;
extern int NUM_V_SLICES;
extern bool SLICED_RENDERING;
//extern float BOOKMARK_RECT_SIZE;
extern bool RENDER_FREETEXT_BORDERS;
extern float FREETEXT_BOOKMARK_FONT_SIZE;
extern float STRIKE_LINE_WIDTH;
extern int RULER_DISPLAY_MODE;
extern float RULER_COLOR[3];
extern float RULER_MARKER_COLOR[3];
extern bool ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE;
extern bool HIDE_OVERLAPPING_LINK_LABELS;
extern bool PRESERVE_IMAGE_COLORS;
extern bool INVERTED_PRESERVED_IMAGE_COLORS;
extern int SELECTED_TEXT_HIGHLIGHT_STYLE;
extern int HIGHLIGHT_STYLE;
extern int OVERVIEW_HIGHLIGHT_STYLE;
extern bool VISUALIZE_RULER_THRESHOLDS;
extern bool DEBUG;
extern int BACKGROUND_HIGHLIGHT_MINIMUM_LIGHTNESS;

extern float BOX_HIGHLIGHT_BOOKMARK_TRANSPARENCY;

extern int NUM_PRERENDERED_NEXT_SLIDES;
extern int NUM_PRERENDERED_PREV_SLIDES;

extern float DEFAULT_SEARCH_HIGHLIGHT_COLOR[3];
extern float DEFAULT_LINK_HIGHLIGHT_COLOR[3];
extern float DEFAULT_SYNCTEX_HIGHLIGHT_COLOR[3];
extern float DEFAULT_TEXT_HIGHLIGHT_COLOR[3];
extern float DEFAULT_VERTICAL_LINE_COLOR[4];
extern float KEYBOARD_SELECTED_TAG_TEXT_COLOR[4];
extern float KEYBOARD_SELECTED_TAG_BACKGROUND_COLRO[4];
extern float QUESTION_BOOKMARK_BACKGROUND_COLOR[4];
extern float QUESTION_BOOKMARK_TEXT_COLOR[3];
extern float OVERVIEW_REFERENCE_HIGHLIGHT_COLOR[3];

extern float VISUAL_MARK_NEXT_PAGE_FRACTION;
extern float VISUAL_MARK_NEXT_PAGE_THRESHOLD;
extern bool ALWAYS_RENDER_BOOKMARKS;

extern int RULER_UNDERLINE_PIXEL_WIDTH;
extern UIRect PORTRAIT_EDIT_PORTAL_UI_RECT;
extern UIRect LANDSCAPE_EDIT_PORTAL_UI_RECT;

extern UIRect PORTRAIT_BACK_UI_RECT;
extern UIRect PORTRAIT_FORWARD_UI_RECT;
extern UIRect LANDSCAPE_BACK_UI_RECT;
extern UIRect LANDSCAPE_FORWARD_UI_RECT;
extern UIRect PORTRAIT_VISUAL_MARK_PREV;
extern UIRect PORTRAIT_VISUAL_MARK_NEXT;
extern UIRect LANDSCAPE_VISUAL_MARK_PREV;
extern UIRect LANDSCAPE_VISUAL_MARK_NEXT;
extern UIRect PORTRAIT_MIDDLE_LEFT_UI_RECT;
extern UIRect PORTRAIT_MIDDLE_RIGHT_UI_RECT;
extern UIRect LANDSCAPE_MIDDLE_LEFT_UI_RECT;
extern UIRect LANDSCAPE_MIDDLE_RIGHT_UI_RECT;
extern UIRect PORTRAIT_EDIT_PORTAL_UI_RECT;
extern UIRect LANDSCAPE_EDIT_PORTAL_UI_RECT;

extern float LINE_SELECT_RULER_COLOR[3];
extern int LINE_SELECT_RULER_DISPLAY_MODE;

extern std::wstring BACK_RECT_TAP_COMMAND;
extern std::wstring BACK_RECT_HOLD_COMMAND;
extern std::wstring FORWARD_RECT_TAP_COMMAND;
extern std::wstring FORWARD_RECT_HOLD_COMMAND;
extern std::wstring TOP_CENTER_TAP_COMMAND;
extern std::wstring TOP_CENTER_HOLD_COMMAND;
extern std::wstring VISUAL_MARK_NEXT_TAP_COMMAND;
extern std::wstring VISUAL_MARK_NEXT_HOLD_COMMAND;
extern std::wstring VISUAL_MARK_PREV_TAP_COMMAND;
extern std::wstring VISUAL_MARK_PREV_HOLD_COMMAND;
extern std::wstring MIDDLE_LEFT_RECT_TAP_COMMAND;
extern std::wstring MIDDLE_LEFT_RECT_HOLD_COMMAND;
extern std::wstring MIDDLE_RIGHT_RECT_TAP_COMMAND;
extern std::wstring MIDDLE_RIGHT_RECT_HOLD_COMMAND;
extern std::wstring MIDDLE_RIGHT_RECT_HOLD_COMMAND;
extern std::wstring TOP_CENTER_TAP_COMMAND;
extern std::wstring TOP_CENTER_HOLD_COMMAND;
extern std::wstring TAG_FONT_FACE;

const int SELECTED_BORDER_PEN_SIZE = 2;
const float SELECTED_BORDER_COLOR[3] = { 0.7f, 0.7f, 0.7f };
extern bool BACKGROUND_PIXEL_FIX;

GLfloat g_quad_vertex[] = {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    -1.0f, 1.0f,
    1.0f, 1.0f
};

GLfloat g_quad_uvs[] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
};

GLfloat g_quad_uvs_rotated[] = {
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 0.0f
};

GLfloat rotation_uvs[4][8] = {
    {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f,
    1.0f, 1.0f
    },
    {
    0.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 1.0f,
    1.0f, 0.0f
    },
    {
    1.0f, 1.0f,
    0.0f, 1.0f,
    1.0f, 0.0f,
    0.0f, 0.0f
    },
    {
    1.0f, 0.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    0.0f, 1.0f
    },
};

// OpenGLSharedResources PdfViewOpenGLWidget::shared_gl_objects;


void generate_bezier_with_endpoints_and_velocity(
    Vec<float, 2> p0, 
    Vec<float, 2> p1, 
    Vec<float, 2> v0, 
    Vec<float, 2> v1, 
    int n_points,
    float thickness,
    std::vector<FreehandDrawingPoint>& output
    ) {
    float alpha = 1.0f / n_points;
    auto q0 = p0;
    auto q1 = v0;
    auto q2 = (p1 - p0) * 3 - v0 * 2 - v1;
    auto q3 = (p0 - p1) * 2 + v0 + v1;

    for (int i = 0; i < n_points; i++) {
        float t = alpha * i;
        float tt = t * t;
        float ttt = tt * t;
        Vec<float, 2> point = q0 + q1 *t + q2 * tt + q3 * ttt;
        output.push_back(FreehandDrawingPoint{AbsoluteDocumentPos{point.x(), point.y()}, thickness});
    }

}

bool num_slices_for_page_rect(PagelessDocumentRect page_rect, int* h_slices, int* v_slices) {
    /*
    determines the number of vertical/horizontal slices when rendering
    normally we don't use slicing when SLICED_RENDERING is false, unless
    there is a giant page which would crash the application if we tried rendering
    it as a single page
    returns true if we are using sliced rendering
    */
    if (page_rect.y1 > 2000.0) {
        *v_slices = static_cast<int>(page_rect.y1 / 500.0f);
        if (SLICED_RENDERING) {
            *h_slices = NUM_H_SLICES;
            return true;
        }
        else {
            *h_slices = 1;
            return true;
        }
    }
    else {
        if (SLICED_RENDERING) {
            *h_slices = NUM_H_SLICES;
            *v_slices = NUM_V_SLICES;
            return true;
        }
        else {
            *h_slices = 1;
            *v_slices = 1;
            return false;
        }
    }
}

std::string read_file_contents(const Path& path) {
#ifdef SIOYEK_MOBILE
    std::wstring actual_path = path.get_path();
    QFile qfile(QString::fromStdWString(path.get_path()));
    qfile.open(QIODeviceBase::Text | QIODeviceBase::ReadOnly);
    std::string content = qfile.readAll().toStdString();
    qfile.close();
    return content;
#else
    std::wifstream stream = open_wifstream(path.get_path());
    std::wstring content;

    if (stream.is_open()) {
        std::wstringstream sstr;
        sstr << stream.rdbuf();
        content = sstr.str();
        stream.close();
        return utf8_encode(content);
    }
    else {
        return "";
    }
#endif
}

void PdfViewOpenGLWidget::render_line_window(float gl_vertical_pos, std::optional<NormalizedWindowRect> ruler_rect) {
#ifdef SIOYEK_OPENGL_BACKEND
    render_line_window_opengl_backend(gl_vertical_pos, ruler_rect);
#else
    if (ruler_rect.has_value()){
        QRect ruler_qrect = document_view->normalized_to_window_qrect(ruler_rect.value());
        ruler_qrect.adjust(-20, 0, 20, 0);

        QColor ruler_color = qcc4(DEFAULT_VERTICAL_LINE_COLOR);
        QRegion full_region(rect());
        QRegion clip_region = full_region.subtracted(ruler_qrect);
        QBrush ruler_brush(ruler_color);
        painter.setClipRegion(clip_region);
        painter.fillRect(rect(), ruler_brush);
    }
#endif
}

void PdfViewOpenGLWidget::render_highlight_window(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels) {
#ifdef SIOYEK_OPENGL_BACKEND
    render_highlight_window_opengl_backend(window_rect, flags,  line_width_in_pixels);
#else
    render_highlight_window_qpainter_backend(window_rect, flags, line_width_in_pixels);
#endif
}


void SioyekRendererBackend::render_highlight_absolute(AbsoluteRect absolute_document_rect, int flags) {
    NormalizedWindowRect window_rect;
    if (scratch()) {
        window_rect = scratch()->absolute_to_window_rect(absolute_document_rect);
    }
    else {
        window_rect = absolute_document_rect.to_window_normalized(dv());
    }
    render_highlight_window(window_rect, flags);
}

void SioyekRendererBackend::render_highlight_document(DocumentRect doc_rect, int flags) {
    NormalizedWindowRect window_rect = doc_rect.to_window_normalized(dv());
    render_highlight_window(window_rect, flags);
}

void SioyekRendererBackend::render_scratchpad() {

    /* bool use_cached_framebuffer = can_use_cached_scratchpad_framebuffer(); */

    begin_native_painting();
    clear_background_color();
    end_native_painting();

    for (auto [pixmap, rect] : scratch()->pixmaps) {
        WindowRect window_rect = rect.to_window(scratch());

        QRect window_qrect = QRect(
            window_rect.x0,
            window_rect.y0,
            window_rect.width(),
            window_rect.height()
        );
        draw_pixmap(window_qrect, &pixmap);
    }

    for (auto [pixmap, rect] : document_view->moving_pixmaps) { // highlight moving pixmaps
        WindowRect window_rect = rect.to_window(scratch());
        QRect window_qrect = QRect(window_rect.x0, window_rect.y0, window_rect.width(), window_rect.height());
        fill_rect(window_qrect.adjusted(-2, -2, 2, 2), QColor(255, 255, 0));
        draw_pixmap(window_qrect, &pixmap);
    }

    /* if (use_cached_framebuffer) { */
    /*     painter->drawImage(rect(), cached_framebuffer.value()); */
    /* } */

    begin_native_painting();

    prepare_line_drawing_pipeline();

    std::vector<FreehandDrawing> pending_drawing;
    if (document_view->current_drawing.points.size() > 1) {
        pending_drawing.push_back(document_view->current_drawing);
    }


    auto scratchpad = scratch();
    if (scratchpad->get_non_compiled_drawings().size() > 50 || scratchpad->is_compile_invalid()) {
        compile_drawings(scratchpad, scratchpad->get_all_drawings());
    }

    enable_multisampling();

    render_compiled_drawings();
    prepare_non_compiled_line_drawing_pipeline();
    render_drawings(&painter, scratchpad, scratchpad->get_non_compiled_drawings());
    render_drawings(&painter, scratchpad, document_view->moving_drawings, true);
    render_drawings(&painter, scratchpad, document_view->moving_drawings, false);
    render_drawings(&painter, scratchpad, pending_drawing);

    disable_multisampling();

    bind_default();
    prepare_highlight_pipeline();

    render_selected_rectangle();

    end_native_painting();

}


#ifdef SIOYEK_OPENGL_BACKEND
PdfViewOpenGLWidget::PdfViewOpenGLWidget(DocumentView* document_view_, PdfRenderer* pdf_renderer_, DocumentManager* docman, bool is_helper_, QWidget* parent) :
    QOpenGLWidget(parent)
#else
PdfViewOpenGLWidget::PdfViewOpenGLWidget(DocumentView* document_view_, PdfRenderer* pdf_renderer, DocumentManager* docman, bool is_helper, QWidget* parent) :
    QWidget(parent),
    painter(this),
    document_view(document_view_),
    pdf_renderer(pdf_renderer),
    document_manager(docman),
    is_helper(is_helper)
#endif
{
    document_view = document_view_;
    pdf_renderer = pdf_renderer_;
    document_manager = docman;
    is_helper = is_helper_;

#ifdef SIOYEK_OPENGL_BACKEND
    QSurfaceFormat format;
#ifdef SIOYEK_ANDROID
    format.setVersion(3, 1);
#elif defined(SIOYEK_IOS)
    format.setVersion(3, 0);
#else
    format.setVersion(3, 3);
#endif
    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    this->setFormat(format);
#else

#endif

    for (int i = 0; i < 26; i++) {
        document_view->visible_drawing_mask[i] = true;
    }

    bookmark_icon = QPixmap(":/icons/B.svg");
    //portal_icon = QPixmap(":/end.png");
    portal_icon = QIcon(":/icons/P.svg");
    bookmark_icon_white = QIcon(":/icons/B_white.svg");
    portal_icon_white = QIcon(":/icons/P_white.svg");
    hourglass_icon = QIcon(":/icons/hourglass.svg");
    download_icon = QPixmap(":/icons/download.svg");
    download_icon_dark = QPixmap(":/icons/download_dark.svg");
}

void SioyekRendererBackend::handle_escape() {
}

bool SioyekRendererBackend::valid_document() {
    if (dv()) {
        if (dv()->get_document()) {
            return true;
        }
    }
    return false;
}

void SioyekRendererBackend::render_overview(OverviewState overview, bool draw_border) {
    if (!valid_document()) return;

    float view_width = static_cast<int>(dv()->get_view_width() * dv()->overview_half_width);
    float view_height = static_cast<int>(dv()->get_view_height() * dv()->overview_half_height);

    NormalizedWindowRect window_rect = dv()->get_overview_rect_pixel_perfect(
        dv()->get_view_width(),
        dv()->get_view_height(),
        view_width,
        view_height);
    window_rect.y0 = -window_rect.y0;
    window_rect.y1 = -window_rect.y1;

    if (overview.source_rect.has_value()) {
        window_rect = overview.source_rect->to_window_normalized(document_view);
    }

    if (!window_rect.is_visible()){
        return;
    }

    // make sure overview zoom level is set (if it is -1, then we should fit it to page)
    float zoom_level = dv()->get_overview_zoom_level(overview);
    overview.zoom_level = zoom_level;
    // overview.highlight_rects = overview.ge

    render_overview_backend(window_rect, overview, draw_border);
#ifdef SIOYEK_OPENGL_BACKEND
#else
    if (window_rect.height() > 0) {
        std::swap(window_rect.y0, window_rect.y1);
    }
    render_overview_qpainter_backend(window_rect, overview, draw_border);
#endif

    if (dv()->overview_page && (dv()->overview_page->highlight_rects.size() > 0)) {

        if ((dv()->overview_page->overview_type == "reference") || (dv()->overview_page->overview_type == "reflink")) {
            QRect download_rect = dv()->get_overview_download_rect().to_qrect();
            render_ui_icon_for_current_color_mode(download_icon, download_icon_dark, download_rect);

            painter.beginNativePainting();
        }
    }
}

void PdfViewOpenGLWidget::render_overview_backend(NormalizedWindowRect window_rect, OverviewState overview, bool draw_border) {
    render_overview_opengl_backend(window_rect, overview, draw_border);
}

void PdfViewOpenGLWidget::draw_overview_background(std::optional<OverviewState> maybe_overview){

    float border_vertices[4 * 2];
    get_overview_window_vertices(border_vertices, maybe_overview);

    float bg_color[] = { 1.0f, 1.0f, 1.0f };
    get_background_color(bg_color);

#ifdef SIOYEK_OPENGL_BACKEND
    glDisable(GL_BLEND);

    glUseProgram(shared_gl_objects.highlight_program);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(border_vertices), border_vertices, GL_DYNAMIC_DRAW);
    glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, bg_color);
    glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
#else
    QRect overview_qrect = document_view->normalized_to_window_qrect(document_view->get_overview_rect());
    painter.fillRect(overview_qrect, QBrush(convert_float3_to_qcolor(bg_color)));
#endif
}

void PdfViewOpenGLWidget::draw_overview_border(std::optional<OverviewState> maybe_overview, float* color){
    float border_color[3] = {0.5f, 0.5f, 0.5f};
#ifdef SIOYEK_OPENGL_BACKEND
    glUseProgram(shared_gl_objects.highlight_program);
    glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, color ? color : border_color);
    render_highlight_window(document_view->get_overview_rect(maybe_overview), HRF_BORDER);
#else

    set_highlight_color(border_color, 1);
    render_highlight_window(document_view->get_overview_rect(maybe_overview), HRF_BORDER);
#endif
}

Document* SioyekRendererBackend::doc(std::optional<OverviewState> overview){
    if (overview){
        if (overview->doc != nullptr) {
            return overview->doc;
        }
    }

    return dv()->get_document();
}

void SioyekRendererBackend::render_page(int page_number, std::optional<OverviewState> overview, ColorPalette forced_color_palette, bool stencils_allowed) {
    if (!valid_document()) return;

    int nh, nv;

    float page_width = doc(overview)->get_page_width(page_number);
    float page_height = doc(overview)->get_page_height(page_number);
    PagelessDocumentRect page_rect({ 0, 0, page_width, page_height });

    if ((page_width < 0) || (page_height < 0)) return;

    float zoom_level = overview.has_value() ? overview->get_zoom_level(dv()) : dv()->get_zoom_level();

    bool is_sliced = num_slices_for_page_rect(page_rect, &nh, &nv);

    ColorPalette actual_color_palette = get_actual_color_palette(forced_color_palette);
    for (int i = 0; i < nh * nv; i++) {
        int v_index = i / nh;
        int h_index = i % nh;

        int rendered_width = -1;
        int rendered_height = -1;

        int index = i;

        if (!is_sliced) {
            index = -1;
        }

        // todo: just replace this with page_width and page_height from above
        float slice_document_width = doc(overview)->get_page_width(page_number);
        float slice_document_height = doc(overview)->get_page_height(page_number);
        PagelessDocumentRect slice_document_rect;
        slice_document_rect.x0 = 0;
        slice_document_rect.x1 = slice_document_width;
        slice_document_rect.y0 = 0;
        slice_document_rect.y1 = slice_document_height;
        slice_document_rect = get_index_rect(slice_document_rect, i, nh, nv);

        NormalizedWindowRect slice_window_rect = overview.has_value() ?
             document_view->document_to_overview_rect(DocumentRect(slice_document_rect, page_number)) :
             DocumentRect(slice_document_rect, page_number).to_window_normalized(dv());

        // we add some slack so we pre-render nearby slices
        NormalizedWindowRect full_window_rect;
        full_window_rect.x0 = -1;
        full_window_rect.x1 = 1;
        full_window_rect.y0 = -1.5f;
        full_window_rect.y1 = 1.5f;

        // don't render invisible slices
        if (is_sliced && (!rects_intersect(slice_window_rect, full_window_rect))) {
            continue;
        }

        auto texture = pdf_renderer->find_rendered_page(doc(overview)->get_path(),
            page_number,
            actual_color_palette,
            doc(overview)->should_render_pdf_annotations(),
            index,
            nh,
            nv,
            zoom_level,
            get_device_pixel_ratio(),
            &rendered_width,
            &rendered_height);

        if (is_helper && !texture) {
            is_helper_waiting_for_render = true;
        }


        // when rotating, we swap nv and nh 
        int nh_ = nh;
        int nv_ = nv;

        if (document_view->rotation_index % 2 == 1) {
            std::swap(rendered_width, rendered_height);
            std::swap(nh_, nv_);
        }

        float page_vertices[4 * 2];
        float slice_height = doc(overview)->get_page_height(page_number) / nv_;
        float slice_width = doc(overview)->get_page_width(page_number) / nh_;

        PagelessDocumentRect page_rect;
        PagelessDocumentRect full_page_rect({ 0,
                0,
                 doc(overview)->get_page_width(page_number),
                 doc(overview)->get_page_height(page_number)
        });

        fz_irect irect = fz_round_rect(fz_transform_rect(full_page_rect,
            fz_scale(zoom_level, zoom_level)));
        WindowRect full_page_irect;
        full_page_irect.x0 = irect.x0;
        full_page_irect.x1 = irect.x1;
        full_page_irect.y0 = irect.y0;
        full_page_irect.y1 = irect.y1;

        PagelessDocumentRect page_content = full_page_rect;
        if (dv()->get_page_space_x() < 0) {
            if (page_number % NUM_PAGE_COLUMNS > 0) {
                page_content.x0 -= dv()->get_page_space_x();
            }
            if(page_number % NUM_PAGE_COLUMNS < (NUM_PAGE_COLUMNS - 1)) {
                page_content.x1 += dv()->get_page_space_x();
            }
        }

#ifdef SIOYEK_OPENGL_BACKEND
        if (BACKGROUND_PIXEL_FIX || (document_view->is_two_page_mode() && (stencils_allowed))) {
            if (BACKGROUND_PIXEL_FIX) {
                page_content.x1 -= 1.0f / zoom_level;
                page_content.y1 -= 1.0f / zoom_level;
            }

            glClear(GL_STENCIL_BUFFER_BIT);
            enable_stencil();
            write_to_stencil();
            draw_stencil_rects(page_number, {page_content});
            use_stencil_to_write(true);
        }
#else
        if (dv()->is_two_page_mode()){
            QRect window_rect = DocumentRect{page_content, page_number}.to_window(document_view).to_qrect();
            painter.setClipRect(window_rect);
        }
#endif

        if (is_sliced) {

            if (document_view->rotation_index == 1) {
                std::swap(h_index, v_index);
                h_index = nv - h_index - 1;
            }
            else if (document_view->rotation_index == 2) {
                v_index = nv - v_index - 1;
            }
            else if (document_view->rotation_index == 3) {
                std::swap(h_index, v_index);
            }


            page_rect = { h_index * slice_width,
                v_index * slice_height,
                (h_index + 1) * slice_width,
                (v_index + 1) * slice_height
            };

            WindowRect page_irect;
            page_irect.x0 = ((full_page_irect.x1 - full_page_irect.x0) / nh_) * h_index;
            page_irect.x1 = ((full_page_irect.x1 - full_page_irect.x0) / nh_) * (h_index + 1);
            if (h_index == (nh_ - 1)) {
                page_irect.x1 = full_page_irect.x1;
            }

            page_irect.y0 = ((full_page_irect.y1 - full_page_irect.y0) / nv_) * v_index;
            page_irect.y1 = ((full_page_irect.y1 - full_page_irect.y0) / nv_) * (v_index + 1);
            if (v_index == (nv_ - 1)) {
                page_irect.y1 = full_page_irect.y1;
            }

            float w = full_page_rect.x1 - full_page_rect.x0;
            float h = full_page_rect.y1 - full_page_rect.y0;

            page_rect.x0 = static_cast<float>(page_irect.x0) / static_cast<float>(full_page_irect.x1 - full_page_irect.x0) * w;
            page_rect.x1 = static_cast<float>(page_irect.x1) / static_cast<float>(full_page_irect.x1 - full_page_irect.x0) * w;
            page_rect.y0 = static_cast<float>(page_irect.y0) / static_cast<float>(full_page_irect.y1 - full_page_irect.y0) * h;
            page_rect.y1 = static_cast<float>(page_irect.y1) / static_cast<float>(full_page_irect.y1 - full_page_irect.y0) * h;
        }
        else {

            page_rect = full_page_rect;
        }


#ifdef SIOYEK_QT6
        float device_pixel_ratio = static_cast<float>(get_device_pixel_ratio());
#else
        float device_pixel_ratio = QApplication::desktop()->devicePixelRatioF();
#endif

        if (DISPLAY_RESOLUTION_SCALE > 0) {
            device_pixel_ratio *= DISPLAY_RESOLUTION_SCALE;
        }

        // bool is_not_exact = true;
        // if ((full_page_irect.y1 - full_page_irect.y0) % nv == 0) {
        //     is_not_exact = false;
        // }
        NormalizedWindowRect window_rect = overview.has_value() ?
            document_view->document_to_overview_rect(DocumentRect(page_rect, page_number), overview) :
            dv()->document_to_window_rect_pixel_perfect(DocumentRect(page_rect, page_number),
            static_cast<int>(rendered_width / device_pixel_ratio),
            static_cast<int>(rendered_height / device_pixel_ratio), is_sliced);


        render_texture(texture, window_rect, forced_color_palette);

        
        if (dv()->is_two_page_mode() && (stencils_allowed)) {
            disable_stencil();
        }

#ifdef SIOYEK_OPENGL_BACKEND
        if ((document_view->get_current_color_mode() != ColorPalette::Normal) &&
            (PRESERVE_IMAGE_COLORS) && (!overview.has_value()) &&
            (forced_color_palette == ColorPalette::None) &&
            (stencils_allowed)) {
            // render images in forced palette mode
            fz_stext_page * stext_page = dv()->get_document()->get_stext_with_page_number(page_number);
            std::vector<PagelessDocumentRect> image_rects = get_image_blocks_from_stext_page(stext_page);

            glClear(GL_STENCIL_BUFFER_BIT);
            enable_stencil();
            write_to_stencil();
            draw_stencil_rects(page_number, image_rects);
            use_stencil_to_write(true);
            ColorPalette target_palette = ColorPalette::Normal;
            if (document_view->get_current_color_mode() == ColorPalette::Custom && INVERTED_PRESERVED_IMAGE_COLORS) {
                target_palette = ColorPalette::Dark;
            }

            render_page(page_number, overview, target_palette, false);
            disable_stencil();
        }

        if (!dv()->is_presentation_mode() && (!overview.has_value()) && (!dv()->is_two_page_mode())){
            render_page_separator(page_number, page_vertices);
        }
#endif // SIOYEK_OPENGL_BACKEND
    }

}

void PdfViewOpenGLWidget::render_page_separator(int page_number, float* page_vertices) {
    // render page separator
    glUseProgram(shared_gl_objects.separator_program);

    PagelessDocumentRect separator_rect({
        0,
        doc()->get_page_height(page_number) - PAGE_SEPARATOR_WIDTH / 2,
        doc()->get_page_width(page_number),
        doc()->get_page_height(page_number) + PAGE_SEPARATOR_WIDTH / 2
        });


    if (PAGE_SEPARATOR_WIDTH > 0) {

        NormalizedWindowRect separator_window_rect = DocumentRect(separator_rect, page_number).to_window_normalized(dv());
        rect_to_quad(separator_window_rect, page_vertices);

        glUniform3fv(shared_gl_objects.separator_background_color_uniform_location, 1, PAGE_SEPARATOR_COLOR);

        glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

}


void SioyekRendererBackend::my_render() {

    begin_native_painting();
    prepare_initial_render_pipeline();

    if (!valid_document()) {

        clear_background_buffers(0, 0, 0, GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        if (is_helper) {
            //painter->endNativePainting();
            draw_empty_helper_message("No portals yet");
        }
        else {
            std::wstring last_opened_file_path = L"";

            if (document_view) {
                last_opened_file_path = document_view->last_opened_file_path;
            }

            if (last_opened_file_path.size() == 0) {
                std::vector<std::wstring> prev_files = get_last_opened_file_name();
                if (prev_files.size() > 0) {
                    last_opened_file_path = prev_files[0];
                }

            }

            if (document_view && document_view->was_set_to_null) {
                draw_empty_helper_message("No document");
            }
            else {
                if (last_opened_file_path.size() > 0) {
                    draw_empty_helper_message("Document " + QString::fromStdWString(last_opened_file_path) + " does not exist");
                }
                else {
                    draw_empty_helper_message("No document");
                }
            }

        }
        return;
    }

    std::vector<int> visible_pages;
    dv()->get_visible_pages(dv()->get_view_height(), visible_pages);

    clear_background_color();


    if (dv()->is_presentation_mode()) {
        int presentation_page_number = dv()->get_presentation_page_number().value();
        if (PRERENDER_NEXT_PAGE) {
            // request the next page so it is scheduled for rendering in the background thread

            for (int i = 0; i < NUM_PRERENDERED_NEXT_SLIDES; i++) {

                if ((presentation_page_number + i + 1) < doc()->num_pages()) {
                    pdf_renderer->find_rendered_page(dv()->get_document()->get_path(),
                        presentation_page_number + i + 1,
                        document_view->get_current_color_mode(),
                        dv()->get_document()->should_render_pdf_annotations(),
                        -1,
                        1,
                        1,
                        dv()->get_zoom_level(),
                        get_device_pixel_ratio(),
                        nullptr,
                        nullptr);
                }
            }
            for (int i = 0; i < NUM_PRERENDERED_PREV_SLIDES; i++) {

                if ((presentation_page_number - i - 1) >= 0) {
                    pdf_renderer->find_rendered_page(dv()->get_document()->get_path(),
                        presentation_page_number - i - 1,
                        document_view->get_current_color_mode(),
                        dv()->get_document()->should_render_pdf_annotations(),
                        -1,
                        1,
                        1,
                        dv()->get_zoom_level(),
                        get_device_pixel_ratio(),
                        nullptr,
                        nullptr);
                }
            }
        }
        render_page(presentation_page_number);
    }
    else {

        is_helper_waiting_for_render = false;

        for (int page : visible_pages) {
            render_page(page);

            if (document_view->should_highlight_links) {
                prepare_link_highlight_state();
                const std::vector<PdfLink>& links = dv()->get_document()->get_page_merged_pdf_links(page);
                for (auto link : links) {
                    for (auto link_rect : link.rects) {
                        render_highlight_document({ link_rect, page });
                    }
                }
            }
        }
        // prerender pages
        if (visible_pages.size() > 0) {
            int num_pages = dv()->get_document()->num_pages();
            int max_page = visible_pages[visible_pages.size() - 1];
            for (int i = 1; i < (PRERENDERED_PAGE_COUNT + 1); i++) {
                if (max_page + i < num_pages) {
                    float page_width = dv()->get_document()->get_page_width(max_page + i);
                    float page_height = dv()->get_document()->get_page_width(max_page + i);
                    PagelessDocumentRect page_rect({ 0, 0, page_width, page_height });
                    int nh, nv;
                    num_slices_for_page_rect(page_rect, &nh, &nv);

                    for (int k = 0; k < nh * nv; k++) {
                        pdf_renderer->find_rendered_page(doc()->get_path(),
                            max_page + i,
                            document_view->get_current_color_mode(),
                            doc()->should_render_pdf_annotations(),
                            k,
                            nh,
                            nv,
                            dv()->get_zoom_level(),
                            get_device_pixel_ratio(),
                            nullptr,
                            nullptr);
                    }
                }
            }
        }
    }

#ifdef SIOYEK_OPENGL_BACKEND
    if (document_view->fastread_mode) {

        auto rects = dv()->get_document()->get_highlighted_character_masks(dv()->get_center_page_number());

        if (rects.size() > 0) {
            enable_stencil();
            write_to_stencil();
            draw_stencil_rects(dv()->get_center_page_number(), rects);
            use_stencil_to_write(false);
            render_transparent_background();
            disable_stencil();

        }
    }
#endif

    render_search_result_highlights(visible_pages);

    prepare_highlight_pipeline();

    if (document_view->should_show_synxtex_highlights()) {

        std::array<float, 3> synctex_highlight_color = cc3(DEFAULT_SYNCTEX_HIGHLIGHT_COLOR);
        set_highlight_color(&synctex_highlight_color[0], 0.3f);
        for (auto synctex_hl_rect : document_view->synctex_highlights) {
            render_highlight_document(synctex_hl_rect, HRF_FILL);
        }
    }

    if (dv()->should_show_text_selection_marker) {
        std::optional<AbsoluteRect> control_character_rect = dv()->get_control_rect();
        if (control_character_rect) {
            float rectangle_color[] = { 0.0f, 1.0f, 1.0f };
            set_highlight_color(rectangle_color, 0.3f);
            render_highlight_absolute(control_character_rect.value(), HRF_FILL | HRF_BORDER);
        }
    }

    if (document_view->character_highlight_rect) {
        float rectangle_color[] = { 0.0f, 1.0f, 1.0f };
        set_highlight_color(rectangle_color, 0.3f);
        render_highlight_absolute(document_view->character_highlight_rect.value(), HRF_FILL | HRF_BORDER);

        if (document_view->wrong_character_rect) {
            float wrong_color[] = { 1.0f, 0.0f, 0.0f };
            set_highlight_color(wrong_color, 0.3f);
            render_highlight_absolute(document_view->wrong_character_rect.value(), HRF_FILL | HRF_BORDER);
        }
    }
    
    render_selected_rectangle();


    render_debug_highlights();

    draw_pending_freehand_drawings(visible_pages);

    render_portals();

    render_tags();



    if (document_view->should_show_rect_hints) {
        std::vector<std::pair<QRect, QString>> hints = get_hint_rect_and_texts();
        int flags = Qt::TextWordWrap | Qt::AlignCenter;

        for (auto [hint_rect, hint_text] : hints) {
            painter.fillRect(hint_rect, QColor(0, 0, 0, 200));
        }

        painter.setPen(QColor(255, 255, 255, 255));

        for (auto [hint_rect, hint_text] : hints) {
            painter.drawText(hint_rect, flags, hint_text);
        }
    }

    // painter->beginNativePainting();
    begin_native_painting();


    bind_vertex_array();
    bind_default();
    prepare_highlight_pipeline();
    render_highlight_annotations();
    render_text_highlights();
    render_ruler();
    render_bookmark_annotations();

    bind_default();
    { // require bind_default
        if (VISUALIZE_RULER_THRESHOLDS) {
            prepare_highlight_pipeline();
            render_ruler_thresholds();
        }

        if (document_view->overview_page) {
            render_overview(document_view->overview_page.value());
        }
    }

    end_native_painting();
}

void PdfViewOpenGLWidget::bind_vertex_array() {
    glBindVertexArray(vertex_array_object);
}

void SioyekRendererBackend::render_ruler_thresholds(){
    NormalizedWindowRect top_rect;
    top_rect.x0 = -1;
    top_rect.x1 = 1;
    top_rect.y1 = 1;
    top_rect.y0 = VISUAL_MARK_NEXT_PAGE_FRACTION;
    render_highlight_window(top_rect, HRF_FILL);

    NormalizedWindowRect bottom_rect;
    bottom_rect.x0 = -1;
    bottom_rect.x1 = 1;
    bottom_rect.y1 = -1 + VISUAL_MARK_NEXT_PAGE_THRESHOLD;
    bottom_rect.y0 = -1;
    render_highlight_window(bottom_rect, HRF_FILL);
}

PdfViewOpenGLWidget::~PdfViewOpenGLWidget() {
}

bool SioyekRendererBackend::handle_mouse_move_event(QMouseEvent* mouse_event) {

    if (is_helper && (dv() != nullptr)) {

        int x = mouse_event->pos().x();
        int y = mouse_event->pos().y();

        int x_diff = x - last_mouse_down_window_x;
        int y_diff = y - last_mouse_down_window_y;

        float x_diff_doc = x_diff / dv()->get_zoom_level();
        float y_diff_doc = y_diff / dv()->get_zoom_level();

        dv()->set_offsets(last_mouse_down_document_offset_x + x_diff_doc, last_mouse_down_document_offset_y - y_diff_doc);
        return true;
    }
    return false;
}

bool SioyekRendererBackend::handle_mouse_press_event(QMouseEvent* mevent) {
    if (is_helper && (dv() != nullptr)) {
        int window_x = mevent->pos().x();
        int window_y = mevent->pos().y();

        if (mevent->button() == Qt::MouseButton::LeftButton) {
            last_mouse_down_window_x = window_x;
            last_mouse_down_window_y = window_y;

            last_mouse_down_document_offset_x = dv()->get_offset_x();
            last_mouse_down_document_offset_y = dv()->get_offset_y();
        }
    }
    return false;
}

bool SioyekRendererBackend::handle_mouse_release_event(QMouseEvent* mouse_event) {
    if (is_helper && (dv() != nullptr)) {

        int x = mouse_event->pos().x();
        int y = mouse_event->pos().y();

        int x_diff = x - last_mouse_down_window_x;
        int y_diff = y - last_mouse_down_window_y;

        float x_diff_doc = x_diff / dv()->get_zoom_level();
        float y_diff_doc = y_diff / dv()->get_zoom_level();

        dv()->set_offsets(last_mouse_down_document_offset_x + x_diff_doc, last_mouse_down_document_offset_y - y_diff_doc);

        OpenedBookState new_book_state = dv()->get_state().book_state;
        if (this->on_link_edit) {
            (this->on_link_edit.value())(new_book_state);
        }
        return true;

    }
    return false;
}

bool SioyekRendererBackend::handle_wheel_event(QWheelEvent* wevent) {

    if (is_helper && (dv() != nullptr)) {

        bool is_control_pressed = QApplication::queryKeyboardModifiers().testFlag(Qt::ControlModifier)
            || QApplication::queryKeyboardModifiers().testFlag(Qt::MetaModifier);

        if (is_control_pressed) {
            if (wevent->angleDelta().y() > 0) {
                float pev_zoom_level = dv()->get_zoom_level();
                float new_zoom_level = pev_zoom_level * ZOOM_INC_FACTOR;
                dv()->set_zoom_level(new_zoom_level, true);
            }

            if (wevent->angleDelta().y() < 0) {
                float pev_zoom_level = dv()->get_zoom_level();
                float new_zoom_level = pev_zoom_level / ZOOM_INC_FACTOR;
                dv()->set_zoom_level(new_zoom_level, true);
            }
        }
        else {
            float prev_doc_x = dv()->get_offset_x();
            float prev_doc_y = dv()->get_offset_y();
            float prev_zoom_level = dv()->get_zoom_level();

            float delta_y = wevent->angleDelta().y() * VERTICAL_MOVE_AMOUNT / prev_zoom_level;
            float delta_x = wevent->angleDelta().x() * VERTICAL_MOVE_AMOUNT / prev_zoom_level;

            dv()->set_offsets(prev_doc_x + delta_x, prev_doc_y - delta_y);
        }

        OpenedBookState new_book_state = dv()->get_state().book_state;
        if (this->on_link_edit) {
            (this->on_link_edit.value())(new_book_state);
        }
        return true;
    }
    return false;
}

void PdfViewOpenGLWidget::mouseMoveEvent(QMouseEvent* mouse_event) {

    if (handle_mouse_move_event(mouse_event)) update();
}

void PdfViewOpenGLWidget::mousePressEvent(QMouseEvent* mevent) {

    if (handle_mouse_press_event(mevent)) update();
}

void PdfViewOpenGLWidget::mouseReleaseEvent(QMouseEvent* mouse_event) {

    if (handle_mouse_release_event(mouse_event)) update();
}

void PdfViewOpenGLWidget::wheelEvent(QWheelEvent* wevent) {
    if (handle_wheel_event(wevent)) update();

}

void SioyekRendererBackend::register_on_link_edit_listener(std::function<void(const OpenedBookState&)> listener) {
    this->on_link_edit = listener;
}

void SioyekRendererBackend::draw_empty_helper_message(QString message) {
    // should be called with native painting disabled

    QFontMetrics fm(QApplication::font());
#ifdef SIOYEK_QT6
    int message_width = fm.boundingRect(message).width();
#else
    int message_width = fm.width(message);
#endif
    int message_height = fm.height();

    int view_width = dv()->get_view_width();
    int view_height = dv()->get_view_height();

    painter.drawText(view_width / 2 - message_width / 2, view_height / 2 - message_height / 2, message);
}


void PdfViewOpenGLWidget::bind_program(ColorPalette forced_palette) {
#ifdef SIOYEK_OPENGL_BACKEND
    ColorPalette mode = forced_palette == ColorPalette::None ? document_view->get_current_color_mode() : forced_palette;

    if (mode == ColorPalette::Dark) {
        glUseProgram(shared_gl_objects.rendered_dark_program);
        glUniform1f(shared_gl_objects.dark_mode_contrast_uniform_location, DARK_MODE_CONTRAST);
    }
    else if (mode == ColorPalette::Custom) {
        glUseProgram(shared_gl_objects.custom_color_program);
        float transform_matrix[16];
        get_custom_color_transform_matrix(transform_matrix);
        glUniformMatrix4fv(shared_gl_objects.custom_color_transform_uniform_location, 1, GL_TRUE, transform_matrix);

    }
    else {
        glUseProgram(shared_gl_objects.rendered_program);
        //glUniform1f(shared_gl_objects.gamma_uniform_location, GAMMA);
    }
#endif
}


void PdfViewOpenGLWidget::enable_stencil() {
#ifdef SIOYEK_OPENGL_BACKEND
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);
#endif
}

void PdfViewOpenGLWidget::write_to_stencil() {
#ifdef SIOYEK_OPENGL_BACKEND
    glStencilFunc(GL_NEVER, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
#endif
}

void PdfViewOpenGLWidget::use_stencil_to_write(bool eq) {
#ifdef SIOYEK_OPENGL_BACKEND
    if (eq) {
        glStencilFunc(GL_EQUAL, 1, 0xFF);
    }
    else {
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
    }
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
#endif
}

void PdfViewOpenGLWidget::disable_stencil() {
#ifdef SIOYEK_OPENGL_BACKEND
    glDisable(GL_STENCIL_TEST);
#else
    painter.setClipRect(rect());
#endif
}

void PdfViewOpenGLWidget::render_transparent_background() {
#ifdef SIOYEK_OPENGL_BACKEND

    float bar_data[] = {
        -1, -1,
        1, -1,
        -1, 1,
        1, 1
    };

    glDisable(GL_CULL_FACE);
    glUseProgram(shared_gl_objects.vertical_line_program);

    float background_color[4] = { 1.0f, 1.0f, 1.0f, 1 - FASTREAD_OPACITY };

    if (document_view->get_current_color_mode() == ColorPalette::Normal) {
    }
    else if (document_view->get_current_color_mode() == ColorPalette::Dark) {
        background_color[0] = background_color[1] = background_color[2] = 0;
    }
    else {
        background_color[0] = CUSTOM_BACKGROUND_COLOR[0];
        background_color[1] = CUSTOM_BACKGROUND_COLOR[1];
        background_color[2] = CUSTOM_BACKGROUND_COLOR[2];
    }

    glUniform4fv(shared_gl_objects.line_color_uniform_location,
        1,
        background_color);

    float time = 0;
    glUniform1f(shared_gl_objects.line_time_uniform_location, time);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bar_data), bar_data, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisable(GL_BLEND);
#endif
}

void PdfViewOpenGLWidget::draw_stencil_rects(const std::vector<NormalizedWindowRect>& rects) {
#ifdef SIOYEK_OPENGL_BACKEND
    std::vector<float> window_rects;

    for (auto rect : rects) {

        float triangle1[6] = {
            rect.x0, rect.y0,
            rect.x0, rect.y1,
            rect.x1, rect.y0
        };
        float triangle2[6] = {
            rect.x1, rect.y0,
            rect.x0, rect.y1,
            rect.x1, rect.y1
        };

        for (int i = 0; i < 6; i++) window_rects.push_back(triangle1[i]);
        for (int i = 0; i < 6; i++) window_rects.push_back(triangle2[i]);
    }

    glUseProgram(shared_gl_objects.stencil_program);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, window_rects.size() * sizeof(float), window_rects.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, rects.size() * 6);
    glDisableVertexAttribArray(0);
#endif

}

void PdfViewOpenGLWidget::draw_stencil_rects(int page, const std::vector<PagelessDocumentRect>& rects) {
    std::vector<NormalizedWindowRect> normalized_rects;
    for (auto rect : rects) {
        normalized_rects.push_back(DocumentRect(rect, page).to_window_normalized(dv()));
    }
    draw_stencil_rects(normalized_rects);
}

void SioyekRendererBackend::setup_text_painter() {

    int bgcolor[4];
    int textcolor[4];

    convert_color4(KEYBOARD_SELECT_BACKGROUND_COLOR, bgcolor);
    convert_color4(KEYBOARD_SELECT_TEXT_COLOR, textcolor);
    QBrush background_brush = QBrush(QColor(bgcolor[0], bgcolor[1], bgcolor[2], bgcolor[3]));
    QFont font(QString::fromStdWString(TAG_FONT_FACE));
    font.setStyleHint(QFont::Monospace);
    font.setPixelSize(KEYBOARD_SELECT_FONT_SIZE);
    painter.setBackgroundMode(Qt::BGMode::OpaqueMode);
    painter.setBackground(background_brush);
    painter.setPen(QColor(textcolor[0], textcolor[1], textcolor[2], textcolor[3]));
    painter.setFont(font);
}

void SioyekRendererBackend::get_overview_window_vertices(float out_vertices[2 * 4], std::optional<OverviewState> maybe_overview) {


    if (maybe_overview && maybe_overview->source_rect) {
        NormalizedWindowRect normalized_rect = maybe_overview->source_rect->to_window_normalized(document_view);
        out_vertices[0] = normalized_rect.x0;
        out_vertices[1] = normalized_rect.y1;
        out_vertices[2] = normalized_rect.x0;
        out_vertices[3] = normalized_rect.y0;
        out_vertices[4] = normalized_rect.x1;
        out_vertices[5] = normalized_rect.y0;
        out_vertices[6] = normalized_rect.x1;
        out_vertices[7] = normalized_rect.y1;
    }
    else {
        float overview_offset_x = document_view->overview_offset_x;
        float overview_offset_y = document_view->overview_offset_y;
        float overview_half_width = document_view->overview_half_width;
        float overview_half_height = document_view->overview_half_height;
        out_vertices[0] = overview_offset_x - overview_half_width;
        out_vertices[1] = overview_offset_y + overview_half_height;
        out_vertices[2] = overview_offset_x - overview_half_width;
        out_vertices[3] = overview_offset_y - overview_half_height;
        out_vertices[4] = overview_offset_x + overview_half_width;
        out_vertices[5] = overview_offset_y - overview_half_height;
        out_vertices[6] = overview_offset_x + overview_half_width;
        out_vertices[7] = overview_offset_y + overview_half_height;
    }


}



void SioyekRendererBackend::get_background_color(float out_background[3]) {

    if (document_view->get_current_color_mode() == ColorPalette::Normal) {
        out_background[0] = out_background[1] = out_background[2] = 1;
    }
    else if (document_view->get_current_color_mode() == ColorPalette::Dark) {
        out_background[0] = out_background[1] = out_background[2] = 0;
    }
    else {
        out_background[0] = CUSTOM_BACKGROUND_COLOR[0];
        out_background[1] = CUSTOM_BACKGROUND_COLOR[1];
        out_background[2] = CUSTOM_BACKGROUND_COLOR[2];
    }
}

//MOVE should probably be moved to RenderState
void SioyekRendererBackend::clear_all_selections() {
    document_view->cancel_search();
    dv()->selected_character_rects.clear();
}

void PdfViewOpenGLWidget::bind_points(const std::vector<float>& points) {
#ifdef SIOYEK_OPENGL_BACKEND
    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.line_points_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points[0]) * points.size(), &points[0], GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
#else
    qDebug() << "bind_points not implemented";
#endif
}

void PdfViewOpenGLWidget::bind_default() {
#ifdef SIOYEK_OPENGL_BACKEND
    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
#endif
}

void PdfViewOpenGLWidget::add_coordinates_for_window_point(DocumentView* dv, float window_x, float window_y, float r, int point_polygon_vertices, std::vector<float>& out_coordinates){

    float thickness_x = dv->get_zoom_level() / width();
    float thickness_y = dv->get_zoom_level() / height();

    out_coordinates.push_back(window_x);
    out_coordinates.push_back(window_y);

    for (int i = 0; i <= point_polygon_vertices; i++) {
        out_coordinates.push_back(window_x + r * thickness_x * std::cos(2 * M_PI * i / point_polygon_vertices) / 2);
        out_coordinates.push_back(window_y + r * thickness_y * std::sin(2 * M_PI * i / point_polygon_vertices) / 2);
    }
}

template<typename T>
T lerp(T p1, T p2, float alpha) {
    float x = p1.x + alpha * (p2.x - p1.x);
    float y = p1.y + alpha * (p2.y - p1.y);
    return T{ x, y };
}

template<typename T>
T bezier_lerp(T p1, T p2, T p3, T p4, float alpha) {
    T q1 = lerp(p1, p2, alpha);
    T q2 = lerp(p2, p3, alpha);
    T q3 = lerp(p3, p4, alpha);

    T r1 = lerp(q1, q2, alpha);
    T r2 = lerp(q2, q3, alpha);
    return lerp(r1, r2, alpha);
}


FreehandDrawing smoothen_drawing(FreehandDrawing original) {
    if (original.points.size() < 3) {
        return original;
    }

    std::vector<Vec<float, 2>> velocities_at_points;
    velocities_at_points.reserve(original.points.size());

    velocities_at_points.push_back(original.points[1].pos - original.points[0].pos);

    for (int i = 1; i < original.points.size()-1; i++) {
        float alpha = 0.5f;
        velocities_at_points.push_back((original.points[i+1].pos - original.points[i - 1].pos) * alpha);
    }
    velocities_at_points.push_back(original.points[original.points.size()-1].pos - original.points[original.points.size()-2].pos);

    int N_POINTS = 4;
    FreehandDrawing smoothed_drawing;
    smoothed_drawing.creattion_time = original.creattion_time;
    smoothed_drawing.type = original.type;
    smoothed_drawing.alpha = original.alpha;
    smoothed_drawing.points.reserve((original.points.size()-1) * N_POINTS);

    for (int i = 0; i < original.points.size()-1; i++){
        generate_bezier_with_endpoints_and_velocity(
            original.points[i].pos,
            original.points[i + 1].pos,
            velocities_at_points[i],
            velocities_at_points[i + 1],
            N_POINTS,
            original.points[i].thickness,
            smoothed_drawing.points
        );
        /* for (auto p : bezier_curve) { */
        /*     smoothed_drawing.points.push_back(FreehandDrawingPoint{ AbsoluteDocumentPos{ p.x(), p.y() }, original.points[i].thickness}); */
        /* } */
    }
    return smoothed_drawing;

}

void PdfViewOpenGLWidget::compile_drawings(DocumentView* dv, const std::vector<FreehandDrawing>& drawings) {
#ifdef SIOYEK_OPENGL_BACKEND
    return compile_drawings_opengl_backend(dv, drawings);
#else


    if (drawings.size() == 0) {
        dv->scratchpad->on_compile();
    }

    QPixmap pixmap(rect().size());
    pixmap.fill(Qt::transparent);
    QPainter pixmap_painter(&pixmap);
    pixmap_painter.setRenderHints(QPainter::RenderHint::Antialiasing, true);
    render_drawings(&pixmap_painter, dv, drawings);

    CachedScratchpadPixmapData cached_pixmap_data;
    cached_pixmap_data.offset_x = dv->get_offset_x();
    cached_pixmap_data.offset_y = dv->get_offset_y();
    cached_pixmap_data.zoom_level = dv->get_zoom_level();
    cached_pixmap_data.pixmap = pixmap;
    dv->scratchpad->cached_pixmap = std::make_unique<CachedScratchpadPixmapData>(cached_pixmap_data);
    dv->scratchpad->on_compile();


#endif
}



//void PdfViewOpenGLWidget::compile_drawings_into_vertex_and_index_buffers(std::vector<float>& line_coordinates) {


void PdfViewOpenGLWidget::render_compiled_drawings() {

#ifdef SIOYEK_OPENGL_BACKEND
    float mode_highlight_colors[26 * 3];
    for (int i = 0; i < 26; i++) {
        auto res = cc3(&HIGHLIGHT_COLORS[3 * i]);
        mode_highlight_colors[i * 3 + 0] = res[0];
        mode_highlight_colors[i * 3 + 1] = res[1];
        mode_highlight_colors[i * 3 + 2] = res[2];
    }

    auto scratchpad = scratch();
    if (scratchpad->cached_compiled_drawing_data.has_value()) {
        float scale[] = {
            2 * scratchpad->get_zoom_level() / scratchpad->get_view_width() ,
            2 * scratchpad->get_zoom_level() / scratchpad->get_view_height()
        };
        float offset[] = { scratchpad->get_offset_x() * scale[0], scratchpad->get_offset_y() * scale[1]};
        float color[] = { 1.0f, 0.0f, 0.0f, 1.0f };
        //float color[] = {1.0f, 0.0f, 0.0f, 1.0f};

        glBindVertexArray(scratchpad->cached_compiled_drawing_data->vao);

        glBindBuffer(GL_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->vertex_buffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->lines_type_index_buffer);
        glVertexAttribIPointer(1, 1, GL_INT, 0, 0);
        glEnableVertexAttribArray(1);


        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->index_buffer);

        glUseProgram(shared_gl_objects.compiled_drawing_program);
        glUniform2fv(shared_gl_objects.compiled_drawing_offset_uniform_location, 1, offset);
        glUniform2fv(shared_gl_objects.compiled_drawing_scale_uniform_location, 1, scale);
        glUniform3fv(shared_gl_objects.compiled_drawing_colors_uniform_location, 26, mode_highlight_colors);
        //glUniform4fv(shared_gl_objects.compiled_drawing_color_uniform_location, 1, color);
        glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        glDrawElements(GL_TRIANGLE_STRIP, scratchpad->cached_compiled_drawing_data->n_elements, GL_UNSIGNED_INT, 0);

        //// bind dots buffers
        glBindBuffer(GL_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->dots_vertex_buffer);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->dots_uv_buffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        glBindBuffer(GL_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->dots_type_index_buffer);
        glVertexAttribIPointer(2, 1, GL_INT, 0, 0);
        glEnableVertexAttribArray(2);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, scratchpad->cached_compiled_drawing_data->dots_index_buffer);

        glUseProgram(shared_gl_objects.compiled_dots_program);
        glUniform2fv(shared_gl_objects.compiled_dot_offset_uniform_location, 1, offset);
        glUniform2fv(shared_gl_objects.compiled_dot_scale_uniform_location, 1, scale);
        glUniform3fv(shared_gl_objects.compiled_dot_color_uniform_location, 26, mode_highlight_colors);

        glEnable(GL_BLEND);
        glDrawElements(GL_TRIANGLE_STRIP, scratchpad->cached_compiled_drawing_data->n_dot_elements, GL_UNSIGNED_INT, 0);
        glDisable(GL_BLEND);

    }
#else
    if (dv()->scratchpad->cached_pixmap) {
        painter.drawPixmap(rect(), dv()->scratchpad->cached_pixmap->pixmap);
    }
#endif
}

void PdfViewOpenGLWidget::render_drawings(QPainter* p, DocumentView* dv, const std::vector<FreehandDrawing>& drawings, bool highlighted) {

#ifdef SIOYEK_OPENGL_BACKEND
    if (drawings.size() == 0) return;

    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    float time_diff = last_scratchpad_update_datetime.msecsTo(QDateTime::currentDateTime());
    last_scratchpad_update_datetime = QDateTime::currentDateTime();

    const int N_POINT_VERTICES = 10;
    float thickness_x = dv->get_zoom_level() / width();
    float thickness_y = dv->get_zoom_level() / height();
    //float thickness_y = thickness_x * width() / height();

    for (int i = drawings.size() - 1; i >= 0; i--) {
        auto drawing = drawings[i];

        if (DEBUG_SMOOTH_FREEHAND_DRAWINGS) {
            drawing = smoothen_drawing(drawing);
        }
        //if (drawings.size() % 2 == 0){
        //    drawing = smoothen_drawing(drawing);
        //}

        if (drawing.points.size() <= 0) {
            continue;
        }
        if (!document_view->visible_drawing_mask[drawing.type - 'a']) {
            continue;
        }

        float current_drawing_color_[4] = { HIGHLIGHT_COLORS[(drawing.type - 'a') * 3],
            HIGHLIGHT_COLORS[(drawing.type - 'a') * 3 + 1],
             HIGHLIGHT_COLORS[(drawing.type - 'a') * 3 + 2],
            drawing.alpha
        };
        float current_drawing_color[4] = {0};
        get_color_for_current_mode(current_drawing_color_, current_drawing_color);
        current_drawing_color[3] = current_drawing_color_[3];

        if (highlighted) {
            current_drawing_color[0] = 1.0f;
            current_drawing_color[1] = 1.0f;
            current_drawing_color[2] = 0.0f;

        }

        if (drawing.points.size() == 1) {
            std::vector<float> coordinates;
            //float window_x, window_y;
            float thickness = drawing.points[0].thickness;

            NormalizedWindowPos window_pos = dv->absolute_to_window_pos({ drawing.points[0].pos.x, drawing.points[0].pos.y });
            add_coordinates_for_window_point(dv, window_pos.x, window_pos.y, thickness * 2, N_POINT_VERTICES, coordinates);

            bind_points(coordinates);
            glUniform4fv(shared_gl_objects.freehand_line_color_uniform_location, 1, current_drawing_color);
            glDrawArrays(GL_TRIANGLE_FAN, 0, coordinates.size() / 2);
            continue;
        }

        if (DEBUG_DISPLAY_FREEHAND_POINTS) {
            for (auto point : drawing.points) {
                std::vector<float> coordinates;
                //float window_x, window_y;
                float thickness = drawing.points[0].thickness;

                NormalizedWindowPos window_pos = dv->absolute_to_window_pos({ point.pos.x, point.pos.y });
                add_coordinates_for_window_point(dv, window_pos.x, window_pos.y, thickness * 2, N_POINT_VERTICES, coordinates);

                bind_points(coordinates);
                glUniform4fv(shared_gl_objects.freehand_line_color_uniform_location, 1, current_drawing_color);
                glDrawArrays(GL_TRIANGLE_FAN, 0, coordinates.size() / 2);
            }
            continue;
        }

        glUniform4fv(shared_gl_objects.freehand_line_color_uniform_location, 1, current_drawing_color);

        std::vector<NormalizedWindowPos> window_positions;
        window_positions.reserve(drawing.points.size());

        for (auto p : drawing.points) {
            NormalizedWindowPos window_pos = dv->absolute_to_window_pos({ p.pos.x, p.pos.y });
            window_positions.push_back(NormalizedWindowPos{ window_pos.x, window_pos.y });
        }

        std::vector<float> coordinates;
        /* coordinates.reserve(drawing.points.size() * 8 - 4); */
        std::vector<float> begin_point_coordinates;
        std::vector<float> end_point_coordinates;
        /* std::vector<std::vector<float>> all_point_coordinates; */

        float first_line_x = (window_positions[1].x - window_positions[0].x) * width();
        float first_line_y = (window_positions[1].y - window_positions[0].y) * height();
        float first_line_size = sqrt(first_line_x * first_line_x + first_line_y * first_line_y);
        first_line_x = first_line_x / first_line_size;
        first_line_y = first_line_y / first_line_size;
        float highlight_factor = highlighted ? 3.0f : 1.0f;

        float first_ortho_x = -first_line_y * thickness_x * drawing.points[0].thickness * highlight_factor;
        float first_ortho_y = first_line_x * thickness_y * drawing.points[0].thickness * highlight_factor;


        coordinates.push_back(window_positions[0].x - first_ortho_x);
        coordinates.push_back(window_positions[0].y - first_ortho_y);
        coordinates.push_back(window_positions[0].x + first_ortho_x);
        coordinates.push_back(window_positions[0].y + first_ortho_y);
        add_coordinates_for_window_point(dv, window_positions[0].x, window_positions[0].y, drawing.points[0].thickness * 2, N_POINT_VERTICES, begin_point_coordinates);
        float prev_line_x = first_line_x;
        float prev_line_y = first_line_y;

        for (int line_index = 0; line_index < drawing.points.size() - 1; line_index++) {
            float line_direction_x = (window_positions[line_index + 1].x - window_positions[line_index].x) * width();
            float line_direction_y = (window_positions[line_index + 1].y - window_positions[line_index].y) * height();
            float line_size = sqrt(line_direction_x * line_direction_x + line_direction_y * line_direction_y);
            line_direction_x = line_direction_x / line_size;
            line_direction_y = line_direction_y / line_size;

            float ortho_x1 = -line_direction_y * thickness_x * drawing.points[line_index].thickness * highlight_factor;
            float ortho_y1 = line_direction_x * thickness_y * drawing.points[line_index].thickness * highlight_factor;

            float ortho_x2 = -line_direction_y * thickness_x * drawing.points[line_index + 1].thickness * highlight_factor;
            float ortho_y2 = line_direction_x * thickness_y * drawing.points[line_index + 1].thickness * highlight_factor;

            float dot_prod_with_prev_direction = (prev_line_x * line_direction_x + prev_line_y * line_direction_y);

            coordinates.push_back(window_positions[line_index].x - ortho_x1);
            coordinates.push_back(window_positions[line_index].y - ortho_y1);
            coordinates.push_back(window_positions[line_index].x + ortho_x1);
            coordinates.push_back(window_positions[line_index].y + ortho_y1);

            coordinates.push_back(window_positions[line_index + 1].x - ortho_x1);
            coordinates.push_back(window_positions[line_index + 1].y - ortho_y1);
            coordinates.push_back(window_positions[line_index + 1].x + ortho_x2);
            coordinates.push_back(window_positions[line_index + 1].y + ortho_y2);

            /* std::vector<float> point_coordinates; */
            /* add_coordinates_for_window_point(dv, window_positions[line_index+1].x, window_positions[line_index+1].y, drawing.points[line_index+1].thickness * 2, N_POINT_VERTICES, point_coordinates); */
            /* all_point_coordinates.push_back(point_coordinates); */

        }

        add_coordinates_for_window_point(dv,
            window_positions[drawing.points.size() - 1].x,
            window_positions[drawing.points.size() - 1].y,
            drawing.points[drawing.points.size() - 1].thickness * 2 * highlight_factor,
            N_POINT_VERTICES,
            end_point_coordinates
        );

        std::vector<int> indices;


        //draw the lines
        bind_points(coordinates);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, coordinates.size() / 2);

        // draw a point at the start and end of drawing
        bind_points(begin_point_coordinates);
        glDrawArrays(GL_TRIANGLE_FAN, 0, begin_point_coordinates.size() / 2);
        bind_points(end_point_coordinates);
        glDrawArrays(GL_TRIANGLE_FAN, 1, end_point_coordinates.size() / 2);

        //for (auto coords : all_point_coordinates) {
        //    bind_points(coords);
        //    glDrawArrays(GL_TRIANGLE_FAN, 0, coords.size() / 2);
        //}
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
#else



    float last_thickness = -1;
    char last_drawing_type = 0;

    for (auto& drawing: drawings){
        if (drawing.points.size() > 0 && ((drawing.points[0].thickness != last_thickness) || (drawing.type != last_drawing_type))){
            float current_thickness = drawing.points[0].thickness * dv->get_zoom_level();
            float current_drawing_color_[4] = { HIGHLIGHT_COLORS[(drawing.type - 'a') * 3],
                HIGHLIGHT_COLORS[(drawing.type - 'a') * 3 + 1],
                 HIGHLIGHT_COLORS[(drawing.type - 'a') * 3 + 2],
                drawing.alpha
            };
            QColor color(convert_float4_to_qcolor(current_drawing_color_));
            last_thickness = current_thickness;
            last_drawing_type = drawing.type;

            if (highlighted){
                color = Qt::yellow;
                current_thickness *= 3;
            }
            p->setPen(QPen(color, current_thickness, Qt::PenStyle::SolidLine, Qt::PenCapStyle::RoundCap, Qt::PenJoinStyle::RoundJoin));
        }
        if (drawing.points.size() == 1){
            WindowPos begin = drawing.points[0].pos.to_window(dv);
            p->drawPoint(QPoint(begin.x, begin.y));
        }
        else{

            for (int i = 1; i < drawing.points.size(); i++){
                WindowPos begin = drawing.points[i-1].pos.to_window(dv);
                WindowPos end = drawing.points[i].pos.to_window(dv);
                p->drawLine(QLine(begin.x, begin.y, end.x, end.y));
            }
        }
    }

    
#endif
}


bool SioyekRendererBackend::is_normalized_y_in_window(float y) {
    return (y >= -1) && (y <= 1);
}

bool SioyekRendererBackend::is_normalized_y_range_in_window(float y0, float y1) {
    if (y0 <= -1.0 && y1 >= 1.0f) return true;
    return is_normalized_y_in_window(y0) || is_normalized_y_in_window(y1);
}

void SioyekRendererBackend::render_portal_rect(AbsoluteRect portal_rect, bool is_pending, std::optional<float> completion_ratio) {
    NormalizedWindowRect window_rect = portal_rect.to_window_normalized(dv());

    if (is_normalized_y_range_in_window(window_rect.y0, window_rect.y1)) {
        fz_irect portal_window_rect = dv()->normalized_to_window_rect(window_rect);
        QRect window_qrect = QRect(portal_window_rect.x0, portal_window_rect.y0, fz_irect_width(portal_window_rect), fz_irect_height(portal_window_rect));
        QColor fill_color = QColor(0, 178, 255);
        QColor complete_color = QColor(255, 132, 0);
        QColor not_started_color = QColor(255, 0, 0);

        if (is_pending) {
            //draw_icon(hourglass_icon, window_qrect);
            float adjust_factor = document_view->get_zoom_level();
            QRect adjust_rect = window_qrect.adjusted(adjust_factor * 1.5, adjust_factor, -adjust_factor * 1.5, -adjust_factor);

            if (completion_ratio) {

                if (completion_ratio.value() < 0) {
                    painter.fillRect(adjust_rect, not_started_color);
                }
                else {
                    painter.fillRect(adjust_rect, fill_color);
                }

                if (completion_ratio.value() >= 0) {
                    float completed_height = adjust_rect.height() * completion_ratio.value();
                    painter.fillRect(adjust_rect.x(), adjust_rect.y() + adjust_rect.height() - completed_height, adjust_rect.width(), completed_height, complete_color);
                }
            }
            draw_icon(portal_icon, window_qrect);
        }
        else{
            render_ui_icon_for_current_color_mode(portal_icon, portal_icon_white, window_qrect);
        }
    }
}

struct UIRectDescriptor {
    UIRect* ui_rect;
    std::wstring* tap_command;
    std::wstring* hold_command;
    std::string name;
};

Qt::ScreenOrientation PdfViewOpenGLWidget::get_orientation() {
    return screen()->orientation();
}

int PdfViewOpenGLWidget::get_width() {
    return width();
}

int PdfViewOpenGLWidget::get_height() {
    return height();
}

std::vector<std::pair<QRect, QString>> SioyekRendererBackend::get_hint_rect_and_texts() {
    std::vector<std::pair< QRect, QString>> res;

    std::vector<UIRectDescriptor> rect_descriptors;

    if (get_orientation() == Qt::PortraitOrientation) {
        rect_descriptors = {
            UIRectDescriptor {&PORTRAIT_BACK_UI_RECT, &BACK_RECT_TAP_COMMAND, &BACK_RECT_HOLD_COMMAND, "back"},
            UIRectDescriptor {&PORTRAIT_FORWARD_UI_RECT, &FORWARD_RECT_TAP_COMMAND, &FORWARD_RECT_HOLD_COMMAND, "forward"},
            UIRectDescriptor {&PORTRAIT_VISUAL_MARK_PREV, &VISUAL_MARK_PREV_TAP_COMMAND, &VISUAL_MARK_PREV_HOLD_COMMAND, "move_ruler_prev"},
            UIRectDescriptor {&PORTRAIT_VISUAL_MARK_NEXT, &VISUAL_MARK_NEXT_TAP_COMMAND, &VISUAL_MARK_NEXT_HOLD_COMMAND, "move_ruler_next"},
            UIRectDescriptor {&PORTRAIT_MIDDLE_LEFT_UI_RECT, &MIDDLE_LEFT_RECT_TAP_COMMAND, &MIDDLE_LEFT_RECT_HOLD_COMMAND, "middle_left"},
            UIRectDescriptor {&PORTRAIT_MIDDLE_RIGHT_UI_RECT, &MIDDLE_RIGHT_RECT_TAP_COMMAND, &MIDDLE_RIGHT_RECT_HOLD_COMMAND, "middle_right"},
            UIRectDescriptor {&PORTRAIT_EDIT_PORTAL_UI_RECT, &TOP_CENTER_TAP_COMMAND, &TOP_CENTER_HOLD_COMMAND, "edit_portal"},
        };
    }
    else {
        rect_descriptors = {
            UIRectDescriptor {&LANDSCAPE_BACK_UI_RECT, &BACK_RECT_TAP_COMMAND, &BACK_RECT_HOLD_COMMAND, "back"},
            UIRectDescriptor {&LANDSCAPE_FORWARD_UI_RECT, &FORWARD_RECT_TAP_COMMAND, &FORWARD_RECT_HOLD_COMMAND, "forward"},
            UIRectDescriptor {&LANDSCAPE_VISUAL_MARK_PREV, &VISUAL_MARK_PREV_TAP_COMMAND, &VISUAL_MARK_PREV_HOLD_COMMAND, "move_ruler_prev"},
            UIRectDescriptor {&LANDSCAPE_VISUAL_MARK_NEXT, &VISUAL_MARK_NEXT_TAP_COMMAND, &VISUAL_MARK_NEXT_HOLD_COMMAND, "move_ruler_next"},
            UIRectDescriptor {&LANDSCAPE_MIDDLE_LEFT_UI_RECT, &MIDDLE_LEFT_RECT_TAP_COMMAND, &MIDDLE_LEFT_RECT_HOLD_COMMAND, "middle_left"},
            UIRectDescriptor {&LANDSCAPE_MIDDLE_RIGHT_UI_RECT, &MIDDLE_RIGHT_RECT_TAP_COMMAND, &MIDDLE_RIGHT_RECT_HOLD_COMMAND, "middle_right"},
            UIRectDescriptor {&LANDSCAPE_EDIT_PORTAL_UI_RECT, &TOP_CENTER_TAP_COMMAND, &TOP_CENTER_HOLD_COMMAND, "edit_portal"},
        };
    }

    for (auto desc : rect_descriptors) {
        bool is_visual = QString::fromStdString(desc.name).startsWith("move_ruler");

        if (is_visual || (desc.ui_rect->enabled && ((desc.hold_command->size() > 0) || (desc.tap_command->size() > 0)))) {
            QString str = "";
            if (desc.tap_command->size() > 0) {
                str += "tap: " + QString::fromStdWString(*desc.tap_command);
            }
            if (desc.hold_command->size() > 0) {
                if (str.size() > 0) str += "\n";
                str += "hold: " + QString::fromStdWString(*desc.hold_command);
            }
            if (is_visual) {
                if (str.size() > 0) str += "\n";
                str += "visual: " + QString::fromStdString(desc.name);
            }
            res.push_back(std::make_pair(desc.ui_rect->to_window(get_width(), get_height()), str));
        }
    }
    return res;
} 


void SioyekRendererBackend::get_color_for_current_mode(const float* input_color, float* output_color) {
    return get_color_for_mode(document_view->get_current_color_mode(), input_color, output_color);
}

std::array<float, 3> SioyekRendererBackend::cc3(const float* input_color) {
    std::array<float, 3> result;
    get_color_for_current_mode(input_color, &result[0]);
    return result;
}

std::array<float, 4> SioyekRendererBackend::cc4(const float* input_color) {
    std::array<float, 4> result;
    get_color_for_current_mode(input_color, &result[0]);
    result[3] = input_color[3];
    return result;
}

QColor SioyekRendererBackend::qcc3(const float* input_color) {
    return qconvert_color3(input_color, document_view->get_current_color_mode());
}

QColor SioyekRendererBackend::qcc4(const float* input_color) {
    std::array<float, 4> result;
    get_color_for_current_mode(input_color, &result[0]);
    result[3] = input_color[3];
    return convert_float4_to_qcolor(&result[0]);
}

void SioyekRendererBackend::render_ui_icon_for_current_color_mode(const QIcon& icon_black, const QIcon& icon_white, QRect window_qrect, bool is_highlighted){

    float visible_annotation_icon_color[3] = {1, 1, 1};
    float mode_color[3] = {0};
    if (!is_highlighted) {
        get_color_for_current_mode(visible_annotation_icon_color, mode_color);
    }
    else {
        float visible_annotation_highlight_color[3] = {1, 1, 0};
        get_color_for_current_mode(visible_annotation_highlight_color, mode_color);
    }
    int num_adjust_pixels = static_cast<int>(window_qrect.width() * 0.065f);

    fill_rect(window_qrect.adjusted(num_adjust_pixels, num_adjust_pixels, -num_adjust_pixels, -num_adjust_pixels), convert_float3_to_qcolor(mode_color));

    if (is_bright(mode_color)){
        draw_icon(icon_black, window_qrect);
    }
    else{
        draw_icon(icon_white, window_qrect);
    }
}

void SioyekRendererBackend::render_text_highlights(){

    std::array<float, 3> text_highlight_color;

    bool is_inverted = false;
    if ((SELECTED_TEXT_HIGHLIGHT_STYLE == SelectedTextHighlightStyle::Inverted) || (dv()->is_line_select_mode())) {
        is_inverted = true;
    }

    if (is_inverted) {
        text_highlight_color = { 1, 1, 1 };
    }
    else {
        text_highlight_color = cc3(DEFAULT_TEXT_HIGHLIGHT_COLOR);
    }

    set_highlight_color(&text_highlight_color[0], 0.3f);
    std::vector<AbsoluteRect> bounding_rects;
    merge_selected_character_rects(*dv()->get_selected_character_rects(), bounding_rects);

    for (auto rect : bounding_rects) {

        int line_pending_flags = HRF_FILL | HRF_INVERTED;
        int normal_flags = HRF_FILL | HRF_BORDER;
        if (SELECTED_TEXT_HIGHLIGHT_STYLE == SelectedTextHighlightStyle::Inverted) {
            std::swap(line_pending_flags, normal_flags);
        }
        else if (SELECTED_TEXT_HIGHLIGHT_STYLE == SelectedTextHighlightStyle::Background) {
            normal_flags = HRF_PAINTOVER | HRF_FILL;
        }

        if (dv()->is_line_select_mode()) {
            render_highlight_absolute(rect, line_pending_flags);
        }
        else {
            render_highlight_absolute(rect, normal_flags);
        }
    }
}

void SioyekRendererBackend::render_highlight_annotations(){
    std::vector<AbsoluteRect> borders_to_draw;

    if (doc()->can_use_highlights()) {
        const std::vector<Highlight>& highlights = doc()->get_highlights();
        std::vector<std::string> visible_highlight_uuids = dv()->get_visible_highlight_uuids();

        for (size_t ind = 0; ind < visible_highlight_uuids.size(); ind++) {
            std::string uuid = visible_highlight_uuids[ind];
            Highlight* highlight = doc()->get_highlight_with_uuid(uuid);

            if (!highlight) continue;

            float last_bottom = -1;
            for (size_t j = 0; j < highlight->highlight_rects.size(); j++) {
                NormalizedWindowRect window_rect = highlight->highlight_rects[j].to_window_normalized(dv());
                if (j > 0) {
                    // making sure the highlights are air tight
                    if (std::abs(window_rect.y0 - last_bottom) < 0.01f) {
                        window_rect.y0 = last_bottom;
                    }
                }
                last_bottom = window_rect.y1;

                std::array<float, 3> adjusted_highlight_color;

                if ((HIGHLIGHT_STYLE == HighlightStyle::HighlightBackground) && (BACKGROUND_HIGHLIGHT_MINIMUM_LIGHTNESS > 0)) {
                    // make sure the lightness of background highlights is at least BACKGROUND_HIGHLIGHT_MINIMUM_LIGHTNESS
                    auto raw_color_ = get_highlight_type_color(highlight->type);
                    std::array<float, 3> raw_color;
                    raw_color[0] = raw_color_[0];
                    raw_color[1] = raw_color_[1];
                    raw_color[2] = raw_color_[2];

                    QColor qcolor = QColor::fromRgbF(raw_color[0], raw_color[1], raw_color[2]);
                    int h, s, l;
                    qcolor.getHsl(&h, &s, &l);
                    qcolor.setHsl(h, s, std::max(qcolor.lightness(), BACKGROUND_HIGHLIGHT_MINIMUM_LIGHTNESS));
                    raw_color[0] = qcolor.redF();
                    raw_color[1] = qcolor.greenF();
                    raw_color[2] = qcolor.blueF();
                    get_color_for_current_mode(&raw_color[0], &adjusted_highlight_color[0]);
                }
                else {
                    get_color_for_current_mode(get_highlight_type_color(highlight->type), &adjusted_highlight_color[0]);
                }


                if (highlight->uuid == document_view->get_selected_highlight_uuid()) {
                    adjusted_highlight_color[0] *= 0.5f;
                    adjusted_highlight_color[1] *= 0.5f;
                    adjusted_highlight_color[2] *= 0.5f;
                }
                set_highlight_color(&adjusted_highlight_color[0], 0.3f);
                int flags = 0;
                if (std::isupper(highlight->type)) {
                    flags |= HRF_UNDERLINE;
                }
                else if (highlight->type == '_') {
                    flags |= HRF_STRIKE;
                }


                if (flags == 0) {
                    if (HIGHLIGHT_STYLE == HighlightStyle::HighlightBorder) {
                        flags = HRF_BORDER;
                    }
                    else {
                        flags |= HRF_FILL;
                        if (HIGHLIGHT_STYLE == HighlightStyle::HighlightBackground) {
                            flags |= HRF_PAINTOVER;
                        }
                    }


                    if (highlight->uuid == document_view->get_selected_highlight_uuid()) {
                        borders_to_draw.push_back(highlight->highlight_rects[j]);
                    }
                }
                render_highlight_window(window_rect, flags);
            }
        }
    }
    if (borders_to_draw.size() > 0) {

        float border_color[3] = { 0 };
        set_highlight_color(border_color, 1.0f);
        for (auto border_rect : borders_to_draw) {
            render_highlight_absolute(border_rect, HRF_BORDER);
        }
    }

}

std::vector<int> SioyekRendererBackend::get_overview_visible_pages(const OverviewState& overview) {

    std::vector<int> visible_pages;

    if (overview.source_rect) {

        float y_begin = overview.absolute_offset_y - overview.source_rect->height() / 2 * dv()->get_zoom_level() / overview.zoom_level;
        float y_end = overview.absolute_offset_y + overview.source_rect->height() / 2 * dv()->get_zoom_level() / overview.zoom_level;
        doc()->get_visible_pages(y_begin, y_end, visible_pages);
        return visible_pages;
    }
    else {
        int page = AbsoluteDocumentPos{ 0, overview.absolute_offset_y }.to_document(doc(overview)).page;
        visible_pages.push_back(page - 1);
        visible_pages.push_back(page);
        visible_pages.push_back(page + 1);
        return visible_pages;
    }
}

bool SioyekRendererBackend::needs_stencil_buffer() {
    return document_view->fastread_mode || document_view->selected_rectangle.has_value() || dv()->overview_page.has_value();
}

void SioyekRendererBackend::render_selected_rectangle() {

    if (document_view->selected_rectangle) {
        float rectangle_color_[] = { 0.0f, 0.0f, 0.0f };
        auto rectangle_color = cc3(rectangle_color_);
        set_highlight_color(&rectangle_color[0], 0.3f);
        render_highlight_absolute(document_view->selected_rectangle.value(), HRF_FILL | HRF_BORDER);
        NormalizedWindowRect window_rect({ -1, -1, 1, 1 });
        render_highlight_window(window_rect, true);
    }
}

bool PdfViewOpenGLWidget::can_use_cached_scratchpad_framebuffer() {
    auto scratchpad = scratch();
    float current_offset_x = scratchpad->get_offset_x();
    float current_offset_y = scratchpad->get_offset_y();
    float current_zoom_level = scratchpad->get_zoom_level();
    if ((current_offset_x == last_cache_offset_x) && (current_offset_y == last_cache_offset_y) && (current_zoom_level == last_cache_zoom_level)){
        return true;
    }
    return false;
}

void SioyekRendererBackend::clear_background_color() {
    float* color = BACKGROUND_COLOR;

    // for some reason clearing the stencil buffer tanks the performance on android 
    // so we only clear it if we absolutely need it
    GLuint flags = 0;

    if (needs_stencil_buffer()) {
        flags = GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
    }
    else {
        flags = GL_COLOR_BUFFER_BIT;
    }
    clear_background_buffers(color[0], color[1], color[2], flags);
}

DocumentView* SioyekRendererBackend::dv(){
    return document_view;
}

ScratchPad* SioyekRendererBackend::scratch(){
    return document_view->scratchpad;
}

#ifdef SIOYEK_OPENGL_BACKEND

GLuint PdfViewOpenGLWidget::LoadShaders(Path vertex_file_path, Path fragment_file_path) {
    //const wchar_t* vertex_file_path = vertex_file_path_.c_str();
    //const wchar_t* fragment_file_path = fragment_file_path_.c_str();
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);


#ifdef SIOYEK_ANDROID
    std::string header = "#version 310 es\n";
#elif defined(SIOYEK_IOS)
    std::string header = "#version 300 core\n";
#else
    std::string header = "#version 330 core\n";
#endif

    std::string vertex_shader_code_utf8 = read_file_contents(vertex_file_path);
    if (vertex_shader_code_utf8.size() == 0) {
        return 0;
    }
    vertex_shader_code_utf8 = header + vertex_shader_code_utf8;

    std::string fragment_shader_code_utf8 = read_file_contents(fragment_file_path);
    if (fragment_shader_code_utf8.size() == 0) {
        return 0;
    }

    fragment_shader_code_utf8 = header + fragment_shader_code_utf8;

    GLint Result = GL_FALSE;
    int InfoLogLength;

    char const* VertexSourcePointer = vertex_shader_code_utf8.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
    glCompileShader(VertexShaderID);

    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }

    char const* FragmentSourcePointer = fragment_shader_code_utf8.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
    glCompileShader(FragmentShaderID);

    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        printf("%s\n", &FragmentShaderErrorMessage[0]);
    }

    // Link the program
    GLuint ProgramID = glCreateProgram();
    glAttachShader(ProgramID, VertexShaderID);
    glAttachShader(ProgramID, FragmentShaderID);
    glLinkProgram(ProgramID);

    // Check the program
    glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
    glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
    }

    glDetachShader(ProgramID, VertexShaderID);
    glDetachShader(ProgramID, FragmentShaderID);

    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);

    return ProgramID;
}

void PdfViewOpenGLWidget::initializeGL() {
    initializeOpenGLFunctions();

    if (!shared_gl_objects.is_initialized) {
        // we initialize the shared opengl objects here. Ideally we should have initialized them before any object
        // of this type was created but we could not use any OpenGL function before initalizeGL is called for the
        // first time.

        shared_gl_objects.is_initialized = true;

        //shared_gl_objects.rendered_program = LoadShaders(concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path, L"simple.fragment"));
        //shared_gl_objects.rendered_dark_program = LoadShaders(concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path, L"dark_mode.fragment"));
        //shared_gl_objects.unrendered_program = LoadShaders(concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path, L"unrendered_page.fragment"));
        //shared_gl_objects.highlight_program = LoadShaders( concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path , L"highlight.fragment"));
        //shared_gl_objects.vertical_line_program = LoadShaders(concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path , L"vertical_bar.fragment"));
        //shared_gl_objects.vertical_line_dark_program = LoadShaders(concatenate_path(shader_path , L"simple.vertex"),  concatenate_path(shader_path , L"vertical_bar_dark.fragment"));

#ifdef SIOYEK_MOBILE
        shared_gl_objects.rendered_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/simple.fragment"));
        shared_gl_objects.rendered_dark_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/dark_mode.fragment"));
        shared_gl_objects.unrendered_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/unrendered_page.fragment"));
        shared_gl_objects.highlight_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/highlight.fragment"));
        shared_gl_objects.vertical_line_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/vertical_bar.fragment"));
        shared_gl_objects.vertical_line_dark_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/vertical_bar_dark.fragment"));
        shared_gl_objects.custom_color_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/custom_colors.fragment"));
        shared_gl_objects.separator_program = LoadShaders(Path(L":/pdf_viewer/shaders/simple.vertex"), Path(L":/pdf_viewer/shaders/separator.fragment"));
        shared_gl_objects.stencil_program = LoadShaders(Path(L":/pdf_viewer/shaders/stencil.vertex"), Path(L":/pdf_viewer/shaders/stencil.fragment"));
        shared_gl_objects.line_program = LoadShaders(Path(L":/pdf_viewer/shaders/line.vertex"), Path(L":/pdf_viewer/shaders/line.fragment"));
        shared_gl_objects.compiled_drawing_program = LoadShaders(Path(L":/pdf_viewer/shaders/compiled_drawing.vertex"), Path(L":/pdf_viewer/shaders/compiled_line.fragment"));
        shared_gl_objects.compiled_dots_program = LoadShaders(Path(L":/pdf_viewer/shaders/dot.vertex"), Path(L":/pdf_viewer/shaders/dot.fragment"));
#else
        shared_gl_objects.rendered_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"simple.fragment"));
        shared_gl_objects.rendered_dark_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"dark_mode.fragment"));
        shared_gl_objects.unrendered_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"unrendered_page.fragment"));
        shared_gl_objects.highlight_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"highlight.fragment"));
        shared_gl_objects.vertical_line_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"vertical_bar.fragment"));
        shared_gl_objects.vertical_line_dark_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"vertical_bar_dark.fragment"));
        shared_gl_objects.custom_color_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"custom_colors.fragment"));
        shared_gl_objects.separator_program = LoadShaders(shader_path.slash(L"simple.vertex"), shader_path.slash(L"separator.fragment"));
        shared_gl_objects.stencil_program = LoadShaders(shader_path.slash(L"stencil.vertex"), shader_path.slash(L"stencil.fragment"));
        shared_gl_objects.line_program = LoadShaders(shader_path.slash(L"line.vertex"), shader_path.slash(L"line.fragment"));
        shared_gl_objects.compiled_drawing_program = LoadShaders(shader_path.slash(L"compiled_drawing.vertex"), shader_path.slash(L"compiled_line.fragment"));
        shared_gl_objects.compiled_dots_program = LoadShaders(shader_path.slash(L"dot.vertex"), shader_path.slash(L"dot.fragment"));
#endif

        shared_gl_objects.dark_mode_contrast_uniform_location = glGetUniformLocation(shared_gl_objects.rendered_dark_program, "contrast");
        //shared_gl_objects.gamma_uniform_location = glGetUniformLocation(shared_gl_objects.rendered_program, "gamma");

        shared_gl_objects.highlight_color_uniform_location = glGetUniformLocation(shared_gl_objects.highlight_program, "highlight_color");
        shared_gl_objects.highlight_opacity_uniform_location = glGetUniformLocation(shared_gl_objects.highlight_program, "opacity");

        shared_gl_objects.line_color_uniform_location = glGetUniformLocation(shared_gl_objects.vertical_line_program, "line_color");
        shared_gl_objects.line_time_uniform_location = glGetUniformLocation(shared_gl_objects.vertical_line_program, "time");

        shared_gl_objects.custom_color_transform_uniform_location = glGetUniformLocation(shared_gl_objects.custom_color_program, "transform_matrix");

        shared_gl_objects.separator_background_color_uniform_location = glGetUniformLocation(shared_gl_objects.separator_program, "background_color");
        shared_gl_objects.freehand_line_color_uniform_location = glGetUniformLocation(shared_gl_objects.line_program, "line_color");

        shared_gl_objects.compiled_drawing_offset_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_drawing_program, "offset");
        shared_gl_objects.compiled_drawing_scale_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_drawing_program, "scale");
        shared_gl_objects.compiled_drawing_colors_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_drawing_program, "type_colors");

        shared_gl_objects.compiled_dot_color_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_dots_program, "type_colors");
        shared_gl_objects.compiled_dot_offset_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_dots_program, "offset");
        shared_gl_objects.compiled_dot_scale_uniform_location = glGetUniformLocation(shared_gl_objects.compiled_dots_program, "scale");

        glGenBuffers(1, &shared_gl_objects.vertex_buffer_object);
        glGenBuffers(1, &shared_gl_objects.uv_buffer_object);
        glGenBuffers(1, &shared_gl_objects.line_points_buffer_object);

        glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex), g_quad_vertex, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_uvs), g_quad_uvs, GL_DYNAMIC_DRAW);

    }

    //vertex array objects can not be shared for some reason!
    glGenVertexArrays(1, &vertex_array_object);
    glBindVertexArray(vertex_array_object);

    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void PdfViewOpenGLWidget::render_line_window_opengl_backend(float gl_vertical_pos, std::optional<NormalizedWindowRect> ruler_rect) {

    float bar_height = 4.0f;

    float bar_data[] = {
        -1, gl_vertical_pos,
        1, gl_vertical_pos,
        -1, gl_vertical_pos - bar_height,
        1, gl_vertical_pos - bar_height
    };


    glDisable(GL_CULL_FACE);
    glUseProgram(shared_gl_objects.vertical_line_program);

    std::array<float, 4> vertical_line_color = cc4(DEFAULT_VERTICAL_LINE_COLOR);

    glUniform4fv(shared_gl_objects.line_color_uniform_location,
        1,
        &vertical_line_color[0]);

    float time = 0;
    glUniform1f(shared_gl_objects.line_time_uniform_location, time);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(line_data), line_data, GL_DYNAMIC_DRAW);
    //glDrawArrays(GL_LINES, 0, 2);

    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bar_data), bar_data, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if ((get_ruler_display_mode() != RulerDisplayMode::HighlightBelow) && ruler_rect.has_value()) {
        float gl_vertical_begin_pos = ruler_rect->y0;
        float ruler_left_pos = ruler_rect->x0;
        float ruler_right_pos = ruler_rect->x1;
        float top_bar_data[] = {
            -1, gl_vertical_begin_pos + bar_height,
            1, gl_vertical_begin_pos + bar_height,
            -1, gl_vertical_begin_pos,
            1, gl_vertical_begin_pos
        };

        float left_bar_data[] = {
            -1, gl_vertical_begin_pos,
            ruler_left_pos, gl_vertical_begin_pos,
            -1, gl_vertical_pos,
            ruler_left_pos, gl_vertical_pos
        };
        float right_bar_data[] = {
            ruler_right_pos, gl_vertical_begin_pos,
            1, gl_vertical_begin_pos,
            ruler_right_pos, gl_vertical_pos,
            1, gl_vertical_pos
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(top_bar_data), top_bar_data, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBufferData(GL_ARRAY_BUFFER, sizeof(left_bar_data), left_bar_data, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBufferData(GL_ARRAY_BUFFER, sizeof(right_bar_data), right_bar_data, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisable(GL_BLEND);

}

void PdfViewOpenGLWidget::render_highlight_window_opengl_backend(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels) {

    if (document_view->is_rotated()) {
        return;
    }

    float scale_factor = dv()->get_zoom_level() / dv()->get_view_height();

    float line_width_window = STRIKE_LINE_WIDTH * scale_factor;

    if (line_width_in_pixels > 0){
        line_width_window = static_cast<float>(line_width_in_pixels) / dv()->get_view_height();
    }

    glEnable(GL_BLEND);
    if (flags & HighlightRenderFlags::HRF_INVERTED) {
        glBlendFuncSeparate(GL_ONE_MINUS_DST_COLOR, GL_ZERO, GL_ONE, GL_ZERO);
    }
    else if (flags & HighlightRenderFlags::HRF_PAINTOVER) {
        if (is_background_dark()) {
            glBlendFuncSeparate(GL_ONE_MINUS_DST_COLOR, GL_DST_COLOR, GL_ONE, GL_ONE);
        }
        else {
            glBlendFuncSeparate(GL_ONE_MINUS_SRC_COLOR, GL_SRC_COLOR, GL_ONE, GL_ONE);
        }
    }
    else {
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
    }

    glDisable(GL_CULL_FACE);

    glUseProgram(shared_gl_objects.highlight_program);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    if (flags & HRF_UNDERLINE) {
        float underline_data[] = {
            window_rect.x0, window_rect.y1 + line_width_window,
            window_rect.x1, window_rect.y1 + line_width_window,
            window_rect.x0, window_rect.y1 - line_width_window,
            window_rect.x1, window_rect.y1 - line_width_window
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(underline_data), underline_data, GL_DYNAMIC_DRAW);
    }
    else if (flags & HRF_STRIKE) {
        float strike_data[] = {
            window_rect.x0, (window_rect.y1 + window_rect.y0) / 2 ,
            window_rect.x1, (window_rect.y1 + window_rect.y0) / 2 ,
            window_rect.x0, (window_rect.y1 + window_rect.y0) / 2 - 2 * line_width_window,
            window_rect.x1, (window_rect.y1 + window_rect.y0) / 2 - 2 * line_width_window
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(strike_data), strike_data, GL_DYNAMIC_DRAW);
    }
    else {
        float quad_vertex_data[] = {
            window_rect.x0, window_rect.y1,
            window_rect.x1, window_rect.y1,
            window_rect.x0, window_rect.y0,
            window_rect.x1, window_rect.y0
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
    }


    // no need to draw the fill color if we are in underline/strike mode
    if (flags & HRF_FILL) {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    if (flags & HRF_BORDER) {

        float line_data[] = {
            window_rect.x0, window_rect.y0,
            window_rect.x1, window_rect.y0,
            window_rect.x1, window_rect.y1,
            window_rect.x0, window_rect.y1
        };

        // glDisable(GL_BLEND);
        glBufferData(GL_ARRAY_BUFFER, sizeof(line_data), line_data, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINE_LOOP, 0, 4);
    }
    if (flags & HRF_UNDERLINE) {
        glDisable(GL_BLEND);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    if (flags & HRF_STRIKE) {
        // glDisable(GL_BLEND);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

void PdfViewOpenGLWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);

    if (dv()) {
        dv()->on_view_size_change(w, h);
    }
}

void PdfViewOpenGLWidget::compile_drawings_opengl_backend(DocumentView* dv, const std::vector<FreehandDrawing>& drawings) {
    ScratchPad* scratchpad = scratch();
    if (scratchpad->cached_compiled_drawing_data) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->index_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->vertex_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->dots_index_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->dots_vertex_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->dots_uv_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->lines_type_index_buffer);
        glDeleteBuffers(1, &scratchpad->cached_compiled_drawing_data->dots_type_index_buffer);
        glDeleteVertexArrays(1, &scratchpad->cached_compiled_drawing_data->vao);
        scratchpad->cached_compiled_drawing_data = {};
    }
    if (drawings.size() == 0) {
        scratchpad->on_compile();
        return;
    }


    //float thickness_x = dv->get_zoom_level() / width();
    //float thickness_y = dv->get_zoom_level() / height();

    float thickness_x = 0.5f;
    float thickness_y = 0.5f;

    std::vector<float> coordinates;
    std::vector<unsigned int> indices;
    std::vector<GLint> type_indices;
    /* std::vector<short> */

    std::vector<float> dot_coordinates;
    std::vector<unsigned int> dot_indices;
    std::vector<GLint> dot_type_indices;
    /* std::vector<short> */
    GLuint primitive_restart_index = 0xFFFFFFFF;

    int index = 0;
    int dot_index = 0;

    auto add_point_coords = [&](FreehandDrawingPoint p, char index) {
        dot_coordinates.push_back(p.pos.x - p.thickness / 2);
        dot_coordinates.push_back(p.pos.y - p.thickness / 2);
        dot_type_indices.push_back(index);
        dot_indices.push_back(dot_index++);

        dot_coordinates.push_back(p.pos.x - p.thickness / 2);
        dot_coordinates.push_back(p.pos.y + p.thickness / 2);
        dot_type_indices.push_back(index);
        dot_indices.push_back(dot_index++);

        dot_coordinates.push_back(p.pos.x + p.thickness / 2);
        dot_coordinates.push_back(p.pos.y - p.thickness / 2);
        dot_type_indices.push_back(index);
        dot_indices.push_back(dot_index++);

        dot_coordinates.push_back(p.pos.x + p.thickness / 2);
        dot_coordinates.push_back(p.pos.y + p.thickness / 2);
        dot_type_indices.push_back(index);
        dot_indices.push_back(dot_index++);

        dot_indices.push_back(0xFFFFFFFF);
    };

    for (auto drawing : drawings) {
        if (DEBUG_SMOOTH_FREEHAND_DRAWINGS) {
            drawing = smoothen_drawing(drawing);
        }

        if (drawing.points.size() <= 0) {
            continue;
        }
        if (!document_view->visible_drawing_mask[drawing.type - 'a']) {
            continue;
        }
        if (drawing.points.size() == 1) {
            add_point_coords(drawing.points[0], drawing.type - 'a');

            continue;

        }

        float first_line_x = (drawing.points[1].pos.x - drawing.points[0].pos.x) * width();
        float first_line_y = (drawing.points[1].pos.y - drawing.points[0].pos.y) * height();
        float first_line_size = sqrt(first_line_x * first_line_x + first_line_y * first_line_y);
        first_line_x = first_line_x / first_line_size;
        first_line_y = first_line_y / first_line_size;

        float first_ortho_x = -first_line_y * thickness_x * drawing.points[0].thickness;
        float first_ortho_y = first_line_x * thickness_y * drawing.points[0].thickness;


        coordinates.push_back(drawing.points[0].pos.x - first_ortho_x);
        coordinates.push_back(drawing.points[0].pos.y - first_ortho_y);
        type_indices.push_back(drawing.type - 'a');
        indices.push_back(index++);
        coordinates.push_back(drawing.points[0].pos.x + first_ortho_x);
        coordinates.push_back(drawing.points[0].pos.y + first_ortho_y);
        type_indices.push_back(drawing.type - 'a');
        indices.push_back(index++);

        float prev_line_x = first_line_x;
        float prev_line_y = first_line_y;

        add_point_coords(drawing.points[0], drawing.type - 'a');
        add_point_coords(drawing.points.back(), drawing.type - 'a');
        for (int line_index = 0; line_index < drawing.points.size() - 1; line_index++) {
            float line_direction_x = (drawing.points[line_index + 1].pos.x - drawing.points[line_index].pos.x);
            float line_direction_y = (drawing.points[line_index + 1].pos.y - drawing.points[line_index].pos.y);
            float line_size = sqrt(line_direction_x * line_direction_x + line_direction_y * line_direction_y);
            line_direction_x = line_direction_x / line_size;
            line_direction_y = line_direction_y / line_size;

            float ortho_x1 = -line_direction_y * thickness_x * drawing.points[line_index].thickness;
            float ortho_y1 = line_direction_x * thickness_y * drawing.points[line_index].thickness;

            float ortho_x2 = -line_direction_y * thickness_x * drawing.points[line_index + 1].thickness;
            float ortho_y2 = line_direction_x * thickness_y * drawing.points[line_index + 1].thickness;

            coordinates.push_back(drawing.points[line_index].pos.x - ortho_x1);
            coordinates.push_back(drawing.points[line_index].pos.y - ortho_y1);
            type_indices.push_back(drawing.type - 'a');
            indices.push_back(index++);

            coordinates.push_back(drawing.points[line_index].pos.x + ortho_x1);
            coordinates.push_back(drawing.points[line_index].pos.y + ortho_y1);
            type_indices.push_back(drawing.type - 'a');
            indices.push_back(index++);

            coordinates.push_back(drawing.points[line_index + 1].pos.x - ortho_x1);
            coordinates.push_back(drawing.points[line_index + 1].pos.y - ortho_y1);
            type_indices.push_back(drawing.type - 'a');
            indices.push_back(index++);

            coordinates.push_back(drawing.points[line_index + 1].pos.x + ortho_x2);
            coordinates.push_back(drawing.points[line_index + 1].pos.y + ortho_y2);
            type_indices.push_back(drawing.type - 'a');
            indices.push_back(index++);

        }
        indices.push_back(primitive_restart_index);

        //std::vector<int> indices;

        //bind_points(coordinates);
        //glDrawArrays(GL_TRIANGLE_STRIP, 0, coordinates.size() / 2);
    }
    scratchpad->cached_compiled_drawing_data = compile_drawings_into_vertex_and_index_buffers(
        coordinates,
        indices,
        type_indices,
        dot_coordinates,
        dot_indices,
        dot_type_indices);
    scratchpad->on_compile();
}

CompiledDrawingData PdfViewOpenGLWidget::compile_drawings_into_vertex_and_index_buffers(const std::vector<float>& line_coordinates,
    const std::vector<unsigned int>& indices,
    const std::vector<GLint>& line_type_indices,
    const std::vector<float>& dot_coordinates,
    const std::vector<unsigned int>& dot_indices,
    const std::vector<GLint>& dot_type_indices){
    //std::vector<unsigned int> indices;
    //int num_rectangles = line_coordinates.size() /

    std::vector<float> dot_uv_coordinates;
    dot_uv_coordinates.reserve(dot_coordinates.size());
    float uv_map[4 * 2] = { 0.0f, 0.0f,
                            1.0f, 0.0f,
                            0.0f, 1.0f,
                            1.0f, 1.0f
    };

    for (int i = 0; i < dot_coordinates.size(); i++) {
        dot_uv_coordinates.push_back(uv_map[i % 8]);
    }

    GLuint compiled_drawing_vao;
    glGenVertexArrays(1, &compiled_drawing_vao);
    GLuint compiled_vertex_array,
        compiled_index_array,
        dots_vertex_buffer,
        dots_index_buffer,
        dots_uv_buffer,
        lines_type_index_buffer,
        dots_type_index_buffer;

    glGenBuffers(1, &compiled_vertex_array);
    glGenBuffers(1, &compiled_index_array);
    glGenBuffers(1, &dots_vertex_buffer);
    glGenBuffers(1, &dots_index_buffer);
    glGenBuffers(1, &dots_uv_buffer);
    glGenBuffers(1, &lines_type_index_buffer);
    glGenBuffers(1, &dots_type_index_buffer);

    glBindVertexArray(compiled_drawing_vao);

    glBindBuffer(GL_ARRAY_BUFFER, compiled_vertex_array);
    glBufferData(GL_ARRAY_BUFFER, line_coordinates.size() * sizeof(float), line_coordinates.data(), GL_STATIC_DRAW);


    //glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    //glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, lines_type_index_buffer);
    glBufferData(GL_ARRAY_BUFFER, line_type_indices.size() * sizeof(GLint), line_type_indices.data(), GL_STATIC_DRAW);

    //glVertexAttribPointer(1, 1, GL_INT, GL_FALSE, 2 * sizeof(float), (void*)0);
    //glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, dots_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, dot_coordinates.size() * sizeof(float), dot_coordinates.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, dots_uv_buffer);
    glBufferData(GL_ARRAY_BUFFER, dot_uv_coordinates.size() * sizeof(float), dot_uv_coordinates.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, dots_type_index_buffer);
    glBufferData(GL_ARRAY_BUFFER, dot_type_indices.size() * sizeof(GLint), dot_type_indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, compiled_index_array);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dots_index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, dot_indices.size() * sizeof(unsigned int), dot_indices.data(), GL_STATIC_DRAW);

    CompiledDrawingData res;
    res.vao = compiled_drawing_vao;
    res.vertex_buffer = compiled_vertex_array;
    res.index_buffer = compiled_index_array;
    res.dots_vertex_buffer = dots_vertex_buffer;
    res.dots_index_buffer = dots_index_buffer;
    res.dots_uv_buffer = dots_uv_buffer;
    res.lines_type_index_buffer = lines_type_index_buffer;
    res.dots_type_index_buffer = dots_type_index_buffer;
    res.n_elements = indices.size();
    res.n_dot_elements = dot_indices.size();
    return res;

}

void PdfViewOpenGLWidget::paintGL() {

    do_paint();
}


void PdfViewOpenGLWidget::render_overview_opengl_backend(NormalizedWindowRect window_rect, OverviewState overview, bool draw_border) {

    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glClear(GL_STENCIL_BUFFER_BIT);
    enable_stencil();
    write_to_stencil();
    draw_stencil_rects({ window_rect });
    use_stencil_to_write(true);

    draw_overview_background(overview);

    for (auto visible_page : get_overview_visible_pages(overview)) {
        render_page(visible_page, overview, ColorPalette::None, false);
    }

    std::optional<SearchResult> highlighted_result = document_view->get_current_search_result();
    // highlight the overview search result
    if (highlighted_result) {
        glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
        glUseProgram(shared_gl_objects.highlight_program);
        glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &DEFAULT_SEARCH_HIGHLIGHT_COLOR[0]);
        glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);

        for (auto rect : highlighted_result->rects) {
            NormalizedWindowRect target = document_view->document_to_overview_rect(DocumentRect{ rect, highlighted_result->page });
            render_highlight_window(target, HRF_FILL | HRF_BORDER);
        }
    }

    // if (document_view->overview_highlights.size() > 0) {
    //     glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    //     glUseProgram(shared_gl_objects.highlight_program);
    //     glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &OVERVIEW_REFERENCE_HIGHLIGHT_COLOR[0]);
    //     //glUniform3fv(g_shared_resources.highlight_color_uniform_location, 1, highlight_color_temp);
    //     for (auto rect : document_view->overview_highlights) {
    //         NormalizedWindowRect target = document_view->document_to_overview_rect(rect);
    //         render_highlight_window(target, HRF_FILL);
    //     }
    // }

    if (overview.highlight_rects.size() > 0) {
        glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
        glUseProgram(shared_gl_objects.highlight_program);
        glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &OVERVIEW_REFERENCE_HIGHLIGHT_COLOR[0]);
        //glUniform3fv(g_shared_resources.highlight_color_uniform_location, 1, highlight_color_temp);
        for (auto rect : overview.highlight_rects) {
            NormalizedWindowRect target = document_view->document_to_overview_rect(rect);
            if (OVERVIEW_HIGHLIGHT_STYLE == HighlightStyle::HighlightTransparent) {
                render_highlight_window(target, HRF_FILL);
            }
            else if (OVERVIEW_HIGHLIGHT_STYLE == HighlightStyle::HighlightBorder) {
                render_highlight_window(target, HRF_BORDER);
            }
            else {
                render_highlight_window(target, HRF_FILL | HRF_PAINTOVER);
            }
        }
    }

    disable_stencil();

    if (draw_border) {
        draw_overview_border(overview);
    }

    return;
}

#endif  // SIOYEK_OPENGL_BACKEND

void PdfViewOpenGLWidget::begin_native_painting(){
    painter.beginNativePainting();
}

void PdfViewOpenGLWidget::end_native_painting(){
    painter.endNativePainting();

}

void PdfViewOpenGLWidget::clear_background_buffers(float r, float g, float b, GLuint buffer_flags){
    glClearColor(r, g, b, 1.0f);
    glClear(buffer_flags);
}

void SioyekRendererBackend::draw_pixmap(QRect rect, QPixmap* pixmap){
    painter.drawPixmap(rect, *pixmap);
}

void SioyekRendererBackend::fill_rect(QRect rect, const QColor& color){
    painter.fillRect(rect, color);
}

void PdfViewOpenGLWidget::prepare_line_drawing_pipeline(){

#ifdef SIOYEK_OPENGL_BACKEND
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBindVertexArray(vertex_array_object);
    bind_default();
#endif
}

void PdfViewOpenGLWidget::prepare_non_compiled_line_drawing_pipeline(){

#ifdef SIOYEK_OPENGL_BACKEND
     glEnableVertexAttribArray(0);
     glUseProgram(shared_gl_objects.line_program);
#else
    qDebug() << "prepare_non_compiled_line_drawing_pipeline not implemented";
#endif
}

void PdfViewOpenGLWidget::enable_multisampling(){

#ifdef SIOYEK_OPENGL_BACKEND
     glEnable(GL_MULTISAMPLE);
#else
    painter.setRenderHints(QPainter::RenderHint::Antialiasing, true);
#endif
}

void PdfViewOpenGLWidget::disable_multisampling(){

#ifdef SIOYEK_OPENGL_BACKEND
     glDisable(GL_MULTISAMPLE);
#else
    painter.setRenderHints(QPainter::RenderHint::Antialiasing, false);
#endif
}

void PdfViewOpenGLWidget::prepare_highlight_pipeline(){

#ifdef SIOYEK_OPENGL_BACKEND
    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glUseProgram(shared_gl_objects.highlight_program);
#endif
}

void PdfViewOpenGLWidget::render_texture(std::optional<SioyekTextureType> texture, NormalizedWindowRect window_rect, ColorPalette forced_color_palette){


#ifdef SIOYEK_OPENGL_BACKEND
    float vertices[8];
    rect_to_quad(window_rect, vertices);
    if (texture.has_value() && std::holds_alternative<GLuint>(texture.value())) {
        bind_program(forced_color_palette);
        GLuint t = std::get<GLuint>(texture.value());
        glBindTexture(GL_TEXTURE_2D, t);
    }
    else {
        if (!SHOULD_DRAW_UNRENDERED_PAGES) {
            return;
        }
        float white[3] = {1, 1, 1};
        std::array<float, 3> bgcolor = cc3(white);
        glUseProgram(shared_gl_objects.highlight_program);
        glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, &bgcolor[0]);
    }

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_uvs), rotation_uvs[document_view->rotation_index], GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
    glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
#else
    if (!texture) return;
    QRect window_qrect = document_view->normalized_to_window_qrect(window_rect);
    painter.drawPixmap(window_qrect, *texture);

#endif
}

void PdfViewOpenGLWidget::prepare_initial_render_pipeline(){
#ifdef SIOYEK_OPENGL_BACKEND
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glBindVertexArray(vertex_array_object);
    bind_default();
#endif
}

void PdfViewOpenGLWidget::prepare_link_highlight_state(){
    std::array<float, 3> link_highlight_color = cc3(DEFAULT_LINK_HIGHLIGHT_COLOR);
#ifdef SIOYEK_OPENGL_BACKEND

    glUseProgram(shared_gl_objects.highlight_program);
    glUniform3fv(shared_gl_objects.highlight_color_uniform_location,
                 1,
                 &link_highlight_color[0]);
    glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, 0.3f);
#else
    set_highlight_color(&link_highlight_color[0], 0.3f);
#endif
}


void PdfViewOpenGLWidget::set_highlight_color(const float* color, float alpha){
#ifdef SIOYEK_OPENGL_BACKEND
    glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, color);
    glUniform1f(shared_gl_objects.highlight_opacity_uniform_location, alpha);
#else
    float rgba[4] = {color[0], color[1], color[2], alpha};
    painter.setPen(convert_float4_to_qcolor(rgba));
#endif
}

void PdfViewOpenGLWidget::prepare_for_line_drawing() {
    glUseProgram(shared_gl_objects.line_program);
    glEnableVertexAttribArray(0);
}

void SioyekRendererBackend::draw_pending_freehand_drawings(const std::vector<int>& visible_pages){

    prepare_for_line_drawing();

    std::vector<FreehandDrawing> pending_drawing;
    if (document_view->current_drawing.points.size() > 1) {
        pending_drawing.push_back(document_view->current_drawing);
    }
    enable_multisampling();
    for (auto page : visible_pages) {

        render_drawings(&painter, dv(), doc()->get_page_drawings(page));
    }
    render_drawings(&painter, dv(), document_view->moving_drawings, true);
    render_drawings(&painter, dv(), document_view->moving_drawings, false);
    render_drawings(&painter, dv(), pending_drawing);

    disable_multisampling();
    bind_default();

}

void SioyekRendererBackend::draw_icon(const QIcon& icon, QRect rect){
    icon.paint(&painter, rect);
}




void PdfViewOpenGLWidget::do_paint(){

    painter.begin(this);

    QColor red_color = QColor::fromRgb(255, 0, 0);
    painter.setPen(red_color);

    if (scratch() == nullptr) {
        my_render();
    }
    else {
        render_scratchpad();
    }
    painter.end();
}

ColorPalette SioyekRendererBackend::get_actual_color_palette(ColorPalette forced_color_palette){
    return forced_color_palette == ColorPalette::None ? document_view->get_current_color_mode() : forced_color_palette;

}

const QPainter& SioyekRendererBackend::get_painter() {
    return painter;
}

//void PdfViewOpenGLWidget::initialize_latex() {
//    if (!is_latex_initialized) {
//
//        tex::LaTeX::init("C:\\sioyek\\sioyek-new\\sioyek\\microtex_resources");
//        is_latex_initialized = true;
//    }
//}

bool SioyekRendererBackend::is_background_dark() {
    bool is_dark = false;
    if (document_view->get_current_color_mode() == ColorPalette::Dark) {
        is_dark = true;
    }
    else if (document_view->get_current_color_mode() == ColorPalette::Custom) {
        is_dark = CUSTOM_BACKGROUND_COLOR[0] + CUSTOM_BACKGROUND_COLOR[1] + CUSTOM_BACKGROUND_COLOR[2] < 1.0f;
    }
    return is_dark;
}

int SioyekRendererBackend::get_ruler_display_mode() {
    if (dv()->is_line_select_mode()) {
        return LINE_SELECT_RULER_DISPLAY_MODE;
    }
    return RULER_DISPLAY_MODE;
}

void SioyekRendererBackend::render_bookmark_annotations() {

    if (!doc()->can_use_highlights()) return;

    const std::vector<BookMark>& bookmarks = doc()->get_bookmarks();
    for (int i = 0; i < bookmarks.size(); i++) {
        if (bookmarks[i].begin_y != -1) {
            if (bookmarks[i].end_x == -1) {

                NormalizedWindowPos bookmark_window_pos = dv()->absolute_to_window_pos(
                    { bookmarks[i].begin_x, bookmarks[i].begin_y }
                );

                if (bookmark_window_pos.x > -1.5f && bookmark_window_pos.x < 1.5f &&
                    bookmark_window_pos.y > -1.5f && bookmark_window_pos.y < 1.5f) {

                    NormalizedWindowRect bookmark_normalized_window_rect = bookmarks[i].get_rectangle()->to_window_normalized(dv());

                    fz_irect window_rect = dv()->normalized_to_window_rect(bookmark_normalized_window_rect);
                    QRect window_qrect = QRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                    //bool is_highlighted = i == document_view->get_selected_bookmark_index();
                    bool is_highlighted = document_view->get_selected_bookmark_uuid() == bookmarks[i].uuid;

                    render_ui_icon_for_current_color_mode(bookmark_icon, bookmark_icon_white, window_qrect, is_highlighted);

                }
            }
            else {
                WindowRect window_rect = bookmarks[i].get_rectangle()->to_window(dv());
                NormalizedWindowRect window_rect_normalized = bookmarks[i].get_rectangle()->to_window_normalized(dv());
                if (!window_rect_normalized.is_visible()) {
                    continue;
                }

                QRect window_qrect = QRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));

                float scroll_amount = document_view->get_bookmark_scroll_amount(bookmarks[i].uuid);

                if (RENDER_FREETEXT_BORDERS) {
                    painter.drawRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                }


                QString desc_qstring = QString::fromStdWString(bookmarks[i].description);

                if (bookmarks[i].uuid == document_view->get_selected_bookmark_uuid()) {
                    painter.save();
                    QColor pen_color = convert_float3_to_qcolor(&SELECTED_BORDER_COLOR[0]);
                    painter.setPen(QPen(pen_color, SELECTED_BORDER_PEN_SIZE, Qt::DotLine));
                    QRect fill_rect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                    painter.fillRect(fill_rect, QColor(255, 255, 0, 128));
                    painter.drawRect(
                        window_rect.x0 - SELECTED_BORDER_PEN_SIZE / 2,
                        window_rect.y0 - SELECTED_BORDER_PEN_SIZE / 2,
                        fz_irect_width(window_rect) + SELECTED_BORDER_PEN_SIZE,
                        fz_irect_height(window_rect) + SELECTED_BORDER_PEN_SIZE
                    );
                    painter.restore();
                }

                if (bookmarks[i].is_question() || bookmarks[i].is_summary()) {
                    QColor question_background_color = qcc4(QUESTION_BOOKMARK_BACKGROUND_COLOR);
                    QRect fill_rect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                    painter.fillRect(fill_rect, question_background_color);

                }
                else {
                    std::optional<char> background_type = bookmarks[i].get_background_type();
                    if (background_type && background_type.value() >= 'a' && background_type.value() <= 'z') {
                        QColor highlight_background_color = qcc3(&HIGHLIGHT_COLORS[3 * (background_type.value() - 'a')]);
                        QRect fill_rect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                        painter.fillRect(fill_rect, highlight_background_color);
                    }
                }

                std::optional<char> bm_type = bookmarks[i].get_type();
                if (bookmarks[i].description[0] == '#' && !(desc_qstring.startsWith("#summarize"))) {

                    QString box_text = desc_qstring.split(' ')[0];
                    if (bm_type.has_value()) {
                        painter.setPen(convert_float3_to_qcolor(&HIGHLIGHT_COLORS[3 * (bm_type.value() - 'a')]));
                        painter.drawRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                        }

                    if (!bm_type.has_value()) {
                        //painter.drawRect(window_rect.x0, window_rect.y0, fz_irect_width(window_rect), fz_irect_height(window_rect));
                    }
                    else {
                        char mode = bm_type.value();
                        // draw an empty box with the color type of lowercase and draw a transparent filled box if uppercase
                        if (mode >= 'a' && mode <= 'z') {
                            std::array<float, 3> box_color = cc3(&HIGHLIGHT_COLORS[3 * (mode - 'a')]);
                            painter.setPen(convert_float3_to_qcolor(&box_color[0]));
                        }
                        else if (mode >= 'A' && mode <= 'Z') {
                            mode = mode - 'A' + 'a';
                            std::array<float, 3> box_color = cc3(&HIGHLIGHT_COLORS[3 * (mode - 'a')]);
                            float box_color_with_alpha[4];
                            box_color_with_alpha[0] = box_color[0];
                            box_color_with_alpha[1] = box_color[1];
                            box_color_with_alpha[2] = box_color[2];
                            box_color_with_alpha[3] = BOX_HIGHLIGHT_BOOKMARK_TRANSPARENCY;

                            QColor qcolor = convert_float4_to_qcolor(box_color_with_alpha);
                            QBrush brush(qcolor);

                            painter.fillRect(window_rect.to_qrect(), brush);
                        }
                    }
                    }

                auto [pixmap, was_exact] = pdf_renderer->get_bookmark_renderer()->request_rendered_bookmark(bookmarks[i], document_view->get_zoom_level(), scroll_amount, get_device_pixel_ratio(), dv()->get_current_color_mode());
                if (pixmap && (was_exact || bookmarks[i].is_latex() || (!ALWAYS_RENDER_BOOKMARKS))) {
                    painter.drawPixmap(window_qrect, *pixmap);
                }
                else {
                    if (ALWAYS_RENDER_BOOKMARKS) {
                        // rendering bookmarks on the main thrad can prevent flickering, but may
                        // cause the UI to slow down a little e.g. when zooming
                        pdf_renderer->get_bookmark_renderer()->render_freetext_bookmark(
                            bookmarks[i],
                            &painter,
                            dv()->get_zoom_level(),
                            scroll_amount,
                            get_device_pixel_ratio(),
                            window_qrect,
                            dv()->get_current_color_mode(),
                            true);
                    }
                }

                }

            }
        }
}

void SioyekRendererBackend::render_search_result_highlights(const std::vector<int>& visible_pages) {
    document_view->search_results_mutex.lock();
    if (document_view->search_results.size() > 0) {

        int index = document_view->current_search_result_index;
        if (index == -1) index = 0;

        SearchResult& current_search_result = document_view->search_results[index];
        current_search_result.fill(doc());

        prepare_highlight_pipeline();

        std::array<float, 3> unselected_search_highlight_color = cc3(UNSELECTED_SEARCH_HIGHLIGHT_COLOR);

        if (SHOULD_HIGHLIGHT_UNSELECTED_SEARCH) {

            std::vector<int> visible_search_indices = document_view->get_visible_search_results(visible_pages);
            set_highlight_color(&unselected_search_highlight_color[0], 0.3f);
            for (int visible_search_index : visible_search_indices) {
                if (visible_search_index != document_view->current_search_result_index) {
                    SearchResult& res = document_view->search_results[visible_search_index];
                    res.fill(doc());
                    for (auto rect : res.rects) {
                        render_highlight_document(DocumentRect { rect, res.page });
                    }
                }
            }
        }

        std::array<float, 3> search_highlight_color = cc3(DEFAULT_SEARCH_HIGHLIGHT_COLOR);
        set_highlight_color(&search_highlight_color[0], 0.3f);
        for (auto rect : current_search_result.rects) {
            render_highlight_document(DocumentRect { rect, current_search_result.page });
        }
    }
    document_view->search_results_mutex.unlock();
}

void SioyekRendererBackend::render_ruler() {
    if (dv()->is_ruler_mode()) {

        float vertical_line_end = dv()->get_ruler_window_y();
        /* std::optional<NormalizedWindowRect> ruler_rect = document_view->get_ruler_window_rect(); */
        // NormalizedWindowRect DocumentView::document_to_window_rect_pixel_perfect(DocumentRect doc_rect, int pixel_width, int pixel_height, bool banded) {
        //std::optional<NormalizedWindowRect> ruler_rect = {};
        std::optional<NormalizedWindowRect> ruler_rect = dv()->get_ruler_window_rect();

        if (dv()->is_line_select_mode()) {
            // in line select mode we want the ruler to just fit to the line (no x and y padding)
            DocumentRect ruler_document_rect = dv()->get_ruler_rect()->to_document(doc());
            int ruler_pixel_width = static_cast<int>(ruler_document_rect.rect.width() * dv()->get_zoom_level());
            int ruler_pixel_height = static_cast<int>(ruler_document_rect.rect.height() * dv()->get_zoom_level());
            ruler_rect = dv()->document_to_window_rect_pixel_perfect(ruler_document_rect, ruler_pixel_width, ruler_pixel_height, false);
        }

        auto ruler_display_mode = get_ruler_display_mode();
        if ((!ruler_rect.has_value()) || (ruler_display_mode == RulerDisplayMode::Slit) || (ruler_display_mode == RulerDisplayMode::HighlightBelow)) {
            render_line_window(vertical_line_end, dv()->get_ruler_window_rect());
        }
        else {
            int flags = 0;

            if (ruler_display_mode == RulerDisplayMode::Underline) {
                flags |= HRF_UNDERLINE;
            }

            else if (ruler_display_mode == RulerDisplayMode::HighlightRuler) {
                flags |= HRF_PAINTOVER;
                flags |= HRF_FILL;
            }
            else if (ruler_display_mode == RulerDisplayMode::Box) {
                flags |= HRF_BORDER;
            }


            //auto ruler_color_adjusted = cc3(RULER_COLOR);
            const float* ruler_color = RULER_COLOR;

            if (dv()->is_line_select_mode()) {
                ruler_color = LINE_SELECT_RULER_COLOR;
            }

            auto ruler_color_adjusted = cc3(ruler_color);
            if (ADJUST_ANNOTATION_COLORS_FOR_DARK_MODE) {
                ruler_color = &ruler_color_adjusted[0];
            }

            set_highlight_color(&ruler_color[0], 1.0f);
            render_highlight_window(ruler_rect.value(), flags, RULER_UNDERLINE_PIXEL_WIDTH);
        }
        if (document_view->underline) {
            prepare_highlight_pipeline();
            set_highlight_color(RULER_MARKER_COLOR, 1.0f);

            AbsoluteRect underline_rect;
            underline_rect.x0 = document_view->underline->x - 3.0f;
            underline_rect.x1 = document_view->underline->x + 3.0f;

            underline_rect.y0 = document_view->underline->y - 1.0f;
            underline_rect.y1 = document_view->underline->y + 1.0f;
            NormalizedWindowRect underline_window_rect = underline_rect.to_window_normalized(dv());
            float mid_y = ruler_rect->y1;
            /* float underline_height = underline_window_rect.width() / 2.0f; */
            /* float underline_height = ruler_rect->height(); */
            float underline_height = 4 * static_cast<float>(RULER_UNDERLINE_PIXEL_WIDTH) / static_cast<float>(dv()->get_view_height());
            underline_window_rect.y0 = mid_y - underline_height / 2;
            underline_window_rect.y1 = mid_y + underline_height / 2;

            render_highlight_window(underline_window_rect, HRF_FILL);
        }
    }

}

void SioyekRendererBackend::render_debug_highlights(){

    if (dv()->debug_highlight_rects.size() > 0) {

        //float dbg_color[3] = { 1, 0, 1 };
        for (int i = 0; i < dv()->debug_highlight_rects.size(); i++) {

            int index = i % 26;
            std::array<float, 3> text_highlight_color = cc3(&HIGHLIGHT_COLORS[3 * index]);
            set_highlight_color(&text_highlight_color[0], 0.3f);

            for (int j = 0; j < dv()->debug_highlight_rects[i].size(); j++) {
                render_highlight_absolute(dv()->debug_highlight_rects[i][j], HRF_FILL | HRF_BORDER);
            }
        }
    }
}

void SioyekRendererBackend::render_portals() {

    const std::vector<Portal>& portals = doc()->get_portals();
    prepare_highlight_pipeline();
    float color[] = { 1, 1, 1 };
    set_highlight_color(color, 0.3f);
    if (doc()->can_use_highlights()) {
        for (int i = 0; i < portals.size(); i++) {
            if (portals[i].is_icon()) {
                if (!portals[i].is_merged_rect_valid) {
                    portals[i].update_merged_rect(doc());
                    portals[i].is_merged_rect_valid = true;
                }
                if (portals[i].merged_rect) {
                    render_highlight_absolute(portals[i].get_rectangle().value(), HRF_FILL | HRF_INVERTED);
                }
            }
        }
    }

    for (int i = 0; i < portals.size(); i++) {
        if (portals[i].is_pinned()) {
            bool is_portal_selected = dv()->get_selected_pinned_portal_uuid() == portals[i].uuid;
            float selected_border_color[] = { 1, 0, 0 };
            OverviewState portal_overview_state;

            portal_overview_state.source_rect = portals[i].get_rectangle();
            portal_overview_state.absolute_offset_x = portals[i].dst.book_state.offset_x;
            portal_overview_state.absolute_offset_y = portals[i].dst.book_state.offset_y;
            portal_overview_state.zoom_level = portals[i].dst.book_state.zoom_level * dv()->get_zoom_level();
            portal_overview_state.source_portal = portals[i];

            portal_overview_state.doc = document_manager->get_document_with_checksum(portals[i].dst.document_checksum);
            if (portal_overview_state.doc) {
                if (!portal_overview_state.doc->get_is_opened()) {
                    portal_overview_state.doc->open(true);
                }

                render_overview(portal_overview_state, !is_portal_selected);
                if (is_portal_selected) {
                    //draw_overview_border(portal_overview_state, selected_border_color);
                    end_native_painting();

                    WindowRect window_rect = portal_overview_state.source_rect->to_window(dv());
                    QColor pen_color = convert_float3_to_qcolor(&SELECTED_BORDER_COLOR[0]);
                    painter.setPen(QPen(pen_color, SELECTED_BORDER_PEN_SIZE, Qt::DotLine));
                    painter.drawRect(
                        window_rect.x0 - SELECTED_BORDER_PEN_SIZE / 2 - 1,
                        window_rect.y0 - SELECTED_BORDER_PEN_SIZE / 2 + 1,
                        fz_irect_width(window_rect) + SELECTED_BORDER_PEN_SIZE + 1,
                        fz_irect_height(window_rect) + SELECTED_BORDER_PEN_SIZE + 1
                    );
                    begin_native_painting();
                }
            }
        }
    }

    end_native_painting();


    if (doc()->can_use_highlights()) {

        for (int i = 0; i < document_view->pending_download_portals.size(); i++) {
            auto pending_rect = document_view->pending_download_portals[i].pending_portal.get_rectangle();
            if (pending_rect.has_value()) {
                render_portal_rect(pending_rect.value(), true, document_view->pending_download_portals[i].downloaded_fraction);
            }
        }
        for (int i = 0; i < portals.size(); i++) {
            if (portals[i].is_icon()) {
                if (!portals[i].merged_rect) {
                    render_portal_rect(portals[i].get_rectangle().value(), false, {});
                }
            }
        }

        if (document_view->current_pending_portal) {
            Portal portal = document_view->current_pending_portal->second;
            if (portal.is_visible()) {
                render_portal_rect(portal.get_rectangle().value(), true, {});
            }
        }

    }
}

void SioyekRendererBackend::render_tags() {

    std::vector<PdfLink> all_visible_links;
    if (document_view->should_highlight_words && (!document_view->overview_page)) {
        setup_text_painter();

        std::vector<std::string> tags = get_tags(document_view->word_rects.size());

        for (size_t i = 0; i < document_view->word_rects.size(); i++) {
            //auto [rect, page] = word_rects[i];
            DocumentRect current_word_rect = document_view->word_rects[i];
            if (current_word_rect.page == -1) continue;


            NormalizedWindowRect window_rect = current_word_rect.to_window_normalized(dv());

            int view_width = static_cast<float>(dv()->get_view_width());
            int view_height = static_cast<float>(dv()->get_view_height());

            int window_x0 = static_cast<int>(window_rect.x0 * view_width / 2 + view_width / 2);
            int window_y0 = static_cast<int>(-window_rect.y0 * view_height / 2 + view_height / 2);

            if (dv()->should_highlight_rect_mode) {
                auto center = window_rect.to_window(dv()).center();
                window_x0 = center.x - painter.font().pixelSize() / 2;
            }

            if (i > 0 && (!dv()->should_highlight_rect_mode)) {
                if (std::abs(document_view->word_rects[i - 1].rect.x0 - current_word_rect.rect.x0) < 5) {
                    window_y0 = static_cast<int>(-window_rect.y1 * view_height / 2 + view_height / 2);
                }
            }

            int window_y1 = static_cast<int>(-window_rect.y1 * view_height / 2 + view_height / 2);

            bool highlighted = document_view->is_tag_highlighted(tags[i]);
            QString remaining_tag = QString::fromStdString(tags[i]);
            if (document_view->tag_prefix.size() > 0) {
                if (remaining_tag.startsWith(QString::fromStdString(document_view->tag_prefix))) {
                    remaining_tag = remaining_tag.mid(document_view->tag_prefix.size());
                }
                else {
                    remaining_tag = "";
                }
            }

            QColor rect_highlight_color;
            QColor rect_highlight_color_opaque;
            QColor rect_inverted_color;
            if (dv()->should_highlight_rect_mode) {

                int index = i % 26;
                rect_highlight_color = QColor::fromRgbF(HIGHLIGHT_COLORS[3 * (index % 26)], HIGHLIGHT_COLORS[3 * (index % 26) + 1], HIGHLIGHT_COLORS[3 * (index % 26) + 2], 0.3f);
                rect_highlight_color_opaque = QColor::fromRgbF(HIGHLIGHT_COLORS[3 * (index % 26)], HIGHLIGHT_COLORS[3 * (index % 26) + 1], HIGHLIGHT_COLORS[3 * (index % 26) + 2]);
                rect_inverted_color = QColor::fromRgbF(1.0f - HIGHLIGHT_COLORS[3 * (index % 26)], 1.0f - HIGHLIGHT_COLORS[3 * (index % 26) + 1], 1.0f - HIGHLIGHT_COLORS[3 * (index % 26) + 2]);
                WindowRect wr = window_rect.to_window(dv());
                painter.fillRect(wr.to_qrect(), QBrush(rect_highlight_color));
            }

            if (remaining_tag.size() > 0) {
                if (highlighted) {
                    auto original_pen = painter.pen();
                    auto original_background = painter.background();
                    painter.setPen(qcc4(KEYBOARD_SELECTED_TAG_TEXT_COLOR));
                    painter.setBackground(qcc4(KEYBOARD_SELECTED_TAG_BACKGROUND_COLRO));
                    painter.drawText(window_x0, (window_y0 + window_y1) / 2, remaining_tag);
                    painter.setPen(original_pen);
                    painter.setBackground(original_background);
                }
                else {
                    if (dv()->should_highlight_rect_mode) {
                        painter.setPen(rect_inverted_color);
                        painter.setBackground(rect_highlight_color_opaque);
                    }

                    painter.drawText(window_x0, (window_y0 + window_y1) / 2, remaining_tag);
                }
            }
        }
    }

    if (document_view->should_highlight_links && document_view->should_show_numbers && (!document_view->overview_page)) {

        dv()->get_visible_links(all_visible_links);
        setup_text_painter();
        for (size_t i = 0; i < all_visible_links.size(); i++) {
            std::stringstream ss;
            ss << i;
            std::string index_string = ss.str();

            if (!NUMERIC_TAGS) {
                index_string = get_aplph_tag(i, all_visible_links.size());
            }

            //auto [page, link] = all_visible_links[i];
            auto link = all_visible_links[i];

            bool should_draw = true;

            // some malformed doucments have multiple overlapping links which makes reading
            // the link labels difficult. Here we only draw the link text if there are no
            // other close links. This has quadratic runtime but it should not matter since
            // there are not many links in a single PDF page.
            if (HIDE_OVERLAPPING_LINK_LABELS) {
                for (int j = i + 1; j < all_visible_links.size(); j++) {
                    auto other_link = all_visible_links[j];
                    float distance = std::abs(other_link.rects[0].x0 - link.rects[0].x0) + std::abs(other_link.rects[0].y0 - link.rects[0].y0);
                    if (distance < 10) {
                        should_draw = false;
                    }
                }
            }

            NormalizedWindowRect window_rect = DocumentRect(link.rects[0], link.source_page).to_window_normalized(dv());

            int view_width = static_cast<float>(dv()->get_view_width());
            int view_height = static_cast<float>(dv()->get_view_height());

            int window_x = static_cast<int>(window_rect.x0 * view_width / 2 + view_width / 2);
            int window_y = static_cast<int>(-window_rect.y0 * view_height / 2 + view_height / 2);

            if (document_view->tag_prefix.size() > 0) {
                if (index_string.find(document_view->tag_prefix) != 0) {
                    should_draw = false;
                }
                else {
                    index_string = index_string.substr(document_view->tag_prefix.size());
                }
            }
            if (should_draw) {
                painter.drawText(window_x, window_y, index_string.c_str());
            }
        }
    }

}

float PdfViewOpenGLWidget::get_device_pixel_ratio() {
    return devicePixelRatioF();
}

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//void PdfViewQPainterWidget::paintEvent(QPaintEvent* event){
//    do_paint();
//}
//
//
//void PdfViewQPainterWidget::render_highlight_window_qpainter_backend(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels){
//    if (document_view->is_rotated()) {
//        return;
//    }
//
//    float scale_factor = dv()->get_zoom_level() / dv()->get_view_height();
//
//    float line_width_window = STRIKE_LINE_WIDTH * scale_factor;
//
//    if (line_width_in_pixels > 0){
//        line_width_window = static_cast<float>(line_width_in_pixels) / dv()->get_view_height();
//    }
//
//    QRect pixel_window_rect = document_view->normalized_to_window_qrect(window_rect);
//    auto bottom_right = pixel_window_rect.bottomRight();
//    auto top_left = pixel_window_rect.topLeft();
//    auto center = pixel_window_rect.center();
//
//
//    if (flags & HRF_UNDERLINE) {
//        auto current_color = painter.pen().color();
//        current_color.setAlpha(255);
//        painter.setPen(QPen(current_color, line_width_in_pixels));
//        painter.drawLine(bottom_right.x(), bottom_right.y(), top_left.x(), bottom_right.y());
//        return;
//    }
//    else if (flags & HRF_STRIKE) {
//        auto current_color = painter.pen().color();
//        current_color.setAlpha(255);
//        QRect strike_rect(top_left.x(), center.y(), pixel_window_rect.width(), static_cast<int>(line_width_window * height()));
//        painter.fillRect(strike_rect, QBrush(current_color));
//        return;
//    }
//
//    // no need to draw the fill color if we are in underline/strike mode
//    if (flags & HRF_FILL) {
//
//        if (flags & HRF_INVERTED){
//            auto original_composition_mode = painter.compositionMode();
//            painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);
//            painter.setPen(QColor(0xff, 0xff, 0xff));
//            painter.fillRect(pixel_window_rect, QBrush(QColor(0xff, 0xff, 0xff)));
//            painter.setCompositionMode(original_composition_mode);
//        }
//        else if (flags & HRF_PAINTOVER){
//            auto original_composition_mode = painter.compositionMode();
//            bool is_dark = is_background_dark();
//
//            QColor color = painter.pen().color();
//            color.setAlpha(255);
//
//            if (is_dark) {
//                painter.setCompositionMode(QPainter::RasterOp_SourceOrDestination);
//                color = color.darker();
//            }
//            else {
//                color.setRed(255 - color.red());
//                color.setGreen(255 - color.green());
//                color.setBlue(255 - color.blue());
//                painter.setCompositionMode(QPainter::RasterOp_NotSourceAndDestination);
//            }
//
//            painter.fillRect(pixel_window_rect, QBrush(color));
//            painter.setCompositionMode(original_composition_mode);
//        }
//        else{
//            painter.fillRect(pixel_window_rect, QBrush(painter.pen().color()));
//        }
//    }
//
//    if (flags & HRF_BORDER) {
//        auto original_color = painter.pen().color();
//        auto dealphad = original_color;
//        dealphad.setAlpha(255);
//        painter.setPen(dealphad);
//        // painter.pen().color().setAlpha(255);
//        painter.drawRect(pixel_window_rect);
//        painter.setPen(original_color);
//    }
//}
//
//void PdfViewQPainterWidget::render_overview_qpainter_backend(NormalizedWindowRect window_rect, OverviewState overview, bool draw_border){
//
//    QRect overview_rect = document_view->normalized_to_window_qrect(window_rect);
//    QRegion overview_region = QRegion(overview_rect);
//    painter.setClipRegion(overview_region);
//
//    draw_overview_background();
//
//    for (auto page : get_overview_visible_pages(overview)) {
//        render_page(page, overview, ColorPalette::None, false);
//    }
//
//    std::optional<SearchResult> highlighted_result = document_view->get_current_search_result();
//    if (highlighted_result) {
//
//        set_highlight_color(&DEFAULT_SEARCH_HIGHLIGHT_COLOR[0], 0.3f);
//        for (auto rect : highlighted_result->rects) {
//            NormalizedWindowRect target = document_view->document_to_overview_rect(DocumentRect{ rect, highlighted_result->page }, overview);
//            render_highlight_window(target, HRF_FILL | HRF_BORDER);
//        }
//    }
//    if (overview.highlight_rects.size() > 0) {
//        set_highlight_color(&OVERVIEW_REFERENCE_HIGHLIGHT_COLOR[0], 0.3f);
//        for (auto rect : overview.highlight_rects) {
//            NormalizedWindowRect target = document_view->document_to_overview_rect(rect, overview);
//            render_highlight_window(target, HRF_FILL);
//        }
//    }
//    painter.setClipRect(rect());
//
//    if (draw_border) {
//        draw_overview_border(overview);
//    }
//}
//
//void PdfViewQPainterWidget::resizeEvent(QResizeEvent* event){
//    QWidget::resizeEvent(event);
//
//    if (dv()) {
//        dv()->on_view_size_change(event->size().width(), event->size().height());
//    }
//}
//
//void PdfViewQPainterWidget::clear_background_buffers(float r, float g, float b, GLuint buffer_flags){
//    if (buffer_flags & GL_COLOR_BUFFER_BIT){
//        int r_ = static_cast<int>(r * 255.0f);
//        int g_ = static_cast<int>(g * 255.0f);
//        int b_ = static_cast<int>(b * 255.0f);
//        QBrush background_brush(QColor(r_, g_, b_));
//        painter.fillRect(rect(), background_brush);
//    }
//}
//
//void PdfViewQPainterWidget::begin_native_painting(){
//}
//
//void PdfViewQPainterWidget::end_native_painting(){
//}
//
//void PdfViewQPainterWidget::render_texture(SioyekTextureType texture, NormalizedWindowRect window_rect, ColorPalette forced_color_palette){
//    if (!texture) return;
//    QRect window_qrect = document_view->normalized_to_window_qrect(window_rect);
//    painter.drawPixmap(window_qrect, *texture);
//}
