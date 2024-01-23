This repository contains files related to a data set.

See [recorder](./recorder/readme.md)

## Dataset content

- [x] Raw binary recordings (not published)
- [ ] Events in AEDAT4 files
- [x] Coordinates in JSON files
- [ ] HDF5 files with frames and coordinates with 1ms precision

## Dataset structure
For each shape/tool for each transformation and for each recording we generate three files
1. `.hdf5` file **without timestamps**, containing the following keys
    * `coordinates` (as positions and quaternion in camera coordinates): $N \times (X, Y, Z, Q_X, Q_Y, Q_Z, Q_W)$
    * `indices`: $N \times (Polarity, X, Y)$
    * `values`: $N \times Count$
2. `events.aedat4`: [AEDAT4](https://docs.inivation.com/software/software-advanced-usage/file-formats/aedat-4.0.html) formatted events (**with timestamps**)
3. `coordinates.json`: JSON object with name(s) of shapes/tools, mapping to a list of 8-tuples: $(Timestamp, X, Y, Z, Q_X, Q_Y, Q_Z, Q_W)$
    * Timestamps in the coordinate files are synchronized with the timestamps in the `events.aedat4` file.
