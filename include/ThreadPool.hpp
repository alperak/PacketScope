#ifndef THREADPOOL_HPP_
#define THREADPOOL_HPP_

#include <atomic>
#include <functional>
#include <thread>
#include <vector>

#include "ThreadSafeQueue.hpp"

/**
 * @brief Simple fixed-size thread pool.
 *
 * Characteristics:
 *  - Fire and forget task execution
 *  - Graceful shutdown (drain the queue and exit)
 *  - No futures, no task results
 *
 * The ThreadPool owns the worker lifecycle and shutdown policy.
 * The underlying queue remains generic and unaware of shutdown.
 */
class ThreadPool {
public:

    /**
     * @brief Constructs a thread pool with a fixed number of worker threads.
     *
     * @param threadCount Number of worker threads.
     * If zero is provided, at least one thread is created.
     */
    explicit ThreadPool(std::size_t threadCount) {
        const std::size_t count = std::max(threadCount, std::size_t{1});
        workers_.reserve(count);

        for (std::size_t i = 0; i < count; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    }

    /**
     * @brief Destructor performs a graceful shutdown.
     *
     * Guarantees that:
     *  - No submitted task is lost
     *  - All queued tasks are executed
     *  - All worker threads are joined
     */
    ~ThreadPool() {
        shutdown();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Submits a new task for execution.
     *
     * Tasks submitted after shutdown has started are ignored.
     *
     * @tparam F Callable type (lambda, function, functor)
     */
    template <typename F>
    void submit(F&& task) {
        if (!stopped_) {
            tasks_.push(std::function<void()>(std::forward<F>(task)));
        }
    }

    /**
     * @brief Graceful shutdown.
     *
     * Behavior:
     *  - Stop accepting new tasks
     *  - Process all already queued tasks
     *  - Exit all worker threads
     *
     * Safe to call multiple times.
     */
    void shutdown() {
        if (stopped_.exchange(true)) {
            return; // Already shut down
        }

        // One poison pill per worker thread.
        // An empty std::function signals worker termination.
        for (std::size_t i = 0; i < workers_.size(); ++i) {
            tasks_.push(std::function<void()>{});
        }

        joinAll();
    }

private:

    /**
     * @brief Worker thread main loop.
     *
     * Continuously pops tasks from the queue and executes them.
     * Receiving an empty std::function acts as a shutdown signal.
     */
    void workerLoop() {
        while (true) {
            std::function<void()> task = tasks_.pop();

            // Poison pill: empty function means exit
            if (!task) {
                return;
            }

            task();
        }
    }

    /**
     * @brief Joins all worker threads.
     */
    void joinAll() {
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    std::vector<std::thread> workers_;
    ThreadSafeQueue<std::function<void()>> tasks_;
    std::atomic<bool> stopped_{false};
};

#endif
