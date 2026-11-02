#include "HiRA.h"

#include <cstring>
#include <iostream>

HiRA::HiRA() : Detector("HiRA"), mb1(0x1ff3, 1), mb2(0x2ff3, 2) {
	Clear();
}
HiRA::~HiRA() {}

void HiRA::Clear(){
	timestamp = 0;
	strips.clear();
	mb1.Clear();
	mb2.Clear();
	adc1.Clear();
	adc2.Clear();
	tdc.Clear();
}

bool HiRA::Fail(const char *stage){
	fFailures[stage]++;
	return false;
}

// consume the 0xffff 0xffff that follows each CAEN module
static bool separator(const uint16_t *&p, const uint16_t *end){
	if (p + 2 > end || p[0] != 0xffff || p[1] != 0xffff){
		return false;
	}
	p += 2;
	return true;
}

bool HiRA::Unpack(const Ring::Fragment &frag){
	size_t nwords = frag.payloadBytes / 2;
	fWords.resize(nwords);
	std::memcpy(fWords.data(), frag.payload, nwords * 2);
	const uint16_t *p   = fWords.data();
	const uint16_t *end = p + nwords;

	// ---- preamble ----
	if ((nwords < 7) || (p[0] != nwords - 1)){
		return Fail("preamble");
	}
	timestamp = (uint64_t)p[3] | ((uint64_t)p[4] << 16)
	          | ((uint64_t)p[5] << 32) | ((uint64_t)p[6] << 48);
	p += 7;

	// ---- stack, in readout order ----
	if (!mb1.Decode(p, end)){
		return Fail("HINP mb1");
	}
	if (!mb2.Decode(p, end)){
		return Fail("HINP mb2");
	}
	if ((!adc1.Decode(p, end)) || (!separator(p, end))){
		return Fail("ADC1 V785");
	}
	if ((!adc2.Decode(p, end)) || (!separator(p, end))){
		return Fail("ADC2 V785");
	}
	if ((!tdc.Decode(p, end))  || (!separator(p, end))){
		return Fail("TDC V1190");
	}
	if (p != end){
		return Fail("words left over");
	}

	strips = mb1.strips;
	strips.insert(strips.end(), mb2.strips.begin(), mb2.strips.end());

	if (
		(adc1.present) &&
		(adc2.present) && 
		(adc1.eventCounter != adc2.eventCounter)
	){
		fCounterSkew++;
	}
	fEvents++;
	fStripHits += strips.size();
	return true;
}

bool HiRA::RelativeTime(const V1190Hit &hit, double &t) const{
	int ref = tdcMap.Get(hit.channel).referenceChannel;
	if (ref < 0){
		t = hit.time;			// no reference: raw
		return true;
	}
	for (size_t i = 0; i < tdc.hits.size(); i++){
		const V1190Hit &r = tdc.hits[i];
		if (
			(r.channel == ref) && 
			(r.order == 0) && 
			(!r.trailing)
		){
			t = (double)hit.time - (double)r.time;
			return true;
		}
	}
	return false;				// reference channel did not fire
}

void HiRA::PrintSummary() const{
	std::cout << "HiRA: " << fEvents << " events decoded, " << fStripHits << " strip hits\n";
	if (fCounterSkew){
		std::cout << "  \033[1;31mADC1/ADC2 event counters disagreed in "
		          << fCounterSkew << " events\033[0m\n";
	}
	std::map<std::string, long>::const_iterator it;
	for (it = fFailures.begin(); it != fFailures.end(); ++it){
		std::cout << "  \033[1;31mHiRA failed at " << it->first << ": " << it->second << "\033[0m\n";
	}
}
