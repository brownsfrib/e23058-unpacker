#ifndef PIXIE_H
#define PIXIE_H

// C/C++ includes
#include <cstdint>
#include <vector>

// Unpacker includes
#include "Detector.h"

// Fill in once the DDAS hit decoding is written
struct PixieHit {
	uint32_t sourceID  = 0;
	uint64_t timestamp = 0;	// event-builder timestamp of the fragment
	// int crate, slot, channel; uint16_t energy; std::vector<uint16_t> trace; ...
};

/**
 * !\brief Pixie-16 / DDAS. SKELETON: counts fragments, decodes nothing yet.
 *
 * Unlike FROG, Pixie contributes ONE FRAGMENT PER HIT, so Unpack() is
 * called many times per event and should push_back into 'hits'.
 *
 * In glomTest each payload starts with a uint32 size in 16-bit words
 * (0x88 -> 272 bytes, 0x10a -> 532 bytes with trace), then a module-info
 * word (0x0f2e01f4, low 16 bits = 500 MSPS), then the Pixie list-mode hit.
 */
class Pixie : public Detector{
	public:
		Pixie();
		~Pixie();

		void Clear() override;
		bool Unpack(const Ring::Fragment &frag) override;
		void PrintSummary() const override;

		std::vector<PixieHit> hits;

	private:
		long fFrags = 0;
};

#endif // PIXIE_H
