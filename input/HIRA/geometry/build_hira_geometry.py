#!/usr/bin/env python3
"""
build_hira_geometry.py - HiRA theta/phi tables from the alignment survey.

    cd input/HIRA/geometry
    python3 build_hira_geometry.py                    # uses the settings below
    python3 build_hira_geometry.py --plot             # also writes HiRA_coverage.png
    (then ../../../bin/toBinary_HIRA - see input/HIRA/README.md)

Inputs
    hira_config.json                         one entry per telescope (slots, codes, thickness)
    ../../alignment/FROG_HIRA_LANA_Alignment_ORIGINAL.xlsx
                                             survey: 'HIRA' sheet (4 frame fiducials per
                                             telescope) and 'Barrels' sheet (beam axis)
    ../../alignment/target.txt               target position X Y Z [m] (shared with FROG)
    ../ADCMapping.csv, ../../TDC/TDCMapping.csv   CsI -> electronics channels (optional)

Outputs (all angles in degrees)
    HiRA_strips.csv                one row per strip   (12 x 64)
    HiRA_pixels.csv                one row per pixel   (12 x 32 x 32), incl. the CsI behind it
    HiRA_csi.csv                   one row per CsI     (48)
    HiRA_channel_theta_phi.txt     strip table in the format write_hira_channels reads,
                                   so the current unpacker picks up real angles today

Geometry
  * Each telescope frame has 4 fiducials. The two SIDE fiducials sit at mid-height on the
    left/right edges; TOP is 3/4 and BOTTOM 1/4 of the way across from the left edge.
    "Left" = the side with lower theta. Which survey point is which is worked out from
    the distances (side pair = one frame width apart), not from the point names.
  * The front silicon is FRAME_TO_SI_MM behind the frame, centred on the frame.
  * Front (EF) strips run along the frame's up/down axis; EF0 is at the left edge.
    Back (EB) strips run left/right; EB0 is at the bottom edge.
  * Frame: z = beam direction, y = lab vertical (up), x = y cross z (beam left).
    theta = angle from the beam axis, phi = atan2(y, x) (0 = beam left, 90 = up,
    180 = beam right), reported on 0..360 by default (PHI_0_TO_360).
  * Strips are 1-D: a front strip spans the whole detector height, so its phi range
    is wide. Use the PIXEL table for hit angles; strip ranges are for checks.
  * dtheta/dphi = HALF-WIDTH of the element as seen from the target, (max - min)/2.
    dtheta_sys/dphi_sys (CSV only) = 1-sigma shift of the element centre from a
    Monte-Carlo of the survey, target-position and frame-offset uncertainties below.
"""
import argparse, csv, json, math, os, re
import numpy as np
import openpyxl

# ============================== settings ======================================
CONFIG_FILE   = "hira_config.json"
SURVEY_FILE   = "../../alignment/FROG_HIRA_LANA_Alignment_ORIGINAL.xlsx"
TARGET_FILE   = "../../alignment/target.txt"
ADC_MAP_FILE  = "../ADCMapping.csv"            # set to None to skip
TDC_MAP_FILE  = "../../TDC/TDCMapping.csv"     # set to None to skip
OUT_DIR       = "."

FRAME_TO_SI_MM   = 10.0      # frame sits this far in FRONT of the front silicon face
ACTIVE_MM        = 62.3      # active area is ACTIVE_MM x ACTIVE_MM
N_STRIPS         = 32        # per face
STRIP_PITCH_MM   = ACTIVE_MM / N_STRIPS   # 1.947 mm; strip centre-to-centre spacing
STRIP_WIDTH_MM   = 1.8       # sensitive width of one strip (pitch minus inter-strip gap)
NOMINAL_THICK_UM = 1500      # used when a telescope's thickness_um is null

# Target position: read from TARGET_FILE (shared with the FROG geometry).
# If that file is missing, "normals" = point where the telescope normals meet,
# projected onto the beam axis (the value printed below can be copied into it).
TARGET = "normals"

