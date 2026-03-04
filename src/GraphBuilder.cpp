#include "../include/GraphBuilder.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cctype> // per isdigit

void GraphBuilder::producerWorker() {
    int fd = open(filepath.c_str(), O_RDONLY);
    if (fd == -1) {
        chunk_queue.set_finished(); // Mai lasciare i consumatori appesi
        return;
    }

    struct stat st;
    if (fstat(fd, &st) == -1 || st.st_size == 0) {
        close(fd);
        chunk_queue.set_finished();
        return;
    }

    char* data = static_cast<char*>(mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
    if (data == MAP_FAILED) {
        close(fd);
        chunk_queue.set_finished();
        return;
    }

    char* ptr = data;
    char* end = data + st.st_size;
    bool header_skipped = false;
    
    std::vector<std::pair<int, int>> current_chunk;
    current_chunk.reserve(1024); 

    try {
        while (ptr < end) {
            // 1. Salta spazi bianchi, ritorni a capo e commenti
            while (ptr < end && (std::isspace(*ptr) || *ptr == '%' || *ptr == '#')) {
                if (*ptr == '%' || *ptr == '#') {
                    // Salta tutta la riga di commento
                    while (ptr < end && *ptr != '\n') ptr++;
                } else {
                    ptr++;
                }
            }

            if (ptr >= end) break;

            // 2. Parsing robusto del primo intero (Sorgente)
            int v1 = 0;
            if (ptr < end && std::isdigit(*ptr)) {
                while (ptr < end && std::isdigit(*ptr)) {
                    v1 = v1 * 10 + (*ptr++ - '0');
                }
            } else {
                // Se non è un numero, avanziamo per evitare loop infiniti
                if (ptr < end) ptr++;
                continue;
            }

            // 3. Salta separatori tra i due numeri
            while (ptr < end && !std::isdigit(*ptr) && *ptr != '\n' && *ptr != '%' && *ptr != '#') ptr++;

            // 4. Parsing del secondo intero (Destinazione)
            int v2 = 0;
            if (ptr < end && std::isdigit(*ptr)) {
                while (ptr < end && std::isdigit(*ptr)) {
                    v2 = v2 * 10 + (*ptr++ - '0');
                }
            } else {
                continue;
            }

            // 5. Gestione Header MTX (Righe Colonne Archi)
            if (!header_skipped) {
                header_skipped = true;
                continue; 
            }

            // 6. Aggiunta al chunk [cite: 102]
            current_chunk.emplace_back(v1, v2);

            if (current_chunk.size() >= 1024) {
                chunk_queue.push(std::move(current_chunk)); 
                current_chunk.clear();
                current_chunk.reserve(1024);
            }
        }
    } catch (...) {
        // In caso di errore imprevisto, assicuriamo la chiusura della coda
    }

    if (!current_chunk.empty()) {
        chunk_queue.push(std::move(current_chunk));
    }

    chunk_queue.set_finished(); 
    munmap(data, st.st_size);
    close(fd);
}

void GraphBuilder::consumerWorker(LocalGraphState& state) {
    while (auto chunk = chunk_queue.pop()) {
        for (auto& [src, dest] : *chunk) {
            // Normalizzazione: MTX parte da 1, C++ da 0
            int s = src - 1;
            int d = dest - 1;

            int max_id = std::max(s, d);
            if (max_id >= (int)state.local_in_adj.size()) {
                state.local_in_adj.resize(max_id + 1);
                state.local_out_degrees.resize(max_id + 1, 0);
            }

            state.local_in_adj[d].push_back(s); // Archi entranti per CSR [cite: 80, 86]
            state.local_out_degrees[s]++;       // Grado uscente per nodi dead-end 
            state.max_node = std::max(state.max_node, max_id);
        }
    }
}

GraphData GraphBuilder::build() {
    std::vector<LocalGraphState> local_states(num_workers);
    std::vector<std::thread> consumers;

    // 1. Avvio Produttore e Consumatori [cite: 99]
    std::thread producer(&GraphBuilder::producerWorker, this);
    for (int i = 0; i < num_workers; ++i) {
        consumers.emplace_back(&GraphBuilder::consumerWorker, this, std::ref(local_states[i]));
    }
    producer.join();
    for (auto& t : consumers) t.join();

    // 2. Determinazione dimensione globale 
    int total_nodes = 0;
    for (const auto& s : local_states) total_nodes = std::max(total_nodes, s.max_node + 1);

    if (total_nodes == 0) return GraphData(0, {}, {}, {0});

    // 3. Calcolo Out-Degrees e Conteggi Locali
    std::vector<int> final_out_degrees(total_nodes, 0);
    std::vector<std::vector<int>> thread_node_counts(num_workers, std::vector<int>(total_nodes, 0));

    for (int i = 0; i < num_workers; ++i) {
        // Unisci out-degrees
        for (int j = 0; j < (int)local_states[i].local_out_degrees.size(); ++j) {
            final_out_degrees[j] += local_states[i].local_out_degrees[j];
        }
        // Registra quanti archi entranti ha ogni thread per ogni nodo
        for (int j = 0; j < (int)local_states[i].local_in_adj.size(); ++j) {
            thread_node_counts[i][j] = (int)local_states[i].local_in_adj[j].size();
        }
    }

    // 4. Costruzione in_offsets (CSR Format) [cite: 80, 87]
    std::vector<int> in_offsets(total_nodes + 1, 0);
    size_t current_offset = 0;
    for (int j = 0; j < total_nodes; ++j) {
        in_offsets[j] = static_cast<int>(current_offset);
        for (int i = 0; i < num_workers; ++i) {
            current_offset += thread_node_counts[i][j];
        }
    }
    in_offsets[total_nodes] = static_cast<int>(current_offset);

    // 5. Merge Parallelo con controlli di sicurezza
    std::vector<int> final_in_edges(current_offset);
    std::vector<std::thread> merger_threads;

    for (int i = 0; i < num_workers; ++i) {
        merger_threads.emplace_back([&, i]() {
            for (int j = 0; j < total_nodes; ++j) {
                // Calcolo posizione di scrittura
                int write_idx = in_offsets[j];
                for (int prev_thread = 0; prev_thread < i; ++prev_thread) {
                    write_idx += thread_node_counts[prev_thread][j];
                }
                
                // SICUREZZA: Verifica se il thread 'i' ha visto il nodo 'j'
                if (j < (int)local_states[i].local_in_adj.size()) {
                    const auto& local_adj = local_states[i].local_in_adj[j];
                    if (!local_adj.empty()) {
                        std::copy(local_adj.begin(), local_adj.end(), final_in_edges.begin() + write_idx);
                    }
                }
            }
        });
    }

    for (auto& t : merger_threads) t.join();

    // Restituisce il contenitore immutabile GraphData [cite: 19, 56, 83]
    return GraphData(total_nodes, std::move(final_out_degrees), 
                     std::move(final_in_edges), std::move(in_offsets));
}