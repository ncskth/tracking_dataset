import argparse
import pathlib

import h5py
import tqdm


def verify_single(frames):
    bbox = frames.parent / "bbox.h5"
    mask = frames.parent / "mask.h5"

    with h5py.File(frames, mode="r") as f:
        n_frames = f["frame_keys"][-1][0]
      
    with h5py.File(bbox, mode="r") as bbox:
        assert len(bbox.keys()) == 1
        assert bbox[list(bbox.keys())[0]].shape[0] == n_frames, "bbox shape mismatch"

    with h5py.File(mask, mode="r") as mask:
        assert len(mask.keys()) == 1
        assert mask[list(mask.keys())[0]].shape[0] == n_frames, "mask shape mismatch"

def verify_all(files):
    for file in tqdm.tqdm(files):
        try:
          verify_single(file)
        except Exception as e:
            print(f"Error in {file}: {e}")
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=pathlib.Path)
    args = parser.parse_args()
    files = list(args.root.rglob("frames.h5"))
    verify_all(files)
