#include "DDAS.h"

#include <cstddef>

bool DDAS::LooksLikeModuleInfo(uint32_t word){
	uint32_t msps = word & 0xffff;
	return msps == 100 || msps == 250 || msps == 500;
}

bool DDAS::Decode(const uint32_t *&p, const uint32_t *end, DDASHit &h,
                  bool keepTrace, std::string &error){
	const uint32_t *q = p;
	h = DDASHit();

	// ---- size + module info
	if (end - q < 6){
		error = "short hit";
		return false;
	}
	uint32_t nShorts = *q++;
	size_t   hitWords = nShorts / 2;		// 32-bit words, including the size word
	if (hitWords < 6 || (ptrdiff_t)hitWords > end - p){
		error = "bad size word";
		return false;
	}
	uint32_t info = *q++;
	if (!LooksLikeModuleInfo(info)){
		error = "bad module info word";
		return false;
	}
	h.msps       = info & 0xffff;
	h.adcBits    = (info >> 16) & 0xff;
	h.hwRevision = (info >> 24) & 0xff;

	// ---- header word 0
	uint32_t w0 = *q++;
	if (h.hwRevision == 17){			// Rev H
		h.channel = w0 & 0x3f;
		h.slot    = (w0 >> 6) & 0xf;
		h.crate   = (w0 >> 10) & 0x3;
	}
	else {
		h.channel = w0 & 0xf;
		h.slot    = (w0 >> 4) & 0xf;
		h.crate   = (w0 >> 8) & 0xf;
	}
	h.headerLength  = (w0 >> 12) & 0x1f;
	h.channelLength = (w0 >> 17) & 0x3fff;
	h.finishCode    = (w0 >> 31) & 0x1;

	// ---- header words 1, 2: timestamp + CFD
	uint32_t tLow = *q++;
	uint32_t w2   = *q++;
	h.timestampRaw = ((uint64_t)(w2 & 0xffff) << 32) | tLow;
	double cfd = 0;
	if (h.msps == 500){
		h.cfdTrigSource = (w2 >> 29) & 0x7;
		h.cfdFraction   = (w2 >> 16) & 0x1fff;
		h.cfdFail       = (h.cfdTrigSource == 7) ? 1 : 0;
		cfd = (h.cfdFraction / 8192.0 + h.cfdTrigSource - 1) * 2.0;
		h.coarseTime = h.timestampRaw * 10;
	}
	else if (h.msps == 250){
		h.cfdFail       = (w2 >> 31) & 0x1;
		h.cfdTrigSource = (w2 >> 30) & 0x1;
		h.cfdFraction   = (w2 >> 16) & 0x3fff;
		cfd = (h.cfdFraction / 16384.0 - h.cfdTrigSource) * 4.0;
		h.coarseTime = h.timestampRaw * 8;
	}
	else {						// 100 MSPS
		h.cfdFail     = (w2 >> 31) & 0x1;
		h.cfdFraction = (w2 >> 16) & 0x7fff;
		cfd = (h.cfdFraction / 32768.0) * 10.0;
		h.coarseTime = h.timestampRaw * 10;
	}
	h.time = (double)h.coarseTime + cfd;

	// ---- header word 3
	uint32_t w3 = *q++;
	h.energy      = w3 & 0xffff;
	h.traceLength = (w3 >> 16) & 0x7fff;
	h.overflow    = (w3 >> 31) & 0x1;

	// ---- lengths must agree
	if (h.headerLength < 4 || h.channelLength != h.headerLength + h.traceLength / 2 ||
	    hitWords < (size_t)(2 + h.channelLength)){
		error = "inconsistent lengths";
		return false;
	}

	// ---- optional header blocks, in DDASFormat order: energy sums, QDC, external TS
	int extra = h.headerLength - 4;
	bool esums = false, qdc = false, ets = false;
	switch (extra){
		case 0:                                         break;
		case 2:                               ets = true; break;
		case 4:  esums = true;                          break;
		case 6:  esums = true;                ets = true; break;
		case 8:                qdc = true;              break;
		case 10:               qdc = true;    ets = true; break;
		case 12: esums = true; qdc = true;              break;
		case 14: esums = true; qdc = true;    ets = true; break;
		default:
			error = "unexpected header length";
			return false;
	}
	if (esums){
		h.energySums.assign(q, q + 4);
		q += 4;
	}
	if (qdc){
		h.qdcSums.assign(q, q + 8);
		q += 8;
	}
	if (ets){
		h.externalTimestamp    = ((uint64_t)(q[1] & 0xffff) << 32) | q[0];
		h.hasExternalTimestamp = true;
		q += 2;
	}

	// ---- trace
	if (keepTrace && h.traceLength > 0){
		h.trace.reserve(h.traceLength);
		for (int i = 0; i < h.traceLength / 2; i++){
			h.trace.push_back(q[i] & 0xffff);
			h.trace.push_back(q[i] >> 16);
		}
	}

	p += hitWords;		// the size word is authoritative for where the next hit starts
	return true;
}
