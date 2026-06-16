#include <sgkit/core/ThreadPool.h>

#include <cstdio>
#include <cassert>
#include <vector>
#include <chrono>
#include <atomic>

using namespace sgkit::core;

static int testsPassed = 0;
static int testsFailed = 0;

#define SGK_CHECK(cond, msg) \
    do { if (cond) testsPassed++; else { testsFailed++; std::printf("FAIL: %s\n", msg); } } while(0)

int main()
{
    std::printf("Running ThreadPool tests...\n\n");

    // Test 1: basic enqueue
    {
        ThreadPool pool(4);
        auto f = pool.Enqueue([]() -> int { return 42; });
        SGK_CHECK(f.get() == 42, "basic enqueue");
    }

    // Test 2: multiple tasks
    {
        ThreadPool pool(4);
        std::vector<std::future<int>> results;
        for (int i = 0; i < 100; ++i)
            results.push_back(pool.Enqueue([i]() { return i * i; }));

        pool.WaitAll();
        bool allOk = true;
        for (size_t i = 0; i < results.size(); ++i)
        {
            if (results[i].get() != static_cast<int>(i * i))
            { allOk = false; break; }
        }
        SGK_CHECK(allOk, "100 tasks");
    }

    // Test 3: concurrent access
    {
        ThreadPool pool(8);
        std::atomic<int> counter{0};
        std::vector<std::future<void>> results;
        for (int i = 0; i < 1000; ++i)
            results.push_back(pool.Enqueue([&counter]() { counter++; }));

        pool.WaitAll();
        SGK_CHECK(counter.load() == 1000, "atomic counter 1000");
    }

    std::printf("\nResults: %d passed, %d failed\n", testsPassed, testsFailed);
    return testsFailed > 0 ? 1 : 0;
}
