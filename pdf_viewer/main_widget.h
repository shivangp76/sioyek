#pragma once

#ifndef MAIN_WIDGET_DEFINE
#define MAIN_WIDGET_DEFINE

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_set>

#include <qnetworkaccessmanager.h>
#include <qquickwidget.h>
#include <qjsondocument.h>
#include <qmainwindow.h>
#include <qfilesystemwatcher.h>
#include <qlayout.h>

#ifdef SIOYEK_IOS
#include <QApplicationStateChangeEvent>
#endif

#include "book.h"
#include "input.h"
#include "path.h"
#include "background_tasks.h"


class SelectionIndicator;
class QLocalSocket;
class QLineEdit;
class QTextEdit;
class QTimer;
class QPushButton;
class QDragEvent;
class QDropEvent;
class QScrollBar;
class QTextToSpeech;
class QStringListModel;
class QLabel;
class TouchTextSelectionButtons;
class DrawControlsUI;
class SearchButtons;
class HighlightButtons;
class SioyekNetworkManager;
class QMediaPlayer;
class QTemporaryFile;
class QProcess;
class QTemporaryFile;

struct fz_context;
struct fz_stext_char;

class DocumentView;
class ScratchPad;
class Document;
class InputHandler;
class ConfigManager;
class PdfRenderer;
class CachedChecksummer;
class CommandManager;
class Command;
class SioyekRendererBackend;
class DatabaseManager;
class DocumentManager;
class TextToSpeechHandler;
class SioyekBookmarkTextBrowser;

enum class DrawingMode {
    Drawing,
    PenDrawing,
    None
};

struct WindowFollowLastState {
    float offset_x;
    float offset_y;
    float zoom_level;
    int pos_x;
    int pos_y;
    int width;
    int height;
};

bool operator==(const WindowFollowLastState& lhs, const WindowFollowLastState& rhs);

struct WindowFollowData{
    AbsoluteRect rect;
    qint64 pid;
    std::string bookmark_uuid = "";
    QTemporaryFile* file = nullptr;
    bool updating_text_editor = false;
    std::optional<WindowFollowLastState> last_state={};
    QDateTime creation_time;
};



struct RecentlyUpdatedPortalState {
    std::string uuid;
    QDateTime last_modification_time;
};

struct StatusMessage {
    QString message;
    QDateTime datetime;
    QString id;
};

#ifdef SIOYEK_IOS
struct AVSpeechSynthesizer;
#endif



//struct LastDocumentChecksum {
//    Document* doc = nullptr;
//    std::optional<std::string> checksum;
//};

//struct StatusString {
//    //std::unordered_map<QString>
//    QString actual_status_string;
//    QString role_string;
//
//};

struct DeletedObject {
    std::string document_checksum;
    std::variant<BookMark, Portal, Highlight> object;
};

struct HighQualityPlayState {
    bool is_playing = false;
    int page_number = -1;
    int start_line = -1;
    std::vector<PagelessDocumentRect> line_rects;
    std::vector<float> timestamps;
    Document* doc = nullptr;
    std::optional<PagelessDocumentRect> last_focused_rect = {};
};

struct ShellOutputBookmark{
    std::string uuid;
    qint64 pid;
    QTemporaryFile* output_file;
    QTemporaryFile* document_content_file = nullptr;
    QTemporaryFile* image_file = nullptr;
    QFileSystemWatcher* watcher;
    QProcess* process;
    QString style_string = "";
};

struct ClickSpaceTime {
    QDateTime click_time;
    AbsoluteDocumentPos click_pos;
};

#ifdef SIOYEK_ADVANCED_AUDIO
using SioyekMediaPlayer = MyPlayer;
#else
using SioyekMediaPlayer = QMediaPlayer;
#endif

// if we inherit from QWidget there are problems on high refresh rate smartphone displays
#ifdef SIOYEK_MOBILE
class MainWidget : public QQuickWidget {
#else
class MainWidget : public QMainWindow {
#endif
    Q_OBJECT
public:
    fz_context* mupdf_context = nullptr;
    DatabaseManager* db_manager = nullptr;
    DocumentManager* document_manager = nullptr;
    CommandManager* command_manager = nullptr;
    ConfigManager* config_manager = nullptr;
    SioyekNetworkManager* sioyek_network_manager = nullptr;
    BackgroundTaskManager* background_task_manager = nullptr;
    BackgroundBookmarkRenderer* background_bookmark_renderer = nullptr;
    PdfRenderer* pdf_renderer = nullptr;
    InputHandler* input_handler = nullptr;
    CachedChecksummer* checksummer = nullptr;
    QWidget* central_widget = nullptr;
    QMenuBar* menu_bar = nullptr;
    int window_id;

    //LastDocumentChecksum last_document_checksum;

    QFileSystemWatcher external_command_edit_watcher;
    bool is_external_file_edited = false;

    TextToSpeechHandler* tts = nullptr;
    // is the TTS engine currently reading text?
    bool is_reading = false;
    bool word_by_word_reading = false;
    QString prev_tts_state = "";
    bool tts_has_pause_resume_capability = false;
    bool tts_is_about_to_finish = false;
    std::wstring tts_text = L"";
    std::vector<PagelessDocumentRect> tts_corresponding_line_rects;
    std::vector<PagelessDocumentRect> tts_corresponding_char_rects;
    std::optional<PagelessDocumentRect> last_focused_rect = {};

    SioyekRendererBackend* opengl_widget = nullptr;
    SioyekRendererBackend* helper_opengl_widget_ = nullptr;

    QScrollBar* scroll_bar = nullptr;
    SioyekMediaPlayer* media_player = nullptr;

    QJsonDocument sioyek_documentation_json_document;

    // Some commands can not be executed immediately (e.g. because they require a text or symbol
    // input to be completed) this is where they are stored until they can be executed.
    std::unique_ptr<Command> pending_command_instance = nullptr;
    std::vector<Command*> commands_being_performed;
    std::unique_ptr<Command> last_performed_command = nullptr;

    std::string last_performed_command_name = "";
    int last_performed_command_num_repeats = 0;

    std::vector<int> last_status_string_ids;
    std::wstring last_titlebar_string = L"";

    std::vector<ShellOutputBookmark> shell_output_bookmarks;

    std::optional<std::function<std::pair<QString, std::vector<int>>()>> left_status_string_generator = {};
    std::optional<std::function<std::pair<QString, std::vector<int>>()>> right_status_string_generator = {};
    std::optional<std::function<std::pair<QString, std::vector<int>>()>> titlebar_generator = {};

    std::vector<DeletedObject> deleted_objects;

