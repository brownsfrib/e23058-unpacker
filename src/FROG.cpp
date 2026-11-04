#include "FROG.h"

#include <iostream>

static const double kNone = -10000;

FROG::FROG(FROGData *data) : Detector("FROG"), fData(data) {}
FROG::~FROG() {}

void FROG::Clear(){
	fData->Clear();
	sawHeader = false;
}

bool FROG::Unpack(const Ring::Fragment &frag){
	// nested: FROG sub-event builder -> one inner fragment per board
	if (Ring::SplitFragments(frag.payload, frag.payloadBytes, fInner)){
		bool ok = false;
		for (size_t i = 0; i < fInner.size(); i++){
			if (fInner[i].ringType == Ring::PHYSICS_EVENT){
				ok |= UnpackBoard(fInner[i]);
			}
		}
		return ok;
	}
	return UnpackBoard(frag);		// not nested: the fragment is one board
}

bool FROG::UnpackBoard(const Ring::Fragment &f){
	fBoardFrags++;
	if (f.payloadBytes < 4 || Ring::peek<uint16_t>(f.payload) > f.payloadBytes){
		fBadFrags++;
		return false;
	}
	// A5202 reads through memcpy, so the cast is safe for any alignment
	unsigned short *p = const_cast<unsigned short*>(reinterpret_cast<const unsigned short*>(f.payload));

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

	FROGData &d = *fData;
	uint8_t base = fA5202.mode & 0x0F;
	int board = -1;

	if (base == DTQ_SPECT || base == DTQ_TSPECT){
		const SpectEvent &e = fA5202.spect;
		board = e.hdr.board;
		d.fBoardTrigID.push_back(e.trigger_id);
		d.fBoardTime.push_back(e.hdr.tstamp_us);
		for (size_t i = 0; i < e.channels.size(); i++){
			const SpectChannel &c = e.channels[i];
			d.fBoard.push_back(board);
			d.fChannel.push_back(c.channel);
			d.fLG.push_back(c.has_lg   ? c.energyLG : kNone);
			d.fHG.push_back(c.has_hg   ? c.energyHG : kNone);
			d.fToA.push_back(c.has_toa ? c.toa      : kNone);
			d.fToT.push_back(c.has_tot ? c.tot      : kNone);
		}
		fChannelsPerBoard[board] += e.channels.size();
	}
	else if (base == DTQ_TIMING){
		const TimingEvent &e = fA5202.timing;
		board = e.hdr.board;
		d.fBoardTrigID.push_back(0);
		d.fBoardTime.push_back(e.hdr.tstamp_us);
		for (size_t i = 0; i < e.hits.size(); i++){
			const TimingHit &t = e.hits[i];
			d.fBoard.push_back(board);
			d.fChannel.push_back(t.channel);
			d.fLG.push_back(kNone);
			d.fHG.push_back(kNone);
			d.fToA.push_back(t.has_toa ? t.toa : kNone);
			d.fToT.push_back(t.has_tot ? t.tot : kNone);
		}
		fChannelsPerBoard[board] += e.hits.size();
	}
	else {
		// DTQ_COUNT: decoded into fA5202.counting but not stored
		board = fA5202.counting.hdr.board;
		d.fBoardTrigID.push_back(fA5202.counting.trigger_id);
		d.fBoardTime.push_back(fA5202.counting.hdr.tstamp_us);
	}
	d.fBoardID.push_back(board);
	d.fNBoards = (Int_t)d.fBoardID.size();

	// angles and cut flags are filled later; keep the vectors the same length
	size_t n = d.fBoard.size();
	d.fTheta.resize(n, kNone);  d.fDTheta.resize(n, kNone);
	d.fPhi.resize(n, kNone);    d.fDPhi.resize(n, kNone);
	d.fPass.resize(n, 1);
	d.fMult  = (Int_t)n;
	d.fNPass = (Int_t)n;
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
		std::cout << "  header board " << it->first << ": run " << h.runnumber
		          << "  acqmode 0x" << std::hex << (int)h.acqmode << std::dec
		          << "  LSB " << h.timeconversion << " ns\n";
	}
	for (std::map<int, long>::const_iterator it = fChannelsPerBoard.begin(); it != fChannelsPerBoard.end(); ++it){
		std::cout << "  board " << it->first << ": " << it->second << " channels read\n";
	}
}
