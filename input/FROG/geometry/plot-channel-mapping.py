#!/usr/bin/env python3
"""Plot theta/phi with uncertainties for each mapped channel."""
from collections import defaultdict

import matplotlib.pyplot as plt

INFILE    = "channel_theta_phi.txt"
LABEL_CHS = False   # annotate each point with its Board.Channel (crowded)

data = defaultdict(lambda: defaultdict(list))
with open(INFILE) as f:
    for line in f:
        if line.startswith("#") or not line.strip():
            continue
        bc, barrel, face, sipm, th, dth, ph, dph = line.split()
        d = data[int(barrel)]
        d["label"].append(bc)
        d["theta"].append(float(th))
        d["dtheta"].append(float(dth))
        d["phi"].append(float(ph))
        d["dphi"].append(float(dph))

fig, ax = plt.subplots(figsize=(12, 7))
colors = plt.cm.tab10.colors

for i, (barrel, d) in enumerate(sorted(data.items())):
    ax.errorbar(d["phi"], d["theta"], xerr=d["dphi"], yerr=d["dtheta"],
                fmt="o", ms=3, lw=0.8, capsize=0,
                color=colors[i % len(colors)], label=f"Barrel {barrel}")
    if LABEL_CHS:
        for ph, th, lab in zip(d["phi"], d["theta"], d["label"]):
            ax.annotate(lab, (ph, th), fontsize=5, xytext=(2, 2),
                        textcoords="offset points")

ax.set_xlabel(r"$\phi$ (deg)   [0 = beam left, 90 = up]")
ax.set_ylabel(r"$\theta_{Lab}$ (deg)")
ax.set_xlim(0, 360)
ax.set_xticks(range(0, 361, 45))
ax.set_ylim(bottom=0)
ax.grid(alpha=0.3)
ax.legend(loc="upper right", fontsize=9)
ax.set_title(r"Barrel channels: $\theta$ vs $\phi$ (Board.Channel mapping)")
fig.tight_layout()
plt.show()