# Uncertainties (1 sigma) for the Monte-Carlo, in mm
SIGMA_FIDUCIAL_MM = 0.10     # each surveyed fiducial
SIGMA_TARGET_MM   = 2.0      # target position, each axis (placeholder - target not surveyed)
SIGMA_FRAME_MM    = 0.5      # FRAME_TO_SI_MM
MC_TRIALS         = 200
SEED              = 12345

# phi range: True -> 0..360 (HiRA sits near 180 = beam right, so no wrap-around),
#            False -> -180..180
PHI_0_TO_360 = True

# Which top/bottom fiducial is "top".
#   "handedness"     (default) the one that makes the frame right-handed seen from the
#                    target. With the 2026 survey this is the fiducial 1/4 across from
#                    the low-theta side, and it makes the regular telescopes upright.
#   "three_quarters" the fiducial 3/4 across from the low-theta side, as described.
#                    With this survey that makes every frame mirror-imaged.
TOP_RULE = "handedness"

# Per-telescope override: add  "fiducials": {"left": "P4", "right": "P2", "top": "P1",
# "bottom": "P3"}  to a telescope in hira_config.json to bypass the automatic naming.

# Strip numbering vs geometry. False = EF0 at left edge / EB0 at bottom edge.
EF_REVERSED = False
EB_REVERSED = False

# Area sampling (points across a strip width / along a strip / across a pixel side)
N_ACROSS, N_ALONG, N_PIXEL = 5, 64, 5
# ==============================================================================


# ---------------------------------------------------------------- inputs
def read_survey(path):
    wb = openpyxl.load_workbook(path, data_only=True)
    blocks, cur = {}, None
    for r in wb["HIRA"].iter_rows(values_only=True):
        if isinstance(r[0], str) and r[0].startswith("A::HIRA"):
            cur = int(r[0].split()[1]); blocks[cur] = {}
        elif r[0] in ("P1", "P2", "P3", "P4"):
            blocks[cur][r[0]] = np.array(r[1:4], float)
    barrels = []
    for r in wb["Barrels"].iter_rows(values_only=True):
        if isinstance(r[0], str) and r[0].startswith("Barrel"):
            barrels.append((np.array(r[1:4], float), np.array(r[5:8], float)))
    return blocks, barrels


def read_channel_map(path, has_module):
    """csi id -> (module, channel). Accepts 'ADC Index, Channel, Variable' or 'Channel, Variable'."""
    out = {}
    if not path or not os.path.exists(path):
        return out
    for line in open(path):
        line = line.split("#", 1)[0]
        f = [x.strip() for x in line.split(",")]
        if len(f) < (3 if has_module else 2) or not f[0].isdigit():
            continue
        mod, ch, var = (int(f[0]), int(f[1]), f[2]) if has_module else (0, int(f[0]), f[1])
        m = re.fullmatch(r"csi_(\d+)", var)
        if m:
            cid = int(m.group(1))
            if cid in out:
                print(f"  WARNING {os.path.basename(path)}: csi_{cid} appears twice "
                      f"(channels {out[cid][1]} and {ch}); keeping the first")
                continue
            out[cid] = (mod, ch)
    return out


# ---------------------------------------------------------------- geometry
def beam_axis(barrels):
    mids = np.array([(bl + br) / 2 for bl, br in barrels])
    c = mids.mean(0)
    d = np.linalg.svd(mids - c)[2][0]
    left = np.cross([0, 0, 1], d)
    bl, br = barrels[0]
    if np.dot(bl - br, left) < 0:      # 'BL' must be on beam left -> fixes the direction
        d = -d
    return c, d / np.linalg.norm(d)


def lab_frame(d):
    z = d
    y = np.array([0.0, 0.0, 1.0]); y = y - np.dot(y, z) * z; y /= np.linalg.norm(y)
    x = np.cross(y, z)
    return x, y, z


def angles(points, target, frame):
    """theta, phi in degrees for points (..., 3) seen from target."""
    x, y, z = frame
    r = points - target
    rz, rx, ry = r @ z, r @ x, r @ y
    theta = np.degrees(np.arctan2(np.hypot(rx, ry), rz))
    phi = np.degrees(np.arctan2(ry, rx))
    if PHI_0_TO_360:
        phi = phi % 360.0
    return theta, phi


