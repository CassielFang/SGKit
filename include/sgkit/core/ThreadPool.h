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

/// TaskHandle<T> — async task handle returned by ThreadPool::Enqueue().
///
/// IsReady() polls, Wait() blocks, Get() blocks + returns the result.
/// Move-only. Thread-safe for IsReady / Wait; Get() single-use.
template<typename T>
class TaskHandle
{
public:
    explicit TaskHandle(std::future<T>&& f) : m_future(std::move(f)) {}

    TaskHandle(const TaskHandle&) = delete;
    TaskHandle& operator=(const TaskHandle&) = delete;

    /// Move-construct. The moved-from handle becomes invalid (IsReady()
    /// returns false, Get() / Wait() will throw).
    TaskHandle(TaskHandle&& other) noexcept
        : m_future(std::move(other.m_future))
        , m_consumed(other.m_consumed)
    {
        other.m_consumed = true;   // moved-from behaves as consumed
    }

    TaskHandle& operator=(TaskHandle&& other) noexcept
    {
        if (this != &other)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::lock_guard<std::mutex> otherLock(other.m_mutex);
            m_future = std::move(other.m_future);
            m_consumed = other.m_consumed;
            other.m_consumed = true;
        }
        return *this;
    }

    /// Non-blocking poll. Returns false after Get() (future consumed).
    /// Thread-safe — may be called from any number of threads.
    bool IsReady() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_consumed)
            return false;
        return m_future.valid() &&
               m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    /// Block until done, return result. Call once only.
    /// Not thread-safe against other methods on the same handle.
    T Get()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_consumed = true;
        return m_future.get();
    }

    /// Block until done. Does not consume — IsReady() still true after.
    /// Thread-safe — may be called from any number of threads.
    void Wait()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_consumed)
                return;
        }
        m_future.wait();
    }

private:
    mutable std::mutex m_mutex;
    mutable std::future<T> m_future;
    bool m_consumed = false;
};

// TaskHandle<void> — same API, Get() returns nothing.
template<>
class TaskHandle<void>
{
public:
    explicit TaskHandle(std::future<void>&& f) : m_future(std::move(f)) {}

    TaskHandle(const TaskHandle&) = delete;
    TaskHandle& operator=(const TaskHandle&) = delete;

    TaskHandle(TaskHandle&& other) noexcept
        : m_future(std::move(other.m_future))
        , m_consumed(other.m_consumed)
    {
        other.m_consumed = true;
    }

    TaskHandle& operator=(TaskHandle&& other) noexcept
    {
        if (this != &other)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::lock_guard<std::mutex> otherLock(other.m_mutex);
            m_future = std::move(other.m_future);
            m_consumed = other.m_consumed;
            other.m_consumed = true;
        }
        return *this;
    }

    bool IsReady() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_consumed)
            return false;
        return m_future.valid() &&
               m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }

    void Get()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_consumed = true;
        m_future.get();
    }

    void Wait()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_consumed)
                return;
        }
        m_future.wait();
    }

private:
    mutable std::mutex m_mutex;
    mutable std::future<void> m_future;
    bool m_consumed = false;
};

// ThreadPool — fixed-size thread pool (singleton).
class ThreadPool
{
public:
    static void Create(size_t numThreads);
    static void Destroy();
    static ThreadPool& instance();

    /// Submit a callable + args. Returns immediately; task executes on a worker.
    /// Thread-safe. If the pool is stopping the task is silently dropped.
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

// -- Template implementation ----------------------------------------

template<typename F, typename... Args>
auto ThreadPool::Enqueue(F&& f, Args&&... args) -> TaskHandle<decltype(f(args...))>
{
    using ReturnType = decltype(f(args...));

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<ReturnType> result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop)
            return TaskHandle<ReturnType>(std::move(result));

        m_tasks.emplace([task]() { (*task)(); });
        ++m_activeTasks;
    }

    m_condition.notify_one();
    return TaskHandle<ReturnType>(std::move(result));
}

} // namespace core
} // namespace sgkit
