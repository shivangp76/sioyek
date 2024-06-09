#pragma once

#include <thread>
#include <vector>
#include <deque>
#include <functional>
#include <shared_mutex>
#include <condition_variable>
#include <qobject.h>
#include <qdatetime.h>

#include "book.h"

class BackgroundTaskManager {
private:
    bool is_worker_thread_started = false;
    std::thread worker_thread;
    bool should_stop = false;

    std::mutex pending_tasks_mutex;
    std::condition_variable pending_taks_cv;
    std::deque<std::pair<QObject*, std::function<void()>>> pending_tasks;
    QObject* current_task_parent = nullptr;
public:

    void start_worker_thread();
    void add_task(std::function<void()>&& fn, QObject* parent);
    void delete_tasks_with_parent(QObject* parent);
    ~BackgroundTaskManager();

};

struct RenderedBookmark {
    BookMark bookmark;
    float zoom_level;
    float pixel_ratio;
    bool dark_mode = false;
    QPixmap* pixmap = nullptr;
    QDateTime last_access_time;
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

    bool are_bookmarks_the_same_for_render(const BookMark& bm1, const BookMark& bm2);
    std::vector<int> get_request_indices(const std::vector<RenderedBookmark>& list, const BookMark& bm, float zoom_level, bool dark_mode, bool compare_zoom_level=true);
    bool does_request_exist(const std::vector<RenderedBookmark>& list, const BookMark& bm, float zoom_level, bool dark_mode);
    QPixmap* get_rendered_bookmark(const BookMark& bm, float zoom_level, bool dark_mode);
    void cleanup_bookmarks();
    void initialize_latex();
    void copy_microtex_files();

public:
    BackgroundBookmarkRenderer(BackgroundTaskManager* background_task_manager);

    QPixmap* request_rendered_bookmark(const BookMark& bm, float zoom_level, float pixel_ratio, bool dark_mode);
    void draw_markdown_text(QPainter& painter, QString text, QRect window_qrect, const QFont& font);
    void render_freetext_bookmark(const BookMark& bookmark, QPainter* painter, float zoom_level, float pixel_ratio, bool dark_mode, QRect window_qrect, bool is_from_main_thread=false);

signals:
    void bookmark_rendered();
};
