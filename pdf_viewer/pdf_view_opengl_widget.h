#pragma once

#include <vector>
#include <string>
#include <mutex>
#include <optional>
#include <utility>
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
class DocumentManager;

enum HighlightRenderFlags
{
    HRF_FILL = 1 << 0,
    HRF_BORDER = 1 << 1,
    HRF_UNDERLINE = 1 << 2,
    HRF_STRIKE = 1 << 3,
    HRF_INVERTED = 1 << 4,
    HRF_PAINTOVER = 1 << 5,
    HRF_PENDING = 1 << 6
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


class SioyekRendererBackend {
protected:
    // QPainter painter;
    QIcon bookmark_icon;
    QIcon portal_icon;
    QIcon bookmark_icon_white;
    QIcon portal_icon_white;
    QIcon hourglass_icon;
    QIcon download_icon;
    QIcon download_icon_dark;
    DocumentView* document_view = nullptr;
    PdfRenderer* pdf_renderer = nullptr;
    DocumentManager* document_manager = nullptr;
    std::unique_ptr<CachedScratchpadPixmapData> cached_scratchpad_pixmap = {};
    bool is_helper = false;
    int last_mouse_down_window_x = 0;
    int last_mouse_down_window_y = 0;
    float last_mouse_down_document_offset_x = 0;
    float last_mouse_down_document_offset_y = 0;
    std::optional<std::function<void(const OpenedBookState&)>> on_link_edit = {};
    int last_cache_num_drawings = -1;
    float last_cache_offset_x = -1;
    float last_cache_offset_y = -1;
    float last_cache_zoom_level = -1;
    QDateTime last_scratchpad_update_datetime;
    float background_clear_color[3] = {0};
    bool qpainter_initialized_for_current_frame = false;
    // we need to know when we are rendering animations so that the main widget can keep updating the view

    virtual void clear_background_buffers(float r, float g, float b, GLuint buffer_flags) = 0;
    virtual void begin_native_painting() = 0;
    virtual void end_native_painting() = 0;
    virtual void render_texture(std::optional<SioyekTextureType> texture, NormalizedWindowRect rect, ColorPalette palette) = 0;
    virtual void render_highlight_window(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels=-1) = 0;
    void render_highlight_absolute(AbsoluteRect absolute_document_rect, int flags);
    ScratchPad* scratch();
    virtual void render_line_window(float vertical_pos, std::optional<NormalizedWindowRect> ruler_rect = {}) = 0;
    void render_highlight_document(DocumentRect doc_rect, int flags=HRF_FILL | HRF_BORDER);
    void my_render();
    virtual void prepare_initial_render_pipeline() = 0;
    bool valid_document();
    void draw_empty_helper_message(QString message);
    void clear_background_color();
    bool needs_stencil_buffer();
    float get_device_pixel_ratio();
    void render_page(int page_number, std::optional<OverviewState> overview = {}, ColorPalette forced_palette = ColorPalette::NoPalette, bool stencils_allowed = true);
    virtual void prepare_link_highlight_state() = 0;
    virtual void prepare_for_line_drawing() = 0;

    virtual void enable_stencil() = 0;
    virtual void write_to_stencil() = 0;
    virtual void draw_stencil_rects(const std::vector<NormalizedWindowRect>& window_rects) = 0;
    virtual void draw_stencil_rects_with_page(int page, const std::vector<PagelessDocumentRect>& rects);
    virtual void use_stencil_to_write(bool eq) = 0;
    virtual void disable_stencil() = 0;
    virtual void disable_stencil_for_two_page();
    virtual void render_transparent_background() = 0;
    virtual void render_overview_backend(NormalizedWindowRect window_rect, OverviewState overview, bool draw_border=true) = 0;
    virtual void render_overview_highlights(OverviewState overview);
    virtual void set_stencil_for_two_page(int page, PagelessDocumentRect page_content, bool stencils_allowed, float zoom_level) = 0;

    virtual void set_highlight_color(const float* color, float alpha) = 0;
    virtual void prepare_highlight_pipeline() = 0;
    virtual std::optional<GraphicsBackendExtras> get_backend_extras();
    virtual QPainter* get_painter() = 0;
    virtual void render_pending_bookmark_rect(NormalizedWindowRect rect);

    void render_search_result_highlights(const std::vector<int>& visible_pages);
    void initialize_stuff();

