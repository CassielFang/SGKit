#include <sgkit/core/ThreadPool.h>

#include <stdexcept>

namespace sgkit {
namespace core {

ThreadPool::ThreadPool(size_t numThreads)
{
    if (numThreads == 0)
        numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 4;  // fallback

    m_workers.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i)
    {
        m_workers.emplace_back(&ThreadPool::WorkerLoop, this);
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_condition.notify_all();

    for (std::thread& worker : m_workers)
    {
        if (worker.joinable())
            worker.join();
    }
}

void ThreadPool::WorkerLoop()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_condition.wait(lock, [this] {
                return m_stop || !m_tasks.empty();
            });

            if (m_stop && m_tasks.empty())
                return;

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        task();
        --m_activeTasks;
        m_finished.notify_one();
    }
}

void ThreadPool::WaitAll()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_finished.wait(lock, [this] {
        return m_tasks.empty() && m_activeTasks == 0;
    });
}

size_t ThreadPool::PendingTasks() const
{
    return m_activeTasks.load();
}

} // namespace core
} // namespace sgkit
