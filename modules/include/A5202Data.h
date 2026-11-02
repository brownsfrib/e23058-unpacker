#ifndef A5202DATA_H
#define A5202DATA_H

// C/C++ includes

#include <cstdint>
#include <vector>

// ---- DataQualifier / acquisition-mode codes (from FERSlib.h) ---------------
enum A5202Mode : uint8_t {
    DTQ_SPECT   = 0x01, // Spectroscopy (energy: LG/HG)
    DTQ_TIMING  = 0x02, // Timing (ToA/ToT list)
    DTQ_TSPECT  = 0x03, // Spectroscopy + Timing (energy + ToA/ToT)
    DTQ_COUNT   = 0x04, // Counting (MCS)
    DTQ_WAVE    = 0x08, // Waveform (not ring-serialized by this readout)
    DTQ_SERVICE = 0x2F  // Service / monitoring event
};

// ---- Per-hit "datatype" byte bits (same scheme in every mode) --------------
enum A5202DataBits : uint8_t {
    DATA_LG  = 0x01, // low-gain energy present
    DATA_HG  = 0x02, // high-gain energy present
    DATA_TOA = 0x10, // time of arrival present
    DATA_TOT = 0x20  // time over threshold present
};

// ---- Run/file header (FileHeader_t) ----------------------------------------
//  Written once at run start as the first PHYSICS_EVENT item, via
//  RBH_writeToRing(true). Unlike physics events it has NO metadata word: its
//  payload is a 25-byte packed FileHeader_t straight after the inclusive size.
//  This is the run-level configuration record (acq mode, time unit, LSB, ...).
struct RunHeader {
    uint8_t  dataformat_major = 0;
    uint8_t  dataformat_minor = 0;
    uint8_t  software_major   = 0;
    uint8_t  software_minor   = 0;
    uint8_t  software_patch   = 0;
    uint16_t boardtype        = 0;
    uint16_t runnumber        = 0;
    uint8_t  acqmode          = 0;    // run-level DataQualifier
    uint16_t energyNCH        = 0;
    uint8_t  timeunit         = 0;    // 0 = LSB, 1 = ns
    float    timeconversion   = 0.0f; // LSB value in ns
    uint64_t startacq_ms      = 0;
    bool     valid            = false;
};

// ---- Fields common to every mode's payload header --------------------------
struct A5202Header {
    uint8_t  mode      = 0;      // DTQ value read from the metadata word
    uint8_t  board     = 0;      // b8
    double   tstamp_us = 0.0;    // ts (board timestamp, us)
    bool     time_in_ns = false; // WDcfg.OutFileUnit: false=raw LSB, true=ns
                                 // (governs how ToA/ToT below were written)
    bool     has_rel_tstamp   = false;
    double   rel_tstamp_us    = 0.0; // present only if (dtq & 0x80)
};

// ============================================================================
//  DTQ_TIMING (0x02) — List event
//  payload: size, board, ts, nhits, then per hit:
//           channel(u8), datatype(u8), [ToA], [ToT]
// ============================================================================
struct TimingHit {
    uint8_t channel   = 0;
    uint8_t datatype  = 0;     // DATA_TOA | DATA_TOT
    bool    has_toa   = false;
    bool    has_tot   = false;
    double  toa       = 0.0;   // raw u32 LSB, or float ns if time_in_ns
    double  tot       = 0.0;   // raw u16 LSB, or float ns if time_in_ns
};

struct TimingEvent {
    A5202Header            hdr;      // mode == DTQ_TIMING
    uint16_t               nhits = 0;
    std::vector<TimingHit> hits;     // size == nhits
};

// ============================================================================
//  DTQ_SPECT (0x01) and DTQ_TSPECT (0x03) — Spectroscopy [+ Timing]
//  payload: size, board, ts, [rel_ts], trgid, chmask, then per MASKED channel:
//           channel(u8), datatype(u8), [LG u16], [HG u16], [ToA], [ToT]
//  ToA/ToT are only ever present when is_tspect (DTQ_TSPECT).
// ============================================================================
struct SpectChannel {
    uint8_t  channel   = 0;
    uint8_t  datatype  = 0;    // DATA_LG | DATA_HG | DATA_TOA | DATA_TOT
    bool     has_lg    = false;
    bool     has_hg    = false;
    bool     has_toa   = false;
    bool     has_tot   = false;
    uint16_t energyLG  = 0;
    uint16_t energyHG  = 0;
    double   toa       = 0.0;  // raw u32 LSB, or float ns if time_in_ns
    double   tot       = 0.0;  // raw u16 LSB, or float ns if time_in_ns
};

struct SpectEvent {
    A5202Header               hdr;          // mode == DTQ_SPECT or DTQ_TSPECT
    bool                      is_tspect = false; // (dtq & 0x02): ToA/ToT present
    uint64_t                  trigger_id = 0;    // trgid
    uint64_t                  chmask     = 0;    // channels with data
    std::vector<SpectChannel> channels;          // one entry per set chmask bit
};

// ============================================================================
//  DTQ_COUNT (0x04) — Counting / MCS
//  payload: size, board, ts, [rel_ts], trgid, chmask, then per MASKED channel:
//           channel(u8), counts(u32)
// ============================================================================
struct CountChannel {
    uint8_t  channel = 0;
    uint32_t counts  = 0;
};

struct CountingEvent {
    A5202Header               hdr;         // mode == DTQ_COUNT
    uint64_t                  trigger_id = 0;
    uint64_t                  chmask     = 0;
    std::vector<CountChannel> channels;
};

// ============================================================================
//  DTQ_WAVE (0x08) — Waveform
//  Mirrors FERSlib WaveEvent_t. NOTE: SaveList() has no ring-buffer branch for
//  waveform, so this will not appear in the .evt stream from this readout. Kept
//  for completeness so an all-modes reader can carry it if a future readout
//  emits it.
// ============================================================================
struct WaveformEvent {
    A5202Header           hdr;          // mode == DTQ_WAVE
    uint64_t              trigger_id = 0;
    uint16_t              ns = 0;       // number of samples
    std::vector<uint16_t> wave_hg;      // high-gain samples
    std::vector<uint16_t> wave_lg;      // low-gain samples
    std::vector<uint8_t>  dig_probes;   // digital probe bits
};

#endif // A5202DATA_H
