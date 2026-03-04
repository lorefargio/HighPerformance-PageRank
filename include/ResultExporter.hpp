#pragma once
#include <vector>
#include <iostream>

/** @brief Helper structure mapping a node ID to its PageRank score. */
struct NodeRank {
    int id;
    double rank;
};

/**
 * @brief Exports top-performing nodes to various streams.
 */
class ResultsExporter {
public:
    /**
     * @brief Sorts and prints the top K nodes by rank.
     * Uses std::partial_sort for O(N log K) efficiency.
     */
    void exportTopK(const std::vector<double>& ranks, int k, std::ostream& out);
};