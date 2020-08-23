#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <queue>

class TaskBase
{
public:
    virtual ~TaskBase() = default;
    virtual void exec() = 0;
    void operator()() { exec(); }
};

template <typename T> 
class Task : public TaskBase
{
public:
    Task(T&& t) : task(std::move(t)) {}
    void exec() override { task(); }

    T task;
};

class TaskQueue {
    using LockType = std::unique_lock<std::mutex>;

public:
    using TaskPtrType = std::unique_ptr<TaskBase>;
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
    auto push(TaskT&& task) -> std::future<decltype(task())> {
        using PkgTask = std::packaged_task<decltype(task())()>;
        auto job = std::unique_ptr<Task<PkgTask>>(new Task<PkgTask>(PkgTask(std::forward<TaskT>(task))));
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

        if (!lock || !myEnabled || myQueue.empty())
            return false;

        task = std::move(myQueue.front());
        myQueue.pop();
        return true;
    }

    template <typename TaskT>
    auto tryPush(TaskT&& task, bool& success) -> std::future<decltype(task())> {
        std::future<decltype(task())> future;
        success = false;
        {
            LockType lock{ myMutex, std::try_to_lock };
            if (!lock)
                return future;

            using PkgTask = std::packaged_task<decltype(task())()>;
            auto job = std::unique_ptr<Task<PkgTask>>(new Task<PkgTask>(PkgTask(std::forward<TaskT>(task))));
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
