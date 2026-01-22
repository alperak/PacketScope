#ifndef THREADSAFEQUEUE_HPP_
#define THREADSAFEQUEUE_HPP_

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

/**
 * @brief Minimal thread safe FIFO queue.
 *
 * Responsibilities:
 *  - Provide safe push/pop operations
 *  - Block consumers when the queue is empty
 *
 * Non responsibilities:
 *  - No shutdown / stop / lifecycle management
 *
 * @tparam T Type of elements stored in the queue
 */
template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue &) = delete;
    ThreadSafeQueue &operator=(const ThreadSafeQueue &) = delete;
    ThreadSafeQueue(ThreadSafeQueue &&) = delete;
    ThreadSafeQueue &operator=(ThreadSafeQueue &&) = delete;

    /**
     * @brief Push a new element into the queue.
     *
     * Notifies exactly one waiting consumer.
     * The notification is done AFTER releasing the lock
     * to avoid unnecessary wakeups and contention.
     *
     * @param value
     */
    void push(T value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(value));
        }
        cv_.notify_one();
    }

    /**
     * @brief Blocking pop
     *
     * Waits until at least one element is available.
     * This function never returns an empty value.
     *
     * @return T
     */
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        // Wake up when there is data to consume
        cv_.wait(lock, [this] { return !queue_.empty(); });

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    /**
     * @brief Non blocking pop
     *
     * @return std::nullopt if the queue is empty.
     */
    std::optional<T> tryPop() {
        std::lock_guard<std::mutex> lock(mutex_);

        if (queue_.empty())
            return std::nullopt;

        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    /**
     * @brief Returns the current number of elemens in the queue.
     *
     * The result is only accurate, if no other thread modifies the queue.
     * Therefore it is rarely practical to use this value in program logic.
     * Intended for monitoring and diagnostic purposes only.
     * It shouldn't be used as a decider.
     *
     * @return std::size_t
     */
    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:

    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

#endif