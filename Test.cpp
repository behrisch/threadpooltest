#include "TestUtilities.h"
#include "WorkStealingThreadPool.h"

void Test_TaskResultIsAsExpected(WorkStealingThreadPool<>& taskSystem)
{
    constexpr size_t taskCount = 10000;

    std::vector<std::future<size_t>> results;

    for (size_t i = 0; i < taskCount; ++i)
        results.push_back(taskSystem.ExecuteAsync([i](int){ return i*i; }));

    for (size_t i = 0; i < taskCount; ++i)
        TEST_ASSERT(i*i == results[i].get());
}

void Test_RandomTaskExecutionTime(WorkStealingThreadPool<>& taskSystem)
{
    constexpr size_t taskCount = 10000;

    std::vector<std::future<void>> results;

    for (size_t i = 0; i < taskCount; ++i)
        results.push_back(taskSystem.ExecuteAsync([](int){LoadCPUForRandomTime();}));

    for (auto& result : results)
        result.wait();
}

void Test_1nsTaskExecutionTime(WorkStealingThreadPool<>& taskSystem)
{
    constexpr size_t taskCount = 10000;
    std::vector<std::future<void>> results;

    for (size_t i = 0; i < taskCount; ++i)
        results.push_back(taskSystem.ExecuteAsync([](int) { LoadCPUFor(std::chrono::nanoseconds(1)); }));

    for (auto& result : results)
        result.wait();
}

void Test_100msTaskExecutionTime(WorkStealingThreadPool<>& taskSystem)
{
    constexpr size_t taskCount = 10;
    std::vector<std::future<void>> results;

    for (size_t i = 0; i < taskCount; ++i)
        results.push_back(taskSystem.ExecuteAsync([](int) { LoadCPUFor(std::chrono::milliseconds(100)); }));

    for (auto& result : results)
        result.wait();
}

void Test_EmptyTask(WorkStealingThreadPool<>& taskSystem)
{
    constexpr size_t taskCount = 10000;

    std::vector<std::future<void>> results;

    for (size_t i = 0; i < taskCount; ++i)
        results.push_back(taskSystem.ExecuteAsync([](int) {}));

    for (auto& result : results)
        result.wait();
}

template<class TaskT>
void RepeatTask(WorkStealingThreadPool<>& taskSystem, TaskT&& task, size_t times)
{
    std::vector<std::future<void>> results;

    // Here we need not to std::forward just copy task.
    // Because if the universal reference of task has bound to an r-value reference 
    // then std::forward will have the same effect as std::move and thus task is not required to contain a valid task. 
    // Universal reference must only be std::forward'ed a exactly zero or one times.
    for (size_t i = 0; i < times; ++i)
        results.push_back(taskSystem.ExecuteAsync(task));

    for (auto& result : results)
        result.wait();
}

void Test_MultipleTaskProducers(WorkStealingThreadPool<>& taskSystem)
{
    constexpr size_t taskCount = 10000;

    std::vector<std::thread> taskProducers{ std::max(1u, std::thread::hardware_concurrency()) };

    for (auto& producer : taskProducers)
        producer = std::thread([&] { RepeatTask(taskSystem, [](int) { LoadCPUForRandomTime(); }, taskCount); });

    for (auto& producer : taskProducers)
    {
        if (producer.joinable())
            producer.join();
    }
}

int main()
{
    std::vector<int> pseudoContext(std::thread::hardware_concurrency(), 0);
    WorkStealingThreadPool<int> stealingTaskSystem(true, pseudoContext);
    WorkStealingThreadPool<int> multiQueueTaskSystem(false, pseudoContext);

    std::cout << "==========================================" << std::endl;
    std::cout << "             FUNCTIONAL TESTS             " << std::endl;
    std::cout << "==========================================" << std::endl;
    DO_TEST(Test_TaskResultIsAsExpected, multiQueueTaskSystem);
    DO_TEST(Test_TaskResultIsAsExpected, stealingTaskSystem);
    std::cout << std::endl;

    std::cout << "==========================================" << std::endl;
    std::cout << "            PERFORMANCE TESTS             " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Number of cores: " << std::thread::hardware_concurrency() << std::endl;
    constexpr size_t NumOfRuns = 10;
    std::cout << std::endl;
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on multiple task queues", NumOfRuns, Test_RandomTaskExecutionTime, multiQueueTaskSystem);
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on work stealing queue ", NumOfRuns, Test_RandomTaskExecutionTime, stealingTaskSystem);
    std::cout << std::endl;

    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on multiple task queues", NumOfRuns, Test_1nsTaskExecutionTime, multiQueueTaskSystem);
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on work stealing queue ", NumOfRuns, Test_1nsTaskExecutionTime, stealingTaskSystem);
    std::cout << std::endl;

    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on multiple task queues", NumOfRuns, Test_100msTaskExecutionTime, multiQueueTaskSystem);
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on work stealing queue ", NumOfRuns, Test_100msTaskExecutionTime, stealingTaskSystem);
    std::cout << std::endl;

    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on multiple task queues", NumOfRuns, Test_EmptyTask, multiQueueTaskSystem);
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on work stealing queue ", NumOfRuns, Test_EmptyTask, stealingTaskSystem);
    std::cout << std::endl;

    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on multiple task queues", NumOfRuns, Test_MultipleTaskProducers, multiQueueTaskSystem);
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on work stealing queue ", NumOfRuns, Test_MultipleTaskProducers, stealingTaskSystem);
    std::cout << std::endl;

    return 0;
}
