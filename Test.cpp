#include "TestUtilities.h"
#include "WorkStealingThreadPool.h"

template<bool workSteal>
void Test_TaskResultIsAsExpected(WorkStealingThreadPool&& taskSystem = WorkStealingThreadPool(workSteal))
{
    constexpr size_t taskCount = 10000;

    std::vector<std::future<size_t>> results;

    for (size_t i = 0; i < taskCount; ++i)
        results.push_back(taskSystem.ExecuteAsync([i] { return i*i; }));

    for (size_t i = 0; i < taskCount; ++i)
        TEST_ASSERT(i*i == results[i].get());
}

template<bool workSteal>
void Test_RandomTaskExecutionTime(WorkStealingThreadPool&& taskSystem = WorkStealingThreadPool(workSteal))
{
    constexpr size_t taskCount = 10000;

    std::vector<std::future<void>> results;

    for (size_t i = 0; i < taskCount; ++i)
        results.push_back(taskSystem.ExecuteAsync(LoadCPUForRandomTime));

    for (auto& result : results)
        result.wait();
}

template<bool workSteal>
void Test_1nsTaskExecutionTime(WorkStealingThreadPool&& taskSystem = WorkStealingThreadPool(workSteal))
{
    constexpr size_t taskCount = 10000;
    std::vector<std::future<void>> results;

    for (size_t i = 0; i < taskCount; ++i)
        results.push_back(taskSystem.ExecuteAsync([] { LoadCPUFor(std::chrono::nanoseconds(1)); }));

    for (auto& result : results)
        result.wait();
}

template<bool workSteal>
void Test_100msTaskExecutionTime(WorkStealingThreadPool&& taskSystem = WorkStealingThreadPool(workSteal))
{
    constexpr size_t taskCount = 10;
    std::vector<std::future<void>> results;

    for (size_t i = 0; i < taskCount; ++i)
        results.push_back(taskSystem.ExecuteAsync([] { LoadCPUFor(std::chrono::milliseconds(100)); }));

    for (auto& result : results)
        result.wait();
}

template<bool workSteal>
void Test_EmptyTask(WorkStealingThreadPool&& taskSystem = WorkStealingThreadPool(workSteal))
{
    constexpr size_t taskCount = 10000;

    std::vector<std::future<void>> results;

    for (size_t i = 0; i < taskCount; ++i)
        results.push_back(taskSystem.ExecuteAsync([] {}));

    for (auto& result : results)
        result.wait();
}

template<class TaskT>
void RepeatTask(WorkStealingThreadPool& taskSystem, TaskT&& task, size_t times)
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

template<bool workSteal>
void Test_MultipleTaskProducers(WorkStealingThreadPool&& taskSystem = WorkStealingThreadPool(workSteal))
{
    constexpr size_t taskCount = 10000;

    std::vector<std::thread> taskProducers{ std::max(1u, std::thread::hardware_concurrency()) };

    for (auto& producer : taskProducers)
        producer = std::thread([&] { RepeatTask(taskSystem, &LoadCPUForRandomTime, taskCount); });

    for (auto& producer : taskProducers)
    {
        if (producer.joinable())
            producer.join();
    }
}

int main()
{
    std::cout << "==========================================" << std::endl;
    std::cout << "             FUNCTIONAL TESTS             " << std::endl;
    std::cout << "==========================================" << std::endl;
    DO_TEST(Test_TaskResultIsAsExpected<false>);
    DO_TEST(Test_TaskResultIsAsExpected<true>);
    std::cout << std::endl;

    std::cout << "==========================================" << std::endl;
    std::cout << "            PERFORMANCE TESTS             " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Number of cores: " << std::thread::hardware_concurrency() << std::endl;
    constexpr size_t NumOfRuns = 10;
    std::cout << std::endl;
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on multiple task queues", NumOfRuns, Test_RandomTaskExecutionTime<false>);
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on work stealing queue ", NumOfRuns, Test_RandomTaskExecutionTime<true>);
    std::cout << std::endl;

    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on multiple task queues", NumOfRuns, Test_1nsTaskExecutionTime<false>);
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on work stealing queue ", NumOfRuns, Test_1nsTaskExecutionTime<true>);
    std::cout << std::endl;

    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on multiple task queues", NumOfRuns, Test_100msTaskExecutionTime<false>);
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on work stealing queue ", NumOfRuns, Test_100msTaskExecutionTime<true>);
    std::cout << std::endl;

    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on multiple task queues", NumOfRuns, Test_EmptyTask<false>);
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on work stealing queue ", NumOfRuns, Test_EmptyTask<true>);
    std::cout << std::endl;

    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on multiple task queues", NumOfRuns, Test_MultipleTaskProducers<false>);
    DO_BENCHMARK_TEST_WITH_DESCRIPTION("thread pool based on work stealing queue ", NumOfRuns, Test_MultipleTaskProducers<true>);
    std::cout << std::endl;

    return 0;
}
