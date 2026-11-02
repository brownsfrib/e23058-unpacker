#ifndef V785_H
#define V785_H

// C/C++ includes
#include <cstdint>

/**
 * !\brief CAEN V785 32-channel peak-sensing ADC (one module, one event).
 *
 * Each 32-bit word sits in the stream as two uint16, LOW half first.
 *   header : type 2, bits 13:8 = number of channels that follow
 *   data   : type 0, bits 20:16 = channel, 11:0 = value,
 *            bit 12 = overflow, bit 13 = underflow
 *   EOB    : type 4, bits 23:0 = event counter
 * GEO is bits 31:27 in every word. If the module sent nothing (next words
 * are the 0xffff separator) Decode() succeeds with present = false.
 */
class V785{
	public:
		static const int kChannels = 32;

		V785();
		void Clear();

		// p enters at the module's first word, leaves just past its EOB
		bool Decode(const uint16_t *&p, const uint16_t *end);

		bool     present;
		uint8_t  geo;
		uint32_t eventCounter;
		bool     has[kChannels];
		uint16_t value[kChannels];
		bool     overflow[kChannels];
		bool     underflow[kChannels];
};

#endif // V785_H
