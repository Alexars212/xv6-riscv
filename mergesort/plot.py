import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

p = [1, 2, 4, 8, 16]
measured = [1.00, 1.91, 3.39, 5.34, 7.08]
amdahl = [1.00, 1.92, 3.36, 5.27, 7.31]

plt.plot(p, p, "k--", label="Linear")
plt.plot(p, amdahl, "s-", label="Amdahl prediction")
plt.plot(p, measured, "o-", label="Measured")
plt.xlabel("Threads / cores")
plt.ylabel("Speedup")
plt.title("Parallel MergeSort, n = 10,000,000")
plt.xticks(p)
plt.legend()
plt.grid(True)
plt.savefig("speedup.png", dpi=150)
