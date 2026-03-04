#include "../include/PageRankEngine.hpp"
#include <cmath>
#include <numeric>
#include <iostream>
#include <algorithm>

/**
 * @brief Costruttore: Inizializza i vettori allineati e identifica i dead-ends.
 */
PageRankEngine::PageRankEngine(const GraphData& g, ThreadPool& p, const AppConfig& c)
    : graph(g), pool(p), config(c) {
    
    int n = graph.get_num_nodes();
    
    // Inizializzazione Rank (usando AlignedVector<float> definito in Common.hpp)
    current_rank.assign(n, 1.0f / n); 
    next_rank.resize(n, 0.0f);

    // Identificazione preventiva dei nodi senza archi uscenti
    // Usiamo l'inverso del grado pre-calcolato in GraphData
    for (int i = 0; i < n; ++i) {
        if (graph.get_inv_out_degree(i) == 0.0f) {
            dead_ends.push_back(i);
        }
    }
}

/**
 * @brief Calcola un blocco di nodi usando il Memory Tiling.
 * Questa tecnica mantiene i dati nella cache L1/L2 più a lungo.
 */
void PageRankEngine::computeChunk(int start, int end, RankType damping_val, 
                                 RankType dead_end_contrib, std::atomic<double>& global_error) {
    double local_error = 0.0;
    const RankType df = static_cast<RankType>(config.damping_factor);
    
    // TILE_SIZE: 512 nodi (circa 2KB, entra perfettamente in L1)
    const int TILE_SIZE = 512; 

    for (int tile_start = start; tile_start < end; tile_start += TILE_SIZE) {
        int tile_end = std::min(tile_start + TILE_SIZE, end);

        for (int i = tile_start; i < tile_end; ++i) {
            RankType rank_sum = 0.0f;
            auto incoming = graph.get_incoming_edges(i);

            // Ottimizzazione: accesso alla memoria con prefetch hardware
            for (int source : incoming) {
                // Pre-carica i dati del rank del nodo sorgente per l'iterazione successiva
                __builtin_prefetch(&current_rank[source + 16], 0, 1);
                
                // Moltiplicazione per l'inverso (molto più veloce della divisione)
                rank_sum += current_rank[source] * graph.get_inv_out_degree(source);
            }

            // Formula PageRank standard
            next_rank[i] = damping_val + df * (rank_sum + dead_end_contrib);
            
            // Accumulo dell'errore (usiamo double per la precisione temporanea)
            local_error += std::abs(static_cast<double>(next_rank[i] - current_rank[i]));
        }
    }

    // Aggiornamento atomico dell'errore globale una sola volta per chunk
    global_error.fetch_add(local_error, std::memory_order_relaxed);
}

/**
 * @brief Loop principale di risoluzione.
 */
std::vector<double> PageRankEngine::solve() {
    int n = graph.get_num_nodes();
    if (n <= 0) return {};

    int n_threads = pool.get_num_threads();
    int chunk_size = (n + n_threads - 1) / n_threads;
    
    // Sincronizzazione tramite barriera C++20
    std::barrier sync_point(n_threads + 1); 

    for (int iter = 0; iter < config.max_iterations; ++iter) {
        std::atomic<double> global_error{0.0};

        // 1. Calcolo del contributo dei nodi pozzo (dead-ends)
        double dead_end_sum = 0.0;
        for (int node : dead_ends) {
            dead_end_sum += current_rank[node];
        }
        
        // Costanti per questa iterazione
        RankType dead_end_contrib = static_cast<RankType>(dead_end_sum / n);
        RankType damping_val = static_cast<RankType>((1.0 - config.damping_factor) / n);

        // 2. Distribuzione del lavoro al pool di thread
        for (int t = 0; t < n_threads; ++t) {
            int start = t * chunk_size;
            int end = std::min(start + chunk_size, n);
            
            pool.enqueue([this, start, end, damping_val, dead_end_contrib, &global_error, &sync_point]() {
                this->computeChunk(start, end, damping_val, dead_end_contrib, global_error);
                sync_point.arrive_and_wait(); 
            });
        }

        // 3. Il thread principale attende la fine del calcolo parallelo
        sync_point.arrive_and_wait(); 

        // 4. Verifica convergenza
        double total_err = global_error.load();
        if (total_err < config.epsilon) {
            std::cout << "[Convergenza] Iterazione " << iter << " - Errore: " << total_err << std::endl;
            break;
        }

        // 5. Swap dei vettori per la prossima iterazione (Zero-copy)
        std::swap(current_rank, next_rank);
    }

    // Conversione finale da float (AlignedVector) a double (std::vector standard) per l'output
    return std::vector<double>(current_rank.begin(), current_rank.end());
}