#include "FROG.h"

FROG::FROG() : Detector("FROG") {}
FROG::~FROG() {}

void FROG::Clear(){
	hits.clear();
	boards.clear();
	sawHeader = false;
}

void FROG::SetHGThreshold(uint16_t thr){
	for (int b = 0; b < kBoards; b++)
		for (int c = 0; c < kChannels; c++)
			fHGThr[b][c] = thr;
}

void FROG::SetHGThreshold(int board, int channel, uint16_t thr){
	if (board >= 0 && board < kBoards && channel >= 0 && channel < kChannels)
		fHGThr[board][channel] = thr;
}

// did this channel get data?
bool FROG::Keep(const FrogHit &hit) const{
	if (!zeroSuppress){
		return true;
	}
	if ((hit.has_toa) || (hit.has_tot)){
		return true;
	}
	if (hit.channel < kChannels){
		uint16_t thr = fHGThr[hit.board][hit.channel];
		if ((thr > 0) && (hit.has_hg) && (hit.hg >= thr)){
			return true;
		}
	}
	return false;
}

bool FROG::Unpack(const Ring::Fragment &frag){
	// nested: FROG sub-event builder -> one inner fragment per board
	if (Ring::SplitFragments(frag.payload, frag.payloadBytes, fInner)){
		bool ok = false;
		for (size_t i = 0; i < fInner.size(); i++){
			if (fInner[i].ringType != Ring::PHYSICS_EVENT){
				continue;
			}
			ok |= UnpackBoard(fInner[i]);
		}
		return ok;
	}
	// not nested: the fragment is one board
	return UnpackBoard(frag);
}

// Using FROGGeometry.h to get angular information from alignment/CAD calibrations
static void SetAngles(FrogHit &hit, const FROGGeometry &geo){
	const FrogAngles &angles 	= geo.Get(hit.board, hit.channel);
	hit.has_angles 			= angles.valid;
	hit.theta			= angles.theta;
	hit.dtheta			= angles.dtheta;
	hit.phi				= angles.phi;
	hit.dphi			= angles.dphi;
}

bool FROG::UnpackBoard(const Ring::Fragment &f){
	fBoardFrags++;

	// sanity: A5202 inclusive size (and, for data, inner size) must fit
	if (f.payloadBytes < 4 || Ring::peek<uint16_t>(f.payload) > f.payloadBytes){
		fBadFrags++;
		return false;
	}

	// A5202 reads through memcpy, so the cast is safe for any alignment
	unsigned short *p = const_cast<unsigned short*>(
		reinterpret_cast<const unsigned short*>(f.payload));

	bool stat = fA5202.Unpack(p);

	if (fA5202.looksLikeHeader){
		fA5202.UnpackFileHeader(p);
		headers[f.sourceID] = fA5202.header;
		sawHeader = true;
		return true;
	}
	if (!stat){
		fBadFrags++;
		return false;
	}

	uint8_t base = fA5202.mode & 0x0F;
	FrogBoard b;
	b.sourceID      = f.sourceID;
	b.mode          = fA5202.mode;
	b.fragTimestamp = f.timestamp;

	if (base == DTQ_SPECT || base == DTQ_TSPECT){
		const SpectEvent &e = fA5202.spect;
		b.board      = e.hdr.board;
		b.time_in_ns = e.hdr.time_in_ns;
		b.triggerID  = e.trigger_id;
		b.tstamp_us  = e.hdr.tstamp_us;
		for (size_t i = 0; i < e.channels.size(); i++){
			const SpectChannel &c = e.channels[i];
			FrogHit h;
			h.board   = b.board;
			h.channel = c.channel;
			h.has_lg  = c.has_lg;   h.lg  = c.energyLG;
			h.has_hg  = c.has_hg;   h.hg  = c.energyHG;
			h.has_toa = c.has_toa;  h.toa = c.toa;
			h.has_tot = c.has_tot;  h.tot = c.tot;

			// Setting angles
			SetAngles(h, geometry); // geometry is public class member (FROGGeometry) of FROG class

			fReadPerBoard[b.board]++;
			if (Keep(h)){
				hits.push_back(h);
				fHitsPerBoard[b.board] += e.channels.size();
			}
		}
	}
	else if (base == DTQ_TIMING){
		const TimingEvent &e = fA5202.timing;
		b.board      = e.hdr.board;
		b.time_in_ns = e.hdr.time_in_ns;
		b.tstamp_us  = e.hdr.tstamp_us;
		for (size_t i = 0; i < e.hits.size(); i++){
			const TimingHit &t = e.hits[i];
			FrogHit h;
			h.board   = b.board;
			h.channel = t.channel;
			h.has_toa = t.has_toa;  
			h.toa = t.toa;
			h.has_tot = t.has_tot;  
			h.tot = t.tot;

			// Setting angles
			SetAngles(h, geometry); // geometry is public class member (FROGGeometry) of FROG class

			fReadPerBoard[b.board]++;
			if (Keep(h)){
				hits.push_back(h);
				fHitsPerBoard[b.board] += e.hits.size();
			}
		}
	}
	else {
		// DTQ_COUNT: decoded into fA5202.counting but not exported yet
		b.board     = fA5202.counting.hdr.board;
		b.triggerID = fA5202.counting.trigger_id;
		b.tstamp_us = fA5202.counting.hdr.tstamp_us;
	}

	boards.push_back(b);
	return true;
}

void FROG::PrintSummary() const{
	std::cout << "FROG: " << fBoardFrags << " board fragments";
	if (fBadFrags){
		std::cout << "  \033[1;31m(" << fBadFrags << " bad)\033[0m";
	}
	std::cout << "\n";
	for (std::map<uint32_t, RunHeader>::const_iterator it = headers.begin(); it != headers.end(); ++it){
		const RunHeader &h = it->second;
		std::cout << "  header sid " << it->first
		          << ": run " << h.runnumber
		          << "  board type " << h.boardtype
		          << "  acqmode 0x" << std::hex << (int)h.acqmode << std::dec
		          << "  timeunit " << (int)h.timeunit
		          << "  LSB " << h.timeconversion << " ns\n";
	}
	std::cout << "  zero suppression " << (zeroSuppress ? "ON" : "OFF") << "\n";
	for (std::map<int, long>::const_iterator it = fReadPerBoard.begin(); it != fReadPerBoard.end(); ++it){
		std::map<int, long>::const_iterator k = fHitsPerBoard.find(it->first);
		long kept = (k == fHitsPerBoard.end()) ? 0 : k->second;
		std::cout << "  board " << it->first << ": " << it->second << " channels read, " << kept << " kept\n";
	}
}
