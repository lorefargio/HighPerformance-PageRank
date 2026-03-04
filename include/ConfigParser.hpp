#pragma once
#include "Common.hpp"

/**
 * @brief Utility class for parsing and validating command-line arguments.
 */
class ConfigParser {
public:
    /**
     * @brief Parses argc/argv and returns a validated AppConfig object.
     * @param argc Argument count.
     * @param argv Argument vector.
     * @return Validated configuration.
     * @throws std::invalid_argument If parameters are missing or malformed.
     */
    static AppConfig parse(int argc, char** argv);

private:
    /** @brief Displays usage instructions to the user. */
    static void printUsage(const char* progName);
};