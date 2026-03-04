#pragma once
#include "GraphData.hpp"
#include "ThreadPool.hpp"
#include "Common.hpp"
#include <vector>
#include <atomic>
#include <barrier>

/**
 * @brief Motore di calcolo del PageRank ottimizzato per la cache.
 * Utilizza RankType (float) e AlignedVector per massimizzare la banda di memoria.
 */
class PageRankEngine {
private:
    const GraphData& graph;
    ThreadPool& pool;
    const AppConfig& config;

    // Utilizziamo i vettori allineati a 64 byte definiti in Common.hpp
    AlignedRankVector current_rank;
    AlignedRankVector next_rank;
    
    std::vector<int> dead_ends;

    /**
     * @brief Funzione worker con firma sincronizzata (usa RankType).
     */
    void computeChunk(int start_node, int end_node, RankType damping_value, 
                      RankType dead_end_contribution, std::atomic<double>& global_error);

public:
    PageRankEngine(const GraphData& g, ThreadPool& p, const AppConfig& c);
    
    /**
     * @brief Risoluzione iterativa con swap dei puntatori.
     */
    std::vector<double> solve();
};