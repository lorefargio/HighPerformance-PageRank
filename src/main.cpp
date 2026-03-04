#include "../include/ConfigParser.hpp"
#include "../include/GraphBuilder.hpp"
#include "../include/PageRankEngine.hpp"
#include "../include/ResultExporter.hpp"
#include "../include/ThreadPool.hpp"
#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include <fstream>
#include <filesystem> 

namespace fs = std::filesystem;

struct TestMetrics {
    int threads;
    double io_time;
    double calc_time;
};

int main(int argc, char** argv) {
    try {
        AppConfig config = ConfigParser::parse(argc, argv);
        std::vector<TestMetrics> all_stats;

        std::cout << "========================================================\n";
        std::cout << (config.test_mode ? "   BENCHMARK MODE - PAGERANK C++20" : "   SINGLE RUN MODE - PAGERANK C++20") << "\n";
        std::cout << "========================================================\n";
        std::cout << "File: " << config.filepath << "\n\n";

        int start_t = config.test_mode ? 1 : config.num_threads;
        int end_t = config.num_threads;

        for (int t = start_t; t <= end_t; ++t) {
            if (config.test_mode) std::cout << "--> Testing with " << t << " thread(s)..." << std::endl;

            GraphBuilder builder(config.filepath, t);
            GraphData graph = builder.build();

            auto start_io = std::chrono::high_resolution_clock::now();
            auto end_io = std::chrono::high_resolution_clock::now();
            double io_elapsed = std::chrono::duration<double>(end_io - start_io).count(); 

            ThreadPool pool(t);
            PageRankEngine engine(graph, pool, config);

            auto start_calc = std::chrono::high_resolution_clock::now();
            std::vector<double> results = engine.solve();
            auto end_calc = std::chrono::high_resolution_clock::now();
            double calc_elapsed = std::chrono::duration<double>(end_calc - start_calc).count();

            all_stats.push_back({t, 0.0, calc_elapsed}); // Focalizziamoci sul tempo di calcolo

            if (t == end_t) {
                ResultsExporter exporter;
                exporter.exportTopK(results, config.top_k, std::cout);
            }
        }


        if (config.test_mode) {
            // Navigate to the parent directory (project root) from the build folder
            fs::path project_root = fs::current_path().parent_path();
            fs::path benchmark_dir = project_root / "benchmarks";
            
            fs::create_directory(benchmark_dir);
            
            std::string out_file = (benchmark_dir / "last_run.csv").string();
            std::ofstream ofs(out_file);
            
            ofs << "threads,calc_time,speedup,efficiency\n";
            double t1_time = all_stats[0].calc_time;

            for (const auto& s : all_stats) {
                double speedup = t1_time / s.calc_time;
                double efficiency = (speedup / s.threads) * 100.0;
                ofs << s.threads << "," << s.calc_time << "," << speedup << "," << efficiency << "\n";
            }
            std::cout << "\n[Success] Benchmark results saved to: " << out_file << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Critical Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}