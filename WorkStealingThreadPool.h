#pragma once

#include "TaskQueue.h"
#include <algorithm>
#include <thread>

class WorkStealingThreadPool
{
public:

    explicit WorkStealingThreadPool(const bool workSteal=true, size_t threadCount = std::max(1u, std::thread::hardware_concurrency()))
    : myQueues{ threadCount }, myTryoutCount(workSteal ? 1 : 0) {
        for (size_t index = 0; index != threadCount; ++index) {
            if (workSteal) {
                myThreads.emplace_back([this, index] { WorkStealRun(index); });
            } else {
                myThreads.emplace_back([this, index] { Run(index); });
            }
        }
    }

    ~WorkStealingThreadPool() {
        for (auto& queue : myQueues) {
            queue.setEnabled(false);
        }
        for (auto& thread : myThreads) {
            thread.join();
        }
    }

    template<typename TaskT>
    auto ExecuteAsync(TaskT&& task) -> std::future<decltype(task())> {
        const auto index = myQueueIndex++;
        if (myTryoutCount > 0) {
            for (size_t n = 0; n != myQueues.size() * myTryoutCount; ++n) {
                // Here we need not to std::forward just copy task.
                // Because if the universal reference of task has bound to an r-value reference 
                // then std::forward will have the same effect as std::move and thus task is not required to contain a valid task. 
                // Universal reference must only be std::forward'ed a exactly zero or one times.
                bool success = false;
                auto result = myQueues[(index + n) % myQueues.size()].tryPush(task, success); 

                if (success) {
                    return std::move(result);
                }
            }
        }
        return myQueues[index % myQueues.size()].push(std::forward<TaskT>(task));
    }

private:
    void Run(size_t queueIndex) {
        while (myQueues[queueIndex].isEnabled()) {
            TaskQueue::TaskPtrType task;
            if (myQueues[queueIndex].waitAndPop(task)) {
                (*task)();
            }
        }
    }

    void WorkStealRun(size_t queueIndex) {
        while (myQueues[queueIndex].isEnabled()) {
            TaskQueue::TaskPtrType task;
            for (size_t n = 0; n != myQueues.size()*myTryoutCount; ++n) {
                if (myQueues[(queueIndex + n) % myQueues.size()].tryPop(task)) {
                    break;
                }
            }
            if (!task && !myQueues[queueIndex].waitAndPop(task)) {
                return;
            }
            (*task)();
        }
    }

private:
    std::vector<TaskQueue> myQueues;
    std::atomic<size_t>    myQueueIndex{ 0 };
    const size_t myTryoutCount;
    std::vector<std::thread> myThreads;
};
