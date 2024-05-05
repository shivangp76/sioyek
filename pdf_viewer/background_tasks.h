#pragma once

#include <thread>
#include <vector>
#include <deque>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <qobject.h>

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
