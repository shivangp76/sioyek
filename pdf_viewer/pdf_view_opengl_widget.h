#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <thread>
#include <mutex>
#include <optional>
#include <utility>
#include <memory>
#include <array>

//#include <qopenglfunctions_3_1.h>
#include <qopenglwidget.h>
#include <qopenglextrafunctions.h>
#include <qopenglfunctions.h>
#include <qopengl.h>
#include <qrhiwidget.h>>
#include <rhi/qrhi.h>


#ifndef SIOYEK_QT6
#include <qdesktopwidget.h>
#endif

#include <qpainter.h>

#include "coordinates.h"
#include "book.h"
#include "path.h"
#include "document_view.h"

class DocumentView;
class PdfRenderer;
class ConfigManager;
class ScratchPad;

enum HighlightRenderFlags
{
    HRF_FILL = 1 << 0,
    HRF_BORDER = 1 << 1,
    HRF_UNDERLINE = 1 << 2,
    HRF_STRIKE = 1 << 3,
    HRF_INVERTED = 1 << 4
};

struct OpenGLSharedResources {
    GLuint vertex_buffer_object = 0;
    GLuint uv_buffer_object = 0;
    GLuint line_points_buffer_object = 0;
    GLuint rendered_program = 0;
    GLuint rendered_dark_program = 0;
    GLuint custom_color_program = 0;
    GLuint unrendered_program = 0;
    GLuint highlight_program = 0;
    GLuint vertical_line_program = 0;
    GLuint vertical_line_dark_program = 0;
    GLuint separator_program = 0;
    GLuint stencil_program = 0;
    GLuint line_program = 0;
    GLuint compiled_drawing_program = 0;
    GLuint compiled_dots_program = 0;

    GLint dark_mode_contrast_uniform_location = 0;
    GLint highlight_color_uniform_location = 0;
    GLint highlight_opacity_uniform_location = 0;
    GLint line_color_uniform_location = 0;
    GLint line_time_uniform_location = 0;

    GLint compiled_drawing_offset_uniform_location = 0;
    GLint compiled_drawing_scale_uniform_location = 0;
    GLint compiled_drawing_colors_uniform_location = 0;
    GLint compiled_dot_color_uniform_location = 0;
    GLint compiled_dot_offset_uniform_location = 0;
    GLint compiled_dot_scale_uniform_location = 0;

    //GLint gamma_uniform_location = 0;

    GLint custom_color_transform_uniform_location = 0;

    GLint separator_background_color_uniform_location = 0;
    GLint freehand_line_color_uniform_location = 0;

    GLuint compiled_points_vertex_buffer_object = 0;
    GLuint compiled_points_index_buffer_object = 0;

    //GLuint test_vao;
    //GLuint test_vbo;
    //GLuint test_ibo;

    bool is_initialized = false;
};



class PdfViewOpenGLWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
//class PdfViewOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_1 {
public:


private:

    OpenGLSharedResources shared_gl_objects;
    bool is_opengl_initialized = false;
    GLuint vertex_array_object;
    DocumentView* document_view = nullptr;
    ScratchPad* scratchpad = nullptr;
    PdfRenderer* pdf_renderer = nullptr;

    bool is_helper = false;
    std::string tag_prefix = "";
    std::vector<std::string> highlighted_tags;

    std::optional<AbsoluteRect> pending_portal_rect = {};

    QIcon bookmark_icon;
    QIcon portal_icon;
    QIcon bookmark_icon_white;
    QIcon portal_icon_white;
    QIcon hourglass_icon;

    std::optional<AbsoluteRect> character_highlight_rect = {};
    std::optional<AbsoluteRect> wrong_character_rect = {};
    bool show_control_rect;

    std::optional<AbsoluteDocumentPos> underline = {};
    std::vector<DocumentRect> overview_highlights;

    int rotation_index = 0;
    bool fastread_mode = false;
    int selected_highlight_index = -1;
    int selected_bookmark_index = -1;

    int last_mouse_down_window_x = 0;
    int last_mouse_down_window_y = 0;

    float last_mouse_down_document_offset_x = 0;
    float last_mouse_down_document_offset_y = 0;
    std::vector<AbsoluteRect> pending_download_portals;

    QDateTime creation_time;

    std::optional<std::function<void(const OpenedBookState&)>> on_link_edit = {};

    std::optional<AbsoluteRect> selected_rectangle = {};

    GLuint LoadShaders(Path vertex_file_path_, Path fragment_file_path_);
protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void set_render_highlight_color(float* color);
    void set_render_highlight_opacity(float opacity);

    void render_highlight_window(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels=-1);
    void render_highlight_absolute(AbsoluteRect absolute_document_rect, int flags);
    void render_line_window(float vertical_pos, std::optional<NormalizedWindowRect> ruler_rect = {});
    void render_highlight_document(DocumentRect doc_rect, int flags=HRF_FILL | HRF_BORDER);
    void paintGL() override;
    void my_render(QPainter* painter);
    void render_scratchpad(QPainter* painter);
    void add_coordinates_for_window_point(DocumentView* dv, float window_x, float window_y, float r, int point_polygon_vertices, std::vector<float>& out_coordinates);
    void render_drawings(DocumentView* dv, const std::vector<FreehandDrawing>& drawings, bool highlighted = false);
    void render_compiled_drawings();
    void render_line(DocumentView* dv, FreehandDrawing drawing);
    std::vector<std::pair<QRect, QString>> get_hint_rect_and_texts();

    void enable_stencil();
    void write_to_stencil();
    void draw_stencil_rects(const std::vector<NormalizedWindowRect>& window_rects);
    void draw_stencil_rects(int page, const std::vector<PagelessDocumentRect>& rects);
    void use_stencil_to_write(bool eq);
    void disable_stencil();

    void render_transparent_background();

    int last_cache_num_drawings = -1;
    float last_cache_offset_x = -1;
    float last_cache_offset_y = -1;
    float last_cache_zoom_level = -1;
    QDateTime last_scratchpad_update_datetime;

    void bind_program(DocumentView::ColorPalette forced_palette=DocumentView::ColorPalette::None);
    void bind_points(const std::vector<float>& points);
    void bind_default();

public:

    bool visible_drawing_mask[26];
    FreehandDrawing current_drawing;
    std::vector<FreehandDrawing> moving_drawings;
    std::vector<PixmapDrawing> moving_pixmaps;

    std::vector<DocumentRect> synctex_highlights;
    QTime synctex_highlight_time;
    std::vector<MarkedDataRect> marked_data_rects;

    PdfViewOpenGLWidget(DocumentView* document_view, PdfRenderer* pdf_renderer, bool is_helper, QWidget* parent = nullptr);
    ~PdfViewOpenGLWidget();

    void handle_escape();

    std::map<int, std::vector<MarkedDataRect>> get_marked_data_rect_map();

    bool valid_document();
    void render_overview(OverviewState overview);
    void render_page(int page_number, bool in_overview=false, DocumentView::ColorPalette forced_palette=DocumentView::ColorPalette::None, bool stencils_allowed=true);
    void set_synctex_highlights(std::vector<DocumentRect> highlights);
    bool should_show_synxtex_highlights();
    bool has_synctex_timed_out();
    void on_document_view_reset();
    void mouseMoveEvent(QMouseEvent* mouse_event) override;
    void mousePressEvent(QMouseEvent* mevent) override;
    void mouseReleaseEvent(QMouseEvent* mevent) override;
    void wheelEvent(QWheelEvent* wevent) override;
    void register_on_link_edit_listener(std::function<void(const OpenedBookState&)> listener);
    void draw_empty_helper_message(QPainter* painter, QString message);
    std::vector<NormalizedWindowRect> get_overview_border_rects();
    Document* doc(bool overview=false);
    DocumentView* dv();

    void rotate_clockwise();
    void rotate_counterclockwise();

    bool is_rotated();
    void toggle_fastread_mode();
    void setup_text_painter(QPainter* painter);
    void get_overview_window_vertices(float out_vertices[2 * 4]);

    void set_selected_rectangle(AbsoluteRect selected);
    void clear_selected_rectangle();
    void clear_all_selections();

    std::optional<AbsoluteRect> get_selected_rectangle();

    void set_typing_rect(DocumentRect rect, std::optional<DocumentRect> wrong_rect);

    Document* get_current_overview_document();

    void get_custom_color_transform_matrix(float matrix_data[16]);
    void get_background_color(float out_background[3]);
    void set_underline(AbsoluteDocumentPos abspos);
    void clear_underline();
    bool is_normalized_y_in_window(float y);
    bool is_normalized_y_range_in_window(float y0, float y1);
    void render_portal_rect(QPainter* painter, AbsoluteRect portal_absolute_rect, bool is_pending);
    void set_pending_download_portals(std::vector<AbsoluteRect>&& portal_rects);
    void get_color_for_current_mode(const float* input_color, float* output_color);
    void render_ui_icon_for_current_color_mode(QPainter* painter, const QIcon& icon_black, const QIcon& icon_white, QRect rect, bool is_highlighted=false);
    void render_text_highlights();
    void render_highlight_annotations();
    std::array<float, 3> cc3(const float* input_color);
    std::array<float, 4> cc4(const float* input_color);
    QColor qcc3(const float* input_color);
    QColor qcc4(const float* input_color);
    void set_overview_highlights(const std::vector<DocumentRect>& rects);
    bool needs_stencil_buffer();
    void draw_overview_background();
    void draw_overview_border();

    void render_selected_rectangle();
    void set_scratchpad(ScratchPad* pad);
    ScratchPad* get_scratchpad();
    bool can_use_cached_scratchpad_framebuffer();
    void compile_drawings(DocumentView* dv, const std::vector<FreehandDrawing>& drawings);
    void clear_background_color();
    CompiledDrawingData compile_drawings_into_vertex_and_index_buffers(const std::vector<float>& line_coordinates,
        const std::vector<unsigned int>& indices,
        const std::vector<GLint>& line_type_indices,
        const std::vector<float>& dot_coordinates,
        const std::vector<unsigned int>& dot_indices,
        const std::vector<GLint>& dot_type_indices);
    void set_tag_prefix(std::wstring prefix);
    void clear_tag_prefix();
    void set_selected_highlight_index(int index);
    void set_selected_bookmark_index(int index);
    void set_highlighted_tags(std::vector<std::string> tags);
    bool is_tag_highlighted(const std::string& tag);
    void set_pending_portal_position(std::optional<AbsoluteRect> rect);
};