def identify(pts, target, frame):
    """Name the 4 fiducials: returns dict left/right/top/bottom of points (3,)."""
    names = list(pts)
    # Split the 4 points into two pairs whose midpoints coincide (both pairs are centred
    # on the frame centre). Of those two pairs, the side pair is the shorter one: it is
    # one frame width W apart, top/bottom are sqrt((W/2)^2 + H^2) apart.
    a = names[0]
    splits = [((a, b), tuple(n for n in names if n not in (a, b))) for b in names[1:]]
    mid = lambda p: (pts[p[0]] + pts[p[1]]) / 2
    pair1, pair2 = min(splits, key=lambda s: np.linalg.norm(mid(s[0]) - mid(s[1])))
    dist = lambda p: np.linalg.norm(pts[p[0]] - pts[p[1]])
    side, tb = (pair1, pair2) if dist(pair1) < dist(pair2) else (pair2, pair1)
    th = {n: angles(pts[n], target, frame)[0] for n in side}
    left, right = sorted(side, key=lambda n: th[n])
    e_u = pts[right] - pts[left]; e_u /= np.linalg.norm(e_u)
    frac = {n: np.dot(pts[n] - pts[left], e_u) / np.linalg.norm(pts[right] - pts[left]) for n in tb}
    if TOP_RULE == "three_quarters":
        top, bottom = sorted(tb, key=lambda n: -frac[n])
    else:
        # "handedness": seen from the target, left->right x bottom->top must point back
        # at the target (a real frame cannot be mirror-imaged by mounting it).
        centre = (pts[left] + pts[right]) / 2
        a, b = tb
        top, bottom = (a, b) if np.dot(np.cross(e_u, pts[a] - pts[b]), target - centre) > 0 else (b, a)
    return {"left": left, "right": right, "top": top, "bottom": bottom}, frac


def telescope_axes(p, target):
    """Silicon centre, e_u (left->right), e_v (bottom->top), n (towards target)."""
    centre = (p["left"] + p["right"]) / 2
    e_u = p["right"] - p["left"]; e_u /= np.linalg.norm(e_u)
    v = p["top"] - p["bottom"]; v -= np.dot(v, e_u) * e_u; e_v = v / np.linalg.norm(v)
    n = np.cross(e_u, e_v)
    if np.dot(n, target - centre) < 0:
        n = -n
    return centre, e_u, e_v, n


def converge(centres, normals):
    A, b = np.zeros((3, 3)), np.zeros(3)
    for c, n in zip(centres, normals):
        P = np.eye(3) - np.outer(n, n); A += P; b += P @ c
    return np.linalg.solve(A, b)


# ---------------------------------------------------------------- sampling
def strip_pos(i, reversed_):
    k = (N_STRIPS - 1 - i) if reversed_ else i
    return (k + 0.5 - N_STRIPS / 2) * STRIP_PITCH_MM * 1e-3     # metres from centre


def grid(u0, u1, v0, v1, nu, nv):
    u = np.linspace(u0, u1, nu); v = np.linspace(v0, v1, nv)
    U, V = np.meshgrid(u, v, indexing="ij")
    return U.ravel(), V.ravel()


