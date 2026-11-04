#!/usr/bin/env python3
"""
Scintillator slot positions, angles, and angular extents on the octagonal barrels.

Survey frame: Z is vertical (up). The beam axis is horizontal and is fitted
from the barrel centers (BL/BR midpoints), oriented from barrel 0 toward 7.
Detector axes: w = beam, u = beam left = up x w, v = up.

Face index k: clockwise looking UPSTREAM from barrel 7, starting at the
bottom face (0) -> beam right (2) -> top (4) -> beam left (6).
SiPM index j: beam left -> beam right across each face
(faces 2 and 6 follow VERTICAL_FACE_ORDER).
Theta/phi are measured from the target; phi 0 = beam left, 90 = up.

Inputs : ../../alignment/FROG_HIRA_LANA_Alignment_ORIGINAL.xlsx  ('Barrels' sheet)
         ../../alignment/target.txt                             (X Y Z in m, shared with HiRA)
Outputs: barrel_scintillators.csv        every slot on every barrel
         experimental_scintillators.csv  minus REMOVED_SIPM_BOARDS (what was in the experiment)
"""
import csv
import os
import numpy as np
import openpyxl

# ------------------------------------------------------------------ settings
RADIUS_IS_APOTHEM   = True   # True: radius = center-to-flat. False: center-to-vertex
SLOT_DEPTH_MM       = 1.5    # scintillator center sits this far inside the outer face
EDGE_TO_CENTER      = True   # True: vertex distance is to SiPM center; False: to SiPM edge
SLOT_WIDTH_MM       = 3.0
VERTICAL_FACE_ORDER = "bottom_to_top"   # SiPM order on faces 2 and 6: or "top_to_bottom"
SURVEY_FILE         = "../../alignment/FROG_HIRA_LANA_Alignment_ORIGINAL.xlsx"
TARGET_FILE         = "../../alignment/target.txt"
TARGET_GAP_MM       = 0.0    # only if TARGET_FILE is missing: target this far upstream of barrel 0's upstream face
OUTFILE             = "barrel_scintillators.csv"
EXPERIMENT_OUTFILE  = "experimental_scintillators.csv"

# SiPM boards (= faces) that were NOT installed during the experiment: {barrel: [faces]}
REMOVED_SIPM_BOARDS = {0: [0, 4]}

# barrel: (length_mm, radius_mm, sipms_per_face, vertex_to_outermost_sipm_mm)
GEOM = {
    0: (20.0, 100.0, 12, 5.21),
    2: (60.0,  75.0,  6, 8.34),
    3: (60.0,  70.0,  6, 8.77),
    4: (60.0,  60.0,  6, 7.13),
    5: (60.0,  60.0,  6, 7.13),
    6: (60.0,  50.0,  6, 6.74),
    7: (114.0, 50.0,  6, 6.74),
}


def read_survey(path):
    """{barrel: {"BL": xyz, "BR": xyz}} (m) from the 'Barrels' sheet ("Barrel N", BL X Y Z, -, BR X Y Z)."""
    wb = openpyxl.load_workbook(path, data_only=True)
    survey = {}
    for r in wb["Barrels"].iter_rows(values_only=True):
        if isinstance(r[0], str) and r[0].strip().startswith("Barrel"):
            survey[int(r[0].split()[1])] = {"BL": tuple(map(float, r[1:4])), "BR": tuple(map(float, r[5:8]))}
    return survey


def read_target(path):
    """(x, y, z) in m from the first non-comment line, or None if the file is missing."""
    if not os.path.exists(path):
        return None
    for line in open(path):
        f = line.split("#")[0].split()
        if len(f) >= 3:
            return np.array([float(x) for x in f[:3]])
    raise SystemExit(f"{path}: no 'X Y Z' line")


SURVEY = read_survey(SURVEY_FILE)

FACE_NAMES = ["bottom", "lower beam-right", "beam right", "upper beam-right",
              "top", "upper beam-left", "beam left", "lower beam-left"]

MM = 1e-3
UP = np.array([0.0, 0.0, 1.0])


def centers():
    """Barrel centers: midpoint of the BL and BR dimples."""
    return {b: 0.5 * (np.array(s["BL"]) + np.array(s["BR"])) for b, s in SURVEY.items()}


def fit_beam_axis(cen):
    """Horizontal best-fit line through the barrel centers, oriented 0 -> 7."""
    pts = np.array(list(cen.values()))
    mean = pts.mean(axis=0)
    w = np.linalg.svd(pts - mean)[2][0]
    w -= np.dot(w, UP) * UP
    w /= np.linalg.norm(w)
    if np.dot(w, cen[7] - cen[0]) < 0:
        w = -w
    return mean, w


def face_normal_angle(k):
    """Outward normal of face k, measured from u (beam left) toward v (up)."""
    return np.deg2rad(270.0 - 45.0 * k)    # 0: bottom, 2: beam right, 4: top, 6: beam left


def sipm_direction(theta, u, v):
    """Unit vector along the face in the direction of increasing SiPM index."""
    t = -np.sin(theta) * u + np.cos(theta) * v
    lr = np.dot(t, u)
    if abs(lr) < 1e-9:                      # vertical face: left/right is undefined
        return v if VERTICAL_FACE_ORDER == "bottom_to_top" else -v
    return -t if lr > 0 else t              # point toward beam right


