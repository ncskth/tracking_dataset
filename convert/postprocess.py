#!/usr/bin/env python

import argparse
import time
import imageio
import pathlib
import logging
from typing import Optional

import scipy
import matplotlib.pyplot as plt
import numpy as np
import tqdm
import cv2
import h5py
import ray
from ray.experimental import tqdm_ray
import torch

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


def add_bbox_to_file(
    fr, size=np.array((1280, 720)), add_masks: bool = False, add_bbox: bool = False
):
    mask_shapes = []
    n_frames = fr.frame_number

    # Masks
    with h5py.File(fr.mask_file, mode="w") as mask_write:
        masks = {}
        for polygon in recording.POLYGONS.keys():
            if polygon in fr.shapes_present and add_masks:
                masks[polygon] = mask_write.create_dataset(
                    f"{polygon}_mask",
                    (n_frames, *size),
                    dtype=bool,
                    compression="lzf",
                )

        # Bounding box
        with h5py.File(fr.bbox_file, mode="w") as bbox_write:
            bboxes = {}
            for polygon in recording.POLYGONS.keys():
                if polygon in fr.shapes_present and add_bbox:
                    bboxes[polygon] = bbox_write.create_dataset(
                        f"{polygon}_bbox",
                        (n_frames, 4),
                        dtype="float32",
                        compression="lzf",
                    )

            # Render mask and bbox
            poses = fr.poses
            mask_shapes = [
                s for s in recording.POLYGONS.keys() if s in fr.shapes_present
            ]

            for polygon in mask_shapes:
                buffer = []
                buffer_size = 1024
                buffer_index = 0
                for i in tqdm_ray.tqdm(
                    range(fr.frame_number),
                    desc=f"{fr.file.stem} bbox {polygon}",
                ):
                    mask, bbox = render_mask_and_bbox(
                        poses[polygon][i], recording.POLYGONS[polygon], size
                    )
                    if add_bbox:  # Add bbox, if requested
                        bboxes[polygon][i] = bbox

                    # Add masks in buffer to speed up compression
                    if len(buffer) >= buffer_size:
                        masks[polygon][
                            buffer_index : buffer_index + len(buffer)
                        ] = buffer
                        buffer_index += len(buffer)
                        buffer = []
                    # Add mask to buffer, if requested
                    if add_masks:
                        buffer.append(mask)
                # Add remaining buffered frames
                if len(buffer) > 0:
                    masks[polygon][buffer_index : buffer_index + len(buffer)] = buffer


def generate_video(fr, file, root, fps=30, codec="ffv1", size=(1280, 720)):
    """
    Save a video from a sparse tensor of events
    """
    for polygon in fr.shapes_present:
        # Create a video writer
        filename = root / f"{str(file.parent.name)}_{polygon}.avi"
        # Open mask file
        with recording.MaskFile(fr.mask_file, polygon) as mask_read:
            with imageio.get_writer(filename, fps=fps, codec=codec) as writer:
                for i in tqdm_ray.tqdm(
                    range(fr.frame_number), desc=f"{file.stem} video {polygon}"
                ):
                    # 1. Extract frame
                    frame_pol = fr.frame(i).sum(0).to_dense().permute(2, 1, 0)
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
                    mask_orig = mask_read[i]
                    mask_bool = torch.from_numpy(mask_orig).T.unsqueeze(-1)
                    mask_rgba = torch.concat(
                        [
                            mask_bool.repeat(1, 1, 3) * 0.2,
                            (alpha - 1) * mask_bool,
                        ],
                        dim=-1,
                    )
                    frame = (
                        ((frame_rgba + mask_rgba).clip(0, 1) * 255)
                        .numpy()
                        .astype(np.uint8)
                    )
                    writer.append_data(frame)


@ray.remote
def process_file(file, render_video, masks, bbox, total_bar):
    with recording.EventRecording(file) as fr:
        try:
            n_frames = fr.frame_number
        except:
            logging.warning(f"Error file {file} malformed, no 'frame_number' key")
            return

        try:
            logging.info(f"Processing file {file}")
            if masks or bbox:
                add_bbox_to_file(fr, masks, bbox)
            if render_video:
                generate_video(fr, file, render_video)

            total_bar.update.remote(1)
        except Exception as e:
            logging.error("Caught error", e)


def track_progress(result_ids):
    processed_count = 0
    total_tasks = len(result_ids)
    while processed_count < total_tasks:
        # Get the results of the completed tasks so far
        ready_ids, remaining_ids = ray.wait(
            result_ids, num_returns=len(result_ids), timeout=0.1
        )
        processed_count += len(ready_ids)
        print(f"Processed {processed_count}/{total_tasks} tasks")
        time.sleep(1)  # Small delay to avoid busy waiting


# Filter out malformed files
def filter_files(files):
    for f in files:
        try:
            with recording.EventRecording(f) as fr:
                n_frames = fr.frame_number
        except:
            logging.warning(f"Error file {f} malformed, no 'frame_number' key")
            files.remove(f)
    return files


def main(args):
    ray.init(num_cpus=args.num_cpus)
    start_time = time.time()

    files = list(pathlib.Path(args.root).rglob("frames.h5"))
    video_path = pathlib.Path(args.render_video) if args.render_video else None

    # Filter out malformed files
    files = filter_files(files)

    # Create a progress bar
    remote_tqdm = ray.remote(tqdm_ray.tqdm)
    total_bar = remote_tqdm.remote(total=len(files), desc="Task progress")

    # Start the tasks
    result_ids = [
        process_file.remote(file, video_path, args.masks, args.bbox, total_bar)
        for file in files
    ]

    # Retrieve the results
    results = ray.get(result_ids)

    print(f"Done processing {len(results)} files")
    print(f"Total time: {time.time() - start_time:.2f}s")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "root",
        type=str,
        help="Root directory from which to locate recordings (already processed into frames",
    )
    parser.add_argument("--bbox", action="store_true")
    parser.add_argument("--masks", action="store_true")
    parser.add_argument("--render-video", type=str)
    parser.add_argument(
        "--num-cpus",
        type=int,
        default=None,
        help="Number of CPUs to use. Default is all available",
    )
    main(parser.parse_args())
