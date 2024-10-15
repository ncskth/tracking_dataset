# Script to analyze output of python's profiler
# python -m cProfile ...

import argparse
import pstats

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file", type=str)
    args = parser.parse_args()

    p = pstats.Stats(args.file)
    p.sort_stats(pstats.SortKey.TIME).print_stats(20)
    p.print_callees(20)