    std::array<float, 3> cc3(const float* input_color);
    std::array<float, 4> cc4(const float* input_color);
    QColor qcc3(const float* input_color);
    QColor qcc4(const float* input_color);
    void get_color_for_current_mode(const float* input_color, float* output_color);

    void render_selected_rectangle();
    void render_debug_highlights();
    void draw_pending_freehand_drawings(const std::vector<int>& visible_pages);
    void render_tags();
    std::vector<std::pair<QRect, QString>> get_hint_rect_and_texts();
    void render_portals(const std::vector<int>& visible_pages);
    void render_highlight_annotations(const std::vector<int>& visible_pages);
    void render_bookmark_annotations(const std::vector<int>& visible_pages);
    void render_text_highlights();
    void render_ruler_thresholds();
    void render_ruler();
    virtual void render_ui_icon_for_current_color_mode(const QIcon& icon_black, const QIcon& icon_white, QRect rect, bool is_highlighted=false);
    void fill_rect(QRect rect, const QColor& color);
    ColorPalette get_actual_color_palette(ColorPalette forced_color_palette);
    int get_ruler_display_mode();
    void render_portal_rect(AbsoluteRect portal_absolute_rect, bool is_pending, std::optional<float> completion_ratio);
    void setup_text_painter();
    void get_background_color(float out_background[3]);
    void get_overview_window_vertices(float out_vertices[2 * 4], std::optional<OverviewState> maybe_overview = {});
    std::vector<int> get_overview_visible_pages(const OverviewState& overview);
    void render_scratchpad();
    void add_coordinates_for_window_point(DocumentView* dv, float window_x, float window_y, float r, int point_polygon_vertices, std::vector<float>& out_coordinates);
    void add_coordinates_for_window_point_no_fan(DocumentView* dv, float page_width, float page_height, float window_x, float window_y, float depth, float r, int point_polygon_vertices, std::vector<float>& out_coordinates);

    bool can_use_cached_scratchpad_framebuffer();
    void draw_pixmap(QRect rect, QPixmap* pixmap);

    bool is_normalized_y_in_window(float y);
    bool is_normalized_y_range_in_window(float y0, float y1);

    virtual void render_drawings(QPainter* p, DocumentView* dv, const QList<FreehandDrawing>& drawings, bool highlighted = false);
    virtual void render_page_drawings(QPainter* p, DocumentView* dv, int page, const PageFreehandDrawing& page_drawings, bool highlighted = false);

    void draw_icon(const QIcon& icon, QRect rect);
    void render_overview(OverviewState overview, bool draw_border=true);
    void render_page_separator(int page_number);

    virtual void prepare_non_compiled_line_drawing_pipeline() = 0;
    virtual void render_compiled_drawings() = 0;
    virtual void compile_drawings(DocumentView* dv, const QList<FreehandDrawing>& drawings) = 0;
    virtual void prepare_line_drawing_pipeline() = 0;
    virtual void enable_multisampling() = 0;
    virtual void disable_multisampling() = 0;
    virtual void bind_default() = 0;
    virtual void bind_vertex_array() = 0;
    Qt::ScreenOrientation get_orientation();
    int get_width();
    int get_height();
    virtual void render_original_color_images(int page_number, std::optional<OverviewState> overview, ColorPalette forced_color_palette, bool stencils_allowed) = 0;

    bool handle_mouse_move_event(QMouseEvent* mouse_event);
    bool handle_mouse_press_event(QMouseEvent* mevent);
    bool handle_mouse_release_event(QMouseEvent* mevent);
    bool handle_wheel_event(QWheelEvent* wevent);

public:
    bool is_rendering_animation = false;
    bool is_helper_waiting_for_render = false;

    DocumentView* dv();
    Document* doc(std::optional<OverviewState> overview = {});
    // const QPainter& get_painter();
    bool is_background_dark();
    void register_on_link_edit_listener(std::function<void(const OpenedBookState&)> listener);
    void handle_escape();
    void clear_all_selections();
    virtual bool is_opengl() = 0;
    virtual QWidget* get_widget() = 0;

};

class PdfViewOpenGLWidget : public SioyekRendererBackend, public QOpenGLWidget, protected QOpenGLExtraFunctions {
//class PdfViewOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_1 {
public:


private:

