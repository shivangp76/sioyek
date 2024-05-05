#include "background_tasks.h"

void BackgroundTaskManager::start_worker_thread() {
    worker_thread = std::thread([&]() {
        while (!should_stop) {
            std::unique_lock<std::mutex> lock(pending_tasks_mutex);
            pending_taks_cv.wait(lock, [&]{
                return pending_tasks.size() > 0 || should_stop;
                });
            if (should_stop) {
                break;
            }
            auto [parent, task] = std::move(pending_tasks.front());
            pending_tasks.pop_front();
            current_task_parent = parent;
            lock.unlock();
            task();
            current_task_parent = nullptr;
        }
        });
}

BackgroundTaskManager::~BackgroundTaskManager() {
    if (is_worker_thread_started) {
        should_stop = true;
        pending_taks_cv.notify_one();
        worker_thread.join();
    }
}

void BackgroundTaskManager::delete_tasks_with_parent(QObject* parent) {
    if (is_worker_thread_started) {
        while (parent == current_task_parent) {};

        pending_tasks_mutex.lock();
        auto new_end = std::remove_if(pending_tasks.begin(), pending_tasks.end(), [parent](const std::pair<QObject*, std::function<void()>>& task) {
            return task.first == parent;
            });
        pending_tasks.erase(new_end, pending_tasks.end());
        pending_tasks_mutex.unlock();
    }
}

void BackgroundTaskManager::add_task(std::function<void()>&& fn, QObject* parent) {
    if (!is_worker_thread_started) {
        start_worker_thread();
        is_worker_thread_started = true;
    }

    pending_tasks.push_back(std::make_pair(parent, std::move(fn)));
    pending_taks_cv.notify_one();
}

