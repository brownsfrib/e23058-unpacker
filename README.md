# e23058 unpacker

Glommed NSCLDAQ `.evt` → `output/run_<RUN>.root`, tree **E23058**, branches `FROG.`, `HIRA.`, `TDC.` (+ `eventNumber`, `timestamp`).
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
| `include/`, `src/` | ring items + `Unpacker` (source ID → detector), readers (`FROG`, `HiRA`, `Pixie`), data classes (`FROGData`, `HiRAData`, `TDCData`), `Config`, `TreeWriter` |
| `modules/` | electronics decoders only: A5202, HINP, V785, V1190 |
| `detector-specific/` | Geometry / Calibration / Physics per detector, `TDCMapping` |
| `tools/` | `toBinary_FROG`, `toBinary_HIRA`: geometry text tables → `.dat` |
| `input/` | `FROG/`, `HIRA/`, `TDC/`, `PIXIE/` (configs, tables, generator scripts; see each README), `alignment/` (survey xlsx + shared `target.txt`) |
| `lib/`, `bin/`, `build/`, `output/` | generated, not committed |

## Code flow (per event)
```
Unpacker → reader (FROG sid 3, HiRA sid 7) → FROGData / HiRAData / TDCData
FROG : FROGGeometry (angles) → FROGPhysics (zero suppression, fPass)
HiRA : HiRAGeometry (strips/CsI → telescopes) → HiRACalibration (MeV) → HiRAPhysics (particles)
TDC  : TDCMapping (aux V1190 channels)
→ TreeWriter.Fill()   (event kept if FROG.fNPass > 0 or HiRA fired)
```
Settings: `input/FROG/FROGConfig.dat`, `input/HIRA/HiRAConfig.dat` (`[geometry] [calibration] [physics]`).
Unknown keys are reported at start-up.

## Adding PIXIE
1. `include/PixieData.h` + `src/PixieData.cpp` (copy `FROGData`: flat vectors, `Clear()`, `ClassDef`).
2. Add both to `DATA_HEADERS`/`DATA_SOURCES` in `CMakeLists.txt` and `#pragma link C++ class PixieData+;` to `include/LinkDef.h`.
3. Write `Pixie::Unpack` (`src/Pixie.cpp`) to fill it; Geometry/Physics classes in `detector-specific/` if needed, reading `input/PIXIE/PixieConfig.dat`.
4. In `unpacker.cxx`: `unpacker.Register(6, &pixie)` and `writer.Add("PIXIE.", &pixieData)`. Rebuild.