    OpenGLSharedResources shared_gl_objects;
    GLuint vertex_array_object;
    QPainter painter_;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    GLuint LoadShaders(Path vertex_file_path_, Path fragment_file_path_);
    void render_line_window_opengl_backend(float vertical_pos, std::optional<NormalizedWindowRect> ruler_rect = {});
    void render_highlight_window_opengl_backend(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels=-1);
    void compile_drawings_opengl_backend(DocumentView* dv, const QList<FreehandDrawing>& drawings);
    void render_overview_opengl_backend(NormalizedWindowRect window_rect, OverviewState overview, bool draw_border=true);
    void render_overview_backend(NormalizedWindowRect window_rect, OverviewState overview, bool draw_border = true) override;
    CompiledDrawingData compile_drawings_into_vertex_and_index_buffers(const std::vector<float>& line_coordinates,
        const std::vector<unsigned int>& indices,
        const std::vector<GLint>& line_type_indices,
        const std::vector<float>& dot_coordinates,
        const std::vector<unsigned int>& dot_indices,
        const std::vector<GLint>& dot_type_indices);


    void clear_background_buffers(float r, float g, float b, GLuint buffer_flags) override;
    void begin_native_painting() override;
    void end_native_painting() override;
    void render_texture(std::optional<SioyekTextureType> texture, NormalizedWindowRect rect, ColorPalette palette) override;

    void render_highlight_window(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels=-1) override;
    void render_line_window(float vertical_pos, std::optional<NormalizedWindowRect> ruler_rect = {}) override;
    void render_drawings(QPainter* p, DocumentView* dv, const QList<FreehandDrawing>& drawings, bool highlighted = false) override;
    void render_compiled_drawings() override;

    void enable_stencil() override;
    void write_to_stencil() override;
    void draw_stencil_rects(const std::vector<NormalizedWindowRect>& window_rects) override;
    void use_stencil_to_write(bool eq) override;
    void disable_stencil() override;

    void render_transparent_background() override;

    void bind_program(ColorPalette forced_palette=ColorPalette::NoPalette);
    void bind_points(const std::vector<float>& points);
    void bind_default() override;
    void bind_vertex_array() override;
    void prepare_line_drawing_pipeline() override;
    void prepare_highlight_pipeline() override;
    void prepare_non_compiled_line_drawing_pipeline() override;
    void enable_multisampling() override;
    void disable_multisampling() override;
    void set_highlight_color(const float* color, float alpha) override;
    void set_stencil_for_two_page(int page, PagelessDocumentRect page_content, bool stencils_allowed, float zoom_level) override;

    void prepare_for_line_drawing() override;
    //void render_highlights_and_bookmarks();
    void do_paint();

    void prepare_initial_render_pipeline() override;
    void prepare_link_highlight_state() override;
    void render_original_color_images(int page_number, std::optional<OverviewState> overview, ColorPalette forced_color_palette, bool stencils_allowed) override;
    QPainter* get_painter() override;
public:
    //std::vector<OverviewState> persisted_overviews;


    PdfViewOpenGLWidget(DocumentView* document_view, PdfRenderer* pdf_renderer, DocumentManager* docman, bool is_helper, QWidget* parent = nullptr);
    ~PdfViewOpenGLWidget();

    void mouseMoveEvent(QMouseEvent* mouse_event) override;
    void mousePressEvent(QMouseEvent* mevent) override;
    void mouseReleaseEvent(QMouseEvent* mevent) override;
    void wheelEvent(QWheelEvent* wevent) override;

    void draw_overview_background(std::optional<OverviewState> maybe_overview = {});
    void draw_overview_border(std::optional<OverviewState> maybe_overview = {}, float* color=nullptr);

    void compile_drawings(DocumentView* dv, const QList<FreehandDrawing>& drawings) override;
    bool is_opengl() override;
    QWidget* get_widget() override;
};

class PdfViewQPainterWidget : public QWidget, public SioyekRendererBackend{

protected:
    QPainter painter_;

    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void render_highlight_window_qpainter_backend(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels=-1);
    void render_overview_qpainter_backend(NormalizedWindowRect window_rect, OverviewState overview, bool draw_border=true);


    void clear_background_buffers(float r, float g, float b, GLuint buffer_flags) override;
    void begin_native_painting() override;
    void end_native_painting() override;
    void render_texture(std::optional<SioyekTextureType> texture, NormalizedWindowRect rect, ColorPalette palette) override;

