# PageRank Engine & Hardware Optimization

The `PageRankEngine` is the computational heart of the project, specifically tuned to combat the **Memory Wall**.

## 1. Memory Tiling
PageRank's primary bottleneck is the random access to the `current_rank` vector. To mitigate this, we implement **Tiling**:
- We process nodes in blocks (tiles) of 512.
- This size is chosen to ensure that the frequently accessed data stays within the **L1/L2 caches** ($32KB$ to $256KB$) for the duration of the tile's processing.



## 2. Floating-Point & Alignment
- **Precision**: By switching from `double` to `float`, we halve the memory traffic. On a MacBook 2012 with limited DDR3 bandwidth, this effectively doubles the data throughput.
- **Alignment**: We use a custom `AlignedAllocator` to ensure every rank vector starts on a **64-byte cache line boundary**. This prevents **False Sharing**, where two threads fight for the same cache line, causing a massive performance drop.

## 3. Computation Logic
To optimize the inner loop, we replace the division `rank / out_degree` with a multiplication by a pre-calculated inverse `rank * inv_degree`. Floating-point multiplication is significantly faster than division on almost all CPU architectures.