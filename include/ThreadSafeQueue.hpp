#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

/**
 * @brief Buffer thread-safe per la comunicazione tra produttore (I/O) e consumatori (Parsing)[cite: 14].
 */
template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool finished = false;

public:
    void push(T value) {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(std::move(value));
        cv.notify_one();
    }

    void set_finished() {
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
        cv.notify_all();
    }

    std::optional<T> pop() {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this] { return !queue.empty() || finished; });
        
        if (queue.empty()) return std::nullopt;

        T value = std::move(queue.front());
        queue.pop();
        return value;
    }
};