def angles(p, target, u, v, w):
    """Polar angle from the beam axis; azimuth with 0 = beam left, 90 = up (deg)."""
    d = p - target
    theta = np.degrees(np.arccos(np.dot(d, w) / np.linalg.norm(d)))
    phi = np.degrees(np.arctan2(np.dot(d, v), np.dot(d, u))) % 360.0
    return theta, phi


def angular_extent(p, normal, t, length, target, u, v, w):
    """Min/max theta and phi over the 8 corners of the 3 x 3 mm x length scintillator."""
    h = SLOT_WIDTH_MM / 2 * MM
    _, phi_c = angles(p, target, u, v, w)
    thetas, dphis = [], []
    for a in (-1, 1):
        for b in (-1, 1):
            for c in (-1, 1):
                q = p + a * h * normal + b * h * t + c * (length / 2) * MM * w
                th, ph = angles(q, target, u, v, w)
                thetas.append(th)
                dphis.append((ph - phi_c + 180.0) % 360.0 - 180.0)   # handles 0/360 wrap
    return min(thetas), max(thetas), phi_c + min(dphis), phi_c + max(dphis)


def main():
    cen = centers()
    axis_pt, w = fit_beam_axis(cen)
    u = np.cross(UP, w)          # beam left
    v = UP

    target = read_target(TARGET_FILE)
    if target is None:
        print(f"WARNING: {TARGET_FILE} not found - target = barrel 0 upstream face - TARGET_GAP_MM")
        target = cen[0] - w * (GEOM[0][0] / 2 + TARGET_GAP_MM) * MM

    print("Beam axis w:", np.round(w, 5), "  beam left u:", np.round(u, 5))
    print("Target (m):", np.round(target, 6))
    print(f"\n{'barrel':>6} {'z_along_beam':>13} {'off-axis':>9} {'BL-BR twist':>12} "
          f"{'pitch':>7} {'standoff':>9}   (mm / deg)")

    rows = []
    for b, (length, radius, n, edge) in GEOM.items():
        c = cen[b]
        bl, br = np.array(SURVEY[b]["BL"]), np.array(SURVEY[b]["BR"])

        # alignment checks
        rel = c - axis_pt
        off_axis = np.linalg.norm(rel - np.dot(rel, w) * w) / MM
        d = bl - br
        twist = np.degrees(np.arccos(np.clip(np.dot(d / np.linalg.norm(d), u), -1, 1)))

        apothem = radius if RADIUS_IS_APOTHEM else radius * np.cos(np.pi / 8)
        side = 2.0 * apothem * np.tan(np.pi / 8)            # octagon face width
        first = edge if EDGE_TO_CENTER else edge + SLOT_WIDTH_MM / 2
        pitch = (side - 2 * first) / (n - 1)
        standoff = 0.5 * np.linalg.norm(d) / MM - apothem
        print(f"{b:>6} {np.dot(c - target, w) / MM:13.1f} {off_axis:9.2f} {twist:12.2f} "
              f"{pitch:7.3f} {standoff:9.2f}")
        if pitch < SLOT_WIDTH_MM:
            print(f"  WARNING: barrel {b} pitch {pitch:.2f} mm < slot width")

        r_slot = apothem - SLOT_DEPTH_MM
        for k in range(8):
            th = face_normal_angle(k)
            normal = np.cos(th) * u + np.sin(th) * v
            t = sipm_direction(th, u, v)
            for j in range(n):
                s = -side / 2 + first + j * pitch
                p = c + (r_slot * normal + s * t) * MM
                p_up = p - w * (length / 2) * MM
                p_dn = p + w * (length / 2) * MM

                theta, phi = angles(p, target, u, v, w)
                theta_up, _ = angles(p_up, target, u, v, w)
                theta_dn, _ = angles(p_dn, target, u, v, w)
                th_min, th_max, ph_min, ph_max = angular_extent(p, normal, t, length,
                                                                target, u, v, w)
                rows.append([b, k, FACE_NAMES[k], j,
                             *(f"{x:.6f}" for x in p),
                             *(f"{x:.6f}" for x in p_up),
                             *(f"{x:.6f}" for x in p_dn),
                             f"{theta:.3f}", f"{theta_up:.3f}", f"{theta_dn:.3f}",
                             f"{phi:.3f}",
                             f"{th_min:.3f}", f"{th_max:.3f}",
                             f"{ph_min:.3f}", f"{ph_max:.3f}"])

    header = ["barrel", "face", "face_name", "sipm",
              "x_m", "y_m", "z_m",
              "x_upstream_m", "y_upstream_m", "z_upstream_m",
              "x_downstream_m", "y_downstream_m", "z_downstream_m",
              "theta_deg", "theta_upstream_deg", "theta_downstream_deg", "phi_deg",
              "theta_min_deg", "theta_max_deg", "phi_min_deg", "phi_max_deg"]
    with open(OUTFILE, "w", newline="") as f:
        wr = csv.writer(f)
        wr.writerow(header)
        wr.writerows(rows)
    print(f"\nWrote {len(rows)} scintillator positions to {OUTFILE}")

    kept = [r for r in rows if r[1] not in REMOVED_SIPM_BOARDS.get(r[0], [])]
    with open(EXPERIMENT_OUTFILE, "w", newline="") as f:
        wr = csv.writer(f)
        wr.writerow(header)
        wr.writerows(kept)
    print(f"Wrote {len(kept)} to {EXPERIMENT_OUTFILE} (removed SiPM boards {REMOVED_SIPM_BOARDS})")


if __name__ == "__main__":
    main()