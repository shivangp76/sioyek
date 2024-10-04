#pragma once

#include <vector>
#include <deque>
#include <functional>
#include <shared_mutex>
#include <condition_variable>
#include <qobject.h>
#include <qdatetime.h>
#include <atomic>
#include <unordered_map>
#include <qthread.h>
#include <qrect.h>
#include <qstring.h>

#include "book.h"

class QTextDocument;
class QFont;

class MyThread : public QThread {
    Q_OBJECT
public:
    bool* should_stop;
    std::mutex* pending_tasks_mutex;
    std::condition_variable* pending_taks_cv;
    std::deque<std::pair<QObject*, std::function<void()>>>* pending_tasks;
    QObject** current_task_parent;

    MyThread(
        bool* should_stop_,
        std::mutex* pending_task_mutex_,
        std::condition_variable* pending_task_cv_,
        std::deque<std::pair<QObject*, std::function<void()>>>* pending_tasks_,
        QObject** current_task_parent_);

    void run() override;
};

class BackgroundTaskManager {
private:
    bool is_worker_thread_started = false;
    //std::thread worker_thread;
    std::unique_ptr<MyThread> worker_thread = nullptr;
    bool should_stop = false;

    std::mutex pending_tasks_mutex;
    std::condition_variable pending_taks_cv;
    std::deque<std::pair<QObject*, std::function<void()>>> pending_tasks;
    QObject* current_task_parent = nullptr;
public:

    void start_worker_thread();
    void stop_worker_thread();
    void add_task(std::function<void()>&& fn, QObject* parent);
    void delete_tasks_with_parent(QObject* parent);
    ~BackgroundTaskManager();

};

struct RenderedBookmark {
    BookMark bookmark;
    float zoom_level;
    float pixel_ratio;
    float scroll_amount;
    ColorPalette color_palette = ColorPalette::Normal;
    QPixmap* pixmap = nullptr;
    QDateTime last_access_time;
    bool canceled = false;
    int request_id;
};


class BackgroundBookmarkRenderer : public QObject{
    Q_OBJECT
private:
    std::shared_mutex rendered_bookmarks_mutex;
    std::vector<RenderedBookmark> rendered_bookmarks;
    //std::vector<RenderedBookmark> pending_render_bookmarks;
    BackgroundTaskManager* task_manager;
    bool is_latex_initialized = false;
    std::mutex latex_lock;
    std::atomic<int> next_request_id = 0;

    std::unordered_map<std::string, float> cached_bookmark_heights;

    bool are_bookmarks_the_same_for_render(const BookMark& bm1, const BookMark& bm2);
    std::vector<int> get_request_indices(const std::vector<RenderedBookmark>& list, const BookMark& bm, float zoom_level, float scroll_amount, ColorPalette palette, bool compare_zoom_level=true);
    bool does_request_exist(const std::vector<RenderedBookmark>& list, const BookMark& bm, float zoom_level, float scroll_amount, ColorPalette palette);
    QPixmap* get_rendered_bookmark(const BookMark& bm, float zoom_level, float scroll_amount, ColorPalette palette);
    void cleanup_bookmarks();
    void initialize_latex();
    void copy_microtex_files();

public:
    BackgroundBookmarkRenderer(BackgroundTaskManager* background_task_manager);

    std::pair<QPixmap*, bool> request_rendered_bookmark(const BookMark& bm, float zoom_level, float scroll_amount, float pixel_ratio, ColorPalette palette);
    float draw_markdown_text(QPainter& painter, QString text, QRect window_qrect, float scroll_amount, bool is_from_main_thread, const QFont& font);
    void render_freetext_bookmark(const BookMark& bookmark, QPainter* painter, float zoom_level, float scroll_amount, float pixel_ratio, QRect window_qrect, ColorPalette palette, bool is_from_main_thread=false);
    void release_cache();
    std::optional<RenderedBookmark> get_request_with_id(int id);
    void erase_request_with_id(int id);
    float get_cached_bookmark_height(const std::string& uuid);

signals:
    void bookmark_rendered();
};

void prepare_text_document_for_bookmark_markdown(QTextDocument& td, QString text, QRect window_qrect, const QFont& font);
