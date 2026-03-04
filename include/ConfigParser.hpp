#pragma once
#include "Common.hpp"
#include <vector>

/**
 * @brief Classe dedicata al parsing e alla validazione degli argomenti da riga di comando[cite: 8, 45].
 */
class ConfigParser {
public:
    /**
     * @brief Analizza argc/argv e restituisce una configurazione valida[cite: 8].
     * @throws std::invalid_argument se i parametri non sono validi.
     */
    static AppConfig parse(int argc, char** argv);

private:
    static void printUsage(const char* progName);
};