#include "Pixie.h"

#include <cstring>
#include <iostream>

Pixie::Pixie(PixieData *data, bool keepTraces)
	: Detector("Pixie"), fData(data), fKeepTraces(keepTraces) {}
Pixie::~Pixie() {}

void Pixie::Clear(){
	fData->Clear();
}

bool Pixie::Unpack(const Ring::Fragment &frag){
	fFrags++;
	if (frag.payloadBytes >= 8 && DDAS::LooksLikeModuleInfo(Ring::peek<uint32_t>(frag.payload + 4))){
		return UnpackPayload(frag.payload, frag.payloadBytes);
	}
	// nested: a built body with one inner fragment per hit/module
	if (Ring::SplitFragments(frag.payload, frag.payloadBytes, fInner)){
		bool ok = false;
		for (size_t i = 0; i < fInner.size(); i++){
			if (fInner[i].ringType == Ring::PHYSICS_EVENT){
				ok |= UnpackPayload(fInner[i].payload, fInner[i].payloadBytes);
			}
		}
		return ok;
	}
	fFailures["not a DDAS payload"]++;
	return false;
}

bool Pixie::UnpackPayload(const uint8_t *payload, size_t nbytes){
	size_t nwords = nbytes / 4;
	fWords.resize(nwords);
	std::memcpy(fWords.data(), payload, nwords * 4);
	const uint32_t *p   = fWords.data();
	const uint32_t *end = p + nwords;

	PixieData &d = *fData;
	std::string err;
	int n = 0;
	while (p < end){
		if (!DDAS::Decode(p, end, fHit, fKeepTraces, err)){
			fFailures[err]++;
			break;			// keep the hits already decoded from this payload
		}
		d.fCrate.push_back(fHit.crate);
		d.fSlot.push_back(fHit.slot);
		d.fChannel.push_back(fHit.channel);
		d.fEnergy.push_back((Int_t)fHit.energy);
		d.fTime.push_back(fHit.time);
		d.fCoarseTime.push_back(fHit.coarseTime);
		d.fCFD.push_back((Int_t)fHit.cfdFraction);
		d.fCFDTrigSource.push_back(fHit.cfdTrigSource);
		d.fCFDFail.push_back(fHit.cfdFail);
		d.fFinishCode.push_back(fHit.finishCode);
		d.fOverflow.push_back(fHit.overflow);
		d.fTraceLength.push_back(fHit.traceLength);
		d.fExtTimestamp.push_back(fHit.externalTimestamp);
		d.fEnergySums.push_back(std::vector<UInt_t>(fHit.energySums.begin(), fHit.energySums.end()));
		d.fQDCSums.push_back(std::vector<UInt_t>(fHit.qdcSums.begin(), fHit.qdcSums.end()));
		d.fTrace.push_back(std::vector<UShort_t>(fHit.trace.begin(), fHit.trace.end()));

		int module = fHit.crate * 100 + fHit.slot;
		fHitsByModule[module]++;
		fMSPSByModule[module] = fHit.msps;
		n++;
	}
	d.fMult = (Int_t)d.fCrate.size();

	// mapping is filled later; keep the vectors the same length
	d.fModule.resize(d.fMult, -1);
	d.fDetector.resize(d.fMult, PixieData::kUnmapped);
	d.fBar.resize(d.fMult, -1);
	d.fSide.resize(d.fMult, -1);

	fHits += n;
	return n > 0;
}

void Pixie::PrintSummary() const{
	std::cout << "Pixie: " << fFrags << " fragments, " << fHits << " hits decoded\n";
	for (std::map<int, long>::const_iterator it = fHitsByModule.begin(); it != fHitsByModule.end(); ++it){
		std::cout << "  crate " << it->first / 100 << " slot " << it->first % 100
		          << " (" << fMSPSByModule.at(it->first) << " MSPS): " << it->second << " hits\n";
	}
	for (std::map<std::string, long>::const_iterator it = fFailures.begin(); it != fFailures.end(); ++it){
		std::cout << "  \033[1;31mPixie failed (" << it->first << "): " << it->second << "\033[0m\n";
	}
}
