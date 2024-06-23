import argparse
from collections import defaultdict
import time
from threading import Thread
import math
import imageio
import json
import pathlib

import scipy
import matplotlib.pyplot as plt
import numpy as np
import tqdm
import cv2
import h5py
import ray
from ray.experimental import tqdm_ray
import torch
import torchvision

import recording


def draw_polygon(points, pose):
    (px, py, x, y, z, qw, qx, qy, qz) = pose

    object_q = scipy.spatial.transform.Rotation.from_quat([qx, qy, qz, qw])
    points = object_q.apply(points)
    points += np.array([x, y, z])
    pixels = positions_to_pixels(points)
    return pixels


def positions_to_pixels(positions):
    camera_matrix = np.array(
        [[1700.74372, 0, 650.172869], [0, 1700.74372, 285.398327], [0, 0, 1]]
    )
    positions[:, 1:3] = -positions[:, 1:3]
    vec = np.array([0, 0, 0], dtype=np.float32)
    distortion_coeff = np.array([0, 0, 0, 0, 0], dtype=np.float32)
    pixels, _ = cv2.projectPoints(positions, vec, vec, camera_matrix, distortion_coeff)
    return pixels


def render_mask_and_bbox(pose, polygon, size=np.array((1280, 720))):
    f = plt.figure(figsize=size / 100, dpi=100)
    f.add_axes([0, 0, 1, 1])
    f.gca().axis("off")
    bg = np.zeros(size).T
    f.gca().imshow(bg, cmap="gray")
    canvas = f.canvas
    pixels = draw_polygon(polygon, pose).reshape(-1, 2)
    plt_polygon = plt.Polygon(pixels, color="#ff0")
    f.gca().add_patch(plt_polygon)
    canvas.draw()
    array = (
        np.frombuffer(canvas.buffer_rgba(), dtype=np.uint8)
        .reshape(tuple(size[::-1]) + (4,))[:, :, 0]
        .T
        > 0
    )
    plt.close()
    bbox = plt_polygon.get_extents()
    return array, np.array([bbox.x0, bbox.y0, bbox.x1, bbox.y1])


def add_bbox_to_file(file, size=np.array((1280, 720)), add_bbox=True):

    masks = defaultdict(list)
    bboxes = defaultdict(list)
    mask_shapes = []
    with recording.EventRecording(file) as fr:
        poses = fr.poses
        mask_shapes = [s for s in recording.POLYGONS.keys() if s in fr.shapes_present]

        for polygon in mask_shapes:
            # Skip if dataset already exists
            if f"{polygon}_mask" in fr.keys:
                return
            for i in tqdm_ray.tqdm(range(100)):
                mask, bbox = render_mask_and_bbox(
                    poses[polygon][i], recording.POLYGONS[polygon], size
                )
                masks[polygon].append(mask)
                if add_bbox:
                    bboxes[polygon].append(bbox)

    with h5py.File(file, mode="r+") as fw:  # Read and write mode
        for mask_name, mask in masks.items():
            fw.create_dataset(mask_name + "_mask", data=mask)
        if add_bbox:
            for bbox_name, bbox in bboxes.items():
                fw.create_dataset(bbox_name + "_bbox", data=bbox)


def generate_video(file, fps=20, codec="ffv1", size=(1280, 720)):
    """
    Save a video from a sparse tensor of events
    """
    with recording.EventRecording(file) as fr:
        for polygon in fr.shapes_present:
            # Create a video writer
            filename = file.replace(".h5", f"_{polygon}.avi")
            with imageio.get_writer(filename, fps=fps, codec=codec) as writer:
                for i in tqdm_ray.tqdm(range(0, 20)):
                    # 1. Extract frame
                    frame_pol = fr.frame(i).sum(0).to_dense().permute(1, 2, 0)
                    # Add a blue + alpha channel
                    alpha = torch.ones(*frame_pol.shape[:-1], 1)
                    frame_rgba = torch.concat(
                        [
                            frame_pol.clip(0, 1),
                            torch.zeros(*frame_pol.shape[:-1], 1),
                            alpha,
                        ],
                        dim=-1,
                    )
                    # 2. Extract mask
                    mask_bool = torch.from_numpy(fr.mask(polygon, i)).unsqueeze(-1)
                    mask_rgba = torch.concat(
                        [
                            mask_bool.repeat(1, 1, 3),
                            (1 - alpha) * mask_bool
                        ],
                        dim=-1,
                    )
                    frame = (
                        ((frame_rgba + mask_rgba).clip(0, 1) * 255)
                        .numpy().astype(np.uint8)
                    )
                    writer.append_data(frame)


@ray.remote
def process_file(file, add_bbox, render_video):
    add_bbox_to_file(file, add_bbox=add_bbox)
    if render_video:
        generate_video(file)


def track_progress(result_ids):
    processed_count = 0
    total_tasks = len(result_ids)
    while processed_count < total_tasks:
        # Get the results of the completed tasks so far
        ready_ids, remaining_ids = ray.wait(
            result_ids, num_returns=len(result_ids), timeout=0.1
        )
        result_ids = remaining_ids
        processed_count += len(ready_ids)
        print(f"Processed {processed_count}/{total_tasks} tasks")
        time.sleep(1)  # Small delay to avoid busy waiting


def main(args):
    ray.init()

    # files = list(pathlib.Path(args.root).rglob("*.h5"))
    files = [
        "../basic_shapes/square/translation/2024_03_04_15_59_57_square_none_trans_right_processed/frames.h5"
    ]
    result_ids = [
        process_file.remote(file, not args.ignore_bbox, args.render_video)
        for file in files
    ]

    # Track the progress in a separate thread
    from threading import Thread

    progress_thread = Thread(target=track_progress, args=(result_ids,))
    progress_thread.start()

    # Retrieve the results
    results = ray.get(result_ids)

    # Wait for the progress thread to finish
    progress_thread.join()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=str)
    parser.add_argument("--ignore_bbox", action="store_false")
    parser.add_argument("--render_video", action="store_true")
    main(parser.parse_args())
