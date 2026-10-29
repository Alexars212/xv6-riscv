import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

p = [1, 2, 4, 8, 16]
measured = [1.00, 1.91, 3.39, 5.34, 7.08]

plt.plot(p, p, "k--", label="Linear speedup")
plt.plot(p, measured, "o-", label="Measured speedup")
plt.xlabel("Threads / cores")
plt.ylabel("Speedup")
plt.title("Task 1: Parallel MergeSort speedup, n = 10,000,000")
plt.xticks(p)
plt.legend()
plt.grid(True)
plt.savefig("task1_speedup.png", dpi=150)