    void render_highlight_window(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels=-1) override;
    void render_line_window(float vertical_pos, std::optional<NormalizedWindowRect> ruler_rect = {}) override;
    // void render_drawings(QPainter* p, DocumentView* dv, const std::vector<FreehandDrawing>& drawings, bool highlighted = false) override;
    void render_compiled_drawings() override;

    void enable_stencil() override;
    void write_to_stencil() override;
    void draw_stencil_rects(const std::vector<NormalizedWindowRect>& window_rects) override;
    void use_stencil_to_write(bool eq) override;
    void disable_stencil() override;

    void render_transparent_background() override;

    void bind_program(ColorPalette forced_palette=ColorPalette::NoPalette);
    void bind_points(const std::vector<float>& points);
    void bind_default() override;
    void bind_vertex_array() override;
    void prepare_line_drawing_pipeline() override;
    void prepare_highlight_pipeline() override;
    void prepare_non_compiled_line_drawing_pipeline() override;
    void enable_multisampling() override;
    void disable_multisampling() override;
    void set_highlight_color(const float* color, float alpha) override;

    void prepare_for_line_drawing() override;
    //void render_highlights_and_bookmarks();
    void do_paint();

    void prepare_initial_render_pipeline() override;
    void prepare_link_highlight_state() override;
    void render_overview_backend(NormalizedWindowRect window_rect, OverviewState overview, bool draw_border = true) override;
    void set_stencil_for_two_page(int page, PagelessDocumentRect page_content, bool stencils_allowed, float zoom_level) override;
    void render_original_color_images(int page_number, std::optional<OverviewState> overview, ColorPalette forced_color_palette, bool stencils_allowed) override;
    QPainter* get_painter() override;
public:
    bool is_opengl() override;
    //std::vector<OverviewState> persisted_overviews;


    PdfViewQPainterWidget(DocumentView* document_view, PdfRenderer* pdf_renderer, DocumentManager* docman, bool is_helper, QWidget* parent = nullptr);
    ~PdfViewQPainterWidget();

    void mouseMoveEvent(QMouseEvent* mouse_event) override;
    void mousePressEvent(QMouseEvent* mevent) override;
    void mouseReleaseEvent(QMouseEvent* mevent) override;
    void wheelEvent(QWheelEvent* wevent) override;

    void draw_overview_background(std::optional<OverviewState> maybe_overview = {});
    void draw_overview_border(std::optional<OverviewState> maybe_overview = {}, float* color=nullptr);

    void compile_drawings(DocumentView* dv, const QList<FreehandDrawing>& drawings) override;
    QWidget* get_widget() override;
};

#include <QRhiWidget>
#include <rhi/qrhi.h>


QShader get_rhi_shader(const QString &file_path);

struct SioyekTextureShaderResourceBinding;

struct SioyekTextureRenderCall{
    SioyekTextureType texture;
    NormalizedWindowRect rect;
    std::optional<OverviewState> overview;
    int render_order;
    bool is_icon = false;
    QRhiShaderResourceBindings* bindings=nullptr;
    std::optional<NormalizedWindowRect> scissor_rect = {};
};

struct SioyekHighlightRectRenderCall{
    NormalizedWindowRect rect;
    float color[4] = {0};
    bool in_overview = false;
    int flags;
    int render_order;
    bool is_pending_bookmark = false;
};

struct SioyekDrawingRenderCall{
    DocumentView* dv;
    int page = -1;
    QList<FreehandDrawing> drawings;
    bool highlighted = false;
    int render_order;
};

struct SioyekTextureShaderResourceBinding{
    QRhiTexture* texture;
    QRhiShaderResourceBindings* shader_resource_binding;
    QDateTime last_acess_time;
    QRhiBuffer* uniform_buffer;
    bool in_use_in_current_frame=false;
};

struct SioyekPageDrawingsShaderResources{

    Document* doc = nullptr;
    int page = -1;
    QDateTime last_update_time;
    QDateTime last_use_time;

    std::unique_ptr<QRhiBuffer> uniform_buffer;
    std::unique_ptr<QRhiBuffer> positions;
    std::unique_ptr<QRhiBuffer> colors;
    int num_vertices = 0;

