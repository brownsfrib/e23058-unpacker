#include "HINP.h"

HINP::HINP(uint16_t marker, int motherboard)
	: fMarker(marker), fMotherboard(motherboard){
	Clear();
}

void HINP::Clear(){
	present = false;
	strips.clear();
}

bool HINP::Decode(const uint16_t *&p, const uint16_t *end){
	Clear();
	if (p >= end || *p != fMarker){
		return false;
	}
	const uint16_t *blk = p + 1;
	if (blk + 3 > end){
		return false;
	}
	uint16_t n          = blk[0];
	uint16_t stripWords = blk[2];
	const uint16_t *blkEnd = blk + n + 2;
	if (blkEnd > end || stripWords % 4 != 0 || 7 + stripWords > n){
		return false;
	}

	const uint16_t *s = blk + 7;
	for (int i = 0; i < stripWords / 4; i++, s += 4){
		HINPStrip h;
		uint16_t id   = s[0];
		h.motherboard = fMotherboard;
		h.chip        = (id >> 5) & 0xff;
		int chipCh    = id & 0x1f;
		if (h.chip % 2 == 0){
			h.board = h.chip / 2;
			h.strip = 2 * chipCh;
		}
		else {
			h.board = h.chip / 2 + 1;
			h.strip = 2 * chipCh + 1;
		}
		h.highGain = s[1];
		h.lowGain  = s[2];
		h.time     = s[3];
		strips.push_back(h);
	}

	p = blkEnd;
	present = true;
	return true;
}
