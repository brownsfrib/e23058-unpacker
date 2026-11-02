#ifndef A5202_H
#define A5202_H

// C/C++ includes
#include <cstdint>
#include <cstring>

// Data structures (one per acquisition mode)
#include "A5202Data.h"

/**
 * !\brief unpacks one A5202 / DT5202 (Janus, FERS-5200) ring-item body and
 *         fills the struct that matches the event's acquisition mode.
 *
 * point enters at the inclusive-size word of the body payload, i.e. exactly
 * where channels.cxx leaves it after stripping the ring-item body header.
 * Layout (RingBufferHandler::writeToRing + SaveList):
 *
 *     uint16 inclusiveSize
 *     uint16 metadata          = (acqMode << 8) | timeUnit
 *     <mode payload>           (starts with uint16 innerSize, board, ts ...)
 *     0xFFFFFFFF + 0..3 pad 0xFF
 *
 * The acqMode high byte is the FERS DataQualifier: 0x01 spect, 0x02 timing,
 * 0x03 spect+timing, 0x04 counting. After Unpack(), 'mode' tells you which of
 * the structs below was filled.
 */
class A5202{
	private:
		// little-endian byte-cursor read (memcpy avoids misaligned access)
		template<class T> void get(T &t, const unsigned char *&p){
			std::memcpy(&t, p, sizeof(T));
			p += sizeof(T);
		}

	public:
		A5202();
		~A5202();

		bool Unpack(unsigned short *);	// decode one ring-item body payload
		bool UnpackFileHeader(unsigned short *);	// decode the run/file header
		void Reset();

		unsigned short *point;

		uint8_t mode;			// DataQualifier of last event (0 if none)
		bool looksLikeHeader;		// set when a body has no metadata word

		RunHeader header;		// run-level config (filled by UnpackFileHeader)

		// Exactly one is filled per event, selected by 'mode'
		TimingEvent   timing;		// mode 0x02
		SpectEvent    spect;		// mode 0x01 or 0x03
		CountingEvent counting;		// mode 0x04
		WaveformEvent waveform;		// mode 0x08 (not emitted by this readout)
};
#endif
