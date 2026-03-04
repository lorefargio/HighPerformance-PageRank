#include "../include/ConfigParser.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <filesystem> // Added for file validation

namespace fs = std::filesystem;

void ConfigParser::printUsage(const char* progName) {
    std::cerr << "Usage: " << progName << " <file.mtx> [options]\n"
              << "Options:\n"
              << "  --threads <n>       Number of worker threads (default: 3)\n"
              << "  --damping <f>       Damping factor (default: 0.9)\n"
              << "  --epsilon <e>       Error tolerance (default: 1e-7)\n"
              << "  --max-iter <n>      Iteration limit (default: 100)\n"
              << "  --top-k <k>         Number of results to display (default: 3)\n"
              << "  --test              Enable benchmark mode (runs from 1 to N threads)\n";
}

AppConfig ConfigParser::parse(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        throw std::invalid_argument("Missing file path.");
    }

    std::string filepath = argv[1];
    
    // Check if the file exists before proceeding
    if (!fs::exists(filepath)) {
        throw std::invalid_argument("The file '" + filepath + "' does not exist.");
    }

    int threads = 3;
    double damping = 0.9;
    double epsilon = 1e-7;
    int max_iter = 100;
    int top_k = 3;
    bool test_mode = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--threads" && i + 1 < argc) {
            threads = std::stoi(argv[++i]);
        } else if (arg == "--damping" && i + 1 < argc) {
            damping = std::stod(argv[++i]);
        } else if (arg == "--epsilon" && i + 1 < argc) {
            epsilon = std::stod(argv[++i]);
        } else if (arg == "--max-iter" && i + 1 < argc) {
            max_iter = std::stoi(argv[++i]);
        } else if (arg == "--top-k" && i + 1 < argc) {
            top_k = std::stoi(argv[++i]);
        } else if (arg == "--test") {
            test_mode = true;
        }
    }

    if (threads <= 0) throw std::invalid_argument("Thread count must be > 0.");
    if (damping <= 0.0 || damping >= 1.0) throw std::invalid_argument("Damping factor must be between 0 and 1.");
    if (epsilon <= 0.0) throw std::invalid_argument("Epsilon must be positive.");

    return AppConfig{filepath, damping, epsilon, threads, max_iter, top_k, test_mode};
}