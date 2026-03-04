#pragma once
#include <vector>
#include <span> 
#include "Common.hpp"

/**
 * @brief Immutable container for graph data using Compressed Sparse Row (CSR) format.
 * * Stores the graph structure in a memory-efficient way to maximize cache hits.
 * RankType and AlignedAllocator are used for high-performance memory access.
 */
class GraphData {
private:
    AlignedRankVector inv_out_degrees; /**< Pre-calculated 1.0f / out_degree */
    std::vector<int> in_edges;         /**< Flattened array of source nodes for incoming edges */
    std::vector<int> in_offsets;       /**< Offsets into the in_edges array (CSR structure) */
    int num_nodes;

public:
    /**
     * @brief Constructs a GraphData object.
     * * @param nodes Total number of nodes.
     * @param out_deg Vector of out-degrees for each node.
     * @param edges Vector of source nodes for all edges.
     * @param offsets CSR offsets.
     */
    inline GraphData(int nodes, std::vector<int> out_deg, std::vector<int> edges, std::vector<int> offsets) 
        : in_edges(std::move(edges)), in_offsets(std::move(offsets)), num_nodes(nodes) {
        
        inv_out_degrees.resize(nodes);
        for(int i = 0; i < nodes; ++i) {
            // Store 1/degree to replace expensive division with fast multiplication
            inv_out_degrees[i] = (out_deg[i] > 0) ? (1.0f / static_cast<float>(out_deg[i])) : 0.0f;
        }
    }

    /** @return Total number of nodes in the graph. */
    int get_num_nodes() const { return num_nodes; }

    /** * @brief Gets the pre-calculated inverse out-degree.
     * @param node The node ID.
     * @return The 1.0f/degree value.
     */
    RankType get_inv_out_degree(int node) const { return inv_out_degrees[node]; }

    /**
     * @brief Provides a zero-copy view of incoming edges for a specific node.
     * @param node The target node ID.
     * @return A C++20 span of source node IDs.
     */
    std::span<const int> get_incoming_edges(int node) const {
        return {&in_edges[in_offsets[node]], &in_edges[in_offsets[node+1]]};
    }
};