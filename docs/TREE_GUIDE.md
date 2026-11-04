# Tree E23058

One entry per kept event. `-10000` = not measured / not available.
Lengths: a block's vectors all have the length of its counter (`fMult`, `fEFmulti`, ...).
Angles in degrees from the target: theta from the beam axis; phi 0 = beam left, 90 = up, 180 = beam right (0–360).
`d*` angles are **half-widths** of the element.

```cpp
R__LOAD_LIBRARY(lib/libE23058Data.so)
TFile f("output/run_0100.root"); TTree *t = (TTree*)f.Get("E23058");
t->Draw("HIRA.fTheta:FROG.fTheta", "HIRA.fmulti > 0 && FROG.fNPass > 0");
```

## Event
| member | type | units |
|---|---|---|
| eventNumber | Long64_t | index of kept events |
| timestamp | ULong64_t | built-event timestamp (body header) |

## FROG. (FROGData) — one entry per stored SiPM hit
| member | type | length | units / meaning |
|---|---|---|---|
| fMult | Int_t | – | hits stored (after zero suppression / DROP_FAILED) |
| fBoard | vector<Int_t> | fMult | A5202 board (= inner source ID, 0–5) |
| fChannel | vector<Int_t> | fMult | 0–63 |
| fLG, fHG | vector<Double_t> | fMult | raw ADC (-10000 in timing mode) |
| fToA, fToT | vector<Double_t> | fMult | raw, LSB (0.5 ns) |
| fTheta, fPhi | vector<Double_t> | fMult | deg (-10000 if channel unmapped) |
| fDTheta, fDPhi | vector<Double_t> | fMult | half-width, deg |
| fPass | vector<Int_t> | fMult | 1 = passes FROGPhysics cuts (TOT_CUT, LG_MIN, HG_MIN) |
| fNPass | Int_t | – | hits with fPass == 1 |
| fNBoards | Int_t | – | boards in this event |
| fBoardID | vector<Int_t> | fNBoards | board |
| fBoardTrigID | vector<ULong64_t> | fNBoards | A5202 trigger ID |
| fBoardTime | vector<Double_t> | fNBoards | A5202 time stamp, µs |

## HIRA. (HiRAData) — names follow E15190
Front strips (EF), one entry per strip that fired; the EB block is identical (`fEB...`).
| member | type | length | units |
|---|---|---|---|
| fEFmulti | Int_t | – | |
| fEFnumtel | vector<Int_t> | fEFmulti | telescope 0–11 |
| fEFnumstrip | vector<Int_t> | fEFmulti | 0–31 |
| fEFmb, fEFchip | vector<Int_t> | fEFmulti | electronics address |
| fEFEnergyHi, fEFEnergyLo | vector<Double_t> | fEFmulti | raw HG / LG |
| fEFTime | vector<Double_t> | fEFmulti | raw HINP time |
| fEFEnergyCal | vector<Double_t> | fEFmulti | MeV (HG, or LG above HG_SATURATION) |

CsI, one entry per crystal above CSI_E_RAW_THRESHOLD and not disabled:
| member | type | length | units |
|---|---|---|---|
| fCsImulti | Int_t | – | |
| fCsInumtel | vector<Int_t> | fCsImulti | telescope |
| fCsInumcsi | vector<Int_t> | fCsImulti | 0–3 (0 top-left, 1 bottom-left, 2 bottom-right, 3 top-right) |
| fCsIRawADC | vector<Double_t> | fCsImulti | raw V785 |
| fCsITime | vector<Double_t> | fCsImulti | raw V1190 (first leading edge) |

Particles (HiRAPhysics: front/back energy pairing, ≤ MAX_TRACKS_PER_TELESCOPE per telescope):
| member | type | length | units |
|---|---|---|---|
| fmulti | Int_t | – | |
| fnumtel | vector<Int_t> | fmulti | telescope |
| fnumstripf, fnumstripb | vector<Int_t> | fmulti | EF / EB strip |
| fnumcsi | vector<Int_t> | fmulti | CsI behind the pixel 0–3 (-1 none) |
| fEnergySifHi, fEnergySifLo, fTimeSif | vector<Double_t> | fmulti | raw, front strip |
| fEnergySifCal | vector<Double_t> | fmulti | MeV, front strip |
| fEnergySibHi, fEnergySibLo, fTimeSib, fEnergySibCal | vector<Double_t> | fmulti | same, back strip |
| fCsIADC | vector<Double_t> | fmulti | raw V785 of fnumcsi (-10000 if it did not fire) |
| fTimecsi | vector<Double_t> | fmulti | raw V1190 of fnumcsi |
| fTheta, fPhi | vector<Double_t> | fmulti | deg, pixel centre |
| fdTheta, fdPhi | vector<Double_t> | fmulti | half-width of the pixel, deg |

## TDC. (TDCData) — auxiliary V1190 channels (input/TDC/TDCMapping.csv)
Raw first leading-edge time (V1190 LSB); -10000 = no hit.
| member | type |
|---|---|
| fMasterTrigger, fDiamondStrip0–4, fBeamRF, fDiamondAnode, fUpstreamScintCFD, fFROGOrOr, fFROGMaj, fFROGTrigger, fTimeReset | Double_t |
| fChannels | map<string, Double_t> — only the channels that fired: `for (auto &c : tdc->fChannels)` |

CsI times are in `HIRA.fCsITime`, not here.