def element_samples():
    """Local (u, v) sample points in metres, grouped by element type:
       {group: (keys, U[E,S], V[E,S])}. Elements in a group share a sample count."""
    w, a = STRIP_WIDTH_MM * 1e-3 / 2, ACTIVE_MM * 1e-3 / 2
    els = {}
    for i in range(N_STRIPS):
        u = strip_pos(i, EF_REVERSED)                          # EF: vertical strip at u
        els[("EF", i)] = grid(u - w, u + w, -a, a, N_ACROSS, N_ALONG)
        v = strip_pos(i, EB_REVERSED)                          # EB: horizontal strip at v
        els[("EB", i)] = grid(-a, a, v - w, v + w, N_ALONG, N_ACROSS)
    for i in range(N_STRIPS):
        for j in range(N_STRIPS):
            u, v = strip_pos(i, EF_REVERSED), strip_pos(j, EB_REVERSED)
            els[("PX", i, j)] = grid(u - w, u + w, v - w, v + w, N_PIXEL, N_PIXEL)
    for q, (us, vs) in {0: (-1, 1), 1: (-1, -1), 2: (1, -1), 3: (1, 1)}.items():
        # detector-frame quadrant d: 0 top-left, 1 bottom-left, 2 bottom-right, 3 top-right
        # (seen from the target); the CsI INDEX is assigned from the lab angles, see csi_lab_order
        u0, u1 = sorted((0, us * a)); v0, v1 = sorted((0, vs * a))
        els[("CSI", q)] = grid(u0, u1, v0, v1, 40, 40)
    groups = {}
    for k, (u, v) in els.items():
        g = "PX" if k[0] == "PX" else ("CSI" if k[0] == "CSI" else "STRIP")
        groups.setdefault(g, ([], [], []))
        groups[g][0].append(k); groups[g][1].append(u); groups[g][2].append(v)
    return {g: (keys, np.stack(us), np.stack(vs)) for g, (keys, us, vs) in groups.items()}


def csi_lab_order(t, r):
    """CsI index for each detector quadrant d, from where the crystal sits on a theta/phi plot
    (theta on x, phi on y): 0 = lowest theta & lowest phi (bottom-left), then CLOCKWISE
    1 = top-left, 2 = top-right, 3 = bottom-right. Override per telescope in hira_config.json with
    "csi_order": [index of d=0, d=1, d=2, d=3]."""
    if t.get("csi_order"):
        return list(t["csi_order"])
    th = np.array([r["theta"][("CSI", d)] for d in range(4)])
    ph = np.array([r["phi"][("CSI", d)] for d in range(4)])
    ang = np.degrees(np.arctan2(ph - ph.mean(), th - th.mean())) % 360    # position around the centre
    first = int(np.argmin(np.abs((ang - 225 + 180) % 360 - 180)))        # nearest to bottom-left
    cw = sorted(range(4), key=lambda d: (ang[first] - ang[d]) % 360)    # clockwise from there
    lab = [0] * 4
    for idx, d in enumerate(cw):
        lab[d] = idx
    return lab


def csi_quadrant(ef, eb):
    left = strip_pos(ef, EF_REVERSED) < 0
    top = strip_pos(eb, EB_REVERSED) > 0
    return {(True, True): 0, (True, False): 1, (False, False): 2, (False, True): 3}[(left, top)]


def stats(theta, phi):
    """centre/min/max/rms of theta and phi over the last axis (phi unwrapped around centre)."""
    tc = theta.mean(-1)
    pc = np.degrees(np.arctan2(np.sin(np.radians(phi)).mean(-1), np.cos(np.radians(phi)).mean(-1)))
    if PHI_0_TO_360:
        pc = pc % 360.0
    dphi = (phi - pc[..., None] + 180) % 360 - 180
    return dict(theta=tc, theta_min=theta.min(-1), theta_max=theta.max(-1),
                phi=pc, phi_min=pc + dphi.min(-1), phi_max=pc + dphi.max(-1))


# ---------------------------------------------------------------- main
def read_target(path):
    """X Y Z [m] from the first non-comment line, or "normals" if the file is missing."""
    if not os.path.exists(path):
        print(f"WARNING: {path} not found - using the normals convergence as the target")
        return "normals"
    for line in open(path):
        line = line.split("#")[0].split()
        if len(line) >= 3:
            return [float(x) for x in line[:3]]
    raise SystemExit(f"{path}: no 'X Y Z' line")


