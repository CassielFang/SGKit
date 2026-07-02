# Thread Pool (`sgkit::core::ThreadPool`)

Fixed-size singleton thread pool. Submits callables, returns `TaskHandle<T>`
to query/poll/retrieve the result. `Destroy()` automatically waits for all
pending tasks.

## Quick start

```cpp
#include <sgkit/core/ThreadPool.h>
using namespace sgkit::core;

ThreadPool::Create(0);                      // 0 = auto-detect core count
auto& tp = ThreadPool::instance();

auto h = tp.Enqueue([] { return loadMesh("teapot.obj"); });
if (h.IsReady()) { /* use h.Get() */ }

ThreadPool::Destroy();                      // blocks until tasks drain
```

---

## TaskHandle\<T\>

Returned by `ThreadPool::Enqueue()`. Holds the eventual result of a single task.

| Method | Blocks? | Returns | Callable |
|---|---|---|---|
| `IsReady()` | No | `bool` | Any number of times, any thread |
| `Wait()` | Yes | `void` | Any number of times, any thread |
| `Get()` | Yes | `T` (or `void`) | **Once only** - consumes the result |

### `bool IsReady() const`

Non-blocking poll. Returns `true` when the task has finished executing and the
result is ready. Once `Get()` has been called the underlying `std::future` is
consumed and this returns `false` permanently.

```cpp
auto handle = tp.Enqueue([] { return expensiveCalc(); });

// In a render loop:
while (!handle.IsReady())
{
    renderOneFrame();       // keep the app responsive
}
Result r = handle.Get();
```

### `T Get()`

Blocks until the task completes, then returns the result (or rethrows if the
task threw). May only be called once. Calling from multiple threads
simultaneously on the same handle is not permitted - this matches
`std::future::get()`'s contract.

```cpp
auto h = tp.Enqueue([](int n) { return n * n; }, 42);
int result = h.Get();       // blocks, returns 1764
```

### `void Wait()`

Blocks until the task completes. Does **not** consume the future - `IsReady()`
will still return `true` afterward. May be called multiple times from any
thread.

```cpp
auto h = tp.Enqueue([&] { loadAsync(file); });
h.Wait();                   // sync-point
assert(file.isReady());
```

### `TaskHandle<void>` specialization

When the task returns nothing, `Get()` blocks but returns `void`:

```cpp
auto h = tp.Enqueue([&] { counter++; });
h.Get();                    // blocks, returns nothing
// or equivalently:
h.Wait();
```

### Thread-safety

- `IsReady()` and `Wait()` are internally synchronised - safe to call from
  multiple threads concurrently.
- `Get()` must not be called concurrently with any other method on the same
  handle.
- If thread A calls `Get()` while thread B calls `IsReady()`, thread B will
  see the consumed state and return `false`.

---

## ThreadPool API

| Static method | Notes |
|---|---|
| `Create(size_t numThreads)` | Initialise the singleton. Idempotent. `0` -> auto-detect. |
| `Destroy()` | Blocks until tasks drain, then destroys. Safe no-op when already destroyed. |
| `instance()` | Returns singleton reference. Must call `Create()` first. |

All task submission goes through `instance()`:

```cpp
auto& tp = ThreadPool::instance();
tp.Enqueue(...);
tp.PendingTasks();
```

### `template<typename F, typename... Args> auto Enqueue(F&& f, Args&&... args) -> TaskHandle<decltype(f(args...))>`

Submit any callable with optional arguments. Returns immediately with a handle.

```cpp
auto& tp = ThreadPool::instance();

// Lambda, no args
auto h1 = tp.Enqueue([] { return 42; });              // TaskHandle<int>

// Lambda with captures
auto h2 = tp.Enqueue([&file] { load(file); });        // TaskHandle<void>

// Free function with args
int add(int a, int b) { return a + b; }
auto h3 = tp.Enqueue(add, 3, 4);                      // TaskHandle<int>
```

Tasks execute in FIFO order. Completion order is non-deterministic.

If the pool is stopping (`m_stop == true`), `Enqueue()` silently drops the
task and returns a handle whose `IsReady()` is perpetually `false`.

### `size_t PendingTasks() const`

