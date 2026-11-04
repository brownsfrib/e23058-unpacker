# FROG input

| file | made by |
|---|---|
| `FROGConfig.dat` | by hand (geometry table path, physics cuts) |
| `FROG_channel_theta_phi.dat` | `bin/toBinary_FROG` — read by the unpacker (40-byte records) |
| `geometry/barrel-mapping/*.txt` | by hand: A5202 pin map, BarrelN cabling ("5202 Board" = board ID) |
| `geometry/masterMapping.txt` | `masterMapping.py` |
| `geometry/barrel_scintillators.csv`, `experimental_scintillators.csv` | `barrel-geometry.py` |
| `geometry/channel_theta_phi.txt` | `map-channels.py` |
| `../alignment/` | survey xlsx (`Barrels` sheet) and `target.txt`, shared with HiRA |

## Updating the geometry / mapping (in order)
```
cd input/FROG/geometry
# 1. edit what changed: barrel-mapping/*.txt, REMOVED_SIPM_BOARDS / GEOM in barrel-geometry.py,
#    ../../alignment/target.txt or the survey xlsx
python3 masterMapping.py          # -> masterMapping.txt   (board.channel -> barrel.face.sipm)
python3 barrel-geometry.py        # -> barrel_scintillators.csv, experimental_scintillators.csv
python3 map-channels.py           # -> channel_theta_phi.txt (half-widths)
python3 plot-channel-mapping.py   # optional check
cd ../../..
bin/toBinary_FROG input/FROG/geometry/channel_theta_phi.txt input/FROG/FROG_channel_theta_phi.dat
./bin/unpacker                    # check "FROGGeometry: loaded N channels"
```
Checks: `.dat` size = 40 × channels (288 today = boards 0–4); `map-channels.py` lists unmapped
scintillators and unused board channels. No rebuild is needed for table changes.
Commit the edited sources **and** every regenerated file together.

Notes
- Barrel 0 (board 5) is not mapped yet: add `Barrel0_Mapping.txt`, add it to `masterMapping.py`, rerun.
- `REMOVED_SIPM_BOARDS = {0: [0, 4]}`: barrel 0 faces not installed during the experiment.
- If `target.txt` changes, regenerate HiRA too (input/HIRA/README.md).
