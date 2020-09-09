#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <queue>

template <typename C>
class TaskBase
{
public:
    virtual ~TaskBase() = default;
    virtual void exec(C& context) = 0;
};

template <typename T, typename C>
class Task : public TaskBase<C>
{
public:
    Task(T&& t) : task(std::move(t)) {}
    void exec(C& context) override { task(context); }

    T task;
};

template <typename C>
class TaskQueue {
    using LockType = std::unique_lock<std::mutex>;

public:
    using TaskPtrType = std::unique_ptr<TaskBase<C> >;
    TaskQueue() = default;
    ~TaskQueue() = default;

    void setEnabled(bool enabled) {
        {
            LockType lock{ myMutex };
            myEnabled = enabled;
        }
        if (!enabled) {
            myReady.notify_all();
        }
    }

    bool isEnabled() const {
        LockType lock{ myMutex };
        return myEnabled;
    }

    bool waitAndPop(TaskPtrType& task) {
        LockType lock{ myMutex };
        myReady.wait(lock, [this] { return !myEnabled || !myQueue.empty(); });
        if (myEnabled && !myQueue.empty()) {
            task = std::move(myQueue.front());
            myQueue.pop();
            return true;
        }
        return false;
    }

    template <typename TaskT>
    auto push(TaskT&& task) -> std::future<decltype(task(std::declval<C>()))> {
        using PkgTask = std::packaged_task<decltype(task(std::declval<C>()))(C)>;
        auto job = std::unique_ptr<Task<PkgTask,C>>(new Task<PkgTask,C>(PkgTask(std::forward<TaskT>(task))));
        auto future = job->task.get_future();
        {
            LockType lock{ myMutex };
            myQueue.emplace(std::move(job));
        }

        myReady.notify_one();
        return future;
    }

    bool tryPop(TaskPtrType& task) {
        LockType lock{ myMutex, std::try_to_lock };
        if (!lock || !myEnabled || myQueue.empty()) {
            return false;
        }
        task = std::move(myQueue.front());
        myQueue.pop();
        return true;
    }

    template <typename TaskT>
    auto tryPush(TaskT&& task, bool& success) -> std::future<decltype(task(std::declval<C>()))> {
        std::future<decltype(task(std::declval<C>()))> future;
        success = false;
        {
            LockType lock{ myMutex, std::try_to_lock };
            if (!lock) {
                return future;
            }
            using PkgTask = std::packaged_task<decltype(task(std::declval<C>()))(C)>;
            auto job = std::unique_ptr<Task<PkgTask,C>>(new Task<PkgTask,C>(PkgTask(std::forward<TaskT>(task))));
            future = job->task.get_future();
            success = true;
            myQueue.emplace(std::move(job));
        }

        myReady.notify_one();
        return future;
    }

private:
    TaskQueue(const TaskQueue &) = delete;
    TaskQueue &operator=(const TaskQueue &) = delete;

    std::queue<TaskPtrType> myQueue;
    bool myEnabled = true;
    mutable std::mutex myMutex;
    std::condition_variable myReady;
};
