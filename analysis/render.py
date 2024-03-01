import h5py
import matplotlib.pyplot as plt
import numpy as np
import torch
import torchvision
import sparse
import time
import tqdm
import math
import scipy
import cv2
import json

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
    sp = torch.sparse_coo_tensor(frame_keys.T, frame_values, size=(end, 2, 1280, 720))
    return sp


def sparse_to_video(frames, start, count):
    f = []
    for i in tqdm.trange(count):
        f.append(frames[i + start].to_dense())
    f = torch.stack(f) * 255
    zeros = torch.zeros(len(f), 1, *f.shape[2:])
    return torch.concat([f, zeros], dim=1).permute(0, 3, 2, 1)

def draw_polygon(points, pose):
    (px, py, x, y, z, qw, qx, qy, qz) = pose

    object_q = scipy.spatial.transform.Rotation.from_quat([
        qx,
        qy,
        qz,
        qw
    ])
    points = object_q.apply(points)
    points += np.array([x, y, z])
    pixels = positions_to_pixels(points)
    return pixels

# just rotating in the middle and shifting pixels does not work
def draw_polygon2(points, pose):
    (px, py, x, y, z, qw, qx, qy, qz) = pose

    object_q = scipy.spatial.transform.Rotation.from_quat([
        qx,
        qy,
        qz,
        qw
    ])
    points = object_q.apply(points)
    points += np.array([0, 0, z])
    pixels = positions_to_pixels(points)
    pixels += np.array([px - 650.172869, py - 285.398327])
    return pixels

def positions_to_pixels(positions):
    camera_matrix = np.array([
        [1700.74372, 0, 650.172869],
        [0, 1700.74372, 285.398327],
        [0, 0, 1]
    ])
    positions[:, 1:3] = -positions[:, 1:3]
    vec = np.array([0, 0, 0], dtype=np.float32)
    distortion_coeff = np.array([0, 0, 0, 0, 0], dtype=np.float32)
    pixels, _  = cv2.projectPoints(positions, vec, vec, camera_matrix, distortion_coeff)
    return pixels

def draw_frame(frame, pose, polygon):
    f, a = plt.subplots(1, 1, figsize=(12, 7))
    a.imshow(frame)
    plt.scatter([pose[0]], [pose[1]], marker="+", color="white")
    center = draw_polygon([[0,0,0]], pose).reshape(-1, 2)
    plt.scatter([center[0,0]], [center[0,1]], marker="x", color="red")
    pixels = draw_polygon(polygon, pose).reshape(-1, 2)
    plt_polygon = plt.Polygon(pixels, color="#ffffff50")
    a.add_patch(plt_polygon)
    plt.show()


triangle_offset =  -0;
triangle_width = 0.29;
triangle_height =  triangle_width / 2 * math.tan(60 * math.pi / 180);
triangle_center = triangle_width * math.sqrt(3) / 3;
triangle_polygon = [
        [-(triangle_center), 0, 0],
        [triangle_height - triangle_center, 0, -triangle_width / 2],
        [triangle_height - triangle_center, 0, triangle_width / 2],
        [-(triangle_center), 0, 0]
]
circle_radius = 0.292/2;
iterations = 150;
circle_polygon = []
for i in range(iterations):
    circle_polygon.append([
        math.sin(2 * math.pi * i / iterations) * circle_radius,
        0,
        math.cos(2 * math.pi * i / iterations) * circle_radius,
    ])

offset_x = 0.28 / 2 - 0.022154 - 0.05;
offset_z = 0.22 / 2 + 0.1 + 0.05;
blob_polygon = [
    [offset_x, 0, offset_z],
    [offset_x + 0.022154, 0, offset_z],
    [offset_x + 0.022154, 0, offset_z - 0.1],
    [offset_x + 0.022154 + 0.05, 0, offset_z - 0.05 - 0.1],
    [offset_x + 0.022154 + 0.05, 0, offset_z - 0.05 - 0.1 - 0.22],
    [offset_x + 0.022154 + 0.05 - 0.28, 0, offset_z - 0.05 - 0.1 - 0.22],
    [offset_x + 0.022154 + 0.05 - 0.28, 0, offset_z - 0.05 - 0.1 - 0.22 + 0.25],
    [offset_x, 0, offset_z],
]

file_rec = "../recorder/build/blab/frames.h5"

with h5py.File(file_rec) as f:
    start = time.time()
    sp = get_frames_as_torch(f, 5000, 10000)
    end = time.time()

    print(f"read 5 000 frames in {end - start:.2f} seconds")

    for i in range(100, 10000, 500):
        sp = get_frames_as_torch(f, i, i + 1)
        video = sparse_to_video(sp, i, 1)
        poses = f["blob"]
        draw_frame(video[0], poses[i], blob_polygon)
        plt.show()