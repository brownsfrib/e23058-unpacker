#ifndef V1190_H
#define V1190_H

// C/C++ includes
#include <cstdint>
#include <vector>

struct V1190Hit {
	int      channel  = 0;
	uint32_t time     = 0;		// raw, 19 bits
	bool     trailing = false;	// false = leading edge
	int      order    = 0;		// 0 = first hit on this channel this event
};

/**
 * !\brief CAEN V1190 multi-hit TDC (one module, one event).
 *
 * 32-bit words, LOW half first in the stream. Type = bits 31:27.
 *   0x08 global header  : bits 26:5 event count, 4:0 GEO
 *   0x01 TDC header     : bits 25:24 chip, 23:12 event ID, 11:0 bunch ID
 *   0x00 measurement    : bit 26 trailing, 25:19 channel, 18:0 time
 *   0x03 TDC trailer    : bits 11:0 word count
 *   0x04 TDC error      : bits 14:0 error flags
 *   0x11 trigger time tag (extended): bits 26:0
 *   0x10 global trailer : bits 26:24 status, 20:5 word count, 4:0 GEO
 *   0x18 filler
 * Times are stored RAW; reference subtraction is done by the caller.
 */
class V1190{
	public:
		static const int kChannels = 128;

		V1190();
		void Clear();

		// p enters at the global header, leaves just past the global trailer
		bool Decode(const uint16_t *&p, const uint16_t *end);

		bool     present;
		uint8_t  geo;
		uint32_t eventCount;
		uint32_t triggerTimeTag;
		bool     hasTriggerTimeTag;
		uint8_t  status;	// global trailer status bits (0 = OK)
		uint16_t errorFlags;	// OR of all TDC error words
		std::vector<V1190Hit> hits;

	private:
		int fOrder[kChannels];
};

#endif // V1190_H
