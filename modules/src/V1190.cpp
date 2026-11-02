#include "V1190.h"

static inline uint32_t word32(const uint16_t *p){
	return (uint32_t)p[0] | ((uint32_t)p[1] << 16);	// low half first
}

V1190::V1190(){
	Clear();
}

void V1190::Clear(){
	present           = false;
	geo               = 0;
	eventCount        = 0;
	triggerTimeTag    = 0;
	hasTriggerTimeTag = false;
	status            = 0;
	errorFlags        = 0;
	hits.clear();
	for (int i = 0; i < kChannels; i++){
		fOrder[i] = 0;
	}
}

bool V1190::Decode(const uint16_t *&p, const uint16_t *end){
	Clear();
	if (p + 2 > end){
		return false;
	}
	if (p[0] == 0xffff && p[1] == 0xffff){
		return true;				// empty module
	}

	uint32_t v = word32(p);
	if ((v >> 27) != 0x08){			// not a global header
		return false;
	}
	geo        = v & 0x1f;
	eventCount = (v >> 5) & 0x3fffff;
	p += 2;
	int nwords = 1;

	while (p + 2 <= end){
		v = word32(p);
		p += 2;
		nwords++;
		switch (v >> 27){
			case 0x00: {			// measurement
				V1190Hit h;
				h.channel  = (v >> 19) & 0x7f;
				h.time     = v & 0x7ffff;
				h.trailing = (v >> 26) & 0x1;
				h.order    = fOrder[h.channel]++;
				hits.push_back(h);
				break;
			}
			case 0x01:			// TDC header
			case 0x03:			// TDC trailer
			case 0x18:			// filler
				break;
			case 0x04:			// TDC error
				errorFlags |= v & 0x7fff;
				break;
			case 0x11:			// extended trigger time tag
				triggerTimeTag    = v & 0x7ffffff;
				hasTriggerTimeTag = true;
				break;
			case 0x10: {			// global trailer
				status = (v >> 24) & 0x7;
				int count = (v >> 5) & 0xffff;
				if ((int)(v & 0x1f) != geo || count != nwords){
					return false;	// corrupt or lost words
				}
				present = true;
				return true;
			}
			default:
				return false;		// not V1190 data
		}
	}
	return false;					// ran off the end, no trailer
}
