#include "../include/ConfigParser.hpp"
#include "../include/GraphBuilder.hpp"
#include "../include/PageRankEngine.hpp"
#include "../include/ResultExporter.hpp"
#include "../include/ThreadPool.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include <cmath>

struct TestMetrics {
    int threads;
    double io_time;
    double calc_time;
    double total_time;
};

int main(int argc, char** argv) {
    try {
        // 1. Parsing della configurazione [cite: 33, 70]
        if (argc < 2) {
            std::cerr << "Utilizzo: " << argv[0] << " <file.mtx>\n";
            return 1;
        }

        AppConfig config = ConfigParser::parse(argc, argv);
        int max_threads = config.num_threads;
        std::vector<TestMetrics> all_stats;

        std::cout << "========================================================\n";
        std::cout << "      BENCHMARK PAGERANK - REFACTORING C++20           \n";
        std::cout << "========================================================\n";
        std::cout << "File: " << config.filepath << "\n";
        std::cout << "Max Threads: " << max_threads << "\n\n";

        for (int t = max_threads; t <= max_threads; ++t) {
            std::cout << "--> Esecuzione con " << t << (t == 1 ? " thread..." : " thread...") << std::endl;

            // 2. Costruzione del Grafo (I/O e Sincronizzazione) [cite: 11, 48, 78]
            auto start_io = std::chrono::high_resolution_clock::now();
            GraphBuilder builder(config.filepath, t);
            GraphData graph = builder.build();
            auto end_io = std::chrono::high_resolution_clock::now();
            double io_elapsed = std::chrono::duration<double>(end_io - start_io).count();

            // 3. Setup Calcolo Concorrente [cite: 21, 58, 108]
            ThreadPool pool(t);
            PageRankEngine engine(graph, pool, config);

            auto start_calc = std::chrono::high_resolution_clock::now();
            std::vector<double> results = engine.solve(); // [cite: 38, 75]
            auto end_calc = std::chrono::high_resolution_clock::now();
            double calc_elapsed = std::chrono::duration<double>(end_calc - start_calc).count();

            all_stats.push_back({t, io_elapsed, calc_elapsed, io_elapsed + calc_elapsed});

            // Mostriamo solo i risultati del primo thread per brevità o se è l'ultima run
            if (t == max_threads) {
                ResultsExporter exporter;
                exporter.exportTopK(results, config.top_k, std::cout); // [cite: 40, 77, 146]
            }
        }

        // --- STAMPA DELLE STATISTICHE E CORRELAZIONI ---
        std::cout << "\n============================================================================\n";
        std::cout << std::left << std::setw(10) << "Threads" 
                  << std::setw(15) << "I/O (s)" 
                  << std::setw(15) << "Calc (s)" 
                  << std::setw(15) << "Speedup" 
                  << std::setw(15) << "Efficienza" << "\n";
        std::cout << "----------------------------------------------------------------------------\n";

        double t1_calc = all_stats[0].calc_time;

        for (const auto& s : all_stats) {
            // Speedup S = T1 / Tn 
            double speedup = t1_calc / s.calc_time;
            // Efficienza E = S / n
            double efficiency = speedup / s.threads;

            std::cout << std::left << std::setw(10) << s.threads 
                      << std::fixed << std::setprecision(4)
                      << std::setw(15) << s.io_time 
                      << std::setw(15) << s.calc_time 
                      << std::setw(15) << speedup 
                      << std::setw(15) << (efficiency * 100.0) << "%" << "\n";
        }
        std::cout << "============================================================================\n";
        std::cout << "Nota: Lo speedup ideale e' lineare. Deviazioni indicano limiti di memoria (Memory Wall).\n";

    } catch (const std::exception& e) {
        std::cerr << "Errore critico: " << e.what() << "\n";
        return 1;
    }

    return 0;
}