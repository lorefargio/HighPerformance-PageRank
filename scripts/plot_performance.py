import pandas as pd
import matplotlib.pyplot as plt
import os

def plot_benchmarks():
    csv_path = "benchmarks/last_run.csv"
    
    if not os.path.exists(csv_path):
        print(f"Error: {csv_path} not found. Run pagerank with --test first.")
        return

    # Caricamento dati
    df = pd.read_csv(csv_path)

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    plt.suptitle("Pagerank Performance Analysis (C++20)", fontsize=16)

    # Subplot 1: Speedup
    ax1.plot(df['threads'], df['speedup'], marker='o', label='Measured Speedup', color='#1f77b4', linewidth=2)
    ax1.plot(df['threads'], df['threads'], '--', color='gray', label='Ideal (Linear)')
    ax1.set_title("Speedup vs Threads")
    ax1.set_xlabel("Number of Threads")
    ax1.set_ylabel("Speedup (T1 / Tn)")
    ax1.grid(True, alpha=0.3)
    ax1.legend()

    # Subplot 2: Efficiency
    ax2.bar(df['threads'], df['efficiency'], color='#aec7e8', alpha=0.8)
    ax2.axhline(y=100, color='r', linestyle='--', label='Ideal (100%)')
    ax2.set_title("Efficiency Percentage")
    ax2.set_xlabel("Number of Threads")
    ax2.set_ylabel("Efficiency (%)")
    ax2.set_ylim(0, 110)
    ax2.grid(axis='y', alpha=0.3)

    plt.tight_layout(rect=[0, 0.03, 1, 0.95])
    
    # Salvataggio grafico
    plt.savefig("benchmarks/performance_report.png", dpi=300)
    print("[Success] Chart generated: benchmarks/performance_report.png")
    plt.show()

if __name__ == "__main__":
    plot_benchmarks()