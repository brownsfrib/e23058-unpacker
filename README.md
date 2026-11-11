# e23058 unpacker

Glommed NSCLDAQ `.evt` → `output/run_<RUN>.root`, tree **E23058**, branches `FROG.`, `HIRA.`, `TDC.`, `NWB.`, `NWA.`, `VETO.` (+ `eventNumber`, `timestamp`).
Every member is listed in [docs/TREE_GUIDE.md](docs/TREE_GUIDE.md).

## Build / run
```
cmake -S . -B build && cmake --build build -j      # rebuild after any code or LinkDef change
export REGLOM=/path/to/reglom/  RUN=0100
./bin/unpacker                                     # run from this directory
```
Reading the tree in ROOT needs the data classes. Put `R__LOAD_LIBRARY(lib/libE23058Data.so)` at the top
of a macro, or `export LD_LIBRARY_PATH=$PWD/lib:$LD_LIBRARY_PATH` (classes then load automatically).

## Directories
| | |
|---|---|
| `unpacker.cxx` | main: config, event loop, writer |
| `include/`, `src/` | ring items + `Unpacker` (source ID → detector), readers (`FROG`, `HiRA`, `Pixie`), data classes (`FROGData`, `HiRAData`, `TDCData`, `PixieData`), `Config`, `TreeWriter` |
| `modules/` | electronics decoders only: A5202, HINP, V785, V1190, DDAS (Pixie-16) |
| `detector-specific/` | Geometry / Calibration / Physics per detector, `TDCMapping`, `PixieMapping` |
| `tools/` | `toBinary_FROG`, `toBinary_HIRA`: geometry text tables → `.dat` |
| `input/` | `FROG/`, `HIRA/`, `TDC/`, `PIXIE/` (configs, tables, generator scripts; see each README), `alignment/` (survey xlsx + shared `target.txt`) |
| `lib/`, `bin/`, `build/`, `output/` | generated, not committed |

## Code flow (per event)
```
Unpacker → reader (FROG sid 3, Pixie sid 6, HiRA sid 7) → FROGData / HiRAData / TDCData / PixieData
FROG : FROGGeometry (angles) → FROGPhysics (zero suppression, fPass)
HiRA : HiRAGeometry (strips/CsI → telescopes) → HiRACalibration (MeV) → HiRAPhysics (particles)
TDC  : TDCMapping (aux V1190 channels)
Pixie: DDAS hits (raw) → PixieMapping::Split (module/channel → wall, bar, side) → NWB. / NWA. / VETO.  No calibration.
→ TreeWriter.Fill()   (event kept if FROG.fNPass > 0, HiRA fired, or NWB./NWA./VETO. have hits and KEEP_PIXIE_ONLY 1)
```
Settings: `input/FROG/FROGConfig.dat`, `input/HIRA/HiRAConfig.dat` (`[geometry] [calibration] [physics]`),
`input/PIXIE/PixieConfig.dat` (`[readout]` source ID / traces / event keeping, `[geometry]` channel map).
Unknown keys are reported at start-up.

## PIXIE (LANA + VETO wall)
Channel map: `input/PIXIE/Pixie_ChannelMapping.txt` (`Module Channel Wall Bar Side Index`; Wall B = LANA wall B facing
the target, A = LANA wall A behind it, V = WMU VETO). Module = Pixie slot − `FIRST_SLOT` (PixieConfig.dat, default 2).
Decoding follows FRIBDAQ/DDASFormat (`DDASHitUnpacker`). `NWB.`, `NWA.` and `VETO.` are all `PixieData`, one per wall. Hits on channels not in the map are not written; they are listed at the end of the run.
