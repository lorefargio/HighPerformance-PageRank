#include "../include/ResultExporter.hpp"
#include <algorithm>
#include <iomanip>

void ResultsExporter::exportTopK(const std::vector<double>& ranks, int k, std::ostream& out) {
    int n = static_cast<int>(ranks.size());
    std::vector<NodeRank> nodes(n);

    for (int i = 0; i < n; ++i) {
        nodes[i] = {i, ranks[i]};
    }

    int actual_k = std::min(k, n);

    // O(N log K) efficiency using partial sort
    std::partial_sort(nodes.begin(), nodes.begin() + actual_k, nodes.end(),
                      [](const NodeRank& a, const NodeRank& b) {
                          return a.rank > b.rank;
                      });

    out << "\n--- TOP " << actual_k << " NODES ---\n";
    out << std::fixed << std::setprecision(6);
    for (int i = 0; i < actual_k; ++i) {
        out << "Rank " << i + 1 << ": Node " << nodes[i].id 
            << " (Value: " << nodes[i].rank << ")\n";
    }
}