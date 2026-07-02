#pragma once

#include <sgkit/core/ThreadPool.h>

namespace sgkit {
namespace core {

template<typename T>
TaskHandle<T>::TaskHandle(std::future<T>&& f) : m_future(std::move(f)) {}

template<typename T>
TaskHandle<T>::TaskHandle(TaskHandle&& other) noexcept
    : m_future(std::move(other.m_future))
    , m_consumed(other.m_consumed)
{
    other.m_consumed = true;   // moved-from behaves as consumed
}

template<typename T>
TaskHandle<T>& TaskHandle<T>::operator=(TaskHandle&& other) noexcept
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

template<typename T>
bool TaskHandle<T>::IsReady() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_consumed)
        return false;
    return m_future.valid() &&
        m_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

template<typename T>
T TaskHandle<T>::Get()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_consumed = true;
    return m_future.get();
}

template<typename T>
void TaskHandle<T>::Wait()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_consumed)
            return;
    }
    m_future.wait();
}


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

}
}