    DocumentView* main_document_view = nullptr;
    ScratchPad* scratchpad = nullptr;
    DocumentView* helper_document_view_ = nullptr;

    // A stack of widgets to be displayed (e.g. the bookmark menu or the touch main menu).
    // only the top widget is visible. When a widget is popped, the previous widget in the stack
    // will be shown
    std::vector<QWidget*> current_widget_stack;

    // code to execute when a command is pressed (for example when we type `goto_beginning` in the command)
    // menu, we will call on_command_done("goto_beginning"). The reason that this is a closure instead of just a
    // method is historical. I am too lazy to change it.
    std::function<void(std::string, std::string, bool)> on_command_done = nullptr;

    // List of previous locations in the current session. Note that we keep the history even across files
    // hence why `DocumentViewState` has a `document_path` member
    std::vector<DocumentViewState> history;
    // the index in the `history` array that we will jump to when `prev_state` is called.
    int current_history_index = -1;

    // custom message to be displayed in sioyek's statusbar
    //std::wstring custom_status_message = L"";
    std::vector<StatusMessage> status_messages;
    int current_status_message_index = 0;

    // A flag which indicates whether the application should quit. We use this to inform other threads
    // (e.g. the PDF rendering thread) that they should exit.
    bool* should_quit = nullptr;

    // if the last event before handling mouse release event was a hold gesture
    bool was_hold_gesture = false;

    // last position when mouse was clicked in absolute document space
    AbsoluteDocumentPos last_mouse_down;
    // The document offset (offset_x and offset_y) when mouse was last pressed
    // we use this to update the offset when dragging the mouse in some modes
    // for example in touch mode or when dragging while holding middle mouse button
    //AbsoluteDocumentPos last_mouse_down_document_offset;
    VirtualPos last_mouse_down_document_virtual_offset;

    // last window position when mouse was clicked, we use this in mouse drag mode
    WindowPos last_mouse_down_window_pos;

    // whether we are in rect/point select mode (some commands require a rectangle to be executed
    // for example `delete_freehand_drawings`)
    bool rect_select_mode = false;
    bool point_select_mode = false;

    std::optional<HighQualityPlayState> high_quality_play_state = {};

    // begin/end of current selected rectangle
    std::optional<AbsoluteDocumentPos> rect_select_begin = {};
    std::optional<AbsoluteDocumentPos> rect_select_end = {};


    // when set, mouse wheel moves the ruler
    bool visual_scroll_mode = false;
    bool debug_mode = false;

    bool horizontal_scroll_locked = false;

    // in select highlight mode, we immediately highlight the text when it is selected
    // with highlight type of `select_highlight_type` 
    bool is_select_highlight_mode = false;
    char select_highlight_type = 'a';

    // in smooth scroll mode we scroll the document smoothly instead of jumping to the target
    // `smooth_scroll_speed` is used to keep track of our speed in this mode
    //bool smooth_scroll_mode = false;

    //float smooth_scroll_speed = 0.0f;

    // the timer which periodically checks if the UI/rendering needs updating. Normally the timer value is
    // set to be INTERVAL_TIME (which is 200ms at the time of writing this comment), however, it is set to a much
    // lower value when in smooth scroll mode or when user flicks a document in touch mode
    QTimer* validation_interval_timer = nullptr;
    QTimer* network_timer = nullptr;
    QDateTime last_persistance_datetime;

    std::deque<ClickSpaceTime> recent_clicks;

    // the portal to be edited. This is usually set by `edit_portal` command which jumps to the portal
    // when we go back to the original location by jumping back in history, the portal will be edited
    // to be the new document view state
    std::optional<Portal> portal_to_edit = {};

    // the current freehand drawing mode. None means we are not drawing anything
    // Drawing means we use the mouse to draw a freehand diagram
    // and PenDrawing means we assume the user is using a pen so we treat mouse inputs
    // normally and only use pen inputs for freehand drawing
    DrawingMode freehand_drawing_mode = DrawingMode::None;

    // In mouse drag mode, we use the mouse to drag the document around
    bool mouse_drag_mode = false;

    // In synctex mode right clicking on a document will jump to the corresponding latex file
    // (assuming `inverse_search_command` is properly configured)
    bool synctex_mode = false;

    // are we currently dragging the document
    bool is_dragging = false;
    bool is_dragging_snapped = false;

    float current_drag_min_annotation_x = -1;
    float current_drag_max_annotation_x = -1;

    // are we performing pinch to zoom gesture
    bool is_pinching = false;

    // should we show the status label?
    // If touch mode is enabled, we don't show the status label at all, unless there is another window
    // (e.g. the main menu or search buttons) is visible. That is because the screen space is already
    // very limited in touch devices and we don't want to waste it using a statusbar unless it is absolutely required
    //bool should_show_status_label_ = true;

    bool should_show_status_label(bool check_network=true);

    // the location of current character in sioyek's typing minigame
    std::optional<CharacterAddress> typing_location;

    int main_window_width = 0;
    int main_window_height = 0;

    QMap<QString, QVariant> js_variables;

    QVBoxLayout* layout;
    QWidget* text_command_line_edit_container = nullptr;
    QLabel* text_command_line_edit_label = nullptr;
    QLineEdit* text_command_line_edit = nullptr;
    QLineEdit* status_label_left = nullptr;
    QLabel* status_label_right = nullptr;
    QWidget* status_label = nullptr;
    QPushButton* server_actions_button = nullptr;
    QPushButton* resume_to_server_position_button = nullptr;
    int text_suggestion_index = 0;

    std::vector<WindowFollowData> following_windows;

    std::deque<std::wstring> search_terms;

    // determines if the widget render is invalid and needs to be updated
    // when `validation_interval_timer` fired, we check if this is true
    // and only then we re-render the widget
    // note that this is different from the document being invalid (an example of document
    // being invalid could be a latex document that is changed since being loaded). This just means that
    // the current drawing of MainWidget is not correct (for example due to moving vertically)
    bool is_render_invalidated = false;

    // determines if the UI is invalid and needs to be updated
    // this can be the case for example when updating the search progress
    // note that when render is invalid we automatically validate the UI anyway
    bool is_ui_invalidated = false;

    // this is a niche option to show the last executed command in the status bar
    // (I used it when recording sioyek's video tutorial to show the command being executed)
    bool should_show_last_command = false;

    bool close_event_already_handled = false;

    // the command used to perform synctex inverse searches
    std::wstring inverse_search_command;

    // data used to resize/move the overview. which is a small window which shows the target
    // destination of references/links
    std::optional<OverviewMoveData> overview_move_data = {};
    std::optional<OverviewTouchMoveData> overview_touch_move_data = {};
    std::optional<OverviewResizeData> overview_resize_data = {};


