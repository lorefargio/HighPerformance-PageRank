#pragma once
#include "GraphData.hpp"
#include "ThreadPool.hpp"
#include "Common.hpp"
#include <vector>
#include <atomic>
#include <barrier>

/**
 * @brief Cache-optimized PageRank calculation engine.
 * Implements memory tiling and uses aligned float vectors for maximum bandwidth.
 */
class PageRankEngine {
private:
    const GraphData& graph;
    ThreadPool& pool;
    const AppConfig& config;

    AlignedRankVector current_rank; /**< Current iteration ranks */
    AlignedRankVector next_rank;    /**< Ranks being calculated */
    
    std::vector<int> dead_ends;     /**< Nodes with no outgoing edges */

    /**
     * @brief Worker function to process a specific chunk of nodes.
     * @param start_node Beginning of the range.
     * @param end_node End of the range.
     * @param damping_value Constant damping contribution.
     * @param dead_end_contribution Contribution from dead-end nodes.
     * @param global_error Atomic accumulator for convergence error.
     */
    void computeChunk(int start_node, int end_node, RankType damping_value, 
                      RankType dead_end_contribution, std::atomic<double>& global_error);

public:
    /** @brief Initializes the engine and identifies dead-end nodes. */
    PageRankEngine(const GraphData& g, ThreadPool& p, const AppConfig& c);
    
    /**
     * @brief Iterative solver that runs until convergence or max iterations.
     * @return Final PageRank values as a vector of doubles.
     */
    std::vector<double> solve();
};