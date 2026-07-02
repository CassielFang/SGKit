#pragma once

#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>

namespace sgkit {
namespace core {

/*
* TaskHandle<T> - async task handle returned by ThreadPool::Enqueue().
* 
* IsReady() polls, Wait() blocks, Get() blocks + returns the result.
* Move-only. Thread-safe for IsReady / Wait; Get() single-use.
*/
template<typename T>
class TaskHandle
{
public:
    explicit TaskHandle(std::future<T>&& f);

    // Move-construct. The moved-from handle becomes invalid (IsReady()
    // returns false, Get() / Wait() will throw).
    TaskHandle(TaskHandle&& other) noexcept;
    TaskHandle& operator=(TaskHandle&& other) noexcept;

    // Non-blocking poll. Returns false after Get() (future consumed).
    // Thread-safe - may be called from any number of threads.
    bool IsReady() const;

    // Block until done, return result. Call once only.
    // Not thread-safe against other methods on the same handle.
    T Get();

    // Block until done. Does not consume - IsReady() still true after.
    // Thread-safe - may be called from any number of threads.
    void Wait();

private:
    TaskHandle(const TaskHandle&) = delete;
    TaskHandle& operator=(const TaskHandle&) = delete;

    mutable std::mutex m_mutex;
    mutable std::future<T> m_future;
    bool m_consumed = false;
};

// TaskHandle<void> - same API, Get() returns nothing.
template<>
class TaskHandle<void>
{
public:
    explicit TaskHandle(std::future<void>&& f);

    TaskHandle(TaskHandle&& other) noexcept;
    TaskHandle& operator=(TaskHandle&& other) noexcept;

    bool IsReady() const;

    void Get();

    void Wait();

private:
    TaskHandle(const TaskHandle&) = delete;
    TaskHandle& operator=(const TaskHandle&) = delete;

    mutable std::mutex m_mutex;
    mutable std::future<void> m_future;
    bool m_consumed = false;
};

// ThreadPool - fixed-size thread pool (singleton).
class ThreadPool
{
public:
    static void Create(size_t numThreads);
    static void Destroy();
    static ThreadPool& instance();

    // Submit a callable + args. Returns immediately; task executes on a worker.
    // Thread-safe. If the pool is stopping the task is silently dropped.
    template<typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args) -> TaskHandle<decltype(f(args...))>;

    // Approx. count of queued + in-flight tasks (lock-free, diagnostic only).
    size_t PendingTasks() const;

private:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    void WorkerLoop();

    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::condition_variable m_finished;
    std::atomic<bool> m_stop{false};
    std::atomic<size_t> m_activeTasks{0};
};

}
}

// Template implementations
#include <sgkit/core/ThreadPoolImpl.h>