    // when selecting text, we update the rendering faster, this timer is used 
    // so that we don't update the rendering too fast
    QTime last_text_select_time = QTime::currentTime();


    // last time we updated `smooth_scroll_speed` 
    QTime last_speed_update_time = QTime::currentTime();

    std::optional<ServerStatus> last_server_status = {};

    QJSEngine* sync_js_engine = nullptr;
    std::vector<QJSEngine*> available_async_engines;
    std::mutex available_engine_mutex;
    int num_js_engines = 0;

    bool main_document_view_has_document();
    std::optional<std::string> get_last_opened_file_checksum();


    int get_current_colorscheme_index();
    void set_dark_mode();
    void set_light_mode();
    void set_custom_color_mode();
    void set_color_mode_to_system_theme();
    void toggle_statusbar();
    void toggle_titlebar();

    // search the `paper_name` in one of the configurable when middle-click or shift+middle-clicking on paper's name
    void handle_search_paper_name(QString paper_name, bool is_shift_pressed);
    Q_INVOKABLE void handle_app_tts_resume(bool is_playing, bool is_on_rest, int offset);

    void persist(bool persist_drawings = false);
    std::wstring get_status_string(bool is_right);
    std::wstring get_title_string();
    void handle_escape();
    bool is_waiting_for_symbol();
    void key_event(bool released, QKeyEvent* kevent, bool is_auto_repeat = false);
    void handle_left_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_command_pressed, bool is_alt_pressed);

    void handle_right_click(WindowPos click_pos, bool down, bool is_shift_pressed, bool is_control_pressed, bool is_command_pressed, bool is_alt_pressed);
    void on_config_changed(std::string config_name, bool should_save=false);
    void on_configs_changed(std::vector<std::string>* config_names);

    void on_overview_move_end();
    void next_state();
    void prev_state();
    void update_current_history_index();
    void handle_periodic_network_operations();

    void set_main_document_view_state(DocumentViewState new_view_state);
    void handle_click(WindowPos pos);
    void manage_last_document_checksum();
    void on_checksum_computed();

    void update_following_windows();
    void handle_validation_interval_timeout();
    void update_selected_bookmark_font_size();
    //bool eventFilter(QObject* obj, QEvent* event) override;
    void set_command_textbox_text(const std::wstring& txt);
    void change_selected_highlight_type(char new_type);
    void change_selected_bookmark_text(const std::wstring& new_text);
    void change_selected_highlight_text_annot(const std::wstring& new_text);
    char get_current_selected_highlight_type();
    void show_textbar(const std::wstring& command_name, bool is_password, const std::wstring& initial_value = L"");
    void show_mark_selector();
    void toggle_two_window_mode();
    void toggle_window_configuration();
    void handle_portal();
    void start_creating_rect_portal(AbsoluteDocumentPos location);
    void add_portal(std::wstring source_path, Portal new_link);
    void toggle_fullscreen();
    void set_synctex_mode(bool mode);
    void toggle_synctex_mode();
    void complete_pending_link(const PortalViewState& destination_view_state);
    void long_jump_to_destination(DocumentPos pos);
    void long_jump_to_destination(int page, float offset_y);
    void long_jump_to_destination(float abs_offset_y);
    void execute_command(std::wstring command, std::wstring text = L"", bool wait = false);
    //QString get_status_stylesheet();
    void smart_jump_under_pos(WindowPos pos);
    void open_overview_to_portal(Document* document, Portal portal);
    bool overview_under_pos(WindowPos pos);
    void visual_mark_under_pos(WindowPos pos);
    bool is_network_manager_running(bool* is_downloading = nullptr, std::wstring* message=nullptr);
    QString get_network_status_string();
    void show_download_paper_menu(
        const std::vector<std::wstring>& paper_names,
        const std::vector<std::wstring>& download_urls,
        std::wstring paper_name,
        PaperDownloadFinishedAction action);

    QRect get_main_window_rect();
    QRect get_helper_window_rect();

    void show_password_prompt_if_required();
    void handle_link_click(const PdfLink& link);

    void on_next_text_suggestion();
    void on_prev_text_suggestion();
    void set_current_text_suggestion();
    void set_drag_value_on_small_documents(fvec2& val);
    void toggle_menu_collapse();

    std::wstring get_window_configuration_string();
    std::wstring get_serialized_configuration_string();
    void save_auto_config();

    void handle_close_event(bool is_quiting=false);
    void return_to_last_visual_mark();
    void reload(bool flush = true);
    QNetworkReply* download_paper_with_url(std::wstring paper_url, bool use_archive_url, PaperDownloadFinishedAction action);

    void reset_highlight_links();
    void set_rect_select_mode(bool mode);
    void set_point_select_mode(bool mode);
    void clear_selected_rect();
    void toggle_pdf_annotations();
    void on_paper_downloaded(QNetworkReply* reply);

    std::optional<ShellOutputBookmark> get_shell_output_bookmark_with_uuid(std::string uuid);
    void remove_finished_shell_bookmark_with_index(int index);
    void remove_finished_shell_bookmarks();
    void handle_bookmark_shell_command(QString bookmark_text, std::string uuid, QString text_arg="");
    void on_bookmark_shell_output_updated(std::string bookmark_uuid, QString file_path);

    Document* doc();

    MainWidget(
        fz_context* mupdf_context,
        PdfRenderer* pdf_renderer,
        DatabaseManager* db_manager,
        DocumentManager* document_manager,
        ConfigManager* config_manager,
        CommandManager* command_manager,
        InputHandler* input_handler,
        CachedChecksummer* checksummer,
        SioyekNetworkManager* sioyek_network_manager,
        BackgroundTaskManager* task_manger,
        BackgroundBookmarkRenderer* bookmark_renderer,
        bool* should_quit_ptr,
        QWidget* parent = nullptr
    );
    MainWidget(MainWidget* other);

    ~MainWidget();

    //void handle_command(NewCommand* command, int num_repeats);
    bool handle_command_types(std::unique_ptr<Command> command, int num_repeats, std::wstring* result = nullptr);
    void handle_pending_text_command(std::wstring text);

