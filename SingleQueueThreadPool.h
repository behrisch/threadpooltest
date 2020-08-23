#pragma once

#include "TaskQueue.h"
#include <algorithm>
#include <thread>

class SingleQueueThreadPool
{
public:

    explicit SingleQueueThreadPool(size_t threadCount = std::max(1u, std::thread::hardware_concurrency()));
    ~SingleQueueThreadPool();

    template<typename TaskT>
    auto ExecuteAsync(TaskT&& task) -> std::future<decltype(task())>
    {
        return m_queue.push(std::forward<TaskT>(task));
    }

private:
    void Run();

    TaskQueue m_queue;
    std::vector<std::thread> m_threads;
};

SingleQueueThreadPool::SingleQueueThreadPool(size_t threadCount)
{
    for (size_t n = 0; n != threadCount; ++n)
        m_threads.emplace_back([this] { Run(); });
}

SingleQueueThreadPool::~SingleQueueThreadPool()
{
    const bool finishTasks = false;
    m_queue.setEnabled(finishTasks);

    for (auto& thread : m_threads)
        thread.join();
}

void SingleQueueThreadPool::Run()
{
    while (m_queue.isEnabled())
    {
        TaskQueue::TaskPtrType task;
        if (m_queue.waitAndPop(task)) {
            (*task)();
        }
    }
}
