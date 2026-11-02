#include "V785.h"

static inline uint32_t word32(const uint16_t *p){
	return (uint32_t)p[0] | ((uint32_t)p[1] << 16);	// low half first
}

V785::V785(){
	Clear();
}

void V785::Clear(){
	present      = false;
	geo          = 0;
	eventCounter = 0;
	for (int i = 0; i < kChannels; i++){
		has[i]       = false;
		value[i]     = 0;
		overflow[i]  = false;
		underflow[i] = false;
	}
}

bool V785::Decode(const uint16_t *&p, const uint16_t *end){
	Clear();
	if (p + 2 > end){
		return false;
	}
	if (p[0] == 0xffff && p[1] == 0xffff){
		return true;				// empty module
	}

	uint32_t v = word32(p);
	if (((v >> 24) & 0x7) != 2){		// not a header
		return false;
	}
	geo = v >> 27;
	int count = (v >> 8) & 0x3f;
	p += 2;

	for (int i = 0; i < count; i++){
		if (p + 2 > end){
			return false;
		}
		v = word32(p);
		if (((v >> 24) & 0x7) != 0){	// fewer data words than promised
			break;
		}
		int ch = (v >> 16) & 0x1f;
		has[ch]       = true;
		value[ch]     = v & 0xfff;
		overflow[ch]  = (v >> 12) & 0x1;
		underflow[ch] = (v >> 13) & 0x1;
		p += 2;
	}

	if (p + 2 > end){
		return false;
	}
	v = word32(p);
	if (((v >> 24) & 0x7) != 4){		// no end-of-block
		return false;
	}
	eventCounter = v & 0xffffff;
	p += 2;

	present = true;
	return true;
}
