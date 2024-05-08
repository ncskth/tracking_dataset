This contains two programs, `main` which records data to a file and `event_counter` which runs some rudimentary checks on a recorded data file. This could be structured much better and probably will be but it works at least.

## File Protocol
The protocol is mostly specified in `protocol.h`.

In a binary stream the data comes in the following format repeatedly
```
[uint8 type][struct header for type][maybe data]
```
The only package that has any data is the camera so far. It is followed by `header.num_events` camera events.

### Object ids
* 0 - 9: cameras
* 10: Rectangle

## Building

libNatNet for the optitrack system I have just included as a binary together with its headers so nothing needs to be done there. Metavision (or just [OpenEB](https://github.com/prophesee-ai/openeb) is enough) for the camera needs to be installed separately by the user. [H5pp](https://github.com/DavidAce/h5pp/releases) is required. V1.10 is available as a `.deb` package.

To build
```
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make all
```

to run
```
./event_counter [file]
./main [file]
```


## processed output
The data converter outputs 3 files from the raw recordings

1. A JSON file with timestamps, positions and orientations
1. An AEDAT4 file with timestamps & undistorted events
1. A HDF5 file with event frames with positions and orientations

The JSON & AEDAT4 files are self-explanatory and their timestamps are synchronized.

The HDF5 format is a bit more in-depth. At the root level you should see four "datasets": "frame_values", "frame_indexes", "frame_start_indexes" and finally the name of an object.

"frame_values" and "frame_keys" are the corresponding key and value arrays for a sparse tensor. "frame values" is a simple uint8 and "frame_keys" is a compound type with `(uin32 n, uint8 p, uint16 x, uint16 y)`. `n` is the frame index and `p` is the polarity.

"frame_start_indexes" is a helper array to quickly find specific frames. It specifies the starting point of each frame in the key-value arrays. If you only want to read the frame with index 5 for example, then you can quickly read `frame_keys[frame_start_index[5]..frame_start_index[6]` to only get the data relevant for that frame.

For an example on how to efficiently read the HDF5 file see [this](../analysis/render.py)