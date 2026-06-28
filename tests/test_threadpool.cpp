#include <Windows.h>
#include <sgkit/core/ThreadPool.h>

#include <cstdio>
#include <vector>
#include <chrono>
#include <atomic>
#include <thread>
#include <exception>

using namespace sgkit::core;

static int testsPassed = 0;
static int testsFailed = 0;

#define SGK_CHECK(cond, msg) \
    do { if (cond) testsPassed++; else { testsFailed++; std::printf("FAIL: %s\n", msg); } } while(0)

int main()
{
    std::printf("Running ThreadPool tests...\n\n");

    // ---------------------------------------------------------------
    // 1. Basic Enqueue with return value + Get()
    // ---------------------------------------------------------------
    {
        ThreadPool::Create(4);
        auto h = ThreadPool::instance().Enqueue([]() -> int { return 42; });
        SGK_CHECK(h.Get() == 42, "1. basic Enqueue Get() returns 42");
        ThreadPool::Destroy();
    }

    // ---------------------------------------------------------------
    // 2. TaskHandle<void> — fire-and-forget
    // ---------------------------------------------------------------
    {
        std::atomic<int> val{0};
        ThreadPool::Create(4);
        auto h = ThreadPool::instance().Enqueue([&] { val = 7; });
        h.Wait();
        SGK_CHECK(val.load() == 7, "2. TaskHandle<void> Wait()");
        ThreadPool::Destroy();
    }

    // ---------------------------------------------------------------
    // 3. IsReady() polling lifecycle
    // ---------------------------------------------------------------
    {
        std::atomic<bool> started{false};
        ThreadPool::Create(4);
        auto h = ThreadPool::instance().Enqueue([&] {
            started = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            return 99;
        });

        while (!started.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        SGK_CHECK(!h.IsReady(), "3a. IsReady false while running");
        h.Wait();
        SGK_CHECK(h.IsReady(), "3b. IsReady true after Wait");
        SGK_CHECK(h.Get() == 99, "3c. Get returns 99");
        SGK_CHECK(!h.IsReady(), "3d. IsReady false after Get (consumed)");
        ThreadPool::Destroy();
    }

    // ---------------------------------------------------------------
    // 4. Wait() blocks until completion
    // ---------------------------------------------------------------
    {
        std::atomic<bool> done{false};
        ThreadPool::Create(4);
        auto h = ThreadPool::instance().Enqueue([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            done = true;
        });
        h.Wait();
        SGK_CHECK(done.load(), "4a. Wait() blocks until task completes");
        SGK_CHECK(h.IsReady(), "4b. IsReady true after Wait");
        ThreadPool::Destroy();
    }

    // ---------------------------------------------------------------
    // 5. Multiple parallel tasks — correct results
    // ---------------------------------------------------------------
    {
        ThreadPool::Create(8);
        std::vector<TaskHandle<int>> results;
        for (int i = 0; i < 200; ++i)
            results.push_back(ThreadPool::instance().Enqueue([i] { return i * i; }));

        bool allOk = true;
        for (size_t i = 0; i < results.size(); ++i)
        {
            if (results[i].Get() != static_cast<int>(i * i))
            { allOk = false; break; }
        }
        SGK_CHECK(allOk, "5. 200 parallel tasks correct results");
        ThreadPool::Destroy();
    }

    // ---------------------------------------------------------------
    // 6. Destroy() drains pending tasks
    // ---------------------------------------------------------------
    {
        std::atomic<int> counter{0};
        ThreadPool::Create(4);
        for (int i = 0; i < 500; ++i)
            ThreadPool::instance().Enqueue([&] { ++counter; });
        ThreadPool::Destroy();  // blocks until all 500 complete
        SGK_CHECK(counter.load() == 500, "6. Destroy drains 500 pending tasks");
    }

    // ---------------------------------------------------------------
    // 7. Idempotent Create / Destroy
    // ---------------------------------------------------------------
    {
        ThreadPool::Create(4);
        ThreadPool::Create(8);  // no-op — already created
        ThreadPool::Destroy();
        ThreadPool::Destroy();  // no-op — already destroyed
        SGK_CHECK(true, "7. double Create/Destroy is safe");
    }

    // ---------------------------------------------------------------
    // 8. Enqueue with arguments
    // ---------------------------------------------------------------
    {
        ThreadPool::Create(4);
        auto h = ThreadPool::instance().Enqueue([](int a, int b) { return a + b; }, 10, 20);
        SGK_CHECK(h.Get() == 30, "8. Enqueue with arguments");
        ThreadPool::Destroy();
    }

    // ---------------------------------------------------------------
    // 9. Concurrent Enqueue from multiple threads
    // ---------------------------------------------------------------
    {
        std::atomic<int> counter{0};
        constexpr int kNumProducers = 4;
        constexpr int kTasksPerProducer = 250;

        ThreadPool::Create(8);

        std::thread producers[kNumProducers];
        for (int t = 0; t < kNumProducers; ++t)
        {
            producers[t] = std::thread([&] {
                for (int i = 0; i < kTasksPerProducer; ++i)
                    ThreadPool::instance().Enqueue([&] { ++counter; });
            });
        }
        for (auto& p : producers)
            p.join();

        ThreadPool::Destroy();  // drains all 1000 tasks
        SGK_CHECK(counter.load() == kNumProducers * kTasksPerProducer,
                  "9. concurrent Enqueue from 4 threads");
    }

    // ---------------------------------------------------------------
    // 10. Multiple threads polling IsReady()
    // ---------------------------------------------------------------
    {
        std::atomic<bool> startPolling{false};
        std::atomic<int> pollersReady{0};
        std::atomic<int> pollCount{0};
        std::atomic<bool> taskMayFinish{false};

        ThreadPool::Create(4);
        auto h = ThreadPool::instance().Enqueue([&] {
            while (!taskMayFinish.load())
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        });

        std::thread pollers[4];
        for (int t = 0; t < 4; ++t)
        {
            pollers[t] = std::thread([&] {
                pollersReady++;
                while (!startPolling.load())
                    ;
                while (!h.IsReady())
                    pollCount++;
            });
        }

        while (pollersReady.load() < 4)
            ;
        startPolling = true;

        // Give pollers time to spin, then release the task
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        taskMayFinish = true;
        h.Wait();

        for (auto& p : pollers)
            p.join();

        SGK_CHECK(pollCount.load() > 0, "10. multi-thread IsReady() polling");
        ThreadPool::Destroy();
    }

    // ---------------------------------------------------------------
    // 11. PendingTasks() approximate count
    // ---------------------------------------------------------------
    {
        ThreadPool::Create(4);
        SGK_CHECK(ThreadPool::instance().PendingTasks() == 0,
                  "11a. PendingTasks 0 at start");

        std::atomic<bool> go{false};
        ThreadPool::instance().Enqueue([&] {
            while (!go.load())
                ;
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        for (int i = 0; i < 10; ++i)
            ThreadPool::instance().Enqueue([] {});

        size_t pending = ThreadPool::instance().PendingTasks();
        SGK_CHECK(pending >= 1, "11b. PendingTasks >= 1 while tasks active");

        go = true;
        ThreadPool::Destroy();
    }

    // ---------------------------------------------------------------
    // 12. Exception propagation through Get()
    // ---------------------------------------------------------------
    {
        ThreadPool::Create(4);
        auto h = ThreadPool::instance().Enqueue([]() -> int {
            throw std::runtime_error("task failed");
        });

        bool caught = false;
        try { h.Get(); }
        catch (const std::runtime_error&) { caught = true; }
        SGK_CHECK(caught, "12. exception propagated through Get()");
        ThreadPool::Destroy();
    }

    // ---------------------------------------------------------------
    // 13. Task enqueues another task from within
    // ---------------------------------------------------------------
    {
        std::atomic<int> seq{0};
        ThreadPool::Create(4);

        auto h = ThreadPool::instance().Enqueue([&] {
            seq.store(1);
            ThreadPool::instance().Enqueue([&] { seq.store(2); });
        });

        h.Wait();
        // Allow the chained task to execute
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        SGK_CHECK(seq.load() == 2, "13. task enqueues another task");
        ThreadPool::Destroy();
    }

    // ---------------------------------------------------------------
    // 14. Zero-thread → auto-detect
    // ---------------------------------------------------------------
    {
        ThreadPool::Create(0);
        auto h = ThreadPool::instance().Enqueue([]() -> int { return 1; });
        SGK_CHECK(h.Get() == 1, "14. auto-detected thread count works");
        ThreadPool::Destroy();
    }

    std::printf("\nResults: %d passed, %d failed\n", testsPassed, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
