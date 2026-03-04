#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

/**
 * @brief Gestore del ciclo di vita dei thread. 
 * Mantiene i thread attivi per tutta la durata dell'algoritmo.
 */
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

public:
    explicit ThreadPool(size_t threads);
    ~ThreadPool();

    /**
     * @brief Invia un nuovo task (chiusura lambda) al pool[cite: 26, 63].
     */
    void enqueue(std::function<void()> task);
    
    size_t get_num_threads() const { return workers.size(); }
};