#pragma once
#include <vector>
#include <span> // C++20 per viste sicure e veloci [cite: 91]
#include "Common.hpp"

class GraphData {
private:
    AlignedRankVector inv_out_degrees; // Memorizziamo 1.0f / grado
    std::vector<int> in_edges;    
    std::vector<int> in_offsets;  
    int num_nodes;

public:
    GraphData(int nodes, std::vector<int> out_deg, std::vector<int> edges, std::vector<int> offsets) 
        : in_edges(std::move(edges)), in_offsets(std::move(offsets)), num_nodes(nodes) {
        
        inv_out_degrees.resize(nodes);
        for(int i = 0; i < nodes; ++i) {
            // Se il grado è 0 (dead-end), mettiamo 0 per evitare divisioni per zero
            inv_out_degrees[i] = (out_deg[i] > 0) ? (1.0f / out_deg[i]) : 0.0f;
        }
    }

    // Restituisce l'inverso pre-calcolato
    RankType get_inv_out_degree(int node) const { return inv_out_degrees[node]; }
    std::span<const int> get_incoming_edges(int node) const {
        return {&in_edges[in_offsets[node]], &in_edges[in_offsets[node+1]]};
    }
    int get_num_nodes() const { return num_nodes; }
};