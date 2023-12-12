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

pro tip: mount a ramfs


## HDF5 format
The frames are encoded as a 1-dimensional array using single bits to represent pixels.