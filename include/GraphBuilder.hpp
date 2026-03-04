#pragma once
#include "GraphData.hpp"
#include "ThreadSafeQueue.hpp"
#include <string>
#include <vector>
#include <thread>

/**
 * @brief Implements parallel graph loading and CSR construction.
 * Orchestrates one Producer thread (I/O) and N Consumer threads (Parsing).
 */
class GraphBuilder {
private:
    std::string filepath;
    int num_workers;
    
    /** @brief Thread-safe queue for edge chunks to minimize mutex contention. */
    ThreadSafeQueue<std::vector<std::pair<int, int>>> chunk_queue;

    /** @brief Captures partial graph state for each consumer thread. */
    struct LocalGraphState {
        std::vector<std::vector<int>> local_in_adj; /**< Thread-local adjacency list */
        std::vector<int> local_out_degrees;         /**< Thread-local degree counts */
        int max_node = -1;                          /**< Highest node ID seen by thread */
    };

    /** @brief Worker function for reading file chunks. */
    void producerWorker();
    
    /** @brief Worker function for parsing edge data. */
    void consumerWorker(LocalGraphState& state);

public:
    /**
     * @brief Constructs a GraphBuilder.
     * @param path Path to the .mtx file.
     * @param workers Number of consumer threads.
     */
    explicit GraphBuilder(std::string path, int workers) 
        : filepath(std::move(path)), num_workers(workers) {}

    /**
     * @brief Executes the parallel build workflow.
     * @return A finalized, immutable GraphData object.
     */
    GraphData build();
};