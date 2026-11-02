#include "RingItem.h"

namespace Ring {

bool SplitFragments(const uint8_t *body, size_t nbytes, std::vector<Fragment> &out){
	out.clear();
	if (nbytes < 4){
		return false;
	}

	uint32_t total = peek<uint32_t>(body);
	if (total < 4 || total > nbytes){
		return false;
	}

	size_t p = 4;
	while (p < total){
		if (p + 20 > total){
			return false;
		}
		Fragment f;
		f.timestamp       = peek<uint64_t>(body + p);
		f.sourceID        = peek<uint32_t>(body + p + 8);
		uint32_t fragBytes = peek<uint32_t>(body + p + 12);
		f.barrier         = peek<uint32_t>(body + p + 16);

		const uint8_t *ring = body + p + 20;
		if (fragBytes < 12 || p + 20 + fragBytes > total){
			return false;
		}
		uint32_t ringSize = peek<uint32_t>(ring);
		if (ringSize != fragBytes){	// wrapped ring item fills the fragment
			return false;
		}
		f.ringType = peek<uint32_t>(ring + 4);

		size_t hdr = 8 + BodyHeaderBytes(ring + 8);
		if (hdr > ringSize){
			return false;
		}
		f.payload      = ring + hdr;
		f.payloadBytes = ringSize - hdr;

		out.push_back(f);
		p += 20 + fragBytes;
	}

	if (p != total || out.empty()){
		out.clear();
		return false;
	}
	return true;
}

}
