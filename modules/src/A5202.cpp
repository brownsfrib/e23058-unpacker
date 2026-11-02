#include "A5202.h"

#include <iostream>

// ctor
A5202::A5202(){
	A5202::point = nullptr;
	A5202::Reset();
}

// dtor
A5202::~A5202(){}

void A5202::Reset(){
	A5202::mode = 0;
	A5202::looksLikeHeader = false;

	A5202::timing   = TimingEvent();
	A5202::spect    = SpectEvent();
	A5202::counting = CountingEvent();
	A5202::waveform = WaveformEvent();
	// NOTE: 'header' is run-level and intentionally NOT reset per event
}

// Decode one ring-item body payload and fill the struct for its acq mode.
// point enters at the inclusive-size word (as handed over by channels.cxx).
bool A5202::Unpack(unsigned short *point0){

	A5202::Reset();
	A5202::point = point0;

	const unsigned char *p = reinterpret_cast<const unsigned char *>(point0);

	uint16_t inclusiveSize;	get(inclusiveSize, p);
	uint16_t metadata;	get(metadata, p);
	uint8_t timeUnit = metadata & 0xFF;		// 0 = LSB, 1 = ns
	uint8_t acqMode  = (metadata >> 8) & 0xFF;	// FERS DataQualifier
	A5202::mode = acqMode;

	bool time_ns  = (timeUnit == 1);
	uint8_t base  = acqMode & 0x0F;

	// Reject anything that is not a physics event. A real event always has
	// timeUnit 0 (LSB) or 1 (ns); the file header has no metadata word, so
	// this slot holds dataformat_major (>= 2) instead -> flag it as a header.
	if (timeUnit > 1){
		A5202::looksLikeHeader = true;
		return false;
	}
	if (base != DTQ_SPECT && base != DTQ_TIMING &&
	    base != DTQ_TSPECT && base != DTQ_COUNT){
		return false;
	}

	// Common inner header: innerSize, board, ts
	const unsigned char *pInner = p;
	uint16_t innerSize;	get(innerSize, p);
	uint8_t board;		get(board, p);
	double ts;		get(ts, p);
	const unsigned char *pEnd = pInner + innerSize;	// end of channel data

	// ------------------------------------------------------------------
	// TIMING (0x02): nhits, then per hit channel/datatype/[ToA]/[ToT]
	// ------------------------------------------------------------------
	if (base == DTQ_TIMING){
		uint16_t nhits;	get(nhits, p);
		A5202::timing.hdr.mode       = acqMode;
		A5202::timing.hdr.board      = board;
		A5202::timing.hdr.tstamp_us  = ts;
		A5202::timing.hdr.time_in_ns = time_ns;
		A5202::timing.nhits = nhits;
		for (uint16_t i = 0; i < nhits; i++){
			TimingHit h;
			get(h.channel, p);
			get(h.datatype, p);
			if (h.datatype & DATA_TOA){
				if (time_ns){ float v;    get(v, p); h.toa = v; }
				else        { uint32_t v; get(v, p); h.toa = v; }
				h.has_toa = true;
			}
			if (h.datatype & DATA_TOT){
				if (time_ns){ float v;    get(v, p); h.tot = v; }
				else        { uint16_t v; get(v, p); h.tot = v; }
				h.has_tot = true;
			}
			A5202::timing.hits.push_back(h);
		}
		return true;
	}

	// ------------------------------------------------------------------
	// SPECT (0x01) / TSPECT (0x03): [rel_ts], trgid, chmask, then per
	// masked channel channel/datatype/[LG]/[HG]/[ToA]/[ToT]
	// ------------------------------------------------------------------
	if (base == DTQ_SPECT || base == DTQ_TSPECT){
		if (acqMode & 0x80){				// SaveList omits rel_ts from
			double rel; get(rel, p);		// the spect size field, so
			pEnd += sizeof(double);			// bump pEnd to compensate
			A5202::spect.hdr.has_rel_tstamp = true;
			A5202::spect.hdr.rel_tstamp_us  = rel;
		}
		uint64_t trgid;  get(trgid, p);
		uint64_t chmask; get(chmask, p);
		A5202::spect.hdr.mode       = acqMode;
		A5202::spect.hdr.board      = board;
		A5202::spect.hdr.tstamp_us  = ts;
		A5202::spect.hdr.time_in_ns = time_ns;
		A5202::spect.is_tspect  = (base == DTQ_TSPECT);
		A5202::spect.trigger_id = trgid;
		A5202::spect.chmask     = chmask;
		while (p < pEnd){
			SpectChannel c;
			get(c.channel, p);
			get(c.datatype, p);
			if (c.datatype & DATA_LG){ uint16_t v; get(v, p); c.energyLG = v; c.has_lg = true; }
			if (c.datatype & DATA_HG){ uint16_t v; get(v, p); c.energyHG = v; c.has_hg = true; }
			if (c.datatype & DATA_TOA){
				if (time_ns){ float v;    get(v, p); c.toa = v; }
				else        { uint32_t v; get(v, p); c.toa = v; }
				c.has_toa = true;
			}
			if (c.datatype & DATA_TOT){
				if (time_ns){ float v;    get(v, p); c.tot = v; }
				else        { uint16_t v; get(v, p); c.tot = v; }
				c.has_tot = true;
			}
			A5202::spect.channels.push_back(c);
		}
		return true;
	}

	// ------------------------------------------------------------------
	// COUNTING (0x04): [rel_ts], trgid, chmask, then per masked channel
	// channel/counts
	// ------------------------------------------------------------------
	if (base == DTQ_COUNT){
		if (acqMode & 0x80){				// counting DOES count rel_ts
			double rel; get(rel, p);		// in its size, so pEnd stays
			A5202::counting.hdr.has_rel_tstamp = true;
			A5202::counting.hdr.rel_tstamp_us  = rel;
		}
		uint64_t trgid;  get(trgid, p);
		uint64_t chmask; get(chmask, p);
		A5202::counting.hdr.mode       = acqMode;
		A5202::counting.hdr.board      = board;
		A5202::counting.hdr.tstamp_us  = ts;
		A5202::counting.hdr.time_in_ns = time_ns;
		A5202::counting.trigger_id = trgid;
		A5202::counting.chmask     = chmask;
		while (p < pEnd){
			CountChannel c;
			get(c.channel, p);
			get(c.counts, p);
			A5202::counting.channels.push_back(c);
		}
		return true;
	}

	// Waveform (0x08) is not ring-serialized by this readout; service and
	// any unknown qualifier fall through here.
	return false;
}

// Decode the run/file header (first PHYSICS_EVENT item). It has NO metadata
// word: the inclusive-size word is followed directly by a packed FileHeader_t.
bool A5202::UnpackFileHeader(unsigned short *point0){

	A5202::point = point0;
	const unsigned char *p = reinterpret_cast<const unsigned char *>(point0);

	uint16_t inclusiveSize;	get(inclusiveSize, p);
	get(A5202::header.dataformat_major, p);
	get(A5202::header.dataformat_minor, p);
	get(A5202::header.software_major, p);
	get(A5202::header.software_minor, p);
	get(A5202::header.software_patch, p);
	get(A5202::header.boardtype, p);
	get(A5202::header.runnumber, p);
	get(A5202::header.acqmode, p);
	get(A5202::header.energyNCH, p);
	get(A5202::header.timeunit, p);
	get(A5202::header.timeconversion, p);
	get(A5202::header.startacq_ms, p);
	A5202::header.valid = true;

	return true;
}
