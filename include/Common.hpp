#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cmath>

/**
 * @brief Struttura dati immutabile per la configurazione dell'applicazione.
 * Sostituisce le variabili globali del vecchio codice C[cite: 10].
 */
struct AppConfig {
    const std::string filepath;
    const double damping_factor;
    const double epsilon;
    const int num_threads;
    const int max_iterations;
    const int top_k;
};

// Tipo di dato ottimizzato per la banda di memoria
using RankType = float; 

/**
 * @brief Allocatore allineato a 64 byte per evitare False Sharing.
 * Implementa gli operatori di confronto richiesti dallo standard C++.
 */
template <typename T>
struct AlignedAllocator {
    using value_type = T;

    AlignedAllocator() = default;
    template <class U> AlignedAllocator(const AlignedAllocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        if (n > std::size_t(-1) / sizeof(T)) throw std::bad_alloc();
        
        // Aligned alloc richiede che la dimensione sia multiplo dell'allineamento
        std::size_t size = (n * sizeof(T) + 63) & ~std::size_t(63);
        void* p = std::aligned_alloc(64, size);
        
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    }

    void deallocate(T* p, std::size_t) noexcept {
        std::free(p);
    }

    // --- OPERATORI DI CONFRONTO OBBLIGATORI ---
    // Due allocatori stateless sono sempre uguali
    template <typename U>
    bool operator==(const AlignedAllocator<U>&) const noexcept { return true; }

    template <typename U>
    bool operator!=(const AlignedAllocator<U>&) const noexcept { return false; }
};

using AlignedRankVector = std::vector<RankType, AlignedAllocator<RankType>>;