    void invalidate_render();
    void invalidate_ui();
    void open_document(const Path& path, std::optional<float> offset_x = {}, std::optional<float> offset_y = {}, std::optional<float> zoom_level = {}, std::string downloaded_checksum="");
    void open_document_with_hash(const std::string& hash, std::optional<float> offset_x = {}, std::optional<float> offset_y = {}, std::optional<float> zoom_level = {});
    void open_document_at_location(const Path& path, int page, std::optional<float> x_loc, std::optional<float> y_loc, std::optional<float> zoom_level, bool should_push_state=true);
    void open_document(const DocumentViewState& state);
    void open_document(const PortalViewState& checksum);
    void validate_render();
    void validate_ui();
    void zoom(WindowPos pos, float zoom_factor, bool zoom_in);
    // void focus_text(int page, const std::wstring& text);
    // int get_page_intersecting_rect_index(DocumentRect rect);
    // std::optional<AbsoluteRect> get_page_intersecting_rect(DocumentRect rect);
    // void focus_rect(DocumentRect rect);
    void move_ruler_next();
    void move_ruler_prev();

    void start_dragging();
    void stop_dragging();

    AbsoluteRect move_visual_mark(int offset);
    void on_config_file_changed(ConfigManager* new_config);
    void toggle_mouse_drag_mode();
    void toggle_freehand_drawing_mode();
    void exit_freehand_drawing_mode();
    void toggle_pen_drawing_mode();
    void set_pen_drawing_mode(bool enabled);
    void set_hand_drawing_mode(bool enabled);
    void handle_drawing_ui_visibilty();

    void toggle_dark_mode();
    void toggle_custom_color_mode();
    void do_synctex_forward_search(const Path& pdf_file_path, const Path& latex_file_path, int line, int column);
    //void handle_args(const QStringList &arguments);
    void update_link_with_opened_book_state(Portal lnk, const OpenedBookState& new_state, bool async=false);
    // void schedule_update_link_with_opened_book_state(Portal lnk, const OpenedBookState& new_state);
    void update_closest_link_with_opened_book_state(const OpenedBookState& new_state);
    void set_current_widget(QWidget* new_widget);
    void push_current_widget(QWidget* new_widget, bool hide_previous=true);
    bool pop_current_widget(bool canceled = false);
    void show_current_widget();
    // bool focus_on_visual_mark_pos(bool moving_down);
    void toggle_visual_scroll_mode();
    //void set_overview_link(PdfLink link);
    //void set_overview_position(
    //    int page,
    //    float offset,
    //    std::optional<std::string> overview_type,
    //    std::optional<std::vector<DocumentRect>> overview_highlights = {}
    //);

    //ReferenceType find_location_of_selected_text(int* out_page, float* out_offset, AbsoluteRect* out_rect, std::wstring* out_source_text, std::vector<DocumentRect>* out_highlight_rects = nullptr);
    //TextUnderPointerInfo find_location_of_text_under_pointer(DocumentPos docpos, bool update_candidates = false);
    std::optional<std::wstring> get_current_file_name();
    CommandManager* get_command_manager();

    void move_vertical(float amount);
    bool move_horizontal(float amount, bool force = false);
    void get_window_params_for_one_window_mode(int* main_window_size, int* main_window_move);
    void get_window_params_for_two_window_mode(int* main_window_size, int* main_window_move, int* helper_window_size, int* helper_window_move);
    void apply_window_params_for_one_window_mode(bool force_resize = false);
    void apply_window_params_for_two_window_mode();
    bool helper_window_overlaps_main_window();
    void upload_drawings(bool wait_for_send = false);
    void perform_sync_operations_when_document_is_closed(bool wait_for_send, bool sync_drawings);

    // get rects using tags (tags are strings shown when executing `keyboard_*` commands)
    bool is_rotated();
    void on_new_paper_added(const std::wstring& file_path);
    int get_current_page_number() const;
    std::wstring get_current_page_label();
    void goto_page_with_page_number(int page_number);
    void goto_page_with_label(std::wstring label);
    void set_inverse_search_command(const std::wstring& new_command);
    int get_current_monitor_width(); int get_current_monitor_height();
    std::wstring synctex_under_pos(WindowPos position);
    std::optional<PaperNameWithRects> get_paper_name_under_cursor(bool use_last_hold_point = false);
    QString set_status_message(std::wstring new_status_string, QString id="");
    void remove_self_from_windows();
    // void handle_keyboard_select(const std::wstring& text);
    void push_state(bool update = true);
    void toggle_scrollbar();
    void update_scrollbar();
    void goto_overview();
    void set_mark_in_current_location(char symbol);
    void goto_mark(char symbol);
    void advance_command(std::unique_ptr<Command> command, std::wstring* result = nullptr);
    void add_search_term(const std::wstring& term);
    void perform_search(std::wstring text, bool is_regex = false, bool is_incremental = false);
    void overview_to_definition();
    void portal_to_definition();
    void move_visual_mark_command(int amount);
    void handle_goto_loaded_document();

    void handle_goto_portal_list();
    void handle_goto_bookmark(bool manual_only=false, bool chat=false);
    void handle_goto_bookmark_global(bool manual_only=false);
    std::wstring handle_add_highlight(char symbol);
    void handle_goto_highlight();
    void handle_goto_highlight_global();
    void handle_goto_toc();
    void handle_open_prev_doc();
    void handle_open_all_docs();
    MainWidget* handle_new_window();
    void handle_open_link(const PdfLink& link, bool copy = false);
    void handle_find_references_to_link(const std::wstring& text);
    void handle_find_references_to_selected_text();
    void handle_keys_user_all();
    void handle_prefs_user_all();
    void handle_portal_to_overview();
    void handle_goto_window();
    void handle_toggle_smooth_scroll_mode();
    void handle_overview_to_portal();
    void handle_toggle_typing_mode();
    void handle_delete_highlight_under_cursor();
    std::optional<Highlight> handle_delete_selected_highlight();
    std::optional<BookMark> handle_delete_selected_bookmark();
    std::optional<Portal> handle_delete_selected_portal();
    void handle_start_reading();
    void preload_next_page_for_tts(float rate);
    void set_high_quality_tts_rate(float rate);
    bool is_high_quality_tts_playing();
    void handle_start_reading_high_quality(bool should_preload=false);
    void handle_toggle_reading();
    void handle_stop_reading();
    void handle_play();
    void handle_undo_drawing();
    void handle_pause();
    void handle_semantic_search(const std::wstring& query, bool has_tried_already=false);
    void handle_semantic_search_extractive(const std::wstring& query, bool has_tried_already=false);
    std::wstring handle_freetext_bookmark_perform(const std::wstring& text, const std::string& pending_uuid);
    void handle_special_bookmarks(std::wstring bookmark_text, std::wstring bookmark_uuid);
    void handle_bookmark_ask_query(std::wstring query, std::wstring bookmark_uuid);
    BookMark* add_chunk_to_bookmark(Document* document, std::string bookmark_uuid, QString chunk);
    void handle_bookmark_summarize_query(std::wstring bookmark_uuid);
    void read_current_line();
    void download_paper_under_cursor(bool use_last_touch_pos = false);
    //std::optional<QString> get_direct_paper_name_under_pos(DocumentPos docpos);
    //std::optional<QString> get_paper_name_under_pos(DocumentPos docpos, bool clean = false);
    void handle_debug_command();
    void handle_fulltext_search(std::wstring maybe_file_checksum=L"", std::wstring tag=L"");
    void handle_documentation_search();
    void handle_add_marked_data();
    void handle_undo_marked_data();
    void handle_remove_marked_data();
    void handle_export_marked_data();
    void handle_goto_random_page();
    void hande_turn_on_all_drawings();
    void hande_turn_off_all_drawings();
    void handle_toggle_drawing_mask(char symbol);
    void show_command_palette();
    void auto_login();
    void index_current_document_for_fulltext_search(bool async=false,  std::wstring tag=L"");
    void on_super_fast_search_index_computed();
    bool is_current_document_fulltext_indexed();
    void handle_delete_document_from_fulltext_search_index();
    void scroll_selected_bookmark(int amount);
    void scroll_bookmark_with_uuid(const std::string& uuid, int amount);
    bool ensure_super_fast_search_index();
    void show_items(std::vector<std::wstring> items,
        std::optional<std::function<void(std::wstring)>> on_select = {},
        std::optional<std::function<void(std::wstring)>> on_delete = {});

