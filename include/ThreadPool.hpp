#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

/**
 * @brief Manages a pool of persistent worker threads.
 * Minimizes thread creation overhead during iterative calculations.
 */
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    /** @brief Initializes the pool with the specified number of threads. */
    explicit ThreadPool(size_t threads);
    ~ThreadPool();

    /** @brief Enqueues a task (lambda or function) for execution. */
    void enqueue(std::function<void()> task);
    
    /** @return Number of active threads in the pool. */
    size_t get_num_threads() const { return workers.size(); }
};