    std::unique_ptr<QRhiShaderResourceBindings> shader_resource_binding;
};

class PdfViewRhiWidget : public QRhiWidget, public SioyekRendererBackend{
private:
    QRhi* rhi_ptr = nullptr;
    QPainter painter_;

    const int sample_count = 4;
    QDateTime last_frame_time;
    std::unique_ptr<QRhiBuffer> vertex_buffer_ptr;
    std::unique_ptr<QRhiBuffer> pending_drawings_vertex_buffer_ptr;
    std::unique_ptr<QRhiBuffer> pending_drawing_vertex_colors_ptr;
    std::unique_ptr<QRhiBuffer> pending_drawing_uniform_buffer;
    // std::unique_ptr<QRhiBuffer> drawings_index_buffer_ptr;
    std::unique_ptr<QRhiBuffer> highlights_vertex_buffer_ptr;
    std::unique_ptr<QRhiBuffer> highlights_borders_vertex_buffer_ptr;
    std::unique_ptr<QRhiBuffer> uv_buffer_ptr;
    QImage qpainter_image;
    std::unique_ptr<QRhiTexture> qpainter_texture;
    std::unique_ptr<QRhiBuffer> qpainter_uniform_buffer;
    std::unique_ptr<QRhiBuffer> qpainter_vertex_buffer;
    std::unique_ptr<QRhiShaderResourceBindings> qpainter_resource_binding;

    std::unique_ptr<QRhiShaderResourceBindings> pending_drawings_resource_binding;

    std::vector<std::pair<const QIcon*, std::unique_ptr<QRhiTexture>>> cached_icon_textures;
    QRhiTexture* get_texture_for_icon(const QIcon* icon);

    std::vector<SioyekPageDrawingsShaderResources> cached_page_drawing_shader_resources;

    std::unique_ptr<QRhiGraphicsPipeline> colored_rect_pipeline;
    std::unique_ptr<QRhiGraphicsPipeline> highlight_pipeline;
    std::unique_ptr<QRhiGraphicsPipeline> inverted_highlight_pipeline;
    std::unique_ptr<QRhiGraphicsPipeline> paintover_highlight_pipeline;
    std::unique_ptr<QRhiGraphicsPipeline> highlight_borders_pipeline;
    std::unique_ptr<QRhiGraphicsPipeline> drawing_pipeline;
    // std::unique_ptr<QRhiGraphicsPipeline> test_rect_pipeline;
    // std::unique_ptr<QRhiShaderResourceBindings> test_resource_bindings;

    std::vector<std::unique_ptr<QRhiBuffer>> preallocated_highlight_uniform_buffers;
    std::vector<std::unique_ptr<QRhiShaderResourceBindings>> preallocated_highlight_resource_bindings;

    // std::unique_ptr<QRhiTexture> my_texture;
    std::unique_ptr<QRhiSampler> nearest_sampler;
    std::unique_ptr<QRhiSampler> linear_sampler;

    // float m_rotation = 0.0f;
    float current_highlight_color[4];
    QRhiCommandBuffer* current_frame_command_buffer = nullptr;
    QRhiResourceUpdateBatch* current_frame_resource_update_batch = nullptr;
    std::optional<OverviewState> current_overview  = {};
    int current_object_render_order = 0;
    int current_frame_pending_drawing_vertices = 0;
    int current_frame_overview_object_index = -1;
    std::optional<NormalizedWindowRect> current_stencil = {};
    // int num_frame_drawing_triangles = 0;

    std::vector<SioyekTextureRenderCall> current_frame_texture_render_calls;
    std::vector<SioyekHighlightRectRenderCall> current_frame_highlight_rect_render_calls;
    std::vector<SioyekDrawingRenderCall> current_frame_drawing_render_calls;

    // int current_texture_index = 0;
    std::vector<SioyekTextureShaderResourceBinding> texture_shader_resource_bindings;
    void update_resources_for_current_frame_texture_render_calls(QRhiResourceUpdateBatch* update_batch);
    void update_resources_for_current_frame_highlight_render_calls(QRhiResourceUpdateBatch* update_batch);
    void update_resources_for_current_frame_drawing_calls(QRhiResourceUpdateBatch* update_batch);

    int update_resources_for_single_freehand_drawing(
        QRhiResourceUpdateBatch* update_batch,
        DocumentView* dv,
        int page,
        const QList<FreehandDrawing>& drawings,
        QRhiBuffer* vertex_buffer,
        QRhiBuffer* color_buffer,
        int append_index,
        int prev_num_vertices
    );