def build(args):
    global TARGET
    TARGET = read_target(TARGET_FILE)
    cfg = json.load(open(CONFIG_FILE))
    blocks, barrels = read_survey(SURVEY_FILE)
    axis_c, d = beam_axis(barrels)
    frame = lab_frame(d)

    tels = cfg["telescopes"]
    raw = {t["id"]: blocks[t["survey_block"]] for t in tels}

    # first pass: rough axes from a provisional target to identify the fiducials
    def target_from(normals_target):
        if TARGET == "normals":
            return axis_c + np.dot(normals_target - axis_c, d) * d     # project onto the beam axis
        return np.array(TARGET, float)

    prov = axis_c
    for _ in range(3):                         # iterate: identification needs a target
        named, cents, norms = {}, [], []
        for t in tels:
            ids = t.get("fiducials") or identify(raw[t["id"]], prov, frame)[0]
            p = {k: raw[t["id"]][v] for k, v in ids.items()}
            c, e_u, e_v, n = telescope_axes(p, prov)
            named[t["id"]] = (ids, p); cents.append(c); norms.append(n)
        conv = converge(cents, norms)
        prov = target_from(conv)
    target = prov

    print(f"beam axis direction      {np.round(d, 4)}")
    print(f"normals converge at      {np.round(conv, 4)} m  "
          f"({np.linalg.norm(np.cross(conv - axis_c, d)) * 1e3:.1f} mm off the beam axis)")
    print(f"target used              {np.round(target, 6)} m  (TARGET = {TARGET!r})")
    print(f"frame -> front silicon   {FRAME_TO_SI_MM} mm;  pitch {STRIP_PITCH_MM:.3f} mm, width {STRIP_WIDTH_MM} mm\n")

    groups = element_samples()
    rng = np.random.default_rng(SEED)

    results = {}
    print(" P  survey  left/right/top/bottom  top at  frame 'up' vs lab  dist(mm)  theta range")
    for t in tels:
        ids, p = named[t["id"]]
        _, frac = identify(raw[t["id"]], target, frame)
        top_frac = frac.get(ids["top"], float("nan"))
        # one set of Monte-Carlo geometry variations per telescope, shared by all groups
        trials = []
        for _ in range(MC_TRIALS):
            pj = {k: v + rng.normal(0, SIGMA_FIDUCIAL_MM * 1e-3, 3) for k, v in p.items()}
            tj = target + rng.normal(0, SIGMA_TARGET_MM * 1e-3, 3)
            oj = FRAME_TO_SI_MM + rng.normal(0, SIGMA_FRAME_MM)
            trials.append((pj, tj, oj))

        r = {}
        for keys, U, V in groups.values():
            c, e_u, e_v, n = telescope_axes(p, target)
            si = c - n * FRAME_TO_SI_MM * 1e-3
            pts0 = si + U[..., None] * e_u + V[..., None] * e_v          # (E, S, 3)
            s = stats(*angles(pts0, target, frame))

            # Monte-Carlo of survey / target / frame offset -> spread of element centres:
            # rebuild the telescope from jittered fiducials and frame offset, and view
            # each element centre from a jittered target position.
            cth, cph = [], []
            Uc, Vc = U.mean(1)[:, None], V.mean(1)[:, None]
            for pj, tj, oj in trials:
                c, e_u, e_v, n = telescope_axes(pj, tj)
                si = c - n * oj * 1e-3
                a, b = angles(si + Uc * e_u + Vc * e_v, tj, frame)
                cth.append(a); cph.append(b)
            cth, cph = np.array(cth), np.array(cph)
            s["dtheta_sys"] = cth.std(0)
            s["dphi_sys"] = ((cph - cph.mean(0) + 180) % 360 - 180).std(0)
            s["dtheta"] = (s["theta_max"] - s["theta_min"]) / 2          # half-widths
            s["dphi"] = (s["phi_max"] - s["phi_min"]) / 2
            for name, vals in s.items():
                r.setdefault(name, {}).update(dict(zip(keys, vals)))
        results[t["id"]] = r

        c, e_u, e_v, n = telescope_axes(p, target)
        tilt = math.degrees(math.atan2(np.dot(e_v, frame[0]), np.dot(e_v, frame[1])))
        mount = ("upright" if abs(tilt) < 30 else "upside down" if abs(tilt) > 150
                 else f"rotated {tilt:+.0f} deg")
        dist = np.linalg.norm(c - n * FRAME_TO_SI_MM * 1e-3 - target) * 1e3
        tmin = min(r["theta_min"][("EF", i)] for i in range(N_STRIPS))
        tmax = max(r["theta_max"][("EF", i)] for i in range(N_STRIPS))
        print(f"P{t['id']:<2}  HIRA{t['survey_block']:<3} {ids['left']}/{ids['right']}/{ids['top']}/{ids['bottom']}"
              f"            {top_frac:4.2f}   {mount:<17} {dist:6.1f}   {tmin:5.1f} - {tmax:5.1f}")

    write_outputs(cfg, results)
    if args.plot:
        plot(cfg, results)


