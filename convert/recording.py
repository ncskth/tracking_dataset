import dataclasses
import math

import h5py
import numpy as np
import torch

POLYGONS = {}

triangle_offset = -0
triangle_width = 0.29
triangle_height = triangle_width / 2 * math.tan(60 * math.pi / 180)
triangle_center = triangle_width * math.sqrt(3) / 3
POLYGONS["triangle"] = [
    [-(triangle_center), 0, 0],
    [triangle_height - triangle_center, 0, -triangle_width / 2],
    [triangle_height - triangle_center, 0, triangle_width / 2],
    [-(triangle_center), 0, 0],
]
circle_radius = 0.292 / 2
iterations = 150
circle_polygon = []
for i in range(iterations):
    circle_polygon.append(
        [
            math.sin(2 * math.pi * i / iterations) * circle_radius,
            0,
            math.cos(2 * math.pi * i / iterations) * circle_radius,
        ]
    )
POLYGONS["circle"] = circle_polygon

square_width = 0.29
POLYGONS["square"] = [
    [-square_width / 2.0, 0, -square_width / 2.0],
    [square_width / 2.0, 0, -square_width / 2.0],
    [square_width / 2.0, 0, square_width / 2.0],
    [-square_width / 2.0, 0, square_width / 2.0],
    [-square_width / 2.0, 0, -square_width / 2.0],
]

offset_x = 0.28 / 2 - 0.022154 - 0.05
offset_z = 0.22 / 2 + 0.1 + 0.05
POLYGONS["blob"] = [
    [offset_x, 0, offset_z],
    [offset_x + 0.022154, 0, offset_z],
    [offset_x + 0.022154, 0, offset_z - 0.1],
    [offset_x + 0.022154 + 0.05, 0, offset_z - 0.05 - 0.1],
    [offset_x + 0.022154 + 0.05, 0, offset_z - 0.05 - 0.1 - 0.22],
    [offset_x + 0.022154 + 0.05 - 0.28, 0, offset_z - 0.05 - 0.1 - 0.22],
    [offset_x + 0.022154 + 0.05 - 0.28, 0, offset_z - 0.05 - 0.1 - 0.22 + 0.25],
    [offset_x, 0, offset_z],
]


def get_frames_as_torch(f, start, end):
    start_index = f["frame_start_indexes"][start]
    end_index = f["frame_start_indexes"][end]

    frame_keys_np = f["frame_keys"][start_index:end_index]
    frame_values = f["frame_values"][start_index:end_index]

    num_elements = len(frame_keys_np)
    num_fields = len(frame_keys_np.dtype.names)
    new_shape = (num_elements, num_fields)

    # convert compound type to int32 array
    frame_keys = np.zeros(new_shape, dtype=np.int32)
    for i, name in enumerate(frame_keys_np.dtype.names):
        frame_keys[:, i] = frame_keys_np[name].astype(np.int32)

    frame_keys = frame_keys.astype(np.int32)
    frame_values = frame_values.astype(np.int32)

    frame_keys = torch.from_numpy(frame_keys)
    frame_values = torch.from_numpy(frame_values)
    sp = torch.sparse_coo_tensor(frame_keys.T, frame_values, size=(end, 2, 1280, 720))
    return sp



@dataclasses.dataclass
class EventRecording:

    file: str
    resolution = np.array([1280, 720])

    def frame(self, index):
        return get_frames_as_torch(self.fp, index, index + 1)

    @property
    def frame_indices(self):
        return self.fp["frame_start_indexes"]
    
    @property
    def frame_number(self):
        return self.fp["frame_keys"][-1][0]

    @property
    def frames(self):
        return get_frames_as_torch(self.fp, 0, len(self.frame_indices) - 1)

    @property
    def keys(self):
        return self.fp.keys()

    def mask(self, shape, index):
        return self.fp[shape + "_mask"][index]

    @property
    def poses(self):
        return {p: self.fp[p] for p in self.shapes_present}

    @property
    def shapes_present(self):
        shapes = []
        keys = []
        for p in POLYGONS.keys():
            if p in self.keys:
                shapes.append(p)
        return shapes

    def __enter__(self):
        self.fp = h5py.File(self.file)
        return self

    def __exit__(self, *exc):
        self.fp.close()