    SioyekPageDrawingsShaderResources* get_shader_resources_for_page_drawings(int page, const PageFreehandDrawing& page_drawings);

    void render_qpainter_texture(QRhiCommandBuffer* command_buffer);
    void render_current_frame_textures(QRhiCommandBuffer* command_buffer);
    void render_current_frame_highlights(QRhiCommandBuffer* command_buffer, bool overview);
    void render_current_frame_drawings(QRhiCommandBuffer* command_buffer);

    SioyekTextureShaderResourceBinding* get_shader_resource_binding_for_texture(QRhiTexture* texture, bool is_icon);
    void delete_old_texture_shader_resource_bindings();

    QPainter* get_painter() override;
    void set_overview_scissor(QRhiCommandBuffer* command_buffer, std::optional<OverviewState> overview);
    void reset_overview_scissor(QRhiCommandBuffer* command_buffer);
public:

    PdfViewRhiWidget(DocumentView* document_view, PdfRenderer* pdf_renderer, DocumentManager* docman, bool is_helper, QWidget* parent = nullptr);
    void initialize(QRhiCommandBuffer* command_buffer) override;
    void render(QRhiCommandBuffer* command_buffer) override;
    // void test_render(QRhiCommandBuffer* command_buffer);


    void clear_background_buffers(float r, float g, float b, GLuint buffer_flags) override;
    void begin_native_painting() override;
    void end_native_painting() override;
    void render_texture(std::optional<SioyekTextureType> texture, NormalizedWindowRect rect, ColorPalette palette) override;
    void render_highlight_window(NormalizedWindowRect window_rect, int flags, int line_width_in_pixels=-1) override;
    void render_line_window(float vertical_pos, std::optional<NormalizedWindowRect> ruler_rect = {}) override;
    void prepare_initial_render_pipeline() override;
    void prepare_link_highlight_state() override;
    void prepare_for_line_drawing() override;
    void enable_stencil() override;
    void write_to_stencil() override;
    void draw_stencil_rects(const std::vector<NormalizedWindowRect>& window_rects) override;
    void use_stencil_to_write(bool eq) override;
    void disable_stencil() override;
    void render_transparent_background() override;
    void render_overview_backend(NormalizedWindowRect window_rect, OverviewState overview, bool draw_border=true) override;
    void set_stencil_for_two_page(int page, PagelessDocumentRect page_content, bool stencils_allowed, float zoom_level) override;
    void set_highlight_color(const float* color, float alpha) override;
    void prepare_highlight_pipeline() override;
    void render_drawings(QPainter* p, DocumentView* dv, const QList<FreehandDrawing>& drawings, bool highlighted = false) override;
    void render_page_drawings(QPainter* p, DocumentView* dv, int page, const PageFreehandDrawing& drawings, bool highlighted = false) override;
    void render_page_drawings_impl(QRhiBuffer* uniform_buffer, DocumentView* dv, int page, const QList<FreehandDrawing>& drawings, bool highlighted=false);
    void prepare_non_compiled_line_drawing_pipeline() override;
    void render_compiled_drawings() override;
    void compile_drawings(DocumentView* dv, const QList<FreehandDrawing>& drawings) override;
    void prepare_line_drawing_pipeline() override;
    void enable_multisampling() override;
    void disable_multisampling() override;
    void bind_default() override;
    void bind_vertex_array() override;
    void render_original_color_images(int page_number, std::optional<OverviewState> overview, ColorPalette forced_color_palette, bool stencils_allowed) override;
    bool is_opengl() override;
    bool update_dynamic_buffer_safe(QRhiResourceUpdateBatch* update_batch, QRhiBuffer* buffer, int offset, int size, float* data, int buffer_size);
    void render_ui_icon_for_current_color_mode(const QIcon& icon_black, const QIcon& icon_white, QRect rect, bool is_highlighted=false) override;
    void disable_stencil_for_two_page() override;
    void set_scissor_rect(QRhiCommandBuffer* command_buffer, NormalizedWindowRect scissor_rect);
    QWidget* get_widget() override;
    std::optional<GraphicsBackendExtras> get_backend_extras() override;
    void resizeEvent(QResizeEvent* event) override;
};