def write_outputs(cfg, results):
    adc = read_channel_map(ADC_MAP_FILE, True)
    tdc = read_channel_map(TDC_MAP_FILE, False)
    os.makedirs(OUT_DIR, exist_ok=True)
    f6 = lambda x: f"{x:.4f}"

    def angle_cols(r, k):
        return [f6(r["theta"][k]), f6(r["theta_min"][k]), f6(r["theta_max"][k]), f6(r["dtheta"][k]),
                f6(r["phi"][k]), f6(r["phi_min"][k]), f6(r["phi_max"][k]), f6(r["dphi"][k])]
    ang_hdr = ["theta", "theta_min", "theta_max", "dtheta", "phi", "phi_min", "phi_max", "dphi"]
    sys_cols = lambda r, k: [f6(r["dtheta_sys"][k]), f6(r["dphi_sys"][k])]
    sys_hdr = ["dtheta_sys", "dphi_sys"]

    lab = {t["id"]: csi_lab_order(t, results[t["id"]]) for t in cfg["telescopes"]}
    print("\nCsI index per detector quadrant (top-left, bottom-left, bottom-right, top-right as seen"
          " from the target);\n  index 0 = lowest theta/phi, clockwise on a theta/phi plot")
    for t in cfg["telescopes"]:
        r = results[t["id"]]
        cells = "  ".join(f"q{lab[t['id']][d]}=({r['theta'][('CSI', d)]:5.1f},{r['phi'][('CSI', d)]:6.1f})"
                          for d in sorted(range(4), key=lambda d: lab[t["id"]][d]))
        print(f"  P{t['id']:<2} {lab[t['id']]}{' (override)' if t.get('csi_order') else ''}   {cells}")
    cq = lambda tid, ef, eb: lab[tid][csi_quadrant(ef, eb)]

    with open(os.path.join(OUT_DIR, "HiRA_strips.csv"), "w", newline="") as fs, \
         open(os.path.join(OUT_DIR, "HiRA_channel_theta_phi.txt"), "w") as fl:
        w = csv.writer(fs)
        w.writerow(["telescope", "face", "strip", "motherboard", "slot", "board", "chip",
                    "det_code", "thickness_um", "csi_a", "csi_b"] + ang_hdr + sys_hdr)
        fl.write("# GENERATED by build_hira_geometry.py - input for toBinary_HIRA\n"
                 "# angles in degrees; dtheta/dphi = half-widths\n"
                 "# face: 0 = front (EF, vertical strips), 1 = back (EB, horizontal strips)\n"
                 "# csiA csiB: the two CsI behind this strip (global index 4*telescope + quadrant)\n"
                 "# motherboard chip detCode strip telescope face csiA csiB "
                 "theta thetaMin thetaMax dtheta phi phiMin phiMax dphi\n")
        for t in cfg["telescopes"]:
            r = results[t["id"]]
            thick = t["thickness_um"] or NOMINAL_THICK_UM
            for face, slot in (("EF", t["front_slot"]), ("EB", t["back_slot"])):
                board = slot + 1                                   # decoder board = slot + 1
                for i in range(N_STRIPS):
                    chip = 2 * board - 1 if i % 2 else 2 * board   # odd strips on odd chip
                    k = (face, i)
                    # the two CsI behind the strip: vary the OTHER face across both halves
                    if face == "EF":
                        csi = sorted({4 * t["id"] + cq(t["id"], i, j) for j in (0, N_STRIPS - 1)})
                    else:
                        csi = sorted({4 * t["id"] + cq(t["id"], j, i) for j in (0, N_STRIPS - 1)})
                    w.writerow([t["id"], face, i, t["motherboard"], slot, board, chip,
                                t["det_code"], thick, csi[0], csi[1]] + angle_cols(r, k) + sys_cols(r, k))
                    fl.write(f"{t['motherboard']} {chip} {t['det_code']} {i} {t['id']} "
                             f"{0 if face == 'EF' else 1} {csi[0]} {csi[1]} "
                             + " ".join(angle_cols(r, k)) + "\n")

    with open(os.path.join(OUT_DIR, "HiRA_pixels.csv"), "w", newline="") as fp:
        w = csv.writer(fp)
        w.writerow(["telescope", "ef", "eb", "csi", "theta", "dtheta", "phi", "dphi"] + sys_hdr)
        for t in cfg["telescopes"]:
            r = results[t["id"]]
            for i in range(N_STRIPS):
                for j in range(N_STRIPS):
                    k = ("PX", i, j)
                    w.writerow([t["id"], i, j, 4 * t["id"] + cq(t["id"], i, j),
                                f6(r["theta"][k]), f6(r["dtheta"][k]), f6(r["phi"][k]), f6(r["dphi"][k])]
                               + sys_cols(r, k))

    with open(os.path.join(OUT_DIR, "HiRA_csi.csv"), "w", newline="") as fc:
        w = csv.writer(fc)
        w.writerow(["csi", "telescope", "quadrant", "adc_module", "adc_channel", "tdc_channel"] + ang_hdr + sys_hdr)
        for t in cfg["telescopes"]:
            r = results[t["id"]]
            for q in range(4):                       # q = CsI index (lab order)
                d = lab[t["id"]].index(q)            # detector quadrant holding it
                cid = 4 * t["id"] + q
                am, ac = adc.get(cid, (-1, -1))
                w.writerow([cid, t["id"], q, am, ac, tdc.get(cid, (0, -1))[1]] + angle_cols(r, ("CSI", d))
                           + sys_cols(r, ("CSI", d)))
    print(f"\nwrote HiRA_strips.csv, HiRA_pixels.csv, HiRA_csi.csv, HiRA_channel_theta_phi.txt to {OUT_DIR}/")
    missing = [c for c in range(48) if c not in adc]
    if missing:
        print(f"  CsI without an ADC channel: {missing}")
    missing = [c for c in range(48) if c not in tdc]
    if missing:
        print(f"  CsI without a TDC channel: {missing}")