Lock-free atomic read - approximate count of queued + in-flight tasks.
Use for diagnostics only; the value is stale the moment you read it.

### Destroy()

1. Sets `m_stop = true` and wakes all workers.
2. Waits on `m_finished` until `m_tasks.empty() && m_activeTasks == 0`.
3. Re-wakes workers so they see the stop flag and exit.
4. Joins all worker threads.

New tasks enqueued during or after `Destroy()` are silently dropped.

---

## Usage patterns

### 1. Fire-and-forget

```cpp
ThreadPool::Create(8);
auto& tp = ThreadPool::instance();

for (auto& file : files)
    tp.Enqueue([&] { processFile(file); });

ThreadPool::Destroy();  // blocks until everything is done
```

### 2. Polling for completion

```cpp
auto& tp = ThreadPool::instance();
auto meshHandle = tp.Enqueue([] { return loadMesh("level.obj"); });
auto texHandle = tp.Enqueue([] { return loadTexture("tiles.bmp"); });

while (!meshHandle.IsReady() || !texHandle.IsReady())
{
    // Show a loading screen, pump the message loop, etc.
}
scene.AddMesh(meshHandle.Get());
scene.AddTexture(texHandle.Get());
```

### 3. Bulk submit, gather results

```cpp
auto& tp = ThreadPool::instance();
std::vector<TaskHandle<int>> results;
for (int i = 0; i < 1000; ++i)
    results.push_back(tp.Enqueue([i] { return i * i; }));

for (auto& h : results)
    values.push_back(h.Get());      // Get() blocks per item
```

### 4. Task chaining (enqueue from within a task)

```cpp
auto& tp = ThreadPool::instance();
tp.Enqueue([&] {
    auto part = computeFirstHalf();
    tp.Enqueue([part, &result] {
        result = part + computeSecondHalf();
    });
});
```

This works, but be careful not to create cycles or unbounded recursion.

---

## Thread-safety guarantees

### ThreadPool

| Operation | Safe to call concurrently? |
|---|---|
| `Enqueue()` from N threads | Yes - mutex protects the queue |
| `Enqueue()` + `PendingTasks()` | Yes - `PendingTasks` is lock-free |
| `Enqueue()` during `Destroy()` | Safe - task is silently dropped |
| `Create()` / `Destroy()` | Not meant for concurrent calls; use from main thread |

### TaskHandle

| Operation | Safe to call concurrently? |
|---|---|
| N threads calling `IsReady()` | Yes |
| N threads calling `Wait()` | Yes |
| `IsReady()` + `Wait()` | Yes |
| `Get()` + anything else | No - `Get()` must be the only access at that time |
| `Get()` after `Get()` | No - single-use |

---

## Internals

### Worker loop

```
while (true)
    wait(m_condition) until m_stop || task available
    if m_stop && queue empty -> exit
    dequeue task
    execute task
    lock -> decrement m_activeTasks -> notify m_finished
```

The critical detail: `m_finished.notify_one()` is called **under the mutex**.
Without this, a notification could arrive between the destructor's predicate
check and `wait()`, causing a deadlock.

### Condition variable discipline

| CV | Who waits | Who notifies | Notify under mutex? |
|---|---|---|---|
| `m_condition` | Worker threads (waiting for work) | `Enqueue()` / destructor | No (safe - predicate modified under lock) |
| `m_finished` | Destructor (waiting for drain) | Workers after each task | **Yes** (required - predicate reads atomic `m_activeTasks`) |

For `m_condition`: the worker's `wait()` atomically releases the mutex, and
`Enqueue()` modifies `m_tasks` under the mutex before notifying. No window for
a lost notification.

For `m_finished`: the predicate reads `m_activeTasks` which is modified
outside `m_mutex` in the worker. Holding the lock during `notify_one()`
ensures the destructor either sees the updated value (if it holds the mutex)
or receives the notification (if it is already in `wait()`).

### TaskHandle internals

`TaskHandle<T>` wraps `std::future<T>` with an internal mutex and a `m_consumed`
flag. The mutex prevents data races between concurrent `IsReady()` / `Wait()`
calls. The consumed flag ensures `IsReady()` returns `false` after `Get()` even
though the underlying future is no longer valid.