    std::string get_current_mode_string();

    void show_audio_buttons();
    void set_freehand_thickness(float val);

    // void handle_goto_next_block();
    // void handle_goto_prev_block();

    // Text selection indicators in touch mode
    SelectionIndicator* selection_begin_indicator = nullptr;
    SelectionIndicator* selection_end_indicator = nullptr;

    // When in touch mode, sometimes we use the last touch hold point for some commands
    // for example, if select text button is pressed, we select the text under the last touch hold point
    QPoint last_hold_point;
    QPoint last_press_point;
    qint64 last_press_msecs = 0;
    QTime last_quick_tap_time;
    QTime last_middle_down_time;
    QPoint last_quick_tap_position;
    std::optional<QPoint> context_menu_right_click_pos = {};

    std::optional<QString> add_bookmark_hook_function_name = {};
    std::optional<QString> delete_bookmark_hook_function_name = {};
    std::optional<QString> edit_bookmark_hook_function_name = {};
    std::optional<QString> add_highlight_hook_function_name = {};
    std::optional<QString> delete_highlight_hook_function_name = {};
    std::optional<QString> highlight_annotation_changed_hook_function_name = {};
    std::optional<QString> highlight_type_changed_hook_function_name = {};
    std::optional<QString> add_mark_hook_function_name = {};
    std::optional<QString> add_portal_hook_function_name = {};
    std::optional<QString> delete_portal_hook_function_name = {};
    std::optional<QString> edit_portal_hook_function_name = {};
    std::optional<QString> open_document_hook_function_name = {};
    std::optional<QString> open_new_document_hook_function_name = {};
    std::optional<std::string> download_checksum_when_ready = {};

    // list of pairs of [checksum, highlight text], when the super fast index of document with checksum
    // is computed, we should highlight the text in the document
    std::vector<std::pair<std::string, QString>> highlight_text_when_super_fast_index_is_ready;

    std::optional<RecentlyUpdatedPortalState> recently_updated_portal = {};

    // whether mouse is pressed, `is_pressed` is true, we add mouse positions to `position_buffer`
    bool is_pressed = false;
    // list of mouse positions used to calculate the velocity when flicking in touch mode
    std::deque<std::pair<QTime, QPoint>> position_buffer;

    // indicates if mouse was in next/prev ruler rect in touch mode
    // if this is the case, we use mouse movement to perform next/prev ruler command
    // after a certain threshold, so the user doesn't have to click on the ruler rect 
    bool was_last_mouse_down_in_ruler_next_rect = false;
    bool was_last_mouse_down_in_ruler_prev_rect = false;
    // this is used so we can keep track of mouse movement after press and holding on ruler rect
    WindowPos ruler_moving_last_window_pos;
    int ruler_moving_distance_traveled = 0;
    std::optional<Portal> last_dispplayed_portal = {};

    void set_recently_updated_portal(const std::string& uuid);
    void update_highlight_buttons_position();
    void start_mobile_selection_under_point(AbsoluteDocumentPos point);
    void update_mobile_selection();
    bool handle_quick_tap(WindowPos click_pos);
    bool handle_double_tap(QPoint pos);
    void android_handle_visual_mode();
    void show_highlight_buttons();
    //void clear_highlight_buttons();
    void show_search_buttons();
    void clear_search_buttons();
    void clear_selection_indicators();
    void update_position_buffer();
    bool is_flicking(QPointF* out_velocity);
    void handle_touch_highlight();
    void restore_default_config();
    bool is_in_back_rect(WindowPos pos);
    bool is_in_middle_left_rect(WindowPos pos);
    bool is_in_middle_right_rect(WindowPos pos);
    bool is_in_forward_rect(WindowPos pos);
    bool is_in_edit_portal_rect(WindowPos pos);
    bool is_in_visual_mark_next_rect(WindowPos pos);
    bool is_in_visual_mark_prev_rect(WindowPos pos);
    void handle_pen_drawing_event(QTabletEvent* te);
    void select_freehand_drawings(AbsoluteRect rect);
    void delete_freehand_drawings(AbsoluteRect rect);
    void handle_toggle_text_mark();

    void handle_highlight_text_in_document(std::string document_checksum, QString text_to_highlight);

    void shrink_selection_end();
    void shrink_selection_begin();

    void persist_config();

    void synchronize_pending_link();
    void refresh_all_windows();
    std::optional<PdfLink> get_selected_link(const std::wstring& text);

    void set_brightness(float brightness);
    int num_visible_links();
#ifdef SIOYEK_ANDROID
    //    void onApplicationStateChanged(Qt::ApplicationState applicationState);
    bool pending_intents_checked = false;
    Q_INVOKABLE void on_android_pause();
    Q_INVOKABLE void on_android_resume();
#endif

protected:
    TouchTextSelectionButtons* text_selection_buttons_ = nullptr;
    DrawControlsUI* draw_controls_ = nullptr;
    SearchButtons* search_buttons_ = nullptr;
    //HighlightButtons* highlight_buttons_ = nullptr;
    bool middle_click_hold_command_already_executed = false;

    TouchTextSelectionButtons* get_text_selection_buttons();
    DrawControlsUI* get_draw_controls();
    SearchButtons* get_search_buttons();
    HighlightButtons* get_highlight_buttons();



