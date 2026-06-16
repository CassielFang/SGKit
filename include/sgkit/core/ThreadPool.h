#pragma once

#include <functional>
#include <future>
#include <queue>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace sgkit {
namespace core {

class ThreadPool
{
public:
    ThreadPool(size_t numThreads = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template<typename F, typename... Args>
    auto Enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

    void WaitAll();
    size_t PendingTasks() const;

private:
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
auto ThreadPool::Enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
{
    using ReturnType = decltype(f(args...));

    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );

    std::future<ReturnType> result = task->get_future();

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_stop)
            return result;

        m_tasks.emplace([task]() { (*task)(); });
        ++m_activeTasks;
    }

    m_condition.notify_one();
    return result;
}

} // namespace core
} // namespace sgkit
