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

#ifndef SIOYEK_QT6
#include <qdesktopwidget.h>
#endif


#include <qpainter.h>

#include "coordinates.h"
#include "book.h"
#include "path.h"
#include "document_view.h"
#include "pdf_renderer.h"

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
    HRF_INVERTED = 1 << 4,
    HRF_PAINTOVER = 1 << 5
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



#ifdef SIOYEK_OPENGL_BACKEND
class PdfViewOpenGLWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
#else
class PdfViewOpenGLWidget : public QWidget{
#endif
//class PdfViewOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_1 {
public:


private:

#ifdef SIOYEK_OPENGL_BACKEND
    OpenGLSharedResources shared_gl_objects;
    GLuint vertex_array_object;
#endif
    QPainter painter;

    DocumentView* document_view = nullptr;
    ScratchPad* scratchpad = nullptr;
    PdfRenderer* pdf_renderer = nullptr;

    std::unique_ptr<CachedScratchpadPixmapData> cached_scratchpad_pixmap = {};
    bool is_helper = false;

    QIcon bookmark_icon;
    QIcon portal_icon;
    QIcon bookmark_icon_white;
    QIcon portal_icon_white;
    QIcon hourglass_icon;

    int last_mouse_down_window_x = 0;
    int last_mouse_down_window_y = 0;
    float last_mouse_down_document_offset_x = 0;
    float last_mouse_down_document_offset_y = 0;
    //bool is_latex_initialized = false;

    std::optional<std::function<void(const OpenedBookState&)>> on_link_edit = {};


protected:
#ifdef SIOYEK_OPENGL_BACKEND
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    GLuint LoadShaders(Path vertex_file_path_, Path fragment_file_path_);
    void render_line_window_opengl_backend(float vertical_pos, std::optional<NormalizedWindowRect> ruler_rect = {});
    void render_highlight_window_opengl_backend(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels=-1);
    void compile_drawings_opengl_backend(DocumentView* dv, const std::vector<FreehandDrawing>& drawings);
    void render_overview_opengl_backend(NormalizedWindowRect window_rect, OverviewState overview);
    CompiledDrawingData compile_drawings_into_vertex_and_index_buffers(const std::vector<float>& line_coordinates,
        const std::vector<unsigned int>& indices,
        const std::vector<GLint>& line_type_indices,
        const std::vector<float>& dot_coordinates,
        const std::vector<unsigned int>& dot_indices,
        const std::vector<GLint>& dot_type_indices);
#else
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void render_highlight_window_qpainter_backend(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels=-1);
    void render_overview_qpainter_backend(NormalizedWindowRect window_rect, OverviewState overview);
#endif


    void clear_background_buffers(float r, float g, float b, GLuint buffer_flags);
    void begin_native_painting();
    void end_native_painting();
    void render_texture(SioyekTextureType texture, NormalizedWindowRect rect, ColorPalette palette);
    void render_page_separator();

    void render_highlight_window(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels=-1);
    void render_highlight_absolute(AbsoluteRect absolute_document_rect, int flags);
    void render_line_window(float vertical_pos, std::optional<NormalizedWindowRect> ruler_rect = {});
    void render_highlight_document(DocumentRect doc_rect, int flags=HRF_FILL | HRF_BORDER);
    void my_render();
    void render_scratchpad();
    void add_coordinates_for_window_point(DocumentView* dv, float window_x, float window_y, float r, int point_polygon_vertices, std::vector<float>& out_coordinates);
    void render_drawings(QPainter* p, DocumentView* dv, const std::vector<FreehandDrawing>& drawings, bool highlighted = false);
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

    void bind_program(ColorPalette forced_palette=ColorPalette::None);
    void bind_points(const std::vector<float>& points);
    void bind_default();
    void draw_pixmap(QRect rect, QPixmap* pixmap);
    void fill_rect(QRect rect, const QColor& color);
    void draw_icon(const QIcon& icon, QRect rect);
    void prepare_line_drawing_pipeline();
    void prepare_highlight_pipeline();
    void prepare_non_compiled_line_drawing_pipeline();
    void enable_multisampling();
    void disable_multisampling();
    void set_highlight_color(float* color, float alpha);
    void draw_pending_freehand_drawings(const std::vector<int>& visible_pages);
    void render_highlights_and_bookmarks();
    void do_paint();

    void render_ruler_thresholds();
    void prepare_initial_render_pipeline();
    void prepare_link_highlight_state();
public:
    std::vector<OverviewState> persisted_overviews;
    bool is_helper_waiting_for_render = false;


    PdfViewOpenGLWidget(DocumentView* document_view, PdfRenderer* pdf_renderer, bool is_helper, QWidget* parent = nullptr);
    ~PdfViewOpenGLWidget();

    void handle_escape();

    bool valid_document();
    void render_overview(OverviewState overview);
    void render_page(int page_number, std::optional<OverviewState> overview = {}, ColorPalette forced_palette = ColorPalette::None, bool stencils_allowed = true);
    void mouseMoveEvent(QMouseEvent* mouse_event) override;
    void mousePressEvent(QMouseEvent* mevent) override;
    void mouseReleaseEvent(QMouseEvent* mevent) override;
    void wheelEvent(QWheelEvent* wevent) override;
    void register_on_link_edit_listener(std::function<void(const OpenedBookState&)> listener);
    void draw_empty_helper_message(QString message);
    std::vector<NormalizedWindowRect> get_overview_border_rects();
    Document* doc(std::optional<OverviewState> overview = {});
    DocumentView* dv();

    void setup_text_painter();
    void get_overview_window_vertices(float out_vertices[2 * 4], std::optional<OverviewState> maybe_overview = {});

    void clear_all_selections();
    Document* get_current_overview_document();

    void get_background_color(float out_background[3]);
    bool is_normalized_y_in_window(float y);
    bool is_normalized_y_range_in_window(float y0, float y1);
    void render_portal_rect(AbsoluteRect portal_absolute_rect, bool is_pending, std::optional<float> completion_ratio);
    void get_color_for_current_mode(const float* input_color, float* output_color);
    void render_ui_icon_for_current_color_mode(const QIcon& icon_black, const QIcon& icon_white, QRect rect, bool is_highlighted=false);
    void render_text_highlights();
    void render_highlight_annotations();
    std::array<float, 3> cc3(const float* input_color);
    std::array<float, 4> cc4(const float* input_color);
    QColor qcc3(const float* input_color);
    QColor qcc4(const float* input_color);
    bool needs_stencil_buffer();
    void draw_overview_background(std::optional<OverviewState> maybe_overview = {});
    void draw_overview_border(std::optional<OverviewState> maybe_overview = {});
    //void draw_markdown_text(QString text, QRect window_rect, const QFont& font);

    void render_selected_rectangle();
    void set_scratchpad(ScratchPad* pad);
    ScratchPad* get_scratchpad();
    bool can_use_cached_scratchpad_framebuffer();
    void compile_drawings(DocumentView* dv, const std::vector<FreehandDrawing>& drawings);
    void clear_background_color();
    ColorPalette get_actual_color_palette(ColorPalette forced_color_palette);
    const QPainter& get_painter();
    //void initialize_latex();
};