    void focusInEvent(QFocusEvent* ev);
    void resizeEvent(QResizeEvent* resize_event) override;
    void changeEvent(QEvent* event) override;
#ifdef SIOYEK_IOS
    std::optional<Qt::ApplicationState> last_ios_application_state = {};
    int ios_tts_begin_index_into_document = -1;
    bool ios_was_suspended = false;

    void on_ios_application_state_changed(Qt::ApplicationState state);
    void on_ios_suspend_while_reading();
    void on_ios_resume();
#endif

    // bool handle_visible_object_resize_mouse_move(AbsoluteDocumentPos abs_mpos);
    // bool handle_visible_object_scroll_mouse_move(AbsoluteDocumentPos abs_mpos);
    bool handle_visible_object_cursor_update(AbsoluteDocumentPos abs_mpos);
    void mouseMoveEvent(QMouseEvent* mouse_event) override;

    // we already handle drag and drop on macos elsewhere
// #ifndef Q_OS_MACOS
    void dragEnterEvent(QDragEnterEvent* e) override;
    void dragMoveEvent(QDragMoveEvent* e) override;
    void dropEvent(QDropEvent* event) override;
// #endif

    void closeEvent(QCloseEvent* close_event) override;
    void keyPressEvent(QKeyEvent* kevent) override;
    void keyReleaseEvent(QKeyEvent* kevent) override;
    void mouseReleaseEvent(QMouseEvent* mevent) override;
    void mousePressEvent(QMouseEvent* mevent) override;
    void mouseDoubleClickEvent(QMouseEvent* mevent) override;
    void wheelEvent(QWheelEvent* wevent) override;
    bool event(QEvent* event);
    bool is_mouse_cursor_in_overview();
    bool is_mouse_cursor_in_statusbar();
    std::wstring get_status_part_name_under_cursor();

public:

    void handle_rename(std::wstring new_name);
    bool execute_macro_if_enabled(std::wstring macro_command_string, QLocalSocket* result_socket=nullptr);
    bool execute_macro_from_origin(std::wstring macro_command_string, QLocalSocket* origin);
    bool ensure_internet_permission();
    void handle_command_text_change(const QString& new_text);
    TextToSpeechHandler* get_tts();
    void update_bookmark_with_uuid(const std::string& uuid);
    void update_portal_with_uuid(const std::string& uuid);
    void handle_bookmark_move_finish();
    void handle_portal_move_finish();
    void handle_portal_move();

    void handle_visible_object_move();
    void handle_bookmark_move();

    bool is_middle_click_being_used();
    // void begin_bookmark_move(const std::string& uuid, AbsoluteDocumentPos begin_cursor_pos);
    // void begin_portal_move(const std::string& uuid, AbsoluteDocumentPos begin_cursor_pos, bool is_pending);
    bool should_drag();
    // void move_selected_drawings(AbsoluteDocumentPos new_pos, std::vector<FreehandDrawing>& moved_drawings, std::vector<PixmapDrawing>& moved_pixmaps);
    bool goto_ith_next_overview(int i);
    void on_overview_source_updated();

    void open_document(const std::wstring& doc_path, bool load_prev_state = true, std::optional<OpenedBookState> prev_state = {}, bool foce_load_dimensions = false);

    AbsoluteDocumentPos get_cursor_abspos();
    void cleanup_expired_pending_portals();
    //void update_pending_portal_indices_after_removed_indices(std::vector<int>& removed_indices);
    void close_overview();
    void handle_overview_to_ruler_portal();
    void handle_goto_ruler_portal(std::string tag="");
    void show_touch_buttons(
        std::vector<std::wstring> buttons,
        std::vector<std::wstring> tips,
        std::function<void(int, std::wstring)> on_select, bool top=true);
    void download_and_portal(std::wstring unclean_paper_name, QString full_bib_text, AbsoluteDocumentPos source_pos);
    void download_selected_text();
    void smart_jump_to_selected_text();
    void show_text_prompt(std::wstring initial_value, std::function<void(std::wstring)> on_select);
    void show_touch_buttons_for_overview_type(std::string type);
    void update_touch_overview_buttons(const std::optional<OverviewState>& overview);
    void set_overview_page(std::optional<OverviewState> overview, bool should_update_buttons);
    void export_python_api();
    QJSEngine* take_js_engine(bool async);
    void release_async_js_engine(QJSEngine* engine);

    QJSValue export_javascript_api(QJSEngine& engine, bool is_async);
    void show_custom_option_list(std::vector<std::wstring> option_list);
    void on_socket_deleted(QLocalSocket* deleted_socket);
    Q_INVOKABLE QJsonObject get_json_state();
    Q_INVOKABLE void set_state(QJsonObject state);
    Q_INVOKABLE QJsonObject get_json_annotations();
    Q_INVOKABLE void goto_offset(float x_offset, float y_offset);
    Q_INVOKABLE QJsonObject absolute_to_window_rect_json(QJsonObject absolute_rect_json);

    Q_INVOKABLE void screenshot_js(QString file_path, QJsonObject window_rect);
    Q_INVOKABLE void framebuffer_screenshot_js(QString file_path, QJsonObject window_rect);

    Q_INVOKABLE QString perform_network_request_with_headers(QString method, QString url, QJsonObject headers, QJsonObject request, bool* is_done=nullptr, QByteArray* response=nullptr);
    Q_INVOKABLE void copy_text_to_clipboard(QString str);

    void screenshot(std::wstring file_path);
    void framebuffer_screenshot(std::wstring file_path);

    QJsonArray get_all_json_states();
    void export_command_names(std::wstring file_path);
    void export_config_names(std::wstring file_path);
    void export_default_config_file(std::wstring file_path);
    void print_undocumented_commands();
    void print_documented_but_removed_commands();
    void print_undocumented_configs();
    void print_non_default_configs();
    //void advance_wait_for_render_if_ready();
    bool is_render_ready();
    bool is_search_ready();
    bool is_index_ready();
    void advance_waiting_command(std::string waiting_command_name);
    void handle_select_current_search_match();
    void handle_select_ruler_text();
    void handle_stop_search();
    int get_window_id();
    void add_command_being_performed(Command* new_command);
    void remove_command_being_performed(Command* new_command);
    void load_sioyek_documentation();
    QString get_command_documentation(QString command_name, bool is_config, QString* out_url, QString* out_file_name);
    QString get_command_documentation_with_title(QString command_name);
    QString get_config_documentation_with_title(QString config, QString command_name);
    void show_command_documentation(QString command_name, bool is_config);
    void show_documentation_with_title(QString doctype, QString title);
    void show_markdown_text_widget(QString url, QString text);
    void open_documentation_file_for_name(QString doctype, QString name);
    QString get_related_command_and_configs_string(QJsonArray related_commands, QJsonArray related_configs);
    float get_align_to_top_offset();