def plot(cfg, results):
    import matplotlib; matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    fig, ax = plt.subplots(figsize=(10, 6))
    for t in cfg["telescopes"]:
        r = results[t["id"]]
        th = [r["theta"][("PX", i, j)] for i in range(N_STRIPS) for j in range(N_STRIPS)]
        ph = [r["phi"][("PX", i, j)] for i in range(N_STRIPS) for j in range(N_STRIPS)]
        ax.scatter(th, ph, s=1)
        k0 = ("PX", 0, 0)
        cx = np.mean([r["theta"][("CSI", q)] for q in range(4)])
        cy = np.mean([r["phi"][("CSI", q)] for q in range(4)])
        ax.annotate(f"P{t['id']}", (cx, cy), fontsize=9, weight="bold", ha="center", va="center")
        ax.plot(r["theta"][k0], r["phi"][k0], "k+", ms=7)
        ax.plot(r["theta"][("PX", N_STRIPS - 1, 0)], r["phi"][("PX", N_STRIPS - 1, 0)], "kx", ms=5)
    ax.set_xlabel("theta (deg)"); ax.set_ylabel("phi (deg)")
    ax.set_title("HiRA pixel centres  (+ = EF0/EB0 pixel, x = EF31/EB0 pixel)")
    fig.tight_layout(); fig.savefig(os.path.join(OUT_DIR, "HiRA_coverage.png"), dpi=150)
    print("wrote HiRA_coverage.png")


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--plot", action="store_true", help="also write HiRA_coverage.png")
    build(ap.parse_args())
