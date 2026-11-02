#ifndef HINP_H
#define HINP_H

// C/C++ includes
#include <cstdint>
#include <vector>

struct HINPStrip {
	uint8_t  motherboard = 0;
	uint8_t  chip        = 0;	// raw chip number from the ID word
	uint8_t  board       = 0;	// chip board (two chips per board)
	uint8_t  strip       = 0;	// 0-31 on that board
	uint16_t highGain    = 0;
	uint16_t lowGain     = 0;
	uint16_t time        = 0;
};

/**
 * !\brief One HINP motherboard's XLM block (one event).
 *
 *   marker            0x1ff3, 0x2ff3, ... (one per motherboard)
 *   [0]  N            block = N words + 2 trailing words
 *   [1]  ?            (0 in all data seen)
 *   [2]  stripWords   4 per strip
 *   [3-6]             (0 in all data seen)
 *   [7...]            per strip: ID, high gain, low gain, time
 *   ... padding to N, then 2 trailing words
 *
 * ID word: bits 12:5 chip, 4:0 chip channel. Chips pair into boards:
 *   odd chip  c -> board c/2+1, strip 2*ch+1
 *   even chip c -> board c/2,   strip 2*ch
 * (formula from unpacker-tcl v1.3; checked against data: hits land on
 * neighbouring strips.)
 */
class HINP{
	public:
		HINP(uint16_t marker, int motherboard);
		void Clear();

		// p enters at the marker word, leaves just past the block
		bool Decode(const uint16_t *&p, const uint16_t *end);

		bool present;
		std::vector<HINPStrip> strips;

	private:
		uint16_t fMarker;
		int      fMotherboard;
};

#endif // HINP_H
