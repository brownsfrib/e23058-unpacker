# HiRA input

| file | made by |
|---|---|
| `HiRAConfig.dat` | by hand (table paths, calibration, physics cuts, disabled channels) |
| `HiRA_channel_theta_phi.dat` | `bin/toBinary_HIRA` — strip table read by the unpacker (108-byte records) |
| `ADCMapping.csv` | by hand: V785 module/channel → csi_N |
| `geometry/hira_config.json` | by hand: telescope → motherboard, front/back slot, det code, survey block |
| `geometry/HiRA_strips.csv`, `HiRA_pixels.csv`, `HiRA_csi.csv`, `HiRA_channel_theta_phi.txt`, `HiRA_coverage.png` | `build_hira_geometry.py` |
| `calibration/HiRA_strip_calibration.txt` | strip calibration (**MOCK**) |
| `../TDC/TDCMapping.csv` | V1190 channel → name; csi_N on channel 16 + N |
| `../alignment/` | survey xlsx (`HIRA`, `Barrels` sheets) and `target.txt`, shared with FROG |

## Updating the geometry / mapping (in order)
```
cd input/HIRA/geometry
# 1. edit what changed: hira_config.json, ../ADCMapping.csv, ../../TDC/TDCMapping.csv,
#    ../../alignment/target.txt, settings at the top of build_hira_geometry.py
python3 build_hira_geometry.py --plot   # -> CSVs, HiRA_channel_theta_phi.txt, HiRA_coverage.png
cd ../../..
bin/toBinary_HIRA input/HIRA/geometry/HiRA_channel_theta_phi.txt input/HIRA/HiRA_channel_theta_phi.dat
./bin/unpacker     # expect: loaded 768 strips / 48 CsI / 12288 pixels
```
Checks: `.dat` = 82944 bytes (768 × 108); the builder's table (left/right/top/bottom, "upright",
theta range) looks sane; `HiRA_coverage.png` shows 12 telescopes; the builder lists CsI with no
ADC/TDC channel. Then plot `HIRA.fTheta:HIRA.fPhi` from data. No rebuild is needed for table changes.
Commit the edited sources **and** every regenerated file together.

Notes
- CsI index within a telescope: on a theta/phi plot 0 = lowest theta & phi, then clockwise.
  Assigned from the lab angles (the builder prints the table), so rotated mounts (P1-3, P7) get the
  same convention. Override a telescope with `"csi_order": [i0, i1, i2, i3]` in hira_config.json.
  ADCMapping csi_N = 4*P + index.
- dtheta/dphi = half-widths; `dtheta_sys`/`dphi_sys` (CSV only) = survey/target uncertainty.
- Electronics: board = slot + 1; odd strips on chip 2·board−1, even strips on chip 2·board.
- Telescopes with dead hardware: `DISABLE_ALL TELE<n>` in HiRAConfig.dat, not the geometry.
- If `target.txt` changes, regenerate FROG too (input/FROG/README.md).
