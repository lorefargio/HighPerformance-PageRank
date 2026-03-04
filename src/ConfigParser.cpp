#include "../include/ConfigParser.hpp"
#include <iostream>
#include <stdexcept>
#include <string>

void ConfigParser::printUsage(const char* progName) {
    std::cerr << "Utilizzo: " << progName << " <file.mtx> [opzioni]\n"
              << "Opzioni:\n"
              << "  --threads <n>       Numero di thread worker (default: 3) [cite: 9]\n"
              << "  --damping <f>       Damping factor (default: 0.9) [cite: 9]\n"
              << "  --epsilon <e>       Tolleranza errore (default: 1e-7) [cite: 9]\n"
              << "  --max-iter <n>      Limite iterazioni (default: 100)\n"
              << "  --top-k <k>         Numero di risultati da mostrare (default: 10)\n";
}

AppConfig ConfigParser::parse(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        throw std::invalid_argument("Percorso file mancante.");
    }

    std::string filepath = argv[1];
    int threads = 3;
    double damping = 0.9;
    double epsilon = 1e-7;
    int max_iter = 100;
    int top_k = 3;

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
        }
    }

    // Validazione dei parametri 
    if (threads <= 0) throw std::invalid_argument("Il numero di thread deve essere > 0.");
    if (damping <= 0.0 || damping >= 1.0) throw std::invalid_argument("Damping factor deve essere tra 0 e 1.");
    if (epsilon <= 0.0) throw std::invalid_argument("Epsilon deve essere positivo.");

    // Popola e restituisce la struttura dati immutabile 
    return AppConfig{
        filepath,
        damping,
        epsilon,
        threads,
        max_iter,
        top_k
    };
}