#pragma once
#include <vector>
#include <iostream>

/**
 * @brief Associa un nodo al suo rank per l'ordinamento.
 */
struct NodeRank {
    int id;
    double rank;
};

class ResultsExporter {
public:
    /**
     * @brief Esporta i Top K nodi con il rank più alto.
     * Utilizza std::partial_sort per un'efficienza O(N log K).
     */
    void exportTopK(const std::vector<double>& ranks, int k, std::ostream& out);
};