    QString handle_action_in_menu(std::wstring action);
    std::wstring handle_synctex_to_ruler();
    // void focus_on_line_with_index(int page, int index);
    void show_touch_main_menu();
    void show_touch_page_select();
    void show_touch_highlight_type_select();
    void show_touch_settings_menu();
    void free_document(Document* doc);
    bool is_helper_visible();
    std::wstring get_current_tabs_file_names();
    void open_tabs(const std::vector<std::wstring>& tabs);
    void get_document_views_referencing_doc(std::wstring doc_path, std::vector<DocumentView*>& document_views, std::vector<MainWidget*>& corresponding_widgets, std::vector<bool>& is_helper);
    void restore_document_view_states(const std::vector<DocumentView*>& document_views, const std::vector<DocumentViewState>& states);
    void document_views_open_path(const std::vector<DocumentView*>& document_views, const std::vector<MainWidget*>& main_widgets, const std::vector<bool> is_helpers, std::wstring new_path);
    void update_renamed_document_in_history(std::wstring old_path, std::wstring new_path);
    void maximize_window();
    void toggle_rect_hints();
    void run_command_with_name(std::string command_name, bool should_pop_current_widget=false);
    QStringListModel* get_new_command_list_model();
    void add_password(std::wstring path, std::string password);
    int current_document_page_count();
    void goto_search_result(int nth_next_result, bool overview=false);
    void set_should_highlight_words(bool should_highlight_words);
    void toggle_highlight_links();
    void set_highlight_links(bool should_highlight);
    void rotate_clockwise();
    void rotate_counterclockwise();
    void toggle_fastread();
    void export_json(std::wstring json_file_path);
    void import_json(std::wstring json_file_path);
    bool does_current_widget_consume_quicktap_event();
    SioyekRendererBackend* helper_opengl_widget();
    DocumentView* helper_document_view();
    void initialize_helper();
    void hide_command_line_edit();
    void deselect_document_indices();
    Q_INVOKABLE MainWidget* get_widget_with_id(int window_id);
    Q_INVOKABLE QString run_macro_on_main_thread(QString macro_string, QStringList args, bool wait_for_result=true, int target_window_id=-1);
    Q_INVOKABLE QByteArray perform_network_request(QString url, QString method = "get", QString json_data = "");
    Q_INVOKABLE QString read_file(QString path, bool encode_base_64=false);
    Q_INVOKABLE void execute_macro_and_return_result(QString macro_string, bool* is_done, std::wstring* result, std::optional<QStringList> args = {});
    Q_INVOKABLE QString execute_macro_sync(QString macro, QStringList args = {});
    Q_INVOKABLE void set_variable(QString name, QVariant var);
    Q_INVOKABLE QVariant get_variable(QString name);
    Q_INVOKABLE void register_function_keybind(QString keybind, QString name, QString command_name, QString file_name, int line_number);
    Q_INVOKABLE void register_hook_function(QString type, QString name);
    Q_INVOKABLE void register_function_keybind_async(QString keybind, QString code, QString command_name, QString file_name, int line_number);
    Q_INVOKABLE void report_js_error(QString error_message, QString error_file_path, int error_line);
    Q_INVOKABLE void register_string_keybind(QString keybind, QString name, QString file_name, int line_number);
    void run_startup_js(bool first_run=false);
    Q_INVOKABLE QString get_environment_variable(QString name);
    void run_javascript_command(std::wstring javascript_code, std::optional<std::wstring> entry_point, bool is_async);
    void set_text_prompt_text(QString text);
    DocumentView* dv();
    bool should_draw(bool originated_from_pen);
    bool is_scratchpad_mode();
    void toggle_scratchpad_mode();
    void add_pixmap_to_scratchpad(QPixmap pixmap);
    void save_scratchpad();
    void load_scratchpad();
    void clear_scratchpad();
    void show_draw_controls();
    PaperDownloadFinishedAction get_default_paper_download_finish_action();
    void set_tag_prefix(std::wstring prefix);
    void clear_tag_prefix();
    bool show_contextual_context_menu(QString default_context_menu="");
    void show_context_menu(QString menu="");
    QMenu* get_menu_from_items(std::unique_ptr<MenuItems> items, QWidget* parent);
    void show_recursive_context_menu(std::unique_ptr<MenuItems> items);
    QPoint cursor_pos();
    void clear_current_page_drawings();
    void clear_current_document_drawings();
    void free_renderer_resources_for_current_document();

    // void set_selected_highlight_uuid(std::string uuid);
    // void set_selected_bookmark_uuid(std::string uuid);
    // void set_selected_portal_uuid(std::string uuid, bool is_pinned=false);
    // void clear_selected_object();

    // std::string get_selected_highlight_uuid();
    // std::string get_selected_portal_uuid();
    // std::string get_selected_bookmark_uuid();

    // void handle_generic_tags_pre_perform(const std::vector<VisibleObjectIndex>& visible_objects);
    // void handle_highlight_tags_pre_perform(const std::vector<std::string>& visible_highlight_uuids);
    // void clear_keyboard_select_highlights();
    void handle_goto_link_with_page_and_offset(int page, float y_offset);
    std::optional<std::wstring> get_search_suggestion_with_index(int index);
    bool is_menu_focused();
    void ensure_player_state_(QString state);
    Q_INVOKABLE void ensure_player_state(QString state);
    QString get_rest_of_document_pages_text();
    void focus_on_character_offset_into_document(int character_offset_into_document);
    // void stop_tts_service();
    //void handle_move_smooth_press(bool down);
    void handle_move_smooth_hold(bool down);
    void handle_toggle_two_page_mode();
    void ensure_zero_interval_timer();
    void set_last_performed_command(std::unique_ptr<Command> command);
    void make_current_menu_columns_equal();
    void set_highlighted_tags(std::vector<std::string> tags);
    AbsoluteDocumentPos get_mouse_abspos();
    bool handle_annotation_move_finish();
    void set_fixed_velocity(float vel);
    QMenuBar* create_main_menu_bar();
    void create_menu_from_menu_node(QMenu* parent, MenuNode* items, std::unordered_map<std::string, std::vector<std::string>>& command_key_mappings);
    void delete_menu_nodes(MenuNode* items);
    // void set_pending_portal(std::optional<std::wstring> doc_path, Portal portal);
    // void set_pending_portal(std::optional<std::pair<std::optional<std::wstring>, Portal>> pending_portal);
    bool is_ruler_mode();
    void open_external_text_editor();
    void start_embedded_external_editor(WindowFollowData& follow_data, QString content);
    void open_embedded_external_text_editor();
    void handle_text_edit_return_pressed();
    void call_async_js_function_with_args(const QString& code, QJsonArray args);
    void call_js_function_with_bookmark_arg_with_uuid(const QString& function_name, const std::string& uuid);
    void call_js_function_with_portal_arg_with_uuid(const QString& function_name, const std::string& uuid);
    void call_js_function_with_highlight_arg_with_uuid(const QString& function_name, const std::string& uuid);
    void on_new_bookmark_added(const std::string& uuid);
    void on_bookmark_deleted(const BookMark& bookmark, const std::string& document_checksum);
    void on_bookmark_edited(const std::string& uuid);
    void on_new_highlight_added(const std::string& uuid);
    void on_highlight_deleted(const Highlight& highlight, const std::string& document_checksum);
    void on_mark_added(const std::string& uuid, char type);
    void on_highlight_annotation_edited(const std::string& uuid);
    void on_highlight_type_edited(const std::string& uuid);
    void delete_highlight_with_uuid(const std::string& uuid);
    std::optional<Highlight> delete_current_document_highlight_with_uuid(const std::string& uuid);
    void delete_current_document_highlight(Highlight* hl);
    void on_new_portal_added(const std::string& uuid);
    void on_portal_deleted(const Portal& uuid, const std::string& document_checksum);
    void on_portal_edited(const std::string& uuid);
    void on_open_document(const std::wstring& path);
    void sync_edited_annot(const std::string& annot_type, const std::string& uuid);
    void sync_deleted_annot(const std::string& annot_type, const std::string& uuid);
    std::vector<OpenedBookInfo> get_all_opened_books(bool include_server_books=true, bool force_full_path=false);

    void handle_sync_open_document();

    void handle_server_document_location_mismatch(float local_offset_y, float server_offset_y);

    std::string add_highlight_to_current_document(AbsoluteDocumentPos selection_begin, AbsoluteDocumentPos selection_end, char type);


    void update_highlight_annot_with_uuid(const std::string& uuid, const std::wstring& new_annot);
    std::optional<BookMark> delete_current_document_bookmark(const std::string& uuid);
    void delete_current_document_bookmark_with_bookmark(BookMark* bm);
    void delete_global_bookmark(const std::string& uuid);
    void download_and_portal_to_highlighted_overview_paper();
    void upload_current_file();
    void ocr_current_file();
    void update_current_document_checksum(std::string checksum);
    bool is_current_document_available_on_server();
    void handle_login(std::wstring email, std::wstring password);
    void sync_current_file_location_to_servers(bool wait_for_send=false);
    void sync_document_location_to_servers(Document* doc, float offset_y, bool wait_for_send=false);
    void handle_open_server_only_file();
    void download_and_open(std::string checksum, float offset_y);
    void handle_resume_to_server_location();
    void handle_server_actions_button_pressed();
    void sync_annotations_with_server();
    void sync_newly_added_annot(const std::string& annot_type, const std::string& uuid);
    void delete_current_file_from_server();
    bool is_logged_in();
    void on_set_enum_config_value(std::string config_name, std::wstring config_value);
    void focus_on_high_quality_text_being_read();
    void on_document_changed();
    void do_synchronize();
    void synchronize_if_desynchronized();
    void on_server_hashes_loaded();
    Q_INVOKABLE void update_checksum_impl(std::string old_checksum, std::string new_checksum);
    SioyekMediaPlayer* get_media_player();
    void handle_high_quality_media_end_reached();
    void show_command_menu();

    bool import_local_database(std::wstring path);
    bool import_shared_database(std::wstring path);
    void on_paper_download_begin(QNetworkReply* reply, std::string pending_portal_handle);
    QNetworkReply* download_paper_with_name(std::wstring name, QString full_bib_text, std::optional<PaperDownloadFinishedAction> action = {}, std::string pending_portal_handle="");
    void handle_type_text_into_input(QString txt);
    void send_symbol_to_last_command(char symbol);
    void ensure_titlebar_colors_match_color_mode();
    void pin_current_overview_as_portal();
    // void begin_portal_scroll();
    void set_mouse_cursor_for_side_resize(std::optional<OverviewSide> side);
    bool handle_left_press_touch_mode(WindowPos click_pos);
    bool handle_left_release_touch_mode(WindowPos click_pos);
    bool handle_left_click_point_select(AbsoluteDocumentPos abs_doc_pos);
    void handle_rect_selection_point_press(AbsoluteDocumentPos abs_doc_pos);
    void handle_rect_selection_point_release(AbsoluteDocumentPos abs_doc_pos);
    void handle_overview_download_button_click(AbsoluteDocumentPos abs_doc_pos);
    bool handle_overview_click(WindowPos click_pos, AbsoluteDocumentPos abs_doc_pos);
    // bool handle_visible_object_click(WindowPos click_pos, AbsoluteDocumentPos abs_doc_pos, std::optional<VisibleObjectIndex> visible_object);
    bool handle_visible_object_resize_finish();
    void stop_all_threads();
    void restart_all_threads();
    void show_citers_with_paper_name(std::wstring paper_name);
    void show_citers_of_current_paper();

    void push_deleted_object(DeletedObject object);
    void push_deleted_highlight(std::optional<Highlight> hl);
    void push_deleted_bookmark(std::optional<BookMark> bm);
    void push_deleted_portal(std::optional<Portal> portal);

    void undo_delete();
    void set_renderer_backend(RenderBackend backend);
    void delete_old_backend();
    void delete_old_helper();
    std::vector<WindowRect> get_largest_empty_rects();
    void update_text_selection(AbsoluteDocumentPos abs_mpos);
    void scroll_selected_bookmark_to_end();
    void handle_ask();
    void open_selected_bookmark_in_widget(std::string bookmark_uuid="", bool force_chat=false, bool is_bookmark_pending=false);
    bool is_in_bookmark_widget_mode();
    void accept_new_bookmark_message();
    void update_current_bookmark_widget_text(BookMark* bm);
    QString get_selected_text_in_chat_window();
    std::optional<SioyekBookmarkTextBrowser*> get_current_bookmark_browser();

    void handle_scroll_selected_bookmark_to_ends(bool goto_start);
    void handle_edit_selected_bookmark_with_external_editor();


public slots:
#ifdef SIOYEK_IOS
    void handle_ios_files(const QUrl& url);
#endif
    void highlight_type_color_clicked(int index);
    int update_recent_clicks(AbsoluteDocumentPos mouse_abspos);
    void handle_triple_click(AbsoluteDocumentPos mouse_abspos);
    void repeat_last_command();
};

MainWidget* get_window_with_window_id(int window_id);

#endif
