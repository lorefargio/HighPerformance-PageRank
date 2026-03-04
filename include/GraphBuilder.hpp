#pragma once
#include "GraphData.hpp"
#include "ThreadSafeQueue.hpp"
#include <string>
#include <vector>
#include <thread>

/**
 * @brief Implementa il caricamento parallelo del grafo.
 * Gestisce un thread produttore (I/O) e N thread consumatori (Parsing)[cite: 52, 94].
 */
class GraphBuilder {
private:
    std::string filepath;
    int num_workers;
    
    // Coda di "chunk" di archi per ridurre l'overhead del mutex [cite: 97, 102]
    ThreadSafeQueue<std::vector<std::pair<int, int>>> chunk_queue;

    struct LocalGraphState {
        // Adjacency list locale per evitare data race 
        std::vector<std::vector<int>> local_in_adj;
        std::vector<int> local_out_degrees;
        int max_node = -1;
    };

    void producerWorker();
    void consumerWorker(LocalGraphState& state);

public:
    explicit GraphBuilder(std::string path, int workers) 
        : filepath(std::move(path)), num_workers(workers) {}

    GraphData build(); // Metodo principale che esegue il workflow [cite: 99]
};