from matplotlib import pyplot as plt
import numpy as np
import sys

if len(sys.argv) != 2:
    print("usage: [in file]")
    sys.exit()

data = np.loadtxt(sys.argv[1])
data -= int(np.median(data))
counts, bins = np.histogram(data, bins=100)
plt.stairs(counts, bins / 1000)
plt.xlabel("Event-PC timestamp deviation (ms)")
plt.ylabel("Occurrences")
plt.show()