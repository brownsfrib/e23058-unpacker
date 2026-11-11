#ifndef DDAS_H
#define DDAS_H

// C/C++ includes
#include <cstdint>
#include <string>
#include <vector>

/**
 * !\brief One Pixie-16 list-mode hit as written by DDAS (FRIBDAQ DDASReadout).
 *
 * Bit layout follows FRIBDAQ/DDASFormat (DDASHitUnpacker.cpp, DDASBitMasks.h).
 */
struct DDASHit {
	// module info word
	int      msps        = 0;	// ADC frequency: 100, 250, 500
	int      adcBits     = 0;	// ADC resolution
	int      hwRevision  = 0;	// 17 = Rev H (different crate/slot/channel bits)

	// header word 0
	int      crate       = 0;
	int      slot        = 0;
	int      channel     = 0;
	int      headerLength  = 0;	// 32-bit words: 4 + optional blocks below
	int      channelLength = 0;	// 32-bit words: header + trace
	int      finishCode  = 0;	// 1 = pileup / module flagged the hit

	// header words 1-3
	uint64_t timestampRaw = 0;	// 48-bit clock count
	uint64_t coarseTime   = 0;	// ns (timestampRaw x 10 for 100/500 MSPS, x 8 for 250)
	double   time         = 0;	// ns, coarseTime + CFD correction
	uint32_t cfdFraction  = 0;	// raw CFD fraction
	int      cfdTrigSource = 0;	// 500 MSPS: bits 31:29; 250 MSPS: bit 30
	int      cfdFail      = 0;	// 500 MSPS: trigSource == 7
	uint32_t energy       = 0;	// on-board trapezoidal filter
	int      overflow     = 0;	// ADC out of range
	int      traceLength  = 0;	// samples

	// optional blocks (empty if not enabled in the module)
	std::vector<uint32_t> energySums;	// 4 words
	std::vector<uint32_t> qdcSums;		// 8 words
	uint64_t externalTimestamp = 0;
	bool     hasExternalTimestamp = false;
	std::vector<uint16_t> trace;		// only if Decode(..., keepTrace = true)
};

/**
 * !\brief Pixie-16 / DDAS hit decoder (electronics only, no mapping).
 *
 * One DDAS hit in a fragment payload:
 *     uint32 size          16-bit words in this hit, including this word
 *     uint32 module info   [15:0] MSPS, [23:16] ADC bits, [31:24] hardware revision
 *     uint32 x4            Pixie list-mode header
 *     uint32 x (headerLength - 4)   energy sums / QDC sums / external TS
 *     uint32 x traceLength/2        trace, two 16-bit samples per word
 */
class DDAS{
	public:
		// Decode one hit at p and advance p past it. On failure p is unchanged
		// and 'error' says which stage failed.
		static bool Decode(const uint32_t *&p, const uint32_t *end, DDASHit &hit,
		                   bool keepTrace, std::string &error);

		// true if 'word' looks like a DDAS module info word (MSPS 100/250/500)
		static bool LooksLikeModuleInfo(uint32_t word);
};

#endif // DDAS_H
