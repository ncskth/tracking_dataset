import h5py
import numpy as np
import sys

f = h5py.File(sys.argv[1], 'r')

print(f.keys())

width = 1280
height = 720

print (f["hammer"].shape)

a = np.ndarray( (f["hammer"].shape[0], 1) )

a = (f["hammer"][:][0] < 0) + (f["hammer"][:][0] > width) + (f["hammer"][:][1] < 0) + (f["hammer"][:][1] > width)


i = 0
for v in f["hammer"]:
    print(v)
    if v[0] < 0 or v[0] > width or -v[1] < 0 or -v[1] > height:
        print(i, v)
    i += 1
