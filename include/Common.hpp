#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdlib>

/**
 * @brief Immutable configuration structure for the application.
 * Encapsulates execution parameters, replacing legacy global variables.
 */
struct AppConfig {
    const std::string filepath;     /**< Path to the .mtx graph file */
    const double damping_factor;    /**< PageRank damping factor (usually 0.85) */
    const double epsilon;           /**< Convergence threshold */
    const int num_threads;          /**< Number of worker threads */
    const int max_iterations;       /**< Maximum allowed iterations */
    const int top_k;                /**< Number of top nodes to display */
    const bool test_mode;
};

/** @brief Optimized data type for memory bandwidth (float for higher density) */
using RankType = float; 

/**
 * @brief A 64-byte aligned allocator to prevent False Sharing.
 * Essential for multi-threaded performance on modern CPU cache lines.
 * @tparam T The type of the elements to allocate.
 */
template <typename T>
struct AlignedAllocator {
    using value_type = T;

    AlignedAllocator() = default;
    template <class U> AlignedAllocator(const AlignedAllocator<U>&) noexcept {}

    /**
     * @brief Allocates aligned memory.
     * @param n Number of elements to allocate.
     * @return Pointer to the allocated memory.
     */
    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        if (n > std::size_t(-1) / sizeof(T)) throw std::bad_alloc();
        
        // Ensure size is a multiple of alignment (64 bytes)
        std::size_t size = (n * sizeof(T) + 63) & ~std::size_t(63);
        void* p = std::aligned_alloc(64, size);
        
        if (!p) throw std::bad_alloc();
        return static_cast<T*>(p);
    }

    void deallocate(T* p, std::size_t) noexcept {
        std::free(p);
    }

    /** @brief Allocator equality check (stateless allocators are always equal) */
    template <typename U>
    bool operator==(const AlignedAllocator<U>&) const noexcept { return true; }

    /** @brief Allocator inequality check */
    template <typename U>
    bool operator!=(const AlignedAllocator<U>&) const noexcept { return false; }
};

/** @brief Vector type using 64-byte alignment for PageRank values */
using AlignedRankVector = std::vector<RankType, AlignedAllocator<RankType>>;