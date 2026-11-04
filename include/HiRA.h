#ifndef HIRA_H
#define HIRA_H

// C/C++ includes
#include <cstdint>
#include <map>
#include <string>
#include <vector>

// Unpacker includes
#include "Detector.h"
#include "HINP.h"
#include "V785.h"
#include "V1190.h"

/**
 * !\brief HiRA reader: one VM-USB stack, decoded in stack order.
 *
 * Decoding only. Mapping strips/CsI to telescopes and filling HiRAData is
 * HiRAGeometry::Map(); the auxiliary TDC channels go to TDCData via TDCMapping.
 *
 *   preamble   7 words: word count (rest of payload), 2 x 0, 64-bit timestamp
 *   HINP       motherboard 1 (marker 0x1ff3)
 *   HINP       motherboard 2 (marker 0x2ff3)
 *   V785       ADC1      + 0xffff 0xffff
 *   V785       ADC2      + 0xffff 0xffff
 *   V1190      TDC       + 0xffff 0xffff
 *
 * The separators between CAEN modules are consumed here, not in the
 * module classes. To change the stack, edit Unpack() in HiRA.cpp.
 */
class HiRA : public Detector{
	public:
		HiRA();
		~HiRA();

		void Clear() override;
		bool Unpack(const Ring::Fragment &frag) override;
		void PrintSummary() const override;

		// this event
		uint64_t timestamp;
		std::vector<HINPStrip> strips;	// both motherboards
		HINP  mb1;
		HINP  mb2;
		V785  adc1;
		V785  adc2;
		V1190 tdc;

	private:
		bool Fail(const char *stage);

		std::vector<uint16_t> fWords;	// aligned copy of the payload
		long fEvents      = 0;
		long fStripHits   = 0;
		long fCounterSkew = 0;		// ADC1/ADC2 event counters disagree
		std::map<std::string, long> fFailures;
};

#endif // HIRA_H
