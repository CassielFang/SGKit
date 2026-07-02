#include <sgkit/core/ThreadPool.h>

#include <sgkit/core/DebugOut.h>

sgkit::core::ThreadPool* g_ThreadPool = nullptr;

namespace sgkit {
namespace core {

TaskHandle<void>::TaskHandle(std::future<void>&& f) : m_future(std::move(f)) {}

TaskHandle<void>::TaskHandle(TaskHandle<void>&& other) noexcept
    : m_future(std::move(other.m_future))
    , m_consumed(other.m_consumed)
{
    other.m_consumed = true;
}

TaskHandle<void>& TaskHandle<void>::operator=(TaskHandle<void>&& other) noexcept
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

bool TaskHandle<void>::IsReady() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_consumed)
        return false;
    return m_future.valid() &&
        m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void TaskHandle<void>::Get()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_consumed = true;
    m_future.get();
}

void TaskHandle<void>::Wait()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_consumed)
            return;
    }
    m_future.wait();
}


ThreadPool::ThreadPool(size_t numThreads)
{
    if (numThreads == 0)
        numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0)
        numThreads = 4;  // fallback - hardware_concurrency can return 0 per spec

    m_workers.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i)
    {
        m_workers.emplace_back(&ThreadPool::WorkerLoop, this);
    }
    DebugOut("[SGKit ThreadPool]: module created.");
}

ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_condition.notify_all();

    /**
    * wait until every queued / in-flight task completes.
    * WorkerLoop notifies m_finished under m_mutex, guaranteeing that
    * when the predicate passes no new tasks can be in flight and the
    * notification cannot be lost between the predicate check and wait().
    */
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_finished.wait(lock, [this] {
            return m_tasks.empty() && m_activeTasks == 0;
        });
    }

    // re-notify so any worker still sleeping on m_condition
    // wakes up, sees m_stop && queue empty, and exits its loop.
    m_condition.notify_all();

    for (std::thread& worker : m_workers)
    {
        if (worker.joinable())
            worker.join();
    }
    DebugOut("[SGKit ThreadPool]: module destroyed");
}

void ThreadPool::Create(size_t numThreads)
{
    if (g_ThreadPool) return;
    g_ThreadPool = new ThreadPool(numThreads);
}

void ThreadPool::Destroy()
{
    if (!g_ThreadPool) return;
    delete g_ThreadPool;
    g_ThreadPool = nullptr;
}

ThreadPool& ThreadPool::instance()
{
    return *g_ThreadPool;
}

size_t ThreadPool::PendingTasks() const
{
    return m_activeTasks.load();
}

void ThreadPool::WorkerLoop()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(m_mutex);

            // Sleep until there is work or we are told to stop.
            m_condition.wait(lock, [this] {
                return m_stop || !m_tasks.empty();
            });

            if (m_stop && m_tasks.empty())
                return;

            task = std::move(m_tasks.front());
            m_tasks.pop();
        }

        task();

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            --m_activeTasks;
            m_finished.notify_one();
        }
    }
}

}
}
