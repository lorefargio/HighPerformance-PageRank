#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

/**
 * @brief A thread-safe queue for Producer-Consumer communication.
 * * Used to pass data chunks from the I/O thread to parsing threads.
 * Implementation is kept in the header because it is a template.
 * * @tparam T The type of data stored in the queue.
 */
template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool finished = false;

public:
    ThreadSafeQueue() = default;

    /**
     * @brief Pushes an item into the queue and notifies one waiting thread.
     * @param value The item to move into the queue.
     */
    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(std::move(value));
        cv.notify_one();
    }

    /**
     * @brief Signals that no more items will be added.
     * Notifies all waiting threads to prevent deadlocks.
     */
    void set_finished() {
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
        cv.notify_all();
    }

    /**
     * @brief Removes and returns the front item from the queue.
     * Blocks if the queue is empty until an item is pushed or production finishes.
     * @return std::optional containing the item, or std::nullopt if finished and empty.
     */
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !queue.empty() || finished; });
        
        if (queue.empty()) {
            return std::nullopt;
        }

        T value = std::move(queue.front());
        queue.pop();
        return value;
    }
};