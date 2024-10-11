from numpy.polynomial.polynomial import Polynomial
import argparse
import pathlib
import numpy as np
import tqdm
import h5py
from scipy.interpolate import CubicSpline
import recording
import matplotlib.pyplot as plt


parser = argparse.ArgumentParser()
parser.add_argument('root', type=str)
args = parser.parse_args()


def outlier_find(inp):
    
    x = np.sort(inp, axis=0)
    
    # Compute IQR
    q1 = np.median(x[:int(np.floor(x.shape[0]/2))], axis=0)
    q3 = np.median(x[int(np.ceil(x.shape[0]/2)):], axis=0)
    iqr = q3 - q1
    
    # Compute the lower and upper bounds for detecting outliers
    lower_b = q1 - 4 * iqr
    higher_b = q3 + 4 * iqr
    
    # Find indices where elements are either below the lower bound or above the upper bound
    indices = np.where((inp[:, np.newaxis] < lower_b) | (inp[:, np.newaxis] > higher_b))
    unique_rows = np.unique(indices[0])
    second_dim_zeros = np.all(np.square(inp[:, :]) < 1e-4, axis=1)
    zero_rows = np.where(second_dim_zeros)[0]

    combined_indices = np.unique(np.concatenate((unique_rows, zero_rows)))
    return np.array(list(zip(indices[0], indices[1]))), combined_indices, [lower_b, higher_b, iqr]


def find_sequences(outliers):
    sequences = []
    start_idx = 0
    if len(outliers) == 0:
        return [(0, 0)]
    for i in range(1, len(outliers)):
        if outliers[i] > outliers[i - 1] + 20:
            sequences.append((outliers[start_idx], outliers[i - 1]))
            start_idx = i
    sequences.append((outliers[start_idx], outliers[-1]))
    return sequences


def interpolate(data, outliers):
    for start_idx, end_idx in outliers:
        idx1 = max(0, start_idx-1)
        idx2 = min(data.shape[0] - 1, end_idx+1)

        x = np.array([idx1, idx2])
        y = data[x, :]

        # Perform polynomial interpolation for each column
        for i in range(data.shape[1]):
            p = Polynomial.fit(x, y[:, i], deg=1)

            # Interpolate the values in the outlier range
            interp_range = np.arange(start_idx, end_idx + 1)
            data[interp_range, i] = p(interp_range)

    return data

image_format = 'svg'
files = list(pathlib.Path(args.root).rglob("frames.h5"))
first = True
def shapes_present(self):
        shapes = []
        keys = []
        for p in recording.POLYGONS.keys():
            if p in self.keys():
                shapes.append(p)
        return shapes


for file in tqdm.tqdm(files, total=len(files)):
    with h5py.File(file, 'r+') as fr:
        mask_shapes = [
                        s for s in recording.POLYGONS.keys() if s in shapes_present(fr)
                    ]
        poses = {p: fr[p] for p in shapes_present(fr)}
        name = str(file).split('/')[-2]
        for polygon in mask_shapes:
            np_array = poses[polygon][:]
            first_deriv = np.diff(np_array, axis=0)
            _, first_outliers, iqr_metrics = outlier_find(first_deriv)
            outlier_sequences = find_sequences(first_outliers)
            data = interpolate(np_array, outlier_sequences)
            poses[polygon][:] = data