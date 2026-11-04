#!/usr/bin/env python3
"""
Convert per-scintillator theta/phi (with uncertainties) to A5202 Board.Channel.

Inputs:
  experimental_scintillators.csv - from barrel-geometry.py (installed SiPM boards only)
  masterMapping.txt              - from masterMapping.py, "Board.Channel, Barrel.Board.SiPM" lines
Output:
  channel_theta_phi.txt     - one line per mapped channel
"""
import csv
import math
from collections import defaultdict

GEOM_CSV  = "experimental_scintillators.csv"
MAP_FILE  = "masterMapping.txt"
OUTFILE   = "channel_theta_phi.txt"
CHANNELS_PER_BOARD = 64

# How to turn the full angular range into an uncertainty:
#   "halfwidth": +/- half the full range (hard limits of the scintillator)
#   "rms":       full range / sqrt(12) (standard deviation of a uniform distribution)
UNCERTAINTY = "halfwidth"


def read_mapping(path):
    """Return {(barrel, face, sipm): (board, channel)}; duplicates must agree."""
    mapping = {}
    with open(path) as f:
        for n, line in enumerate(f, 1):
            line = line.strip()
            if not line or line.lower().startswith("board"):
                continue
            bc, bfs = (s.strip() for s in line.split(","))
            board, ch = (int(x) for x in bc.split("."))
            key = tuple(int(x) for x in bfs.split("."))
            if key in mapping and mapping[key] != (board, ch):
                raise ValueError(f"line {n}: {bfs} mapped to both "
                                 f"{mapping[key][0]}.{mapping[key][1]} and {bc}")
            mapping[key] = (board, ch)

    # Each board channel should be used only once
    used = {}
    for key, bc in mapping.items():
        if bc in used:
            print(f"WARNING: channel {bc[0]}.{bc[1]} used by both "
                  f"{'.'.join(map(str, used[bc]))} and {'.'.join(map(str, key))}")
        used[bc] = key
    return mapping


def read_geometry(path):
    """Return {(barrel, face, sipm): (theta, dtheta, phi, dphi)} in degrees."""
    scale = 0.5 if UNCERTAINTY == "halfwidth" else 1.0 / math.sqrt(12.0)
    geom = {}
    with open(path) as f:
        for r in csv.DictReader(f):
            key = (int(r["barrel"]), int(r["face"]), int(r["sipm"]))
            th, ph = float(r["theta_deg"]), float(r["phi_deg"])
            dth = (float(r["theta_max_deg"]) - float(r["theta_min_deg"])) * scale
            dph = (float(r["phi_max_deg"]) - float(r["phi_min_deg"])) * scale
            geom[key] = (th, dth, ph, dph)
    return geom


def main():
    mapping = read_mapping(MAP_FILE)
    geom = read_geometry(GEOM_CSV)

    out = []
    for key, (board, ch) in mapping.items():
        if key not in geom:
            print(f"WARNING: mapping entry {'.'.join(map(str, key))} "
                  f"(channel {board}.{ch}) has no geometry")
            continue
        out.append((board, ch, *key, *geom[key]))
    out.sort()

    with open(OUTFILE, "w") as f:
        f.write(f"# theta/phi in degrees from the target; uncertainty = {UNCERTAINTY}\n")
        f.write("# phi: 0 = beam left, 90 = up\n")
        f.write(f"{'#Board.Ch':>10} {'Barrel':>6} {'Face':>4} {'SiPM':>4} "
                f"{'theta':>9} {'dtheta':>8} {'phi':>9} {'dphi':>8}\n")
        for board, ch, b, k, j, th, dth, ph, dph in out:
            f.write(f"{f'{board}.{ch}':>10} {b:>6} {k:>4} {j:>4} "
                    f"{th:9.3f} {dth:8.3f} {ph:9.3f} {dph:8.3f}\n")
    print(f"Wrote {len(out)} channels to {OUTFILE}")

    # Report geometry channels with no electronics channel
    unmapped = defaultdict(list)
    for key in geom:
        if key not in mapping:
            unmapped[key[0]].append(key)
    for b, keys in sorted(unmapped.items()):
        if len(keys) <= 4:
            names = ", ".join(".".join(map(str, k)) for k in sorted(keys))
            print(f"Unmapped in barrel {b}: {names}")
        else:
            print(f"Unmapped in barrel {b}: {len(keys)} channels")

    # Report unused board channels (only lists them when there are just a few)
    by_board = defaultdict(set)
    for board, ch in mapping.values():
        by_board[board].add(ch)
    for board in sorted(by_board):
        free = sorted(set(range(CHANNELS_PER_BOARD)) - by_board[board])
        if 0 < len(free) <= 4:
            print(f"Board {board}: unused channels {', '.join(f'{board}.{c}' for c in free)}")
        elif free:
            print(f"Board {board}: {len(free)} unused channels")


if __name__ == "__main__":
    main()
