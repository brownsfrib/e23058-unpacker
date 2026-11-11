#ifndef PIXIE_H
#define PIXIE_H

// C/C++ includes
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Unpacker includes
#include "DDAS.h"
#include "Detector.h"
#include "PixieData.h"

/**
 * !\brief Pixie-16 / DDAS reader (LANA + VETO wall, one crate, 10 modules) -> PixieData.
 *
 * DDAS contributes one fragment per hit, so Unpack() is called many times
 * per event and appends to PixieData. A payload may hold several hits back
 * to back; if it is itself a built body (a sub-event builder upstream) it is
 * split and each inner fragment decoded. Raw values only: the channel map is
 * applied afterwards by PixieMapping.
 */
class Pixie : public Detector{
	public:
		explicit Pixie(PixieData *data, bool keepTraces = false);
		~Pixie();

		void Clear() override;
		bool Unpack(const Ring::Fragment &frag) override;
		void PrintSummary() const override;

	private:
		bool UnpackPayload(const uint8_t *payload, size_t nbytes);

		PixieData *fData;
		bool fKeepTraces;
		DDASHit fHit;
		std::vector<uint32_t> fWords;		// aligned copy of the payload
		std::vector<Ring::Fragment> fInner;

		long fFrags = 0;
		long fHits  = 0;
		std::map<std::string, long> fFailures;
		std::map<int, long> fHitsByModule;	// crate*100 + slot
		std::map<int, int>  fMSPSByModule;
};

#endif // PIXIE_H
