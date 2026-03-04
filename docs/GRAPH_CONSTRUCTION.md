# Graph Construction & Parallel I/O

The `GraphBuilder` class is responsible for converting raw Matrix Market (`.mtx`) files into a high-performance **Compressed Sparse Row (CSR)** structure.

## 1. Parallel Producer-Consumer Architecture
To eliminate I/O bottlenecks, we use a multi-threaded pipeline:
- **Producer Thread**: Uses `mmap` (Memory Mapping) to map the file directly into the process's address space. This avoids expensive buffer copying between kernel and user space. It scans the file and pushes raw edge chunks into a `ThreadSafeQueue`.
- **Consumer Threads**: Multiple workers pop chunks from the queue, parse the integers, and populate thread-local adjacency lists.



## 2. CSR Data Structure
Instead of using a standard `std::vector<std::vector<int>>`, which suffers from heavy pointer chasing and memory fragmentation, we flatten the graph into two contiguous arrays:
- `in_edges`: A single array containing all source nodes.
- `in_offsets`: An array of indices pointing to where each node's neighbors start in `in_edges`.

This format is the industry standard for sparse matrix operations because it maximizes **spatial locality** and allows the CPU to fetch data in predictable streams.