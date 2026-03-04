# High-Performance PageRank (C++20)

A high-performance, multi-threaded implementation of the PageRank algorithm optimized for modern CPU architectures. This project demonstrates advanced C++20 techniques, parallel I/O patterns, and low-level memory optimizations to overcome the **Memory Wall** in large-scale graph processing.



## Overview

**PageRank** is a link-analysis algorithm that assigns a numerical weighting to each element of a hyperlinked set of documents, measuring its relative importance within the set.

This implementation focuses on **efficiency** and **scalability**, transitioning from legacy C-style code to a modern, cache-aware C++20 architecture.

---

## Project Structure

* `/include`: Header files (`.hpp`) containing class definitions.
* `/src`: Source files (`.cpp`) with optimized implementations.
* `/docs`: In-depth technical documentation:
    * [Graph Construction & Parallel I/O](docs/GRAPH_CONSTRUCTION.md)
    * [PageRank Engine & Hardware Tuning](docs/PAGERANK_ENGINE.md)
    * [Metrics & Results Interpretation](docs/RESULTS_AND_METRICS.md)
* `/test_data`: Sample `.mtx` files for testing.
* `/scripts`: Python tools for performance visualization.

---

## Key Architectural Features

### 1. Parallel I/O & Graph Construction
The `GraphBuilder` uses a **Producer-Consumer** pattern with `mmap` for zero-copy file reading. This allows the system to parse millions of edges in parallel, bypassing standard `std::ifstream` bottlenecks.



### 2. Cache-Aware Memory Tiling
PageRank is a **Memory-Bound** algorithm. We implement **Memory Tiling** to ensure that the working set of ranks remains within the **L1/L2 cache**, significantly reducing expensive RAM stalls.



### 3. Hardware-Level Tuning
* **Aligned Memory**: Custom `AlignedAllocator` (64-byte) to match CPU cache lines and prevent **False Sharing**.
* **Float Density**: Uses 32-bit `float` to double data density in cache compared to 64-bit doubles.
* **CSR Format**: Stores the graph in a contiguous memory block to maximize spatial locality.

---

## Execution & Benchmarking

### Standard Mode
```bash
./pagerank test_data/web-Stanford.mtx --threads 4
```

## Scalability Benchmarking

This project includes an integrated benchmarking suite to measure speedup and efficiency.

### 1. Run the Test
Execute PageRank with the `--test` flag. The program will automatically profile executions from 1 to N threads and save a CSV report in the `/benchmarks` folder at the project root.
```bash
./pagerank test_data/web-Stanford.mtx --threads 6 --test
```

### 2. Data Visualization (Python)

To generate performance charts, you need Python 3 installed. We recommend using a virtual environment to manage dependencies:

```bash
# 1. Navigate to project root and create a virtual environment
python3 -m venv venv

# 2. Activate the environment
source venv/bin/activate

# 3. Install required libraries
pip install pandas matplotlib

# 4. Run the visualization script
python3 scripts/plot_performance.py

```

The script will generate a high-resolution chart: `benchmarks/performance_report.png`.

---

## Performance Monitoring with `perf`

Analyze hardware-level stalls and cache efficiency:

```bash
# Unlock hardware counters
sudo sysctl -w kernel.perf_event_paranoid=0

# Run profiler
perf stat -e cycles,instructions,cache-references,cache-misses,L1-dcache-load-misses ./pagerank graph.mtx --threads 4
```

---

## Build Instructions

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

---

## License

This project is released under the **MIT License**.


