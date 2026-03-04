#include "../include/PageRankEngine.hpp"
#include <cmath>
#include <numeric>
#include <iostream>
#include <algorithm>

PageRankEngine::PageRankEngine(const GraphData& g, ThreadPool& p, const AppConfig& c)
    : graph(g), pool(p), config(c) {
    
    int n = graph.get_num_nodes();
    
    // Initialize ranks using AlignedVector defined in Common.hpp
    current_rank.assign(n, 1.0f / n); 
    next_rank.resize(n, 0.0f);

    // Identify dead-end nodes (no outgoing edges)
    for (int i = 0; i < n; ++i) {
        if (graph.get_inv_out_degree(i) == 0.0f) {
            dead_ends.push_back(i);
        }
    }
}

/**
 * @brief Processes a range of nodes. Uses Tiling to improve L1/L2 cache hit rate.
 */
void PageRankEngine::computeChunk(int start, int end, RankType damping_val, 
                                 RankType dead_end_contrib, std::atomic<double>& global_error) {
    double local_error = 0.0;
    const RankType df = static_cast<RankType>(config.damping_factor);
    
    // TILE_SIZE optimized for typical 32KB L1 Data Caches
    const int TILE_SIZE = 512; 

    for (int tile_start = start; tile_start < end; tile_start += TILE_SIZE) {
        int tile_end = std::min(tile_start + TILE_SIZE, end);

        for (int i = tile_start; i < tile_end; ++i) {
            RankType rank_sum = 0.0f;
            auto incoming = graph.get_incoming_edges(i);

            for (int source : incoming) {
                // Hardware prefetch to minimize stalls
                __builtin_prefetch(&current_rank[source + 16], 0, 1);
                
                // Fast multiplication instead of heavy division
                rank_sum += current_rank[source] * graph.get_inv_out_degree(source);
            }

            next_rank[i] = damping_val + df * (rank_sum + dead_end_contrib);
            local_error += std::abs(static_cast<double>(next_rank[i] - current_rank[i]));
        }
    }

    global_error.fetch_add(local_error, std::memory_order_relaxed);
}

std::vector<double> PageRankEngine::solve() {
    int n = graph.get_num_nodes();
    if (n <= 0) return {};

    int n_threads = pool.get_num_threads();
    int chunk_size = (n + n_threads - 1) / n_threads;
    
    // Synchronization barrier for iteration phases
    std::barrier sync_point(n_threads + 1); 

    for (int iter = 0; iter < config.max_iterations; ++iter) {
        std::atomic<double> global_error{0.0};

        // Accumulate dead-end contribution
        double dead_end_sum = 0.0;
        for (int node : dead_ends) {
            dead_end_sum += current_rank[node];
        }
        
        RankType dead_end_contrib = static_cast<RankType>(dead_end_sum / n);
        RankType damping_val = static_cast<RankType>((1.0 - config.damping_factor) / n);

        // Dispatch tasks to the pool
        for (int t = 0; t < n_threads; ++t) {
            int start = t * chunk_size;
            int end = std::min(start + chunk_size, n);
            
            pool.enqueue([this, start, end, damping_val, dead_end_contrib, &global_error, &sync_point]() {
                this->computeChunk(start, end, damping_val, dead_end_contrib, global_error);
                sync_point.arrive_and_wait(); 
            });
        }

        sync_point.arrive_and_wait(); 

        // Check for convergence
        double total_err = global_error.load();
        if (total_err < config.epsilon) {
            std::cout << "[Convergence] Iteration " << iter << " - Error: " << total_err << std::endl;
            break;
        }

        std::swap(current_rank, next_rank);
    }

    return std::vector<double>(current_rank.begin(), current_rank.end());
}