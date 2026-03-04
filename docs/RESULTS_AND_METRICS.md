# Results, Metrics & Benchmark Mode

How we extract insights and validate the scalability of the implementation.

## 1. Efficient Result Sorting
To find the "Top K" most important nodes, we use `std::partial_sort`. 
- Unlike a full sort ($O(N \log N)$), `partial_sort` only orders the first $K$ elements, resulting in an $O(N \log K)$ complexity. 
- On a graph with 2 million nodes, this is significantly faster when we only care about the top 10-20 results.

## 2. Scalability Metrics
The `--test` mode provides a detailed analysis of how the code scales with more cores:
- **Speedup ($S$)**: Calculated as $T_{serial} / T_{parallel}$. Ideally, $S$ should equal the number of threads.
- **Efficiency ($E$)**: Calculated as $S / n_{threads}$. It shows how much of each core's potential is actually used.

## 3. Understanding the Memory Wall
When running the benchmark, you may notice that efficiency drops after 2 or 4 threads. This is typically not a software bug but a hardware limitation: the **Memory Wall**. The CPU cores are so fast that the RAM bus cannot feed them data quickly enough, causing the cores to stall (wait) for